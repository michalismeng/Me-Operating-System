#include "memory.h"

extern heap* kernel_heap;

void* malloc(uint32 size)
{
	return heap_alloc(kernel_heap, size);
}

void* calloc(uint32 size)
{
	void* ptr = malloc(size);
	memset(ptr, 0, size);
	return ptr;
}

void free(void* ptr)
{
	heap_free(kernel_heap, ptr);
}

void* realloc(void* ptr, uint32 new_size)
{
	return heap_realloc(kernel_heap, ptr, new_size);
}

virtual_addr mmap(virtual_addr pref, uint32 fd, uint32 offset, uint32 length, uint32 flags, uint32 prot)
{
	// from current_process
	// from local_table get global fd through
	// increase the open_count

	// from vm_contract perform map [pref, pref + length) + global fd + extracted flags
	// on fail perform find area for 'length' and then perform map again
	return 0;
}

#ifdef __cplusplus

void* operator new(uint32 size)
{
	return  heap_alloc(kernel_heap, size);
}

void* operator new[](uint32 size)
{
	return  heap_alloc(kernel_heap, size);
}

void operator delete(void* ptr)
{
	heap_free(kernel_heap, ptr);
}

void operator delete(void* ptr, unsigned int)
{
	heap_free(kernel_heap, ptr);
}

void operator delete[](void* ptr)
{
	heap_free(kernel_heap, ptr);
}

#endif