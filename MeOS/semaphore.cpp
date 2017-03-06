#include "semaphore.h"

void semaphore_init(semaphore* s, int32 initial_value)
{
	s->lock = initial_value;
	queue_init(&s->waiting_threads);
}

void semaphore_wait(semaphore* s)
{
	INT_OFF;

	while (s->lock <= 0)
	{
		queue_insert(&s->waiting_threads, thread_get_current());
		thread_block(thread_get_current());
	}

	s->lock--;

	INT_ON;
}

bool semaphore_try_wait(semaphore* s)
{
	INT_OFF;

	if (s->lock <= 0)
	{
		INT_ON;
		return false;
	}
	else
	{
		s->lock--;
		INT_ON;
		return true;
	}
}

void semaphore_signal(semaphore* s)
{
	INT_OFF;

	if (s->waiting_threads.count > 0)
	{
		// remove a thread from our qaiting queue
		TCB* temp = queue_peek(&s->waiting_threads);
		queue_remove(&s->waiting_threads);

		// notify the thread
		thread_notify(temp);
	}

	s->lock++;

	INT_ON;
}