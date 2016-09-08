#ifndef QUEUE_H_2092016
#define QUEUE_H_2092016

// contains c++ code

#include "types.h"
#include "utility.h"
#include "memory.h"

template<class T>
struct queue_node
{
	T data;
	queue_node* next;
};

template<class T>
struct queue
{
	queue_node<T>* head;
	queue_node<T>* tail;
	uint32 count;	// items in queue
};

template<class T>
void queue_init(queue<T>* q)
{
	q->count = 0;
	q->head = q->tail = 0;
}

template<class T>
void queue_remove(queue<T>* q)
{
	if (q->count == 0)
		return;

	queue_node<T>* temp = q->head;
	q->head = q->head->next;
	delete temp;
	q->count--;

	if (q->head == 0)		// 'count' was 1 and now head looks at a nullptr so adjust tail too
		q->tail = 0;
}

template<class T>
void queue_insert(queue<T>* q, const T& element)
{
	queue_node<T>* node = new queue_node<T>;
	node->next = 0;
	node->data = element;

	if (q->count == 0)
		q->head = q->tail = node;
	else
	{
		q->tail->next = node;
		q->tail = node;
	}

	q->count++;
}

template<class T>
T queue_peek(queue<T>* q)
{
	if (q->count > 0)
		return q->head->data;
	return T();
}

template<class T>
T* queue_peek_ptr(queue<T>* q)
{
	if (q->count > 0)
		return &q->head->data;
	return 0;
}

#endif
