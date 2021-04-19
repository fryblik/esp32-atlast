#include "atlast-1.2-esp32/atlast.h"
#include "atlast-task.h"
#include "multi-io.h"


// Run Data
struct runData rd;

// Run Data mutex
// TODO: rework to use faster task notifications instead?
SemaphoreHandle_t atlastRunMutex = xSemaphoreCreateMutex();


/**
 * Start ATLAST run
 * 
 * Check start flag. If true, start execution. Otherwise wait a bit.
 * Keeps mutex on success.
 * Returns true on program start, false otherwise.
 */
bool startAtlastRun() {
    // Take mutex
    xSemaphoreTake(atlastRunMutex, portMAX_DELAY);

    if (rd.startFlag) {
        // Start execution
        rd.startFlag = false;
        rd.isRunning = true;
        return true;
    } else {
        // Give back mutex and wait for next try
        xSemaphoreGive(atlastRunMutex);
        vTaskDelay(100 / portTICK_RATE_MS);
        return false;
    }
}

/**
 * Reset ATLAST run
 * 
 * Reset Run Data and give back mutex.
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
        // Mutex is taken!

        // Execute commands stored in Run Data
        while (!rd.commands.empty()) {
            // On KILL flag stop execution and reset Run Data
            if (rd.killFlag){
                break;
            }

            // Get pointer to the string in front of queue
            char *command = &rd.commands.front()[0];

            // Give mutex during execution to allow addition of commands to queue
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

        // Reset Run Data and give back mutex
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
    // Append command to Run Data and start execution
    rd.commands.push(command);
    rd.startFlag = true;

    // Give back mutex
    xSemaphoreGive(atlastRunMutex);
}

/**
 * ATLAST init
 * 
 * Initiate Atlast and create interpreter task.
 */
void atlastInit() {
    // Create ATLAST interpreter task
    // TODO: Set reasonable stack size
    xTaskCreate(&atlastInterpreterLoop, "atl_interpreter", 65536, NULL, 5, NULL);

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
 * Does not call ATLAST ABORT (that would reset atlast.c itself).
 */
void atlastKill() {
    // Set KILL flag for interpreter task
    rd.killFlag = true;
    // Set BREAK flag for running ATLAST program
    atl_break();
}