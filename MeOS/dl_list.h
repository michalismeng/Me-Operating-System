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

//template<class T>
//bool dl_list_remove(list<T>* l, list_node<T>* prev)
//{
//	if (prev == 0)
//		return false;
//
//	if (l->count == 0)
//		return false;
//
//	if (l->count == 1)		// prev == head
//	{
//		l->count--;
//		delete prev;
//		l->head = l->tail = 0;
//		return true;
//	}
//
//	list_node<T>* temp = prev->next;
//
//	prev->next = temp->next;
//
//	if (temp == l->tail)
//		l->tail = prev;
//
//	delete temp;
//	l->count--;
//
//	if (l->count == 1)
//		l->tail = l->head;
//
//	if (l->count == 0)
//		l->head = l->tail = 0;
//
//	return true;
//}

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
	if (node == 0 || l->count == 0)
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
	node->next = node->prev = 0;
	return node;
}

//template<class T>
//void list_remove_front(list<T>* l)
//{
//	if (l->count == 0)
//		return;
//
//	list_node<T>* temp = l->head;
//	l->head = l->head->next;
//	l->count--;
//	delete temp;
//
//	if (l->count == 0)
//		l->head = l->tail = 0;
//}

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

//template<class T>
//void list_insert_front(list<T>* l, const T& element)
//{
//	list_node<T>* node = new list_node<T>;
//	node->next = 0;
//	node->data = element;
//
//	if (l->count == 0)
//		l->head = l->tail = node;
//	else
//	{
//		node->next = l->head;
//		l->head = node;
//	}
//
//	l->count++;
//}

//template<class T>
//void list_insert_back(list<T>* l, const T& element)
//{
//	list_node<T>* node = new list_node<T>;
//	node->next = 0;
//	node->data = element;
//
//	if (l->count == 0)
//		l->head = l->tail = node;
//	else
//	{
//		l->tail->next = node;
//		l->tail = node;
//	}
//
//	l->count++;
//}

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

// combines the two lists and stores them in l1. l2 is shallow destroyed.
//template<class T>
//void list_merge_back(list<T>* l1, list<T>* l2)
//{
//	l1->count += l2->count;
//
//	l1->tail->next = l2->head;
//	l1->tail = l2->tail;
//
//	l2->count = 0;
//	l2->head = l2->tail = 0;
//}

// moves the node 'actual' right after the node 'prev'
//template<class T>
//void list_move_node(list<T>* l, list_node<T>* actual, list_node<T>* prev)
//{
//	PANIC("list move node not implemented");
//}

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

// clears the list
//template<class T>
//void dl_list_clear(dl_list<T>* l)
//{
//	while (l->count > 0)
//		dl_list_remove_front(l);
//}

#endif
