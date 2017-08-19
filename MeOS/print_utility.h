#ifndef PRINT_UTILITY_H_19082017
#define PRINT_UTILITY_H_19082017

#include "types.h"
#include "utility.h"

void init_print_utility();

#ifdef __cplusplus
extern "C" {
#endif

void printf(char* fmt, ...);
void printfln(char* fmt, ...);

void PANIC(char* str);
void DEBUG(char* str);
void WARNING(char* str);
void ASSERT(bool expr);

#ifdef __cplusplus
}
#endif

#endif