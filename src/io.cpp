#include <stdio.h>
#include <stdarg.h>
#include <string>

#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

#include "atlast-1.2-esp32/atlast.h"
#include "atlast-task.h"
#include "io.h"
#include "webserver.h"


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