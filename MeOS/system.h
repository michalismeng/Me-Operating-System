#ifdef __cplusplus
extern "C" {
#endif

#ifndef SYSTEM_H
#define SYSTEM_H

#include "types.h"

	uint8 __cdecl inportb(uint16 _port);
	uint16 __cdecl inportw(uint16 _port);
	uint32 inportl(uint16 _port);

	void __cdecl outportb(uint16 port, uint8 data);
	void __cdecl outportw(uint16 port, uint16 data);
	void outportl(uint16 port, uint32 data);

#define low_address(addr) (addr & 0x0000FFFF)
#define high_address(addr) (addr >> 16)

#endif

#ifdef __cplusplus
}
#endif