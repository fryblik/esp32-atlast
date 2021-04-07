#include "atlast-1.2/atlast.h"
#include "atlast-task.h"
#include "multi-io.h"


// Run data
struct runData rd;

// Run data mutex
// TODO: rework to use faster task notifications instead?
SemaphoreHandle_t atlastRunMutex = xSemaphoreCreateMutex();


/**
 * Start ATLAST from file
 * 
 * Check start flag. If true, start execution. Otherwise wait a bit.
 * Keeps mutex on success.
 * Returns true on program start, false otherwise.
 */
bool startAtlastFromFile() {
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
 * Reset ATLAST from file
 * 
 * Close file, reset run data and give back mutex.
 */
void resetAtlastFromFile() {
    rd.codeFile.close();
    rd.isRunning = false;
    rd.killFlag = false;
    xSemaphoreGive(atlastRunMutex);
}

/**
 * ATLAST program
 * 
 * Execute ATLAST code from file in own task.
 */
void atlastFromFile(void * pvParameter) {
    // TODO: atl_mark, atl_unwind

    // Wait for execution
    while(true) {
        // If not ready to start, try again
        if (!startAtlastFromFile())
            continue;

        // Execute code in file line by line
        rd.codeFile = SPIFFS.open(rd.filename);
        while (rd.codeFile.available()) {
            // On KILL flag abort execution
            if (rd.killFlag){
                atl_eval("ABORT");
                // TODO: Implement kill also inside running ATLAST loop (atl_break)
                break;
            }

            // Read line of code and evaluate
            size_t len = rd.codeFile.readBytesUntil('\n', rd.codeBuff, CODE_BUFF_SIZE);
            if (len < CODE_BUFF_SIZE) {
                // Terminate buffer
                rd.codeBuff[len] = '\0';

                // DEBUG: Print code line (use "1 TRACE"?)
                multiPrintf("<> %s\n", rd.codeBuff);

                // Evaluate and print response
                atl_eval(rd.codeBuff);
                multiPrintf("\n  ok\n");
            } else {
                // Buffer overflow - exit program
                multiPrintf("ATLAST RUNTIME ERROR: Too long code line in %s. Exiting.\n", rd.filename);
                break;
            }
        }
        // TODO: Wait for input before exiting without kill?

        // Close file, reset run data and give back mutex
        resetAtlastFromFile(); 
    }
}

/**
 * Evaluate ATLAST
 * 
 * Evaluate ATLAST command.
 */
void atlastCommand(char* command) {
    // TODO: kill (BREAK)
    // TODO: mutex
    // TODO: ATLAST from console, if program not running
    // TODO: (in which source file?) file management with mutex


    // TODO: replace using Run Data? old way:
	// Print incoming command
	multiPrintf("> %s\n", command);

	// Evaluate and print response
	atl_eval(command);
	multiPrintf("\n  ok\n");
}

// TODO: Move all ATLAST init from main.cpp here 