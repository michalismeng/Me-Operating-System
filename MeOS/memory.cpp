#include "memory.h"
#include "spinlock.h"

extern heap* kernel_heap;
spinlock kernel_heap_lock = 0;

void* malloc(uint32 size)
{
	spinlock_acquire(&kernel_heap_lock);
	void* addr = heap_alloc(kernel_heap, size);
	spinlock_release(&kernel_heap_lock);

	return addr;
}

void* calloc(uint32 size)
{
	void* ptr = malloc(size);
	memset(ptr, 0, size);
	return ptr;
}

void free(void* ptr)
{
	spinlock_acquire(&kernel_heap_lock);
	heap_free(kernel_heap, ptr);
	spinlock_release(&kernel_heap_lock);
}

void* realloc(void* ptr, uint32 new_size)
{
	spinlock_acquire(&kernel_heap_lock);
	void* addr = heap_realloc(kernel_heap, ptr, new_size);
	spinlock_release(&kernel_heap_lock);

	return addr;
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
	return  malloc(size);
}

void* operator new[](uint32 size)
{
	return  malloc(size);
}

void operator delete(void* ptr)
{
	free(ptr);
}

void operator delete(void* ptr, unsigned int)
{
	free(ptr);
}

void operator delete[](void* ptr)
{
	free(ptr);
}

#endif