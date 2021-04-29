#include <Arduino.h>
#include <esp_spiffs.h>
#include <stdlib.h>

#include "atlast-1.2-esp32/atldef.h"
#include "atlast-prims.h"
#include "io.h"

// NOTE: Do not forget to add definitions to the table in atlastAddPrims()!

/**
 * Set pin mode
 * 
 * [pin] [mode] -> PINM
 */
prim P_pinm() {
    // Check for stack underflow (need 2 arguments)
    Sl(2);
    // Run function using two items on stack
    pinMode(S1, S0);
    // Pop stack twice
    Pop2;
}

/**
 * Write into pin
 * 
 * [pin] [value] -> PINW
 */
prim P_pinw() {
    Sl(2);
    digitalWrite(S1, S0);
    Pop2;
}

/**
 * Read from pin
 * 
 * [pin] -> PINR -> [value]
 */
prim P_pinr() {
    Sl(1);
    stackitem s = digitalRead(S0);
    S0 = s;
}

/**
 * Delay in ms
 * 
 * [interval] -> DELAY_MS
 * 
 * Shorter periods would require busy wait.
 */
prim P_delay_ms() {
    Sl(1);
    // Prevent extreme task delay on negative argument
    if ((long) S0 >= 0) {
        delay(S0);
    }
    Pop;
}

/**
 * Uptime in milliseconds
 * 
 * UPTIME_MS -> [value]
 * 
 * UINT32 - wraps every 49 days.
 * DOT prints stack items as INT32 - overflow apparent on day 25.
 */
prim P_uptime_ms() {
    // Check for overflow and place timestamp on stack
    So(1);
    Push = xTaskGetTickCount();
}

/**
 * Uptime in seconds
 * 
 * UPTIME_S -> [value]
 */
prim P_uptime_s() {
    // Check for overflow, calculate timestamp and place on stack.
    So(1);
    int32_t time_s = esp_timer_get_time() / 1000000;
    Push = time_s;
}

/**
 * Filesystem size in bytes
 * 
 * FSSIZE -> [value]
 */
prim P_fssize() {
    // Check for overflow, get FS size and place on stack.
    So(1);
    size_t totalBytes, usedBytes;
    esp_spiffs_info(NULL, &totalBytes, &usedBytes);
    Push = totalBytes;
}

/**
 * Used storage in bytes
 * 
 * FSUSED -> [value]
 */
prim P_fsused() {
    // Check for overflow, get used space and place on stack.
    So(1);
    size_t totalBytes, usedBytes;
    esp_spiffs_info(NULL, &totalBytes, &usedBytes);
    Push = usedBytes;
}

/**
 * Free storage in bytes
 * 
 * FSFREE -> [value]
 */
prim P_fsfree() {
    // Check for overflow, calculate free space and place on stack.
    So(1);
    size_t totalBytes, usedBytes;
    esp_spiffs_info(NULL, &totalBytes, &usedBytes);
    size_t freeBytes = totalBytes - usedBytes;
    Push = freeBytes;
}

/**
 * I2C scanner
 * 
 * SCANI2C
 * Doesn't modify the stack, only prints the addresses.
 */
prim P_scani2c() {
    scanI2C();
}

// Primitive definition table
static struct primfcn espPrims[] = {
    {"0PINM",       P_pinm},
    {"0PINW",       P_pinw},
    {"0PINR",       P_pinr},
    {"0DELAY-MS",   P_delay_ms},
    {"0UPTIME_MS",  P_uptime_ms},
    {"0UPTIME_S",   P_uptime_s},
    {"0FSSIZE",     P_fssize},
    {"0FSUSED",     P_fsused},
    {"0FSFREE",     P_fsfree},
    {"0SCANI2C",    P_scani2c},
    {NULL,          (codeptr) 0}
};

/**
 * ATLAST add primitives
 * 
 * Extend ATLAST with custom primitive word definitions.
 */
void atlastAddPrims() {
    atl_primdef(espPrims);
}