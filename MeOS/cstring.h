#ifndef CSTRING_H
#define CSTRING_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

	// get the length of a string
	uint16 strlen(const char* ch);

	// compares two strings: -1 if first un-match of s1 is lower than s2, 0 if equal, 1 if first un-match of s1 is greater than s2
	int8 strcmp(const char* s1, const char* s2);

#ifdef __cplusplus
}
#endif

#endif