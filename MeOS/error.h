#ifndef ERROR_H_16032017
#define ERROR_H_16032017

#include "thread_sched.h"

/* TODO: Define all the valid error codes to standardize them throughout the kernel */

/* The error lives in the thread stack so it is accessible from user-land */

// sets the most recent error for the currently executing thread
void set_last_error(uint32 error);

// retrieves the most recent error for the currently executing thread
uint32 get_last_error();

#endif