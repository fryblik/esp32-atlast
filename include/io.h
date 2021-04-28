#include <stdint.h>

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
 * Multi printf
 * 
 * Print formatted data to multiple outputs.
 */
#ifdef __cplusplus
extern "C" {
#endif
    int multiPrintf(char * format, ...);
#ifdef __cplusplus
}
#endif

/**
 * Incoming text
 * 
 * Handle input string from serial or websocket (evaluate ATLAST).
 * Maximum input length is 256.
 */
void incomingText(char * inputData);

/**
 * Print file list
 * 
 * List all files in CLI.
 */
void printFileList();