#ifndef HEAP_MANAGER_H
#define HEAP_MANAGER_H

#include "types.h"
#include "OrderedArray.h"
#include "utility.h"

#define HEAP_START				0xC0000000
#define HEAP_INITIAL_SIZE		0x100000
#define HEAP_INDEX_SIZE			0x20000
#define HEAP_MAGIC				0x123890AB
#define HEAP_NO_INITIALIZE		0xCCCCCCCC
#define HEAP_MIN_SIZE			0x70000

class MemoryManager;

struct Header			// header of a registry in the heap
{

	Header();
	Header(bool hole, uint32 size);

	void operator() (bool is_hole, uint32 size);
	void Print();		// print data for debug purposes

	uint32 magic;		// magic number for error checking
	bool is_hole;		// determines if the header is a hole or is filled
	uint32 size;		// determines the size of the block including both the header and the end footer
};

struct Footer
{
	Footer();
	Footer(Header* header);

	uint32 magic;		// magic number same as above
	Header* header;		// pointer to the header so we can always return and get information
};

bool Header_Lessthan_Predicate(void* a, void* b);

class Heap
{
public:

	Heap();
	Heap(uint32 start, uint32 end, uint32 max, bool supervisor, bool readonly, MemoryManager* manager);

	void* alloc(uint32 size, bool page_align);			// is page_align is true then the block is allocated on a page boundary
	void  free(void* p);

	void Print() {  for(uint32 i = 0; i < index.GetSize(); i++) ((Header*)index[i])->Print(); }

public:

	OrderedArray index;

	uint32 start_address;		// the start of our allocation space
	uint32 end_address;			// the end of the allocation space. May extend up to max_address
	uint32 max_address;			// The maximum address the heap can expand to

	bool supervisor;			// should extra pages requested be mapped as supervisor only?
	bool readonly;				// should extra pages requested be mapped as read-only?

	MemoryManager* manager;		// a memory manager for the frame management

	int32 FindSmallestHole(uint32 size, bool page_align);		// returns the index of the header that is a hole and has a size of the parameter

	void Expand(uint32 new_size);								// expands the heap
	uint32 Contract(uint32 new_size);							// shrink the heap
};

#endif
