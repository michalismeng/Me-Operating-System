#ifndef VECTOR_H_28102016
#define VECTOR_H_28102016

#include "types.h"
#include "error.h"

#define find_predicate_test(T, name) bool(*name)(T*)

template<class T>
struct vector
{
	T* data;			// data
	uint32 count;		// items in vector
	uint32 r_size;		// reserved size

	T& operator[](uint32 i) { return data[i]; }
};

template<class T>
T& vector_at(vector<T>* v, uint32 pos)
{
	return v->data[pos];
}

template<class T>
T& vector_front(vector<T>* v)
{
	return v->data[0];
}

template<class T>
error_t vector_init(vector<T>* v, uint32 initial_length)
{
	if (initial_length == 0)
		initial_length = 1;

	v->count = 0;

	v->r_size = initial_length;
	v->data = new T[initial_length];

	if (!v->data)
		return ERROR_OCCUR;

	return ERROR_OK;
}

// reserve space for 'num_elements' keeping the current elements inside the vector untouched
template<class T>
error_t vector_reserve(vector<T>* v, int num_elements)
{
	v->data = (T*)realloc(v->data, num_elements * sizeof(T));

	// TODO: Remove this
	if (v->data == 0)		// allocation error
	{
		PANIC("ERROR");
		return ERROR_OCCUR;
	}

	if (!v->data)
		return ERROR_OCCUR;

	v->r_size = num_elements;
	return ERROR_OK;
}

template<class T>
error_t vector_insert_back(vector<T>* v, const T& data)
{
	if (v->count == v->r_size)
		if (vector_reserve(v, 2 * v->count) != ERROR_OK)
			return ERROR_OCCUR;

	v->data[v->count++] = data;
	return ERROR_OK;
}

template<class T>
uint32 vector_find_first(vector<T>* v, bool(*fp)(T*))
{
	if (v == 0)
		return (uint32)-1;

	for (uint32 i = 0; i < v->count; i++)
		if (fp(&v->data[i]))
			return i;

	return (uint32)-1;
}

#endif
