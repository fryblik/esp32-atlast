/* This file is part of Interactive Atlast Forth Interpreter For ESP32.
 * Copyright (C) 2021  Vojtech Fryblik <433796@mail.muni.cz>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "atlast-1.2-esp32/atlast.h"
#include "atlast-prims.h"
#include "atlast-task.h"
#include "io.h"


// Run Data
struct runData rd;

// Run Data mutex and task handle
SemaphoreHandle_t atlastRunMutex = xSemaphoreCreateMutex();
TaskHandle_t atlastTaskHandle = nullptr;


/**
 * ATLAST start run
 * 
 * Check start flag. If true, start execution. Otherwise wait a bit.
 * Returns true on program start, false otherwise.
 */
bool atlastStartRun() {
    // Take mutex
    xSemaphoreTake(atlastRunMutex, portMAX_DELAY);    

    if (rd.startFlag) {
        // Start execution and release mutex
        rd.startFlag = false;
        rd.isRunning = true;
        xSemaphoreGive(atlastRunMutex);
        return true;
    } else {
        // Release mutex and wait 0.1s
        xSemaphoreGive(atlastRunMutex);
        vTaskDelay(100 / portTICK_RATE_MS);
        return false;
    }
}

/**
 * ATLAST reset run
 * 
 * Reset Run Data and release mutex.
 */
void atlastResetRun() {
    // Empty the queue if nonempty
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
 * Execute ATLAST commands from queue when available.
 * Blocks atlastRunMutex.
 * Run in a separate task.
 */
void atlastInterpreterLoop(void * pvParameter) {
    // Wait for execution
    while(true) {
        // If not ready to start, try again
        if (!atlastStartRun())
            continue;
        
        // Take mutex
        xSemaphoreTake(atlastRunMutex, portMAX_DELAY); 

        // Execute commands stored in Run Data
        while (!rd.commands.empty()) {
            // On KILL flag stop execution and reset Run Data
            if (rd.killFlag) {
                break;
            }

            // Get pointer to the string in front of the queue
            char *command = &rd.commands.front()[0];
            // Release mutex during execution to allow addition of commands to queue
            // Note: rd.commands.pop() must not be called in the meantime!
            xSemaphoreGive(atlastRunMutex);
            // Evaluate string in front of the queue
            atl_eval(command);

            // Take mutex again
            xSemaphoreTake(atlastRunMutex, portMAX_DELAY);
            // Remove front string from the queue
            rd.commands.pop();

            // Print acknowledgement of executed command
            multiPrintf("\n< ok\n");
        }

        // Reset Run Data and release mutex
        atlastResetRun(); 
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
    // Append command to Run Data and start execution, if needed
    rd.commands.push(command);
    if (!rd.isRunning) {
        rd.startFlag = true;
    }

    // Release mutex
    xSemaphoreGive(atlastRunMutex);
}

/**
 * ATLAST create task
 * 
 * Create ATLAST machine task.
 */
void atlastCreateTask() {
    xTaskCreate(&atlastInterpreterLoop,
                ATL_TASK_NAME,
                4096,   // Stack size
                NULL,
                5,  // Priority
                &atlastTaskHandle); // Store task handle
}

/**
 * ATLAST init
 * 
 * Initiate ATLAST and create interpreter task.
 */
void atlastInit() {
    // Need to explicitly initialize ATLAST before extending dictionary
    atl_init();

    // Extend ATLAST dictionary with custom word definitions
    atlastAddPrims();

    // Create ATLAST interpreter task
    atlastCreateTask();

    // Run ATLAST source file "/atl/pins.atl"
    xSemaphoreTake(atlastRunMutex, portMAX_DELAY);
    rd.commands.push(
        "file startupfile "     // Create file descriptor
        "\"/atl/pins.atl\" 1 startupfile fopen "    // Open file
        "startupfile fload "    // Execute file
        "startupfile fclose "   // Close file
        "clear"                 // Clear return values from stack
    );
    rd.startFlag = true;    // Star ATLAST machine
    xSemaphoreGive(atlastRunMutex);
}

/**
 * ATLAST kill
 * 
 * Sets KILL flag to reset Run Data.
 * Breaks running ATLAST program.
 * Restarts ATLAST in a new task if requested.
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

        atlastResetRun();
        atlastCreateTask();

        // Command to clear return stack (keeps data stack)
        rd.commands.push("quit");
        rd.startFlag = true;
    }
}