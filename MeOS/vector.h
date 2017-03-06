#ifndef VECTOR_H_28102016
#define VECTOR_H_28102016

#include "types.h"

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
void vector_init(vector<T>* v, uint32 initial_length)
{
	if (initial_length == 0)
		initial_length = 1;

	v->count = 0;

	v->r_size = initial_length;
	v->data = new T[initial_length];
}

// reserve space for 'num_elements' keeping the current elements inside the vector untouched
template<class T>
void vector_reserve(vector<T>* v, int num_elements)
{
	v->data = (T*)realloc(v->data, num_elements * sizeof(T));
	if (v->data == 0)		// allocation error
	{
		PANIC("ERROR");
		return;
	}

	v->r_size = num_elements;
}

template<class T>
void vector_insert_back(vector<T>* v, const T& data)
{
	if (v->count == v->r_size)
		vector_reserve(v, 2 * v->count);

	v->data[v->count++] = data;
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
