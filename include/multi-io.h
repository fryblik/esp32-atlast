#include <stdint.h>

/**
 * Multi printf
 * 
 * Formatted printing to multiple outputs.
 */
// http://sandsprite.com/blogs/index.php?uid=11&pid=304
// if you still need to use the original printf(),
// put this before the redefine:
//typedef int (*realprintf)(const char*, ...);
//realprintf real_printf = printf;
#ifdef __cplusplus
extern "C" {
#endif
    int multiPrintf(char* format, ...);
#ifdef __cplusplus
}
#endif

/**
 * Incoming text
 * 
 * Handle input string from serial or websocket (evaluate ATLAST).
 * Maximum input length is 256.
 */
void incomingText(char* inputData);

/**
 * Print file list
 * 
 * List all files in CLI.
 */
void printFileList();