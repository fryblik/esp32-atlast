#include <ArduinoJson.h>

#define STATIC_JSON_SIZE 384

/**
 * Parse received
 * 
 * Parse data received through websocket.
 */
void parseReceived(void * arg, uint8_t *data, size_t len);

/**
 * Connect to WLAN
 */
void connectWLAN(const char* ssid, const char* password);

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

/**
 * Remove file
 * 
 * Remove file in path location.
 * Return true if file is removed or didn't exist.
 * Return false if file is protected.
 */
bool removeFile(const char * path);

/**
 * Incoming data
 * 
 * Save input data from serial or websocket (new SPIFFS file).
 * Maximum input length is 256.
 */
void incomingData(uint8_t* inputData, size_t len);

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
void incomingJson(const char* inputData, size_t len);