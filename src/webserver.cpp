#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>

#include "atlast-task.h"
#include "io.h"
#include "webserver.h"


// Webserver globals
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncWebSocketClient* currentClient;

// Outgoing CLI text buffer and mutex
std::string wsOutString;
SemaphoreHandle_t wsOutMutex = xSemaphoreCreateMutex();


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
 */
void parseReceived(void * arg, uint8_t * data, size_t len) {
    // Parse arguments
    // NOTE:
    // AwsFrameInfo is confusing. Study the silly structure carefully.
    // Different uses of info->opcode and info->message_opcode:
    // https://github.com/me-no-dev/ESPAsyncWebServer#async-websocket-event
    AwsFrameInfo * info = (AwsFrameInfo*) arg;

    // Message is binary, either single- or multi-frame
    if (info->opcode == WS_BINARY ||
        (info->opcode == WS_CONTINUATION && info->message_opcode == WS_BINARY)) {

        // Handle received data
        incomingData(arg, data, len);
        return;
    }

    // Message is text (JSON), in a single frame, and this is all its data
    if (info->opcode == WS_TEXT && info->final && info->index == 0 && info->len == len) {
        // Verify input length (to protect statically allocated JSON deserializer)
        if (len > 250) {
            multiPrintf("DISCARDED INPUT: Too long (%u > 250).\n", len);
            return;
        }
        // Handle received JSON
        incomingJson((char*) data);
        return;

    } else if (info->message_opcode == WS_TEXT) {
        // Fragmented text (JSON)

        // TODO: Handle fragmented text data? So far not necessary.
        multiPrintf("DISCARDED INPUT: Multi-frame text data received.\n");
    }
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