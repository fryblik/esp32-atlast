#include "atlast-1.2-esp32/atlast.h"
#include "atlast-prims.h"
#include "atlast-task.h"
#include "multi-io.h"


// Run Data
struct runData rd;

// Run Data mutex and task handle
// TODO: rework to use faster task notifications instead?
SemaphoreHandle_t atlastRunMutex = xSemaphoreCreateMutex();
TaskHandle_t atlastTaskHandle = nullptr;


/**
 * Start ATLAST run
 * 
 * Check start flag. If true, start execution. Otherwise wait a bit.
 * Returns true on program start, false otherwise.
 */
bool startAtlastRun() {
    // Take mutex
    xSemaphoreTake(atlastRunMutex, portMAX_DELAY);    

    if (rd.startFlag) {
        // Start execution and release mutex
        rd.startFlag = false;
        rd.isRunning = true;
        xSemaphoreGive(atlastRunMutex);
        return true;
    } else {
        // Release mutex and wait for next try
        xSemaphoreGive(atlastRunMutex);
        vTaskDelay(100 / portTICK_RATE_MS);
        return false;
    }
}

/**
 * Reset ATLAST run
 * 
 * Reset Run Data and release mutex.
 */
void resetAtlastRun() {
    // Reset queue if nonempty
    if (!rd.commands.empty()) {
        std::queue<std::string> emptyQueue;
        std::swap(rd.commands, emptyQueue);
    }
    rd.startFlag = false;
    rd.killFlag = false;
    rd.isRunning = false;
    xSemaphoreGive(atlastRunMutex);
}

/**
 * ATLAST interpreter loop
 * 
 * Execute ATLAST commands when available.
 * Blocks atlastRunMutex.
 * Run in a separate task.
 */
void atlastInterpreterLoop(void * pvParameter) {
    // TODO: atl_load, atl_mark, atl_unwind

    // Wait for execution
    while(true) {
        // If not ready to start, waits and tries again
        if (!startAtlastRun())
            continue;
        
        // Take mutex
        xSemaphoreTake(atlastRunMutex, portMAX_DELAY); 

        // Execute commands stored in Run Data
        while (!rd.commands.empty()) {
            // On KILL flag stop execution and reset Run Data
            if (rd.killFlag) {
                break;
            }

            // Get pointer to the string in front of queue
            char *command = &rd.commands.front()[0];

            // Release mutex during execution to allow addition of commands to queue
            // DIRTY: rd.commands.pop() must not be called in the meantime!!!
            // TODO: Maybe use strcpy instead of pointing to string in queue?
            xSemaphoreGive(atlastRunMutex);

            // Evaluate string in front of queue
            atl_eval(command);

            // Take mutex again
            xSemaphoreTake(atlastRunMutex, portMAX_DELAY);

            // Remove front string from queue
            rd.commands.pop();

            // Print acknowledgement of executed command
            multiPrintf("\n  ok\n");
        }

        // Reset Run Data and release mutex
        resetAtlastRun(); 
    }
}

/**
 * ATLAST command
 * 
 * Evaluate ATLAST command.
 */
void atlastCommand(char* command) {
    // Take mutex
    xSemaphoreTake(atlastRunMutex, portMAX_DELAY);

    // Print incoming command
	multiPrintf("> %s\n", command);
    // TODO: remove this
    multiPrintf("DEBUG: Queue size: %d, start: %d, kill: %d, running: %d\n", rd.commands.size(), rd.startFlag, rd.killFlag, rd.isRunning);
    // Append command to Run Data and start execution
    rd.commands.push(command);
    if (!rd.isRunning) {
        rd.startFlag = true;
    }

    // Release mutex
    xSemaphoreGive(atlastRunMutex);
}

/**
 * ATLAST create task
 */
void atlastCreateTask() {
    // Create ATLAST interpreter task
    xTaskCreate(&atlastInterpreterLoop,
                ATL_TASK_NAME,
                65536, // TODO: Set reasonable stack size
                NULL,
                5,
                &atlastTaskHandle); // Store task handle
}

/**
 * ATLAST init
 * 
 * Initiate Atlast and create interpreter task.
 */
void atlastInit() {
    // Need to explicitly initialize ATLAST before extending dictionary
    atl_init();

    // Extend ATLAST dictionary with custom word definitions
    atlastAddPrims();

    // Create ATLAST interpreter task
    atlastCreateTask();

    // Run ATLAST source file "/atl/run-on-startup.atl"
    xSemaphoreTake(atlastRunMutex, portMAX_DELAY);
    rd.commands.push("file startupfile");
    rd.commands.push("\"/atl/run-on-startup.atl\" 1 startupfile fopen");
    rd.commands.push("startupfile fload");
    rd.commands.push("startupfile fclose");
    rd.startFlag = true;
    xSemaphoreGive(atlastRunMutex);
}

/**
 * ATLAST kill
 * 
 * Sets KILL flag to reset Run Data.
 * Breaks running ATLAST program.
 * Restarts ATLAST in new task if requested.
 * Does not call ATLAST ABORT (that would reset atlast.c state itself).
 */
void atlastKill(bool restartTask) {
    if (rd.isRunning) {
        // Set KILL flag for interpreter task
        rd.killFlag = true;

        // Set BREAK flag for atl_exec
        atl_break();
    }

    // Restart ATLAST task if requested
    if (restartTask) {
        // Prevent passing NULL below (would suspend and delete calling task)
        if (!atlastTaskHandle) {
            multiPrintf("ERROR: Task handle is NULL, cannot restart task.\n");
            return;
        }

        // Suspend and delete task
        vTaskSuspend(atlastTaskHandle);
        vTaskDelete(atlastTaskHandle);

        // Wait for the task to be deleted
        do {
            vTaskDelay(50 / portTICK_PERIOD_MS);
        } while (!strcmp(pcTaskGetTaskName(atlastTaskHandle), ATL_TASK_NAME));

        resetAtlastRun();
        atlastCreateTask();

        // Clear return stack before anything else (keeps data stack)
        rd.commands.push("quit");
        rd.startFlag = true;
    }
}