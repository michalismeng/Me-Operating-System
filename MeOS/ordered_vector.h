#ifndef ORDERED_VECTOR_H_25102016
#define ORDERED_VECTOR_H_25102016

typedef int(*ordered_vector_comp)(const void* a, const void* b);

#include "types.h"
#include "utility.h"
#include "memory.h"

// vector of items sorted in (by default ascending) order
template<class T>
struct ordered_vector
{
	T* data;			// data
	uint32 count;		// items in vector
	uint32 r_size;		// reserved size
};

template<class T>
error_t ordered_vector_init(ordered_vector<T>* v)
{
	v->count = 0;

	v->r_size = 1;
	v->data = (T*)malloc(sizeof(T));

	if (!v->data)
		return ERROR_OCCUR;

	// TODO: Remove this
	if (v->data == 0)
		printfln("data is zero");

	return ERROR_OK;
}

// reserve space for 'num_elements' keeping the current elements inside the vector untouched
template<class T>
error_t ordered_vector_reserve(ordered_vector<T>* v, int num_elements)
{
	v->data = (T*)realloc(v->data, num_elements * sizeof(T));

	// TODO: Remove this
	if (v->data == 0)		// allocation error
	{
		PANIC("Ordered vector received null data");
		return ERROR_OCCUR;
	}

	if (!v->data)
		return ERROR_OCCUR;

	v->r_size = num_elements;
	return ERROR_OK;
}

template<class T>
error_t ordered_vector_insert(ordered_vector<T>* v, T& data)
{
	if (v->count == v->r_size)
		if (ordered_vector_reserve(v, 2 * v->count) != ERROR_OK)
			return ERROR_OCCUR;

	uint32 i = v->count;

	while (i > 0 && data < v->data[i - 1])
	{
		v->data[i] = v->data[i - 1];
		i--;
	}

	v->data[i] = data;
	v->count++;

	return ERROR_OK;
}

template<class T>
bool ordered_vector_remove(ordered_vector<T>* v, uint32 index)
{
	if (index >= v->count)
		return false;

	uint32 i = index;
	v->count--;

	while (i < v->count)
	{
		v->data[i] = v->data[i + 1];
		i++;
	}

	return true;
}

template<class T>
uint32 ordered_vector_find(ordered_vector<T>* v, T& data)
{
	uint32 first = 0, last = v->count - 1;

	while (first <= last)
	{
		uint32 mid = (first + last) / 2;

		if (data < v->data[mid])
			last = mid - 1;
		else if (data > v->data[mid])
			first = mid + 1;
		else
			return mid;
	}

	return (uint32)-1;
}

template<class T>
error_t ordered_vector_uninit(ordered_vector<T>* v)
{
	v->count = 0;
	v->r_size = 0;

	if (free(v->data) != ERROR_OK)
		return ERROR_OCCUR;	
}

#endif
