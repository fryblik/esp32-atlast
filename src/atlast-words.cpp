#include "atlast-1.2/atlast.h"
#include "atlast-words.h"

/**
 * Preload words
 * 
 * Run these FORTH words.
 */
void preloadWords() {
    char *lines[] = {
        // blippy:
        "0 constant LOW",
        "1 constant HIGH", 
        "2 constant OUTPUT",
        "1 constant INPUT",
        "5 constant INPUT_PULLUP", 
        // Our green led:
        "22 constant G-LED",                            // Usage:
        ": output: create dup , output pinm does> @ ;", // g-led output: led
        ": on high digw ;",                             // led on
        ": off low digw ;",                             // led off
        0
    };
    char **line = lines;
    while(*line) {
        atl_eval(*line);
        line++;
    }
}