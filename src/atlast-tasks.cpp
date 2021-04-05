#include <Arduino.h>

#include "atlast-tasks.h"
#include "multi-io.h"

/**
 * ATLAST program
 * 
 * Execute ATLAST code from file in own task.
 */
void atlastFromFile(void * pvParameter) {
    // TODO: Initialize ATLAST

    // Wait for execution
    while(true) {
        // If not ready to start, try again
        if (!startAtlastFromFile())
            continue;

        // Execute code in file line by line
        rd.codeFile = SPIFFS.open(rd.filename);
        while (rd.codeFile.available()) {
            // Kill task?
            if (rd.killFlag)
                break;
                // TODO: Kill inside atlast.c

            // Read line of code and evaluate
            if (rd.codeFile.readBytesUntil('\n', rd.codeBuff, CODE_BUFF_SIZE) < CODE_BUFF_SIZE) {
                // TODO: Print line
                // TODO: Evaluate ATLAST
            } else {
                // Buffer overflow - exit program
                multiPrintf("ATLAST RUNTIME ERROR: Too long code line in %s. Exiting.\n", rd.filename);
                break;
            }
        }

        // Close file, reset run data and give back mutex
        resetAtlastFromFile();
  
        // TODO: Break spaghetti     
    }
}

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