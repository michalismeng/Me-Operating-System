#include "spinlock.h"

void spinlock_init(spinlock* lock)
{
	*lock = 0;
}

void spinlock_acquire(spinlock* lock)
{
	INT_OFF;

	while (*lock == 1)
		thread_sleep(thread_get_current(), 1);		// dummy sleep to loose a turn in the scheduling

	*lock = 1;		// lock acquired

	INT_ON;
}

bool spinlock_try_acquire(spinlock* lock)
{
	INT_OFF;

	if (*lock == 0)
	{
		INT_ON;
		*lock = 1;
		return true;
	}
	else
	{
		INT_ON;
		return false;
	}
}

void spinlock_release(spinlock* lock)
{
	*lock = 0;
}