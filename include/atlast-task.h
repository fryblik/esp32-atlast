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
};
extern struct runData rd;

// Run Data mutex
extern SemaphoreHandle_t atlastRunMutex;
extern TaskHandle_t atlastTaskHandle;


/**
 * ATLAST start run
 * 
 * Check start flag. If true, start execution.
 * Returns true on program start, false otherwise.
 */
bool atlastStartRun();

/**
 * ATLAST reset run
 * 
 * Reset Run Data and release mutex.
 */
void atlastResetRun();

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
 * 
 * Create ATLAST machine task.
 */
void atlastCreateTask();

/**
 * ATLAST init
 * 
 * Initiate ATLAST and create interpreter task.
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