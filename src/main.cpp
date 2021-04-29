#include <Arduino.h>
#include <SPIFFS.h>
#include <Wire.h>

#include "atlast-task.h"
#include "io.h"
#include "webserver.h"
#include "wlan.h"

#define MY_SDA 19
#define MY_SCL 18


// A buffer to hold incoming data
char inputString[256];


void setup() {
    // Initialize UART
    Serial.begin(115200);

    // Initialize I2C
    Wire.begin(MY_SDA, MY_SCL);

    // Initialize SPIFFS filesystem
    if(!SPIFFS.begin()) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        abort();
    }

    // Connect to access point
    if (connectWlan()) {
        // If connected, start web server
        setupWebServer();
    }

    // Initialize ATLAST interpreter and create interpreter task
    atlastInit();
}

void loop() {
    // Read UART input
    if(Serial.available() && serialReadLine(inputString, 256)) {
        // Once a whole line is read, handle received data
        incomingText(inputString);
    }
}