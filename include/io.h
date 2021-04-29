#ifdef __cplusplus  // C files only use multiPrintf() and scanI2C()

#include <ArduinoJson.h>
#include <stdint.h>

#define STATIC_JSON_SIZE 384

/**
 * Serial read line
 * 
 * Store chars from serial input into provided buffer.
 * Loops until newline is read or when size limit is met.
 * Returns true if a complete line has been read, false otherwise.
 * WARNING: Discards the rest of incoming UART buffer!
 */
bool serialReadLine(char * buf, size_t limit);


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

#ifdef __cplusplus
}

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