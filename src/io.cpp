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

#include <ESPAsyncWebServer.h>
#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <SPIFFS.h>
#include <Wire.h>

#include "atlast-1.2-esp32/atlast.h"
#include "atlast-task.h"
#include "io.h"
#include "webserver.h"


// File upload: file path and handle
char uploadPath[32] = "/";
File uploadFileHandle;
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
 * Scan I2C
 * 
 * Scan and print addresses of responding I2C devices.
 * Expects 7-bit address space.
 */
void scanI2C() {
    bool found = false;
    multiPrintf("HEX: DEC:\n");

    // Iterate through valid slave addreses
    for (uint8_t address = 8; address < 120; address++) {
        // Probe address and check return state
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == I2C_ERROR_OK) {
            // Device found, print address in HEX and DEC
            multiPrintf("0x%02x  %3u\n", address, address);
            found = true;
        }
    }

    if (found) {
        multiPrintf("I2C scan completed.\n");
    } else {
        multiPrintf("No I2C device found.\n");
    }
}

/**
 * Write I2C
 * 
 * Write data to I2C slave at specified 7-bit address.
 */
uint8_t writeI2C(uint8_t address, uint8_t * data, size_t len) {
    Wire.beginTransmission(address);
    Wire.write(data, len);
    return Wire.endTransmission();
}

/**
 * Read I2C
 * 
 * Request and read data from I2C slave at specified 7-bit address.
 */
void readI2C(uint8_t address, uint8_t * data, size_t len) {
    // Request `len` amount of data
    Wire.requestFrom(address, len);

    size_t i = 0;
    uint8_t dataByte;

    // Read data while available, store the specified amount
    while (Wire.available()) {
        dataByte = Wire.read();
        if (i < len) {
            data[i] = dataByte;
            i++;
        }
    }
}

/**
 * Multi printf
 * 
 * Print formatted data to multiple outputs.
 */
int multiPrintf(char * format, ...){
    char buffer[256];

    // Format string
    va_list args;
    va_start(args, format);
    int length = vsprintf(buffer, format, args);
    va_end(args);

    // Output serial
    printf("%s", buffer);

    // Add string to outgoing websocket buffer
    xSemaphoreTake(wsOutMutex, portMAX_DELAY);
    wsOutString.append(buffer);
    xSemaphoreGive(wsOutMutex);

    return length;
}

/**
 * Serial read line
 * 
 * Store chars from serial input into provided buffer.
 * Loops until newline is read or when size limit is met.
 * Returns true if a complete line has been read, false otherwise.
 * WARNING: Discards the rest of incoming UART buffer!
 */
bool serialReadLine(char * buf, size_t limit) {
    // Length of incomplete line stored in buffer
    size_t len = 0;

    while (len < limit) {
        // Wait for input
        while (!Serial.available()) {
            delay(50);
        }

        // Read byte
        char newByte = (char) Serial.read();

        // Add byte to buffer, terminate on newline
        if (newByte != '\n' && newByte != '\r') {
            buf[len] = newByte;
        } else {
            buf[len] = '\0';
            // Dump unread bytes
            while (Serial.available()) {
                Serial.read();
            }
            return true;
        }

        len++;
    }

    // Buffer size limit reached, dump unread bytes
    while (Serial.available()) {
        Serial.read();
    }
    return false;
}

/**
 * Print file list
 * 
 * List all files in CLI.
 */
void printFileList() {
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
 
    while(file) {
        multiPrintf("%s\n", file.name());
        file = root.openNextFile();
    }
}

/**
 * Incoming text
 * 
 * Handle input string from serial or websocket (evaluate ATLAST).
 * Maximum input length is 256.
 */
void incomingText(char * inputData) {
    // Pass command to ATLAST interpreter (ignore empty string)
    if (inputData[0]) {
        atlastCommand(inputData);
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
 * Save received part of file from websocket in SPIFFS.
 * Incoming file is stored under 'uploadPath'.
 */
void incomingData(void * arg, uint8_t * data, size_t dataLen) {
    // Parse arguments
    // (won't compile with `#include <ESPAsyncWebServer.h>` in header file)
    AwsFrameInfo * info = (AwsFrameInfo*) arg;

    // If this is the start of a frame
    if (info->index == 0) {
        // Notify of the start of a frame
        multiPrintf("Upload pending: frame %u of \"%s\"...\n",
            info->num, uploadPath);

        // If this is the very first frame (see AwsFrameInfo.opcode)
        if (info->opcode == WS_BINARY) {
            // For safety, try closing potentially open file
            uploadFileHandle.close();
            // Remove old file if it exists, return if it's protected
            if(!removeFile(uploadPath)) {
                // Invalidate uploadPath
                uploadPath[1] = '\0';
                return;
            }
            // Open file for writing
            uploadFileHandle = SPIFFS.open(uploadPath, FILE_WRITE);
            if (!uploadFileHandle) { 
                multiPrintf("Upload error: "
                    "failed to open file \"%s\" for writing.\n", uploadPath);
                return; 
            }
        }
    }

    // Write data to open file and check the length written
    if (uploadFileHandle.write(data, dataLen) != dataLen) {
        // Notify of discrepancy
        multiPrintf("Upload error: \"%s\" not saved entirely!\n", uploadPath);
    }
    
    // If this was the last part of the last frame, close file
    if (info->final && info->index + dataLen == info->len) {
        uploadFileHandle.close();
        multiPrintf("Upload finished: \"%s\"\n", uploadPath);
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
    //multiPrintf("DEBUG: Requested file list\n");
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
    // 4096 = SPI flash block size
    size_t freeStorage = SPIFFS.totalBytes() - SPIFFS.usedBytes() - 4096;
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
    //multiPrintf("DEBUG: Ready to receive file: %s\n", uploadPath);
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
        //multiPrintf("DEBUG: File \"%s\" deleted\n", path);
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
    //multiPrintf("DEBUG: restartTask %d\n", restartTask);

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