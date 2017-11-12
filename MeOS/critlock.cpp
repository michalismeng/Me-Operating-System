#include "critlock.h"
#include "thread_sched.h"

extern uint32 in_critical_section;

void critlock_acquire()
{
	//thread_get_current()->thread_lock = THREAD_LOCK_CRITICAL;
	in_critical_section = 1;
}

void critlock_release()
{
	in_critical_section = 0;
	/*INT_OFF;

	bool yield = thread_get_current()->thread_lock & THREAD_LOCK_YIELD;
	thread_get_current()->thread_lock = THREAD_LOCK_NONE;

	INT_ON;

	if (yield)
		thread_current_yield();*/
}
