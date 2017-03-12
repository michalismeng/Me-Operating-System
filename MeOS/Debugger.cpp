#include "Debugger.h"

// function from kernel.cpp
KEYCODE getch();

void debugf(char * fmt, ...)
{
	va_list l;
	va_start(l, fmt);	// spooky stack magic going on here

	printfln(fmt, l);

	va_end(l);

	// wait until enter
	while (getch() != KEY_RETURN);
}