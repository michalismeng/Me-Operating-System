#include "mmngr_heap.h"

// private functions

uint32 heap_block_size(heap* h, heap_block* b)
{
	if (b->next == 0)
		return h->start_address + h->size - (uint32)b - sizeof(heap_block);
	return (uint32)b->next - (uint32)b - sizeof(heap_block);
}

inline void* heap_block_start_address(heap_block* b)
{
	return ((char*)b + sizeof(heap_block));
}

// merges block b with its next. No condition checks are made
void heap_front_merge_block(heap* h, heap_block* b)
{
	b->next = b->next->next;
	h->current_blocks--;
}

// returns true if the next block of prev is unused
inline bool heap_is_next_unused(heap_block* prev)
{
	return (prev->next != 0 && prev->next->used == false);
}

heap_block* heap_block_create(heap* h, void* base, bool used, heap_block* next, bool flags)
{
	heap_block* block = (heap_block*)base;
	block->magic = HEAP_BLOCK_MAGIC;
	block->used = false;
	block->next = next;
	block->flags = flags;

	h->current_blocks++;

	return block;
}

// Front merges blocks until either the merged block size suits r_size or fail.
// Merged block remains for on-spot defrag purposes.
bool heap_front_merge_size_fit(heap* h, heap_block* prev, uint32 r_size)
{
	while (prev->next != 0 && r_size > heap_block_size(h, prev))
	{
		if (heap_is_next_unused(prev))
			heap_front_merge_block(h, prev);
		else
			break;
	}

	return r_size <= heap_block_size(h, prev);
}

// public functions

heap* heap_create(virtual_addr base, uint32 size)
{
	heap* new_heap = (heap*)base;
	new_heap->start_address = (virtual_addr)((char*)base + sizeof(heap));
	new_heap->size = size - sizeof(heap);
	new_heap->current_blocks = 0;

	heap_block_create(new_heap, (void*)new_heap->start_address, false, 0, 0);
	return new_heap;
}

void* heap_alloc(heap* h, uint32 r_size)
{
	if (h == 0 || r_size == 0)
		return 0;

	heap_block* block = (heap_block*)h->start_address;

	while (block != 0)
	{
		if (block->magic != HEAP_BLOCK_MAGIC)
			return 0;

		if (block->used == true)
			block = block->next;
		else if (heap_front_merge_size_fit(h, block, r_size))	// try create contiguous chunk of at least 'r_size' size
		{
			uint32 size = heap_block_size(h, block);

			if (r_size + sizeof(heap_block) < size)			// try squeeze a heap_block
			{
				heap_block* new_block = heap_block_create(h, (char*)block + sizeof(heap_block) + r_size, false,
					block->next, block->flags);

				block->next = new_block;
			}

			block->used = true;
			return heap_block_start_address(block);
		}
		else											// requested size does not fit so continue to the next block
			block = block->next;						// the contiguous unsused chunk remains.
	}

	// allocation failed
	return 0;
}

uint32 heap_free(heap* h, void* address)
{
	if (h == 0 || address == 0)
		return 1;

	heap_block* block = (heap_block*)((char*)address - sizeof(heap_block));
	if (block->magic != HEAP_BLOCK_MAGIC)
		return 1;

	block->used = false;
	if (block->next != 0 && block->next->used == false)		// front-only merge unused block to partially defrag
	{
		block->next = block->next->next;
		h->current_blocks--;
	}

	return 0;
}

void* heap_realloc(heap* h, void* address, uint32 new_size)
{
	if (h == 0 || address == 0)
		return 0;

	heap_block* block = (heap_block*)((char*)address - sizeof(heap_block));

	if (block->magic != HEAP_BLOCK_MAGIC)
		return 0;

	uint32 block_size = heap_block_size(h, block);

	if (block_size == new_size)
		return address;

	/*if (new_size < block_size)		// request to shrink allocated space
	{
		// try squeeze a new block
		if (heap_is_next_unused(block))			// only one front merge required to squeeze a block
			heap_front_merge_block(h, block);

		uint32 remaining_size = heap_block_size(h, block) - new_size;	// be careful as block size may have changed

		if (remaining_size > sizeof(heap_block))		// squeeze a new block
		{
			heap_block* new_block = heap_block_create(h, (char*)address + new_size, false, block->next, block->flags);
			block->next = new_block;
		}

		return heap_block_start_address(block);
	}
	else if (new_size > block_size)
	{
		if (heap_front_merge_size_fit(h, block, new_size))
		{
		}
	}
	else
		return address;		// re-allocation does nothing as 'new_size' equals the currently allocated 'block_size'*/

#ifndef HEAP_REALLOC_FIRST_IMPLEMENT
		//uint32 block_size = heap_block_size(h, block);

	void* new_addr = heap_alloc(h, new_size);
	if (new_addr == 0)
		return 0;

	memcpy(new_addr, address, min(block_size, new_size));
	heap_free(h, address);

	return new_addr;
#endif
}

uint32 heap_defrag(heap* h)
{
	if (h == 0)
		return 0;

	uint32 blocks_merged = 0;
	heap_block* block = (heap_block*)h->start_address;

	while (block != 0)
	{
		if (block->used == false && block->next != 0 && block->next->used == false)		// merge blocks
		{
			block->next = block->next->next;
			h->current_blocks--;
			blocks_merged++;
		}
		else
			block = block->next;
	}

	return blocks_merged;
}

void heap_display(heap* h)
{
	if (h == 0)
		return;
	printfln("Heap start: %h with size: %x. Currently using %u blocks.", h->start_address, h->size, h->current_blocks);
	heap_block* block = (heap_block*)h->start_address;

	while (block != 0)
	{
		if (block->magic != HEAP_BLOCK_MAGIC)
			printfln("HEAP ERROR");
		uint32 size = heap_block_size(h, block);
		printf("block at: %h with size: %x, ", block, size);
		if (block->used)
			printfln("used");
		else
			printfln("unused");

		block = block->next;
	}
}