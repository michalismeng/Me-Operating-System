#ifndef QUEUE_MPMC_H_121122017
#define QUEUE_MPMC_H_121122017

#include "types.h"
#include "critlock.h"

// multi-producer, multi-consumer, lock-free queue implemented as circular vector
// uses critical section locking (disables scheduler)
template<class T>
struct queue_mpmc
{
	T* buffer;					// buffer that holds the items in queue
	uint32 max_count;			// maximum items in the buffer

	uint32 head_index;			// the head of the queue (owned by the consumer thread)
	uint32 tail_index;			// the tail of the queue (owned by the producer thread)
};

template<class T>
void queue_mpmc_init(queue_mpmc<T>* q, uint32 max_elements)
{
	q->head_index = q->tail_index = 0;
	q->max_count = max_elements;

	q->buffer = new T[max_elements];
}

template<class T>
bool queue_mpmc_remove(queue_mpmc<T>* q)
{
	bool result;

	critlock_acquire();

	if (q->head_index != q->tail_index)
	{
		q->head_index = (q->head_index + 1) % q->max_count;
		result = true;
	}
	else
		result = false;

	critlock_release();

	return result;
}

template<class T>
bool queue_mpmc_insert(queue_mpmc<T>* q, const T& element)
{
	bool result;

	critlock_acquire();

	if ((q->tail_index + 1) % q->max_count != q->head_index)
	{
		q->buffer[q->tail_index] = element;
		q->tail_index = (q->tail_index + 1) % q->max_count;
		result = true;
	}
	else
		result = false;

	critlock_release();

	return result;
}

template<class T>
T queue_mpmc_peek(queue_mpmc<T>* q)
{
	T object;

	critlock_acquire();

	if (q->tail_index != q->head_index)
		object =  q->buffer[q->head_index];

	critlock_release();

	return object;
}

template<class T>
bool queue_mpmc_is_empty(queue_mpmc<T>* q)
{
	bool result;

	critlock_acquire();

	result = (q->tail_index == q->head_index);

	critlock_release();

	return result;
}

// clears the queue based on the caller type (required to mess with the right variable)
template<class T>
void queue_mpmc_clear(queue_mpmc<T>* q, bool reader_caller)
{
	critlock_acquire();

	if (reader_caller)
		q->head_index = q->tail_index;
	else
		q->tail_index = q->head_index;

	critlock_release();
}

#endif