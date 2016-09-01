#ifndef SERIAL_DEBUGGER_H_25072016
#define SERIAL_DEBUGGER_H_25072016

#ifdef __cplusplus
extern "C" {
#endif

#define PORT 0x3f8
#define COM_IRQ 36

#include "system.h"
#include "utility.h"

	// this file is totally incomplete and serves as a quick way to dump text to a serial screen

	void init_serial();

	void serial_printf(char* fmt, ...);
	void serial_dump_mem(char* base, uint32 length);

#ifdef __cplusplus
}
#endif

#endif