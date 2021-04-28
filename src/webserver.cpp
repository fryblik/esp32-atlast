#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>

#include "atlast-task.h"
#include "multi-io.h"
#include "webserver.h"


// Webserver globals
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncWebSocketClient* currentClient;

// Outgoing CLI text buffer and mutex
std::string wsOutString;
SemaphoreHandle_t wsOutMutex = xSemaphoreCreateMutex();

// File upload: file path, filem and size of data received so far
char uploadPath[32] = "/";
File uploadFileHandle;
size_t uploadBytesWritten = 0;
// File paths not to be deleted/overwritten
std::vector<std::string> corePaths {
    "/",
    "/www/codejar.min.js",
    "/www/dracula.css",
    "/www/esp32-atlast.js",
    "/www/favicon.ico",
    "/www/FileSaver.min.js",
    "/www/forth.min.js",
    "/www/index.html",
    "/www/jquery-3.5.1.min.js",
    "/www/linenumbers.min.js",
    "/www/runmode-standalone.min.js",
    "/www/style.css"
};

/**
 * On websocket event
 * 
 * Handle websocket events.
 */
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
    // Figure out the type of WebSocket event
    switch(type) {
        // New client has connected
        case WS_EVT_CONNECT:
            currentClient = client;

            // Notify serial CLI
            Serial.printf("[%u] Connection from ", client->id());
            Serial.println(client->remoteIP().toString());
            // Notify web CLI
            client->text("{\"type\":\"cli\",\"data\":\"Websocket connection established.\"}");

            // Send SPIFFS file list (JSON array)
            wsSendFileList();

            // close oldest client at max clients
            ws.cleanupClients();
            break;

        // Client has disconnected
        case WS_EVT_DISCONNECT:
            currentClient = nullptr;

            Serial.printf("[%u] Client disconnected.\n", client->id());
            break;

        // Handle incoming text or data
        case WS_EVT_DATA:
        {
            // Update current client
            currentClient = client;

            // Parse received data
            parseReceived(arg, data, len);

            // Yield task
            delay(1);
            break;
        }
            
        // For other events, do nothing
        case WS_EVT_ERROR:
        case WS_EVT_PONG:
        default:
            break;
    }
}

/**
 * Parse received
 * 
 * Parse data received through websocket.
 * Ref.: https://github.com/me-no-dev/ESPAsyncWebServer#async-websocket-event
 * TODO: Clean up this mess.
 */
void parseReceived(void * arg, uint8_t * data, size_t len) {
    // Parse arguments
    // WARNING: len = length of current frame,
    //          info->len = total length of all frames,
    //          info->final = always 1,
    //          info->num = always 0,
    //          info->offset = this one actually works as I'd expect.
    //          I don't like this library and it doesn't like me either.
    AwsFrameInfo * info = (AwsFrameInfo*)arg;

    if(info->final && info->index == 0 && info->len == len) {
        // The whole message is in a single frame

        if (info->opcode == WS_TEXT) {
            // Single-frame text (JSON)

            // Verify input length (to protect statically allocated JSON deserializer)
            if (info->len > 250) {
                multiPrintf("DISCARDED INPUT: Too long (%u > 250).\n", (size_t) info->len);
                return;
            }
            // Handle received JSON
            incomingJson((char*) data);

        } else if (info->opcode == WS_BINARY) {
            // Single-frame binary data

            // Reset counter to indicate first frame
            uploadBytesWritten = 0;
            // Handle received data
            incomingData(data, len, info->len);
        }
    } else {
        // Message is comprised of multiple frames/packets

        if (info->opcode == WS_TEXT) {
            // Multi-frame text (JSON)

            // TODO: Handle multi-frame text data. So far unnecessary.
            multiPrintf("DISCARDED INPUT: Multi-frame text data received.\n");

        } else if (info->opcode == WS_BINARY) {
            // Multi-frame binary data

            // Indicate first frame by resetting counter
            if (info->index == 0) {
                uploadBytesWritten = 0;
            }
            // Handle received data
            incomingData(data, len, info->len);
        }
    }
}

/**
 * Connect to WLAN
 */
void connectWLAN(const char* ssid, const char* password) {
    // Connect to access point
    Serial.println("Connecting to WLAN");
    WiFi.begin(ssid, password);
    while ( WiFi.status() != WL_CONNECTED ) {
        delay(500);
        Serial.print(".");
    }

    // Print IP address
    Serial.println("Connected!");
    Serial.print("My IP address: ");
    Serial.println(WiFi.localIP());
}

/**
 * Setup web server
 * 
 * Setup websocket, start the server, configure file hosting and mDNS.
 */
