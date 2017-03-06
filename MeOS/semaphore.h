#ifndef SEMAPHORE_H_02012017
#define SEMAPHORE_H_02012017

#include "types.h"
#include "queue.h"
#include "thread_sched.h"

struct semaphore
{
	int32 lock;
	queue<TCB*> waiting_threads;
};

void semaphore_init(semaphore* s, int32 initial_value);

void semaphore_wait(semaphore* s);				// wait until lock is valid and decrement
bool semaphore_try_wait(semaphore* s);			// try to decrement the semaphore

void semaphore_signal(semaphore* s);			// increment semaphore lock

#endif