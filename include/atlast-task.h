#include <Arduino.h>
#include <SPIFFS.h>
#include <queue>

#include "atlast-1.2-esp32/atldef.h"

#define ATL_TASK_NAME "atl"


// Run Data
struct runData {
    std::queue<std::string> commands;
    bool startFlag;
    volatile bool killFlag;
    bool isRunning;
    atl_statemark mk;   // TODO: unused
};
extern struct runData rd;

// Run Data mutex
extern SemaphoreHandle_t atlastRunMutex;
extern TaskHandle_t atlastTaskHandle;


/**
 * ATLAST interpreter loop
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
 * ATLAST create task
 */
void atlastCreateTask();

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
 * Restarts ATLAST in new task if requested.
 * Does not call ATLAST ABORT (that would reset ATLAST interpreter).
 */
void atlastKill(bool restartTask);