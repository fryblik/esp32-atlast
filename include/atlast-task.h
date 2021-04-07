#include <Arduino.h>
#include <SPIFFS.h>

#define CODE_BUFF_SIZE 1000


// Run data
struct runData {
    String filename;
    File codeFile;
    char codeBuff[CODE_BUFF_SIZE]; // buffer for current line of code
    bool startFlag;
    volatile bool killFlag;
    bool isRunning;
};
extern struct runData rd;

// Run data mutex
extern SemaphoreHandle_t atlastRunMutex;


/**
 * ATLAST program
 * 
 * Execute ATLAST code from file in own task.
 */
void atlastFromFile(void * pvParameter);

/**
 * Evaluate ATLAST
 * 
 * Evaluate ATLAST command.
 */
void atlastCommand(char* command);