void setupWebServer() {
    // Setup websocket
    currentClient = nullptr;
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    // Start server
    server.begin();

    // Serve all files in SPIFFS
    // Request to the root serves "www/index.html"
    server.serveStatic("/", SPIFFS, "/").setDefaultFile("www/index.html");
    // Redirect "/favicon.ico" to "/www/favicon.ico"
    server.serveStatic("/favicon.ico", SPIFFS, "/www/favicon.ico");

    // Start mDNS responder with "esp" hostname (esp.local)
    MDNS.begin("esp");

    // Start the task handling websocket CLI output
    // TODO: Set reasonable stack size
    xTaskCreate(&wsSendCliLoop, "ws_cli_out", 8196, NULL, 5, NULL);
}

/**
 * Send text
 * 
 * Send CLI message JSON to the last connected websocket client.
 * Approximate max data length slightly below doc size (~4000).
 */
void wsSendText(const char * data) {
    if(currentClient && currentClient->status() == WS_CONNECTED) {
        // Allocate JSON document and add elements
        DynamicJsonDocument doc(4096);
        doc["type"] = "cli";
        doc["data"] = data;

        // Serialize document into a buffer
        String output;
        serializeJson(doc, output);

        // Send through websocket
        currentClient->text(output);
    }
}

/**
 * Send CLI loop
 * 
 * Loop to send wsOutString to the last connected websocket client.
 * Run in its own task.
 */
void wsSendCliLoop(void * pvParameter) {
    // Allocate JSON document to populate and send
    DynamicJsonDocument doc(4096);

    while (true) {
        // Wait 10ms
        vTaskDelay(10 / portTICK_RATE_MS);

        if(currentClient && currentClient->status() == WS_CONNECTED) {
            // Client is connected, take mutex and check string buffer
            xSemaphoreTake(wsOutMutex, portMAX_DELAY);
            if (wsOutString.empty()) {
                // Nothing to send
                xSemaphoreGive(wsOutMutex);
                continue;
            }

            // Add elements to JSON document and clear string buffer
            doc["type"] = "cli";            
            doc["data"] = wsOutString;
            wsOutString.clear();
            
            // Give back mutex
            xSemaphoreGive(wsOutMutex);

            // Serialize JSON document into a buffer and clear JSON doc
            String output;
            serializeJson(doc, output);
            doc.clear();

            // Send through websocket
            currentClient->text(output);
        } else {
            // If no client is connected, throw away buffered output
            xSemaphoreTake(wsOutMutex, portMAX_DELAY);
            wsOutString.clear();
            xSemaphoreGive(wsOutMutex);
        }
    }
}

/**
 * Send file list
 * 
 * Send file list JSON.
 */
void wsSendFileList() {
    if(currentClient && currentClient->status() == WS_CONNECTED) {
        // Allocate JSON document and add elements
        DynamicJsonDocument doc(2048);
        doc["type"] = "fileList";
        JsonArray paths = doc.createNestedArray("paths");

        // Fill array with file paths
        File root = SPIFFS.open("/");
	    File file = root.openNextFile();
        while(file) {
            // Cast to non-const char* to force copy
            paths.add((char*) file.name());
            file = root.openNextFile();
	    }

        // Serialize document into a buffer
        String output;
        serializeJson(doc, output);

        // Send through websocket
        currentClient->text(output);
    }
}

/**
 * Send acknowledge
 * 
 * Send (negative-) acknowledge JSON for various requests.
 */
void wsSendAck(const char * type, const char * status, const char * name) {
    if(currentClient && currentClient->status() == WS_CONNECTED) {
        // Allocate JSON document and add elements
        StaticJsonDocument<128> doc;
        doc["type"] = type;
        doc["status"] = status;
        doc["name"] = name;

        // Serialize document into a buffer
        String output;
        serializeJson(doc, output);

        // Send through websocket
        currentClient->text(output);
    }
}

/**
 * Remove file
 * 
 * Remove file in path location.
 * Return true if file is removed or didn't exist.
 * Return false if file is protected.
 * 
 * Might complain repeatedly on multi-frame upload to a protected location.
 * Such is life.
 */
bool removeFile(const char * path) {
    // Do not remove protected file paths
    if (std::find(corePaths.begin(), corePaths.end(), path) != corePaths.end()) {
        multiPrintf("ILLEGAL: Cannot modify core file \"%s\"", path);
        return false;
    }

    // Remove existing file
    if (SPIFFS.exists(path)) {
        SPIFFS.remove(path);
    }
    return true;
}

/**
 * Incoming data
 * 
 * Save currentLen of data frames from websocket in SPIFFS.
 * Expects total length of all frames from totalLen.
 * Incoming file is stored under 'uploadPath'.
 */
