#include <Arduino.h>
#include <stdlib.h>

#include "atlast-1.2-esp32/atldef.h"
#include "atlast-prims.h"

// NOTE: Do not forget to add definitions to the table in atlastAddPrims()!

/**
 * Set pin mode
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
 */
prim P_digw() {
    Sl(2);
    digitalWrite(S1, S0);
    Pop2;
}

/**
 * Read from pin
 */
prim P_digr() {
    Sl(1);
    stackitem s = digitalRead(S0);
    S0 = s;
}

/**
 * Delay in ms
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

// Primitive definition table
static struct primfcn espPrims[] = {
    {"0PINM",       P_pinm},
    {"0DIGW",       P_digw},
    {"0DIGR",       P_digr},
    {"0DELAY-MS",   P_delay_ms},
    {NULL,          (codeptr) 0}
};

/**
 * ATLAST add primitives
 * 
 * Extend ATLAST with custom word definitions.
 */
void atlastAddPrims() {
    // Add primitives to ATLAST dictionary
    atl_primdef(espPrims);
}


// TODO: use atl_primdef()
// TODO: remove ESP32_PRIM def from atlast.c & atlast.h & maybe elsewhere?