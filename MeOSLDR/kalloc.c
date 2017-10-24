#include "kalloc.h"

uint8* alloced;
uint8* max_addr;

void init_kallocations(uint8* base, uint8* _max_addr)
{
	alloced = base;
	max_addr = _max_addr;
}

// full-greedy allocation. claimed space is never de-allocated
void* kalloc(uint32 size)
{
	if (alloced + size >= max_addr)
		return 0;

	uint8* temp = alloced;
	alloced += size;

	return temp;
}

// full-greedy allocation that is align-bytes aligned.
void* kalloc_a(uint32 size, uint32 align)
{
	uint32 align_cost = align - (uint32)alloced % align;
	if (align_cost != align)
		if (kalloc(align_cost) == 0)
			return 0;

	return kalloc(size);
}

uint8* kalloc_get_ptr()
{
	return alloced;
}