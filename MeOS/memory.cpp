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

void operator delete[](void* ptr)
{
	heap_free(kernel_heap, ptr);
}

#endif