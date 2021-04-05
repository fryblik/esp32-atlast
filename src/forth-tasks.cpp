#include <Arduino.h>
#include <SPIFFS.h>

#define CODE_BUFF_SIZE 1000

// Run data
struct runData {
    char filename[32];
    char codeBuff[CODE_BUFF_SIZE]; // buffer for current line of code
    bool startFlag = false;
    bool killFlag = false;
    bool isRunning = false;
} rd;

// Run data mutex
// TODO: rework to use faster task notifications instead?
SemaphoreHandle_t forthRunMutex = xSemaphoreCreateMutex();

/**
 * FORTH program
 * 
 * Execute FORTH code from file in own task.
 */
void forthProgram(void * pvParameter) {
    // Wait for execution
    while(true) {
        // Take mutex
        xSemaphoreTake(forthRunMutex, portMAX_DELAY);

        // Start execution?
        if (rd.startFlag) {
            rd.startFlag = false;
            rd.isRunning = true;
        } else {
            // Give back mutex and wait some more
            xSemaphoreGive(forthRunMutex);
            vTaskDelay(100 / portTICK_RATE_MS);
            continue;
        }

        // Execute code in file line by line
        File codeFile = SPIFFS.open(rd.filename);
        while (codeFile.available()) {
            // Check for KILL flag
            if(!rd.killFlag) {
                // Read line of code and evaluate
                if(codeFile.readBytesUntil('\n', rd.codeBuff, CODE_BUFF_SIZE) < CODE_BUFF_SIZE) {
                    // TODO: Print line?
                    // TODO: Evaluate FORTH
                } else {
                    // TODO: Throw error for line buffer overflow
                }
            } else {
                // TODO: Kill task
                break;
            }

        }

        // Close file and give back mutex
        codeFile.close();
        xSemaphoreGive(forthRunMutex);

        // TODO: Reset run data   
        // TODO: Break spaghetti     
    }
}