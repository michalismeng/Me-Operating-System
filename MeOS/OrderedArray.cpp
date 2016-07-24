#include "OrderedArray.h"

#include "HeapManager.h"

bool Standard_LessThan_Predicate(ordered_type a, ordered_type b)
{
	if(a < b)
		return true;
	return false;
}

OrderedArray::OrderedArray()
{
	array = 0;
	size = max_size = 0;
}

OrderedArray::OrderedArray(uint32 max_size, lessthan_predicate less)
{
	//if (pmmngr_get_paging_enabled() == false)
	//	PANIC("ORDERED ARRAY REQUIRES PAGING ENABLED");

	array = (ordered_type*)pmmngr_alloc_blocks(max_size * sizeof(ordered_type));
	
	memset(array, 0, max_size * sizeof(ordered_type));
	size = 0;
	this->max_size = max_size;
	less_than = less;
}

OrderedArray::OrderedArray(virtual_addr start_address, uint32 max_size, lessthan_predicate less)
{
	if (pmmngr_get_paging_enabled() == false)
		PANIC("ORDERED ARRAY REQUIRES PAGING ENABLED");

	array = (ordered_type*)start_address;

	uint32 count = ceil_division(sizeof(ordered_type) * max_size,  pmmngr_get_block_size());

	for (uint32 i = 0; i < count; i++)
		if (!vmmngr_alloc_page(start_address + i * pmmngr_get_block_size()))
			PANIC("page allocation failed");
	
	memset((void*)start_address, 0, sizeof(ordered_type) * max_size);

	size = 0;
	this->max_size = max_size;
	less_than = less;
}

OrderedArray::~OrderedArray()
{
	DestroyArray();
}

void OrderedArray::DestroyArray()
{
	//pmmngr_free_blocks(array, ceil_division(max_size * sizeof(ordered_type), pmmngr_get_block_size()));
}

bool OrderedArray::Insert(ordered_type item)
{
	if(less_than == 0)		// case (size == max_size) is not taken into account
		return false;

	uint32 iter = 0;
	while(iter < size && less_than(array[iter], item))
		iter++;

	if(iter == size)
	{
		array[size++] = item;
	}
	else							// insert and push all the other elements
	{
		ordered_type temp = array[iter];
		array[iter] = item;

		while(iter < size)
		{
			iter++;					// in here iter will become equal to the size for the last time in the loop

			ordered_type temp2 = array[iter];
			array[iter] = temp;
			temp = temp2;
		}

		size++;
	}

	return true;
}

ordered_type OrderedArray::LookUp(uint32 i)
{
	if(i >= size)
		return 0;
	else
		return array[i];
}

void OrderedArray::RemoveItem(uint32 i)
{
	if(i >= size)
		return;

	while(i < size)
	{
		array[i] = array[i + 1];
		i++;
	}

	size--;
}

ordered_type OrderedArray::operator[] (uint32 index)
{
	return array[index];
}

void OrderedArray::Print()
{
	for (uint32 i = 0; i < size; i++)
		printf("%h ", array[i]);
	return;
	for(uint32 i = 0; i < size; i++)
	{
		if(((Header*)array[i])->magic != HEAP_MAGIC)
			PANIC("error");
		((Header*)array[i])->Print();
		printf("\n");
	}
}

int32 OrderedArray::GetIndex(ordered_type item)
{
	uint32 first = 0, last = size - 1;

	while(first <= last)
	{
		uint32 mid = (first + last) / 2;

		if(array[mid] == item)
			return mid;
		else if(less_than(array[mid], item))	// array is ordered according to the less_than predicate
			first = mid + 1;
		else
			last = mid - 1;
	}

	return -1;
}