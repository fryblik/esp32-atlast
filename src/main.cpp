#include <Arduino.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WebSocketsServer.h>

#include "atlast.h"
#include "atlast-words.h"
#include "multi-io.h"
#include "webserver.h"

// WLAN constants
const char* ssid = "Stable connection";
const char* password = "It'sGonnaWorkISwearIt'sGonnaWork";

// A buffer to hold incoming data
char inputString[256];
// Length of incomplete line stored in inputString
size_t filled = 0;

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

    // TODO: get wlan configuration from file

    // Connect to access point
    connectWLAN(ssid, password);

    // Start web server
    setupWebServer();

    // Start mDNS responder (esp.local)
    MDNS.begin("ESP");

    // Initialize FORTH interpreter
    atl_init();

    // Preload FORTH words
    preloadWords();

    // TODO DEBUG: List SPIFFS files in CLI
    printFileList();
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