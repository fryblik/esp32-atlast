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