#ifndef CSTRING_H
#define CSTRING_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

	// get the length of a string
	uint16 strlen(const char* ch);

	// compares two strings:
	//		-1 if first un-match of s1 is lower than s2
	//		0 if equal
	//		1 if first un-match of s1 is greater than s2
	int8 strcmp(const char* s1, const char* s2);

	// copies source string to destination. destination must be capable of holding the source string.
	void strcpy(char* destination, const char* source);

	// copies source string to destination. Checks if destination can hold the source string.
	uint8 strcpy_s(char* destination, uint16 destsz, const char* source);

	// returns a pointer to the first occurence of the 'c' character given
	char* strchr(const char* str, int c);

#ifdef __cplusplus
}
#endif

#endif