void incomingData(uint8_t * inputData, size_t currentLen, size_t totalLen) {
    // If this is the first frame (nothing written so far)
    if (uploadBytesWritten == 0) {
        // For safety, try closing potentially open file
        uploadFileHandle.close();

        // Remove old file if exists, return if it's protected from modifying
        if(!removeFile(uploadPath)) {
            // Invalidate uploadPath
            uploadPath[1] = '\0';
            return;
        }

        // Open file for writing
        uploadFileHandle = SPIFFS.open(uploadPath, FILE_WRITE);
        if (!uploadFileHandle) { 
            multiPrintf("Failed to open file \"%s\" for writing.\n", uploadPath);
            return; 
        }
    }

    // Write data to open file and count written bytes
	uploadBytesWritten += uploadFileHandle.write(inputData, currentLen);

    // If this was the last frame (all bytes written), close file
    if (uploadBytesWritten == totalLen) {
        uploadFileHandle.close();
        // TODO: Send JSON acknowledge?
        multiPrintf("Upload successful: \"%s\" (%d bytes).\n", uploadPath, totalLen);

        // Reset counter
        uploadBytesWritten = 0;
    }
}

/**
 * Incoming JSON CLI
 * 
 * Handle incoming CLI input.
 */
void incomingJsonCli(StaticJsonDocument<STATIC_JSON_SIZE> & doc) {
    std::string data = doc["data"];
    incomingText(&data[0]);
}

/**
 * Incoming JSON file list
 * 
 * Handle incoming file list request.
 */
void incomingJsonFileList(StaticJsonDocument<STATIC_JSON_SIZE> & doc) {
    multiPrintf("DEBUG: Requested file list\n");
    wsSendFileList();
}

/**
 * Incoming JSON upload
 * 
 * Handle incoming file upload request.
 */
void incomingJsonUpload(StaticJsonDocument<STATIC_JSON_SIZE> & doc) {
    // Compare file size with available space in SPIFFS
    size_t fileSize = doc["size"];
    // TODO: fix free space calculation
    size_t freeStorage = SPIFFS.totalBytes() - SPIFFS.usedBytes();
    if (fileSize > freeStorage) {
        wsSendAck("upload", "tooLarge", "");
        return;
    }

    // Get file path
    const char * filePath = (const char*) doc["name"];
    // Check file path length (SPIFFS limits to 31)
    if (sizeof(filePath) > 31) {
        multiPrintf("TRUNCATED FILEPATH: \"%s\" was longer than 31 characters", filePath);
    }
    // Update global path variable
    strncpy(uploadPath, filePath, 31);

    // Send approval of download
    wsSendAck("upload", "ready", uploadPath);
    multiPrintf("DEBUG: Ready to receive file: %s\n", uploadPath);
}

/**
 * Incoming JSON delete
 * 
 * Handle incoming file deletion request.
 */
void incomingJsonDelete(StaticJsonDocument<STATIC_JSON_SIZE> & doc) {
    // Get file path
    const char * path = (const char*) doc["path"];
    if (removeFile(path)) {
        // File is no more
        wsSendAck("delete", "ok", path);
        multiPrintf("DEBUG: File \"%s\" deleted\n", path);
    } else {
        // Couldn't delete protected file
        wsSendAck("delete", "protected", path);
    }
}

/**
 * Incoming JSON kill
 * 
 * Handle incoming ATLAST kill request.
 */
void incomingJsonKill(StaticJsonDocument<STATIC_JSON_SIZE> & doc) {
    // Get ATLAST task restart parameter
    bool restartTask = doc["restartTask"];
    multiPrintf("DEBUG: restartTask %d\n", restartTask);

    atlastKill(restartTask);
}

/**
 * Incoming JSON
 * 
 * Parse and handle incoming JSON document.
 * Maximum input length is 256.
 */
void incomingJson(const char* inputData) {
    // Deserialize received JSON document
    StaticJsonDocument<STATIC_JSON_SIZE> doc;
    DeserializationError err = deserializeJson(doc, inputData);
    if (err) {
        multiPrintf("JSON deserialization error: %s\n", err);
        return;
    }

    // Decide message type
    if (doc["type"] == "cli") {
        incomingJsonCli(doc);
    } else if (doc["type"] == "fileList") {
        incomingJsonFileList(doc);
    } else if (doc["type"] == "upload") {
        incomingJsonUpload(doc);
    } else if (doc["type"] == "delete") {
        incomingJsonDelete(doc);
    } else if (doc["type"] == "kill") {
        incomingJsonKill(doc);
    }
}