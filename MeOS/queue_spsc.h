#ifndef QUEUE_LF_24072017
#define QUEUE_LF_24072017

// contains c++ code

#include "types.h"
#include "utility.h"
#include "memory.h"

// single-producer, single-consumer, lock-free queue implemented as circular vector
// TODO: there must be bugs in here.
template<class T>
struct queue_spsc
{
	T* buffer;					// buffer that holds the items in queue
	uint32 max_count;			// maximum items in the buffer

	uint32 head_index;			// the head of the queue (owned by the consumer thread)
	uint32 tail_index;			// the tail of the queue (owned by the producer thread)
};

template<class T>
void queue_spsc_init(queue_spsc<T>* q, uint32 max_elements)
{
	q->head_index = q->tail_index = 0;
	q->max_count = max_elements;

	q->buffer = new T[max_elements];
}

template<class T>
bool queue_spsc_remove(queue_spsc<T>* q)
{
	if (q->head_index != q->tail_index)
	{
		q->head_index = (q->head_index + 1) % q->max_count;
		return true;
	}

	return false;
}

template<class T>
bool queue_spsc_insert(queue_spsc<T>* q, const T& element)
{
	if ((q->tail_index + 1) % q->max_count != q->head_index)
	{
		q->buffer[q->tail_index] = element;
		q->tail_index = (q->tail_index + 1) % q->max_count;
		return true;
	}

	return false;
}

template<class T>
T queue_spsc_peek(queue_spsc<T>* q)
{
	if (q->tail_index != q->head_index)
		return q->buffer[q->head_index];
	return T();
}

template<class T>
bool queue_spsc_is_empty(queue_spsc<T>* q)
{
	return (q->tail_index == q->head_index);
}

// clears the queue based on the caller type (required to mess with the right variable)
template<class T>
void queue_spsc_clear(queue_spsc<T>* q, bool reader_caller)
{
	if (reader_caller)
		q->head_index = q->tail_index;
	else
		q->tail_index = q->head_index;
}

#endif