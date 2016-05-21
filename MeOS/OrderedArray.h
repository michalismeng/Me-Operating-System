#ifndef ORDERED_ARRAY_H
#define ORDERED_ARRAY_H

#include "types.h"
#include "utility.h"
#include "mmngr_phys.h"
#include "mmngr_virtual.h"

typedef void* ordered_type;		// our array will hold any data convertible to void* (uint32 and any pointer)

typedef bool (*lessthan_predicate)(ordered_type, ordered_type);

bool Standard_LessThan_Predicate(ordered_type a, ordered_type b);

class OrderedArray
{
public:
	OrderedArray();
	OrderedArray(uint32 max_size, lessthan_predicate less);
	OrderedArray(virtual_addr start_address, uint32 max_size, lessthan_predicate less);

	~OrderedArray();
	void DestroyArray();						// destroy the array

	bool Insert(ordered_type item);				// insert an item based on the less than predicate
	ordered_type LookUp(uint32 index);			// find an item at index in the array
	void RemoveItem(uint32 index);				// remove an element from the array at index: index with boundry check

	int32 GetIndex(ordered_type item);			// return the index item is located at. Uses simple linear search. Can be optimized with b. search

	ordered_type operator[] (uint32 index);		// returns the element at index: index without boundry check

	inline uint32 GetSize()	{ return size; }

	void Print();								// print elements for debug purposes

public:
	ordered_type* array;					// pointer to the start of the array ( of void pointers )
	uint32 size;							// current size of the array
	uint32 max_size;						// max size of the array
	lessthan_predicate less_than;			// less than predicate to sort
};

#endif