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
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <WiFi.h>

#include "io.h"
#include "wlan.h"

#define SSID_MAX_LEN 33
#define PSWD_MAX_LEN 64

// WLAN configuration
struct WlanConf {
    char ssid[SSID_MAX_LEN];
    char password[PSWD_MAX_LEN];
} wlanConf;


/**
 * Get WLAN config JSON
 * 
 * Deserialize and load WLAN configuration from config.json.
 * Loads the first network configuration in the "networks" array.
 * Return true on success, false otherwise.
 */
bool getWlanConfigJSON() {
    // Open configuration file
    File confFile = SPIFFS.open("/cfg/config.json");

    // Allocate JSON document and deserialize
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, confFile);
    if (error) {
        Serial.printf("Failed to deserialize config.json: %s\n", error.c_str());
        return false;
    }

    // Check that required keys exist in JSON
    if (!doc["networks"][0]["ssid"] || !doc["networks"][0]["password"]) {
        return false;
    }

    // Load the first WLAN in the "networks" array
    strcpy(wlanConf.ssid, doc["networks"][0]["ssid"]);
    strcpy(wlanConf.password, doc["networks"][0]["password"]);

    // TODO: Load multiple WLAN configurations?

    return true;
}

/**
 * Get WLAN config serial
 * 
 * Fill wlanConf structure with user-provided credentials.
 * Return true if user provided credentials, false otherwise.
 */
bool getWlanConfigSerial() {
    while (true) {
        yield();
        Serial.println("Enter SSID (or none to give up WLAN connection):\n");

        // Read SSID
        if (!serialReadLine(wlanConf.ssid, SSID_MAX_LEN)) {
            // Invalid read
            Serial.println("Invalid entry, try again.\n");
            continue;
        }

        // If SSID is empty, give up
        if (!wlanConf.ssid[0]) {
            return false;
        }

        Serial.println("Enter password:\n");

        // Read password
        if (!serialReadLine(wlanConf.password, PSWD_MAX_LEN)) {
            // Invalid read
            Serial.println("Invalid entry, try again.\n");
            continue;
        }

        // We've got what we asked for
        return true;
    }
}

/**
 * Connect to WLAN
 */
bool connectWlan() {
    // Load WLAN credential structure from config.json
    if (!getWlanConfigJSON()) {
        // If JSON fails, ask through serial
        if (!getWlanConfigSerial()) {
            // If that fails, give up
            return false;
        }
    }

    // Keep trying until connected or user decides to give up from within
    while (WiFi.status() != WL_CONNECTED) {
        // Connect to access point
        Serial.println("Connecting to WLAN");
        WiFi.begin(wlanConf.ssid, wlanConf.password);

        // Set 15s timeout
        TickType_t tOut = xTaskGetTickCount() + 15000 / portTICK_RATE_MS;

        // Try connecting until timeout
        while (WiFi.status() != WL_CONNECTED && xTaskGetTickCount() < tOut) {
            delay(500);
            Serial.print(".");
        }

        // If connection failed
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("\nFailed to connect to WLAN (timeout).\n");
            
            // Ask user for new credentials
            if (!getWlanConfigSerial()) {
                // User gave up on WLAN
                return false;
            }
        }
    }

    // Celebrate success, print IP address
    Serial.println("Connected!");
    Serial.print("My IP address: ");
    Serial.println(WiFi.localIP());

    return true;
}