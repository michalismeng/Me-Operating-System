#ifndef LIST_H_29092016
#define LIST_H_29092016

// contains c++ code

#include "types.h"
#include "utility.h"
#include "memory.h"

#define LIST_PEEK(l) l->head->data

template<class T>
struct list_node
{
	T data;
	list_node* next;
};

template<class T>
struct list
{
	list_node<T>* head;
	list_node<T>* tail;
	uint32 count;		// items in list
};

template<class T>
void list_init(list<T>* l)
{
	l->count = 0;
	l->head = l->tail = 0;
}

template<class T>
list_node<T>* list_get_prev(list<T>* l, T item)
{
	if (l->count == 0)
		return 0;

	if (l->count == 1)
	{
		return l->head;
	}

	list_node<T>* temp = l->head;

	while (temp != 0)
	{
		if (temp->next->data == item)
			return temp;

		temp = temp->next;
	}

	return 0;
}

template<class T>
bool list_remove(list<T>* l, list_node<T>* prev)
{
	if (prev == 0)
		return false;

	if (l->count == 0)
		return false;

	if (l->count == 1)		// prev == head
	{
		l->count--;
		delete prev;
		l->head = l->tail = 0;
		return true;
	}

	list_node<T>* temp = prev->next;

	prev->next = temp->next;

	if (temp == l->tail)
		l->tail = prev;

	delete temp;
	l->count--;

	if (l->count == 1)
		l->tail = l->head;

	if (l->count == 0)
		l->head = l->tail = 0;

	return true;
}

template<class T>
list_node<T>* list_remove_node(list<T>* l, list_node<T>* prev)
{
	if (prev == 0)
		return false;

	if (l->count == 0)
		return false;

	if (l->count == 1)		// prev == head
	{
		l->count--;
		l->head = l->tail = 0;
		prev->next = 0;
		return prev;
	}

	list_node<T>* temp = prev->next;

	if (temp == l->tail)
		l->tail = prev;

	prev->next = temp->next;
	l->count--;

	if (l->count == 1)
		l->tail = l->head;

	if (l->count == 0)
		l->head = l->tail = 0;

	temp->next = 0;
	return temp;
}

template<class T>
void list_remove_front(list<T>* l)
{
	if (l->count == 0)
		return;

	list_node<T>* temp = l->head;
	l->head = l->head->next;
	l->count--;
	delete temp;

	if (l->count == 0)
		l->head = l->tail = 0;
}

template<class T>
list_node<T>* list_remove_front_node(list<T>* l)
{
	if (l->count == 0)
		return 0;

	list_node<T>* temp = l->head;
	l->head = l->head->next;
	l->count--;

	if (l->count == 0)
		l->head = l->tail = 0;

	temp->next = 0;
	return temp;
}

template<class T>
void list_insert_front(list<T>* l, const T& element)
{
	list_node<T>* node = new list_node<T>;
	node->next = 0;
	node->data = element;

	if (l->count == 0)
		l->head = l->tail = node;
	else
	{
		node->next = l->head;
		l->head = node;
	}

	l->count++;
}

template<class T>
void list_insert_back(list<T>* l, const T& element)
{
	list_node<T>* node = new list_node<T>;
	node->next = 0;
	node->data = element;

	if (l->count == 0)
		l->head = l->tail = node;
	else
	{
		l->tail->next = node;
		l->tail = node;
	}

	l->count++;
}

template<class T>
void list_insert_back_node(list<T>* l, list_node<T>* node)
{
	node->next = 0;
	if (l->count == 0)
		l->head = l->tail = node;
	else
	{
		l->tail->next = node;
		l->tail = node;
	}

	l->count++;
}

// combines the two lists and stores them in l1. l2 is shallow destroyed.
template<class T>
void list_merge_back(list<T>* l1, list<T>* l2)
{
	l1->count += l2->count;

	l1->tail->next = l2->head;
	l1->tail = l2->tail;

	l2->count = 0;
	l2->head = l2->tail = 0;
}

// moves the node 'actual' right after the node 'prev'
template<class T>
void list_move_node(list<T>* l, list_node<T>* actual, list_node<T>* prev)
{
	PANIC("list move node not implemented");
}

// moves the first node to the end of the list
template<class T>
void list_head_to_tail(list<T>* l)
{
	if (l->count == 0 || l->count == 1)
		return;

	// this order is very importan and must be exactly like that
	l->tail->next = l->head;
	l->tail = l->head;
	l->head = l->head->next;
	l->tail->next = 0;
}

// clears the list
template<class T>
void list_clear(list<T>* l)
{
	while (l->count > 0)
		list_remove_front(l);
}

#endif
