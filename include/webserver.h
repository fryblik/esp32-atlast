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

// Outgoing CLI text buffer and mutex
extern std::string wsOutString;
extern SemaphoreHandle_t wsOutMutex;

/**
 * Parse received
 * 
 * Parse data received through websocket.
 */
void parseReceived(void * arg, uint8_t * data, size_t len);

/**
 * Setup web server
 * 
 * Setup websocket, start the server, configure hosting files in SPIFFS.
 */
void setupWebServer();

/**
 * Send text
 * 
 * Send text message to the last connected websocket client.
 */
void wsSendText(const char * data);

/**
 * Send CLI loop
 * 
 * Loop to send wsOutString to the last connected websocket client.
 * Run in its own task.
 */
void wsSendCliLoop(void * pvParameter);

/**
 * Send file list
 * 
 * Send file list JSON for internal use of the web page.
 */
void wsSendFileList();

/**
 * Send acknowledge
 * 
 * Send (negative-) acknowledge JSON for various requests.
 */
void wsSendAck(const char * type, const char * status, const char * name);