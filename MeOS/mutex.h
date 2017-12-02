#ifndef MUTEX_H_13102016
#define MUTEX_H_13102016

/* This primitive is to be replaced by the binary semaphore implementation */

#include "system.h"
#include "types.h"
#include "semaphore.h"

struct mutex
{
	semaphore binary_sem;
};

void mutex_init(mutex* m);

void mutex_acquire(mutex* m);
bool mutex_try_acquire(mutex* m);

void mutex_release(mutex* m);

#endif