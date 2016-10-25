#ifndef MUTEX_H_13102016
#define MUTEX_H_13102016

#include "system.h"
#include "types.h"
#include "utility.h"
#include "queue.h"
#include "process.h"

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