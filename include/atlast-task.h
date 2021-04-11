#include <Arduino.h>
#include <SPIFFS.h>
#include <queue>

#define CODE_BUFF_SIZE 1000


// Run Data
struct runData {
    std::queue<std::string> commands;
    bool startFlag;
    volatile bool killFlag;
    bool isRunning;
};
extern struct runData rd;

// Run Data mutex
extern SemaphoreHandle_t atlastRunMutex;


/**
 * ATLAST program
 * 
 * Execute ATLAST commands when available.
 * Run in a separate task.
 */
void atlastInterpreterLoop(void * pvParameter);

/**
 * ATLAST command
 * 
 * Evaluate ATLAST command.
 */
void atlastCommand(char* command);

/**
 * ATLAST init
 * 
 * Initiate Atlast and create interpreter task.
 */
void atlastInit();

/**
 * ATLAST kill
 * 
 * Sets KILL flag to clear interpreter command queue.
 * Breaks running ATLAST program.
 * Does not call ATLAST ABORT (that would reset ATLAST interpreter).
 */
void atlastKill();