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
 * I2CSCAN
 * Doesn't modify the stack, only prints the addresses.
 */
prim P_i2cscan() {
    scanI2C();
}

/**
 * I2C write
 * 
 * [byte]...[byte] [amount] [address] -> I2CWRITE -> [error]
 * Writes least byte of each of `amount` stack items to I2C slave at `address`.
 * Puts 0 on stack on success, non-0 otherwise.
 */
prim P_i2cwrite() {
    // Check for stack underflow (first 2 arguments)
    Sl(2);
    // Store and pop 2 stack values
    uint8_t address = S0;
    size_t amount = S1;
    Pop2;

    // Check for stack underflow (remaining arguments)
    Sl(amount);
    // Pop items from stack into a data buffer
    uint8_t data[amount];
    for (size_t i = 0; i < amount; i++) {
        data[i] = (uint8_t) S0; // Store least byte from each item
        Pop;
    }
    // Transmit and put return value on stack
    Push = writeI2C(address, data, amount);    
}

/**
 * I2C read
 * 
 * [amount] [address] -> I2CREAD -> [byte]...[byte]
 * Request and read `amount` of bytes from I2C slave at `address`.
 * Puts each byte on stack as a separate item, first received byte on top.
 */
prim P_i2cread() {
    // Check for stack underflow (2 arguments)
    Sl(2);
    // Store and pop 2 stack values
    uint8_t address = S0;
    size_t amount = S1;
    Pop2;

    // Check for stack overflow (number of bytes to read)
    So(amount);
    uint8_t data[amount];
    // Request and read data
    readI2C(address, data, amount);
    // Put data bytes as separate items on stack in reverse order 
    for (int i = amount - 1; i >= 0; i--) {
        Push = data[i];
    }
}

/**
 * Parse accelerometer data
 * 
 * [byte]×6 [range] -> PARSEACCEL
 * Converts and prints previously received MPU9250 accelerometer data.
 * Range: +-2/4/8/16g.
 * For presentation purposes only, this should really be done in Forth.
 */
prim P_parseaccel() {
    // Check for stack underflow (6+1 arguments)
    Sl(7);
    // Take range and pop stack once
    int range = S0;
    Pop;

    // Convert g values for the 3 axes
    float axesXYZ[3];
    for (int i = 0; i < 3; i++) {
        // Take and pop two byte items from stack
        uint8_t hiByte = S0;
        uint8_t loByte = S1;
        Pop2;
        // Glue them together
        int16_t raw = ((int16_t) hiByte) << 8 | loByte;
        // Convert and store
        axesXYZ[i] = ((float) raw) * range / (float) 0x8000;
    }

    // Print out the results
    multiPrintf("X: %f Y: %f Z: %f\n", axesXYZ[0], axesXYZ[1], axesXYZ[2]);
}

// Primitive definition table
static struct primfcn espPrims[] = {
    {"0PINM",       P_pinm},
    {"0PINW",       P_pinw},
    {"0PINR",       P_pinr},
    {"0DELAY_MS",   P_delay_ms},
    {"0UPTIME_MS",  P_uptime_ms},
    {"0UPTIME_S",   P_uptime_s},
    {"0FSSIZE",     P_fssize},
    {"0FSUSED",     P_fsused},
    {"0FSFREE",     P_fsfree},
    {"0I2CSCAN",    P_i2cscan},
    {"0I2CWRITE",   P_i2cwrite},
    {"0I2CREAD",    P_i2cread},
    {"0PARSEACCEL", P_parseaccel},
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