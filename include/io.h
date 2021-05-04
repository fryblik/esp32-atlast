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

#ifdef __cplusplus  // Certain functions are included by C files

#include <ArduinoJson.h>
#include <stdint.h>

#define STATIC_JSON_SIZE 384


extern "C" {
#endif // __cplusplus

/**
 * Multi printf
 * 
 * Print formatted data to multiple outputs.
 */
int multiPrintf(char * format, ...);

/**
 * Scan I2C
 * 
 * Scan and print addresses of responding I2C devices.
 * Expects 7-bit address space.
 */
void scanI2C();

/**
 * Write I2C
 * 
 * Write data to I2C slave at specified 7-bit address.
 */
uint8_t writeI2C(uint8_t address, uint8_t * data, size_t len);

/**
 * Read I2C
 * 
 * Request and read data from I2C slave at specified 7-bit address.
 */
void readI2C(uint8_t address, uint8_t * data, size_t len);

#ifdef __cplusplus
}


/**
 * Serial read line
 * 
 * Store chars from serial input into provided buffer.
 * Loops until newline is read or when size limit is met.
 * Returns true if a complete line has been read, false otherwise.
 * WARNING: Discards the rest of incoming UART buffer!
 */
bool serialReadLine(char * buf, size_t limit);

/**
 * Print file list
 * 
 * List all files in CLI.
 */
void printFileList();

/**
 * Incoming text
 * 
 * Handle input string from serial or websocket (evaluate ATLAST).
 * Maximum input length is 256.
 */
void incomingText(char * inputData);

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
bool removeFile(const char * path);

/**
 * Incoming data
 * 
 * Save received part of file from websocket in SPIFFS.
 * Incoming file is stored under 'uploadPath'.
 */
void incomingData(void * arg, uint8_t * data, size_t dataLen);

/**
 * Incoming JSON CLI
 * 
 * Handle incoming CLI input.
 */
void incomingJsonCli(StaticJsonDocument<STATIC_JSON_SIZE> & doc);

/**
 * Incoming JSON file list
 * 
 * Handle incoming file list request.
 */
void incomingJsonFileList(StaticJsonDocument<STATIC_JSON_SIZE> & doc);

/**
 * Incoming JSON upload
 * 
 * Handle incoming file upload request.
 */
void incomingJsonUpload(StaticJsonDocument<STATIC_JSON_SIZE> & doc);

/**
 * Incoming JSON delete
 * 
 * Handle incoming file deletion request.
 */
void incomingJsonDelete(StaticJsonDocument<STATIC_JSON_SIZE> & doc);

/**
 * Incoming JSON
 * 
 * Parse and handle incoming JSON document.
 */
void incomingJson(const char* inputData);

#endif // __cplusplus