#ifndef MUTEX_H_13102016
#define MUTEX_H_13102016

/* This primitive is to be replaced by the binary semaphore implementation */

#include "system.h"
#include "types.h"
#include "utility.h"
#include "queue.h"
#include "thread_sched.h"

struct mutex
{
	uint32 lock;
	queue<TCB*> waiting_threads;
};

void mutex_init(mutex* m);

void mutex_acquire(mutex* m);
bool mutex_try_acquire(mutex* m);

void mutex_release(mutex* m);

#endif