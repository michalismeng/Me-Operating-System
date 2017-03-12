#ifndef DEBUGGER_H_12032017
#define DEBUGGER_H_12032017

#include "types.h"
#include "utility.h"
#include "keyboard.h"

// prints the given formatted text with a new line appended and awaits for Enter key press
void debugf(char* fmt, ...);

#endif