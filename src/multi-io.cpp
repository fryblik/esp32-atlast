#include <stdio.h>
#include <stdarg.h>
#include <string>

#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

#include "atlast-1.2-esp32/atlast.h"
#include "atlast-task.h"
#include "multi-io.h"
#include "webserver.h"

/**
 * Multi printf
 * 
 * Formatted printing to multiple outputs.
 */
int multiPrintf(char* format, ...){
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
void incomingText(char* inputData) {
	// Pass command to ATLAST interpreter
	atlastCommand(inputData);
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