#include "mmngr_phys.h"

#define PMMNGR_BLOCKS_PER_BYTE	8		// this is used in our bitmap structure
#define PMMNGR_BLOCK_SIZE		4096	// block size in bytes (use same as page size for convenience)
#define PMMNGR_BLOCK_ALIGN		PMMNGR_BLOCK_SIZE

static uint32 mmngr_memory_size = 0;
static uint32 mmngr_used_blocks = 0;
static uint32 mmngr_max_blocks = 0;
static uint32* mmngr_bitmap = 0;


//PRIVATE - AUX FUNCTIONS 

inline void mmap_set(int bit)
{
	mmngr_bitmap[bit / 32] |= 1 << (bit % 32);
}

inline void mmap_unset(int bit)
{
	mmngr_bitmap[bit / 32] &= ~(1 << (bit % 32));
}

inline bool mmap_test(int bit)
{
	return mmngr_bitmap[bit / 32] & (1 << (bit % 32));
}

// returns first free block
int mmap_first_free()
{
	for (uint32 i = 0; i < mmngr_max_blocks / 32; i++)	// 32 blocks per uint32
	{
		if (mmngr_bitmap[i] != 0xFFFFFFFF)	// at least one bit is off
		{
			int bit = 1;
			for (int j = 0; j < 32; j++)
			{
				if (! (mmngr_bitmap[i] & bit))	// bit not set
					return i * 32 + j;

				bit <<= 1;
			}
		}
	}

	return -1;
}

// returns the first count free blocks
int mmap_first_free_s(uint32 count)
{
	if (count == 0)		// error
		return -1;

	if (count == 1)		// mmap_first_free should be used
		return mmap_first_free();

	for (uint32 bit = 0; bit < mmngr_max_blocks;)
	{
		if (mmngr_bitmap[bit / 32] == 0xFFFFFFFF)	// if uint32 is full goto next
		{
			bit += 32;
			continue;
		}

		if (mmap_test(bit) == false)
		{
			uint32 startBit = bit;				// mark the free bit
			while (bit < mmngr_max_blocks)		// start iterating through next bits
			{
				bit++;

				if (mmap_test(bit) == true)
					break;

				if (bit - startBit + 1 == count)	// space found so return the first bit
					return startBit;
			}
		}

		bit++;
	}

	return -1;
}

// INTERFACE FUNCTIONS

void pmmngr_init(uint32 size, physical_addr base)
{
	mmngr_memory_size = size;
	mmngr_bitmap = (uint32*)base;
	mmngr_max_blocks = pmmngr_get_memory_size() * 1024 / PMMNGR_BLOCK_SIZE;
	mmngr_used_blocks = mmngr_max_blocks;

	// by default all memory is in use
	memset(mmngr_bitmap, 0xff, pmmngr_get_block_count() / PMMNGR_BLOCKS_PER_BYTE);
}

void pmmngr_init_region(physical_addr base, uint32 size)
{
	if (size == 0)		// error
		return;

	uint32 aligned_addr = base / PMMNGR_BLOCK_SIZE;				// floor the base address as it is absolute
	uint32 aligned_size = (size - 1) / PMMNGR_BLOCK_SIZE;		// same but make it zero based as it is length

	printfln("init aligned size: %h", aligned_size);

	for (int i = 0; i <= aligned_size; i++)
	{
		if (mmap_test(aligned_addr) == true)	// if block is in use
		{
			mmap_unset(aligned_addr++);			// unset it to make it available
			mmngr_used_blocks--;				// reduce blocks in use
		}
	}

	mmap_set(0);		// 0 always set to disable 0 allocation
	//mmap_set(last_block_bit);  this way we ensure mmap_first_free_s loop does not go out of bounds
}

void pmmngr_deinit_region(physical_addr base, uint32 size)
{
	if (size == 0)		// error
		return;

	uint32 aligned_addr = base / PMMNGR_BLOCK_SIZE;
	uint32 aligned_size = (size - 1) / PMMNGR_BLOCK_SIZE;

	printfln("deinit aligned size: %h", aligned_size);

	for (int i = 0; i <= aligned_size; i++)
	{
		if (mmap_test(aligned_addr) == false)		// if block is not in use
		{
			mmap_set(aligned_addr++);				// set it to make it unavailable
			mmngr_used_blocks++;					// increase used blocks
		}
	}
}

void* pmmngr_alloc_block()
{
	if (pmmngr_get_free_block_count() <= 0)		// out of memory
		return 0;

	int frame = mmap_first_free();

	if (frame == -1)		// out of memory
		return 0;

	mmap_set(frame);		// here we set directly without mmap_test as this is done in mmap_first_free

	physical_addr addr = frame * PMMNGR_BLOCK_SIZE;
	mmngr_used_blocks++;

	return (void*)addr;
}

void pmmngr_free_block(void* block)
{
	physical_addr addr = (physical_addr)block;
	int frame = addr / PMMNGR_BLOCK_SIZE;

	if (mmap_test(frame) == true)		// if it is set (as in init and deinit region)
	{
		mmap_unset(frame);
		mmngr_used_blocks--;
	}
}

void* pmmngr_alloc_blocks(uint32 count)
{
	if (pmmngr_get_free_block_count() < count)	// not enough memory
		return 0;

	int frame = mmap_first_free_s(count);

	if (frame == -1)		// out of memory
		return 0;

	for (int i = 0; i < count; i++)
		mmap_set(frame + i);

	physical_addr addr = frame * PMMNGR_BLOCK_SIZE;
	mmngr_used_blocks += count;

	return (void*)addr;
}

void pmmngr_free_blocks(void* block, uint32 count)
{
	physical_addr addr = (physical_addr)block;
	int frame = addr / PMMNGR_BLOCK_SIZE;

	for (int i = 0; i < count; i++)
	{
		if (mmap_test(frame + i) == true)
		{
			mmap_unset(frame);
			mmngr_used_blocks--;
		}
	}
}

uint32 pmmngr_get_memory_size()
{
	return mmngr_memory_size;
}

uint32 pmmngr_get_block_use_count()
{
	return mmngr_used_blocks;
}

uint32 pmmngr_get_free_block_count()
{
	return mmngr_max_blocks - mmngr_used_blocks;
}

uint32 pmmngr_get_block_count()
{
	return mmngr_max_blocks;
}

uint32 pmmngr_get_block_size()
{
	return PMMNGR_BLOCK_SIZE;
}

void pmmngr_paging_enable(bool flag)
{
	_asm
	{
		mov eax, cr0
		cmp byte ptr [flag], 1
		jne disable

		enable:
			or eax, 0x80000000	// set bit 31 of cr0 register
			jmp done

		disable:
			and eax, 0x7FFFFFFF	// unset bit 31
		
		done:
			mov cr0, eax
	}
}

bool pmmngr_get_paging_enabled()
{
	uint32 res;

	_asm
	{
		mov eax, cr0
		mov dword ptr [res], eax
	}

	if (res & 0x80000000)		// check bit 31
		return true;

	return false;
}

void pmmngr_load_PDBR(physical_addr addr)
{
	_asm 
	{
		mov	eax, dword ptr [addr]
		mov	cr3, eax
	}
}

physical_addr pmmngr_get_PDBR()
{
	physical_addr addr;

	_asm 
	{
		mov	eax, cr3
		mov dword ptr [addr], eax
	}
}
