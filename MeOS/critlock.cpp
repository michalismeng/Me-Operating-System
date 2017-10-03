#include "critlock.h"
#include "thread_sched.h"

void critlock_acquire()
{
	thread_get_current()->thread_lock = THREAD_LOCK_CRITICAL;
}

void critlock_release()
{
	INT_OFF;

	bool yield = thread_get_current()->thread_lock & THREAD_LOCK_YIELD;
	thread_get_current()->thread_lock = THREAD_LOCK_NONE;

	INT_ON;

	if (yield)
		thread_current_yield();
}
