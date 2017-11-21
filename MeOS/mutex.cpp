#include "mutex.h"

void mutex_init(mutex* m)
{
	m->lock = 0;
	queue_init(&m->waiting_threads);
}

void mutex_acquire(mutex* m)
{
	INT_OFF;

	while (m->lock == 1)
	{
		//queue_insert(&m->waiting_threads, thread_get_current());
		//thread_block(thread_get_current());
	}

	m->lock = 1;

	INT_ON;
}

bool mutex_try_acquire(mutex* m)
{
	INT_OFF;

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
	}
}

void mutex_release(mutex* m)
{
	INT_OFF;

	if (m->waiting_threads.count > 0)
	{
		//thread_notify(queue_peek(&m->waiting_threads));
		queue_remove(&m->waiting_threads);
	}

	m->lock = 0;

	INT_ON;
}