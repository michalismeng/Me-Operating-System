#ifndef UTILITY_H
#define UTILITY_H

#include "types.h"
#include "screen.h"
#include "cstring.h"

#ifdef __cplusplus
extern "C" {
#endif

#define isdigit(arg) ( arg >= '0' && arg <= '9' ? 1 : 0 )
#define isalpha(arg) ( ((arg >= 'a' && arg <= 'z') || (arg >= 'A' && arg <= 'Z')) ? 1 : 0 )
#define isalnum(arg) ( (isdigit(arg) || isalpha(arg)) ? 1 : 0 )
#define isupper(arg) ( (arg >= 'A' && arg <= 'Z') ? 1 : 0 )
#define islower(arg) ( (arg >= 'a' && arg <= 'z') ? 1 : 0 )

#define isprint(arg) ( arg >= 0x1f && arg < 0x7f )

#define tolower(arg) (  isupper(arg) ? arg + 32 : arg )
#define toupper(arg) (  islower(arg) ? arg - 32 : arg )

	extern char hexes[];

	void uitoa(uint32 val, char* buffer, uint8 base);

	void itoa(int32 val, char* buffer, uint8 base);

	void printf(char* fmt, ...);
	void printfln(char* fmt, ...);

	void memset(void* base, uint8 val, uint32 length);

	uint32 atoui(char* buffer);
	uint32 atoui_dec(char* buffer, uint16 length);
	uint32 atoui_hex(char* buffer, uint16 length);

	uint32 pow(uint32 base, uint32 exp);
	uint32 ceil_division(uint32 value, uint32 divisor);

	void PANIC(char* str);
	void DEBUG(char* str);
	void WARNING(char* str);
	void ASSERT(bool expr);

#ifdef __cplusplus
}
#endif

#endif