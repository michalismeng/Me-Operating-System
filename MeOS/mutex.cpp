#include "mutex.h"

void mutex_init(mutex* m)
{
	semaphore_init(&m->binary_sem, 1);
}

void mutex_acquire(mutex* m)
{
	semaphore_wait(&m->binary_sem);
}

bool mutex_try_acquire(mutex* m)
{
	PANIC("Not implemented");
	/*INT_OFF;

	if (m->lock == 1)
	{
		INT_ON;
		return false;
	}
	else
	{
		m->lock = 1;
		INT_ON;
		return true;
	}*/

	return false;
}

void mutex_release(mutex* m)
{
	semaphore_signal(&m->binary_sem);
}