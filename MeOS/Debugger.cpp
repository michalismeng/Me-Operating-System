#include "Debugger.h"

void printf_base(char* fmt, va_list l);

// function from kernel.cpp
KEYCODE getch();

void debugf(char * fmt, ...)
{
	va_list l;
	va_start(l, fmt);	// spooky stack magic going on here

	printf_base(fmt, l);

	va_end(l);

	// wait until enter
	while (getch() != KEY_RETURN);
}