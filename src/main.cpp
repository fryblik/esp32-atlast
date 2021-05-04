/* This file is part of Interactive Atlast Forth Interpreter For ESP32.
 * Copyright (C) 2021  Vojtech Fryblik <433796@mail.muni.cz>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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