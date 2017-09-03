#ifndef MMNGR_HEAP_H_14082016
#define MMNGR_HEAP_H_14082016

#include "types.h"
#include "error.h"
#include "utility.h"
#include "mmngr_virtual.h"

#define HEAP_BLOCK_MAGIC 0x1F2E

#undef HEAP_ALLOC_FIRST_IMPLEMENT
#undef HEAP_REALLOC_FIRST_IMPLEMENT

enum HEAP_ERROR
{
	HEAP_NONE,
	HEAP_BAD_MAGIC,
	HEAP_BAD_ARGUMENT,
	HEAP_OUT_OF_MEMORY
};

// Heap header block definition. (One per requested allocation)
struct heap_block
{
	uint16 magic;
	bool used;
	bool flags;
	heap_block* next;
};

// Heap master. Defines a heap region to allocate blocks and space for user.
struct heap
{
	virtual_addr start_address;
	uint32 size;
	uint32 current_blocks;
};

// creates a heap of size at the virtual address base.
heap* heap_create(virtual_addr base, uint32 size);

// allocates size bytes at the heap h for use by a user program.
// also merges every contiguous unused blocks found on its way.
void* heap_alloc(heap* h, uint32 size);

// deallocates a previously allocated space. (May front-merge unused blocks)
error_t heap_free(heap* h, void* address);

// re-allocates a previously allocated space to take up 'new_size' space.
void* heap_realloc(heap* h, void* address, uint32 new_size);

// defrags the heap h merging all contiguous unsued blocks.
uint32 heap_defrag(heap* h);

// displays the heap master entry along with info for every block allocated.
void heap_display(heap* h);

#endif