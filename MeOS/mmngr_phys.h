#ifndef MMNGR_PHYS_H_090516
#define MMNGR_PHYS_H_090516

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include "utility.h"

	typedef uint32 physical_addr;

	struct physical_memory_region
	{
		physical_memory_region() {}
		physical_memory_region(uint32 base, uint32 length) { this->base = base; this->length = length; }

		uint32 base;
		uint32 length;
	};

	// INTERFACE

	// size of physical memory
	extern uint32 mmngr_memory_size;

	// number of blocks in use
	extern uint32 mmngr_used_blocks;

	// maximum block number
	extern uint32 mmngr_max_blocks;

	// bitmap structure where each bit represents a block in memory
	extern uint32* mmngr_bitmap;

	// initialize the physical memory manager. (size in KB)
	void pmmngr_init(uint32 size, physical_addr base);

	// initialize a region for use
	void pmmngr_free_region(physical_memory_region* region);

	// uninitialize a region (make it unusable)
	void pmmngr_reserve_region(physical_memory_region* region);

	// allocates a block of memory
	void* pmmngr_alloc_block();

	// frees an allocated block
	void pmmngr_free_block(void* block);

	// allocates blocks to hold size
	void* pmmngr_alloc_blocks(uint32 size);

	// frees allocated blocks
	void pmmngr_free_blocks(void* block, uint32 size);

	// get the memory amount the manager is initialized to use
	uint32 pmmngr_get_memory_size();

	// get the number of blocks currently in use
	uint32 pmmngr_get_block_use_count();

	// get the number of blocks not in use
	uint32 pmmngr_get_free_block_count();

	// get the total number of blocks
	uint32 pmmngr_get_block_count();

	// get block size in bytes
	uint32 pmmngr_get_block_size();

	// enable or disable paging capability
	void pmmngr_paging_enable(bool flag);

	// get paging status (enabled or disabled)
	bool pmmngr_get_paging_enabled();

	// loads the page directory base register (PDBR)
	void pmmngr_load_PDBR(physical_addr addr);

	// get PDBR physical address
	physical_addr pmmngr_get_PDBR();

	// returns the next/closest block boundry
	uint32 pmmngr_get_next_align(uint32 value);

	// returns the former/closest block boundry
	uint32 pmmngr_get_prev_align(uint32 value);

#ifdef __cplusplus
}
#endif

#endif