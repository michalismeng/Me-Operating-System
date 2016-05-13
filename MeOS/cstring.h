#ifndef CSTRING_H
#define CSTRING_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

uint16 strlength(const char* ch);

uint8 strEqual(char* s1, char* s2);

#ifdef __cplusplus
}
#endif

#endif