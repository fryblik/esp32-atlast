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
 * Close file, reset Run Data and give back mutex.
 */
void resetAtlastRun() {
    rd.isRunning = false;
    rd.killFlag = false;
    xSemaphoreGive(atlastRunMutex);
}

/**
 * ATLAST program
 * 
 * Execute ATLAST commands when available.
 * Run in a separate task.
 */
void atlastInterpreterLoop(void * pvParameter) {
    // TODO: atl_mark, atl_unwind
    // TODO: give mutex to allow adding commands to queue in ATLAST.C?

    // Wait for execution
    while(true) {
        // If not ready to start, try again
        if (!startAtlastRun())
            continue;

        // Execute commands stored in Run Data
        while (!rd.commands.empty()) {
            // On KILL flag abort execution
            if (rd.killFlag){
                atl_eval("ABORT");
                // TODO: Implement kill also inside running ATLAST loop (atl_break)
                break;
            }

            // Evaluate and pop string in front of queue and print response
            atl_eval(&rd.commands.front()[0]);
            rd.commands.pop();
            multiPrintf("\n  ok\n");
            
        }

        // Reset run data and give back mutex
        resetAtlastRun(); 
    }
}

/**
 * Evaluate ATLAST
 * 
 * Evaluate ATLAST command.
 */
void atlastCommand(char* command) {
    // TODO: kill (BREAK)

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
 * Init Atlast
 * 
 * Initiate Atlast and create interpreter task.
 */
void initAtlast() {
    // Initialize ATLAST interpreter
    //atl_init(); // Redundant with PROLOGUE package active

    // DEBUG: List SPIFFS files in CLI
    //printFileList();

    // Create ATLAST interpreter task
    xTaskCreate(&atlastInterpreterLoop, "atl_from_file", 65536, NULL, 5, NULL);

    // Run ATLAST source file "/atl/run-on-startup.atl"
    xSemaphoreTake(atlastRunMutex, portMAX_DELAY);
    rd.commands.push("1 trace"); // DEBUG
    rd.commands.push("file startupfile");
    rd.commands.push("\"/atl/run-on-startup.atl\" 1 startupfile fopen");
    rd.commands.push("startupfile fload");
    rd.commands.push("startupfile fclose");
    rd.startFlag = true;
    xSemaphoreGive(atlastRunMutex);
}

// TODO: Outgoing websocket queue is limited to 16 messages -> buffering needed (try WORDSUNUSED to see behavior)