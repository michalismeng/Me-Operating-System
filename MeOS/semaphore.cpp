#include "semaphore.h"

extern _thread_sched scheduler;

void semaphore_init(semaphore* s, int32 initial_value)
{
	s->lock = initial_value;
	dl_list_init(&s->waiting_threads);
}

void semaphore_wait(semaphore* s)
{
	auto thread = thread_get_current_node();
	INT_OFF;

	if (s->lock <= 0)
	{
		thread->data->state = THREAD_STATE::THREAD_BLOCK;
		dl_list_remove_node(&READY_QUEUE(thread_get_priority(thread->data)), thread);
		dl_list_insert_back_node(&s->waiting_threads, thread);
		thread_current_yield();
	}
	else
		s->lock--;

	INT_ON;
}

bool semaphore_try_wait(semaphore* s)
{
	PANIC("try wait is not implemented");
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
		// remove a thread from our qaiting queue and notify it
		TCB_node* temp = dl_list_remove_front_node(&s->waiting_threads);
		temp->data->state = THREAD_STATE::THREAD_READY;
		dl_list_insert_back_node(&READY_QUEUE(thread_get_priority(temp->data)), temp);
	}
	else
		s->lock++;

	INT_ON;
}