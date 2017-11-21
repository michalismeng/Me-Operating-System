#ifndef DL_LIST_H_08082017
#define DL_LIST_H_08082017

// contains c++ code

#include "types.h"
#include "utility.h"
#include "memory.h"

#define DLLIST_PEEK(l) l->head->data

template<class T>
struct dl_list_node
{
	T data;
	dl_list_node* next;
	dl_list_node* prev;
};

template<class T>
struct dl_list
{
	dl_list_node<T>* head;
	dl_list_node<T>* tail;
	uint32 count;				// items in list
};

template<class T>
void dl_list_init(dl_list<T>* l)
{
	l->count = 0;
	l->head = l->tail = 0;
}

template<class T>
dl_list_node<T>* dl_list_find_node(dl_list<T>* l, T item)
{
	if (l->count == 0)
		return 0;

	for (auto temp = l->head; temp != 0; temp = temp->next)
		if (temp->data == item)
			return temp;

	return 0;
}

template<class T>
dl_list_node<T>* dl_list_remove_node(dl_list<T>* l, dl_list_node<T>* node)
{
	if (l->count == 0)
		return 0;

	if (l->count == 1)		// single element
	{
		l->count = 0;
		l->head = l->tail = 0;
		node->next = node->prev = 0;
		return node;
	}

	// fix head and tail
	if (node == l->head)
	{
		l->head = node->next;
		node->next->prev = 0;
	}
	else if (node == l->tail)
	{
		l->tail = node->prev;
		node->prev->next = 0;
	}
	else
	{
		node->prev->next = node->next;
		node->next->prev = node->prev;
	}

	l->count--;
	node->next = node->prev = 0;		// preserve next and prev to use them for a future insertion (so as not to search for the previous node).
	return node;
}

template<class T>
dl_list_node<T>* dl_list_remove_front_node(dl_list<T>* l)
{
	if (l->count == 0)
		return 0;

	dl_list_node<T>* temp = l->head;

	if (l->count == 1)
	{
		l->count = 0;
		l->head = l->tail = 0;
		temp->next = temp->prev = 0;
		return temp;
	}

	l->head = l->head->next;
	l->head->prev = 0;
	l->count--;

	if (l->count == 0)
	{
		l->head = l->tail = 0;
		PANIC("Can never be here");
	}

	temp->next = temp->prev = 0;
	return temp;
}

template<class T>
void dl_list_insert_back_node(dl_list<T>* l, dl_list_node<T>* node)
{
	node->next = 0;

	if (l->count == 0)
	{
		l->head = l->tail = node;
		node->prev = 0;
	}
	else
	{
		l->tail->next = node;
		node->prev = l->tail;
		l->tail = node;
	}

	l->count++;
}

// moves the first node to the end of the list
template<class T>
void dl_list_head_to_tail(dl_list<T>* l)
{
	if (l->count == 0 || l->count == 1)
		return;

	dl_list_node<T>* temp_head = l->head;

	// this order is very importan and must be exactly like that
	if (l->tail->prev == l->head)
	{
		l->tail->prev = 0;
		l->head = l->tail;
		l->tail->next = temp_head;
		temp_head->next = 0;
		temp_head->prev = l->tail;
		l->tail = temp_head;
	}
	else
	{
		l->head = l->head->next;
		l->head->prev = 0;
		temp_head->next = 0;
		temp_head->prev = l->tail;
		l->tail->next = temp_head;
		l->tail = temp_head;
	}
}

template<class T>
void dl_list_clear_ptrs(dl_list_node<T>* node)
{
	if (!node)
		return;

	node->prev = node->next = 0;
}

#endif
