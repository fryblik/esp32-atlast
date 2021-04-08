#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WebSocketsServer.h>

#include "atlast-1.2-esp32/atlast.h"
#include "atlast-task.h"
#include "multi-io.h"
#include "webserver.h"

// WLAN configuration
struct WlanConf {
    char ssid[33];
    char password[64];
} wlanConf;

// A buffer to hold incoming data
char inputString[256];
// Length of incomplete line stored in inputString
size_t filled = 0;


/**
 * Load config
 * 
 * Deserialize and load configuration from config.json.
 * Loads the first network configuration in the "networks" array.
 * Return true on success, false otherwise.
 */
bool loadConfig() {
    // Open configuration file
    File confFile = SPIFFS.open("/config.json");

    // Allocate JSON document and deserialize
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, confFile);
    if (error) {
        Serial.printf("Failed to deserialize config.json: %s\n", error.c_str());
        return false;
    }

    // Load the first WLAN in the "networks" array
    strcpy(wlanConf.ssid, doc["networks"][0]["ssid"]);
    strcpy(wlanConf.password, doc["networks"][0]["password"]);

    // TODO: Load multiple WLAN configurations

    return true;
}

/**
 * Read serial
 * 
 * Store chars from serial input into inputString buffer.
 * Returns true if a complete line has been read, false otherwise.
 */
bool readSerial() {
    while (Serial.available() && filled < 256) {
        // Read byte from buffer:
        char inChar = (char)Serial.read();

        // Add byte to inputString:
        inputString[filled] = inChar;
        
        // Terminate on newline:
        if (inChar == '\n' && filled > 0) {
            inputString[filled] = '\0';
            filled = 0;
            return true;
        }

        filled++;
    }
    return false;
}

void setup() {
    Serial.begin(115200);

    // Initialize SPIFFS filesystem
    if(!SPIFFS.begin()) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        abort();
    }

    // Load config.json
    // TODO: handle false return
    loadConfig();

    // Connect to access point
    connectWLAN(wlanConf.ssid, wlanConf.password);

    // Start web server
    setupWebServer();

    // Start mDNS responder (esp.local)
    MDNS.begin("ESP");

    // TODO: Make possible use without WLAN

    // Initialize ATLAST interpreter and create interpreter task
    initAtlast();
}

void loop() {
    // Looks like SerialEvent isn't supported
    if (Serial.available()) {
        if(readSerial()) {
            // Once a whole line is read, handle received data
            incomingText(inputString, strlen(inputString));
        }
    }

    // Prevent starvation
    yield();
}