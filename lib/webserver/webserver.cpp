#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

#include "multi-io.h"
#include "webserver.h"

// Webserver globals
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncWebSocketClient* currentClient;

// File path of next upload
char nextPath[32] = "/";
// File paths not to be deleted/overwritten
std::vector<std::string> corePaths {
    "/",
    "/index.html",
    "/jquery-3.5.1.js",
    "/ws.js",
    "/FileSaver.js",
    "/style.css",
    "/favicon.ico"
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
            client->text("{\"type\":\"cli\",\"data\":\"DEBUG: Websocket connection established.\"}");

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
            // Update current client (TODO: write to multiple clients at once?)
            currentClient = client;
            
            // Parse arguments
            AwsFrameInfo * info = (AwsFrameInfo*)arg;
            if(info->final && info->index == 0 && info->len == len) {
                // The whole message is in a single frame

                if (info->opcode == WS_TEXT) {
                    // Single-frame text (JSON)

                    // Verify input length (TODO: necessary?)
                    if (info->len > 250){
                        multiPrintf("DISCARDED INPUT: TOO LONG (%u > 250)\n", (size_t) info->len);
                        break;
                    }
                    // Handle received JSON
                    incomingJSON((char*) data, (size_t) info->len);

                } else if (info->opcode == WS_BINARY) {
                    // Single-frame binary data

                    // Handle received data
                    incomingData(data, (size_t) info->len);
                }
            } else {
                // Message is comprised of multiple frames or the frame is split into multiple packets
                // https://github.com/me-no-dev/ESPAsyncWebServer#async-websocket-event
                // TODO
            }
            break;
        } // End of scope for WS_EVT_DATA
            
        // For other events, do nothing
        case WS_EVT_ERROR:
        case WS_EVT_PONG:
        default:
            break;
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
 * Setup websocket, start the server, configure hosting files in SPIFFS.
 */
void setupWebServer() {
    // Setup websocket
    currentClient = nullptr;
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    // Start server
    server.begin();

    // Serve all files in SPIFFS
    // Request to the root serves "index.html"
    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
}

/**
 * Send text
 * 
 * Send CLI message JSON to the last connected websocket client.
 * Expects max data length: 256
 */
void wsSendText(const char * data) {
    if(currentClient && currentClient->status() == WS_CONNECTED) {
        // Allocate JSON document and add elements
        StaticJsonDocument<384> doc;
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
 * Save input data from serial or websocket (new SPIFFS file).
 */
void incomingData(uint8_t* inputData, size_t len) {
	// Incoming file is stored under 'nextPath'
	// TODO: append to existing file?

    // Remove potential existing file, return if it's protected from modifying
    if(!removeFile(nextPath)) {
        // Invalidate nextPath
        nextPath[1] = '\0';
        return;
    }

	// Open file and write data
	File file = SPIFFS.open(nextPath, FILE_WRITE);
	if (!file) { 
		multiPrintf("Failed to open file \"%s\" for writing.\n", nextPath);
		return; 
	}
	file.write(inputData, len);
	file.close();

    // TODO: Send JSON acknowledge?
	multiPrintf("Upload successful: \"%s\" (%d bytes).\n", nextPath, len);
}

/**
 * Incoming JSON
 * 
 * Parse and handle incoming JSON document.
 * Maximum input length is 256.
 */
void incomingJSON(const char* inputData, size_t len) {
    // Deserialize received JSON document
    StaticJsonDocument<384> doc;
    DeserializationError err = deserializeJson(doc, inputData);
    if (err) {
        multiPrintf("JSON deserialization error: %s\n", err);
        return;
    }

    // Decide message type
    if (doc["type"] == "cli") {
        // CLI input
        // TODO: length unneeded?
        incomingText(doc["data"], sizeof(doc["data"]));

    } else if (doc["type"] == "fileList") {
        // File list requested
        multiPrintf("DEBUG: Requested file list\n");
        wsSendFileList();

    } else if (doc["type"] == "upload") {
        // File upload requested

        // Check file size
        size_t fileSize = doc["size"];
        if (fileSize > 128) { // TODO: change size limit
            wsSendAck("upload", "tooLarge", "");
            return;
        }

        // Get file name
        const char * fileName = (const char*) doc["name"];
        // Check file name length (SPIFFS limits to 31)
        if (sizeof(fileName) > 30) {
            multiPrintf("TRUNCATED FILENAME: \"%s\" was longer than 30 characters", fileName);
        }
        // Update global path variable (preserve leading slash)
        strncpy(&nextPath[1], fileName, 30);

        // Send approval of download
        wsSendAck("upload", "ready", &nextPath[1]);
        multiPrintf("DEBUG: Ready to receive file: %s\n", nextPath);

    } else if (doc["type"] == "delete") {
        // File deletion request
        
        // Get file path
        const char * path = (const char*) doc["path"];
        if (removeFile(path)) {
            // File is no more
            wsSendAck("delete", "ok", &path[1]);
            multiPrintf("DEBUG: File \"%s\" deleted\n", path);
        } else {
            // Couldn't delete protected file
            wsSendAck("delete", "protected", &path[1]);
        }
    }
}