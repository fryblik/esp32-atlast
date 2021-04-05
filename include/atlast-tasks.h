#include <SPIFFS.h>

#define CODE_BUFF_SIZE 1000

#ifndef RUN_DATA
#define RUN_DATA

// Run data
struct runData {
    char filename[32];
    File codeFile;
    char codeBuff[CODE_BUFF_SIZE]; // buffer for current line of code
    bool startFlag = false;
    volatile bool killFlag = false;
    bool isRunning = false;
} rd;

// Run data mutex
// TODO: rework to use faster task notifications instead?
SemaphoreHandle_t atlastRunMutex = xSemaphoreCreateMutex();

#endif // RUN_DATA


/**
 * ATLAST program
 * 
 * Execute ATLAST code from file in own task.
 */
void atlastFromFile(void * pvParameter);

/**
 * Start ATLAST from file
 * 
 * Check start flag. If true, start execution. Otherwise wait a bit.
 * Keeps mutex on success.
 * Returns true on program start, false otherwise.
 */
bool startAtlastFromFile();

/**
 * Reset ATLAST from file
 * 
 * Close file, reset run data and give back mutex.
 */
void resetAtlastFromFile();