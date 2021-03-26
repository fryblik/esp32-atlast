#include <stdio.h>
#include <stdarg.h>
#include <string>

#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

#include "atlast.h"
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

	// Output websocket
	wsSendText(buffer);

	return length;
}

/**
 * Incoming text
 * 
 * Handle input string from serial or websocket (evaluate ATLAST).
 * Maximum input length is 256.
 */
void incomingText(const char* inputData, size_t len) {
	// Copy received text into a null-terminated string
	char terminated_string[len];
	strncpy(terminated_string, inputData, len);
	terminated_string[len] = '\0';

	// Print incoming command
	multiPrintf("> %s\n", terminated_string);

	// Evaluate and print response
	atl_eval(terminated_string);
	multiPrintf("\n  ok\n");
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