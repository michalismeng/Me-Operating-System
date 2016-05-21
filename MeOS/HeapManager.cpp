#include "HeapManager.h"

Header::Header()
{
	size = 0;
	is_hole = false;
	magic = HEAP_NO_INITIALIZE;
}

Header::Header(bool hole, uint32 size)
{
	magic = HEAP_MAGIC;
	is_hole = hole;
	this->size = size;
}

void Header::operator() (bool is_hole, uint32 size)
{
	this->is_hole = is_hole;
	this->size = size;
	magic = HEAP_MAGIC;
}

void Header::Print()
{
	printf("Header location: %h\nMagic Number: %h\nHeader size: %h\nIs hole: %b\n", &(*this), magic, size, is_hole);
}

Footer::Footer()
{
	magic = HEAP_NO_INITIALIZE;
	header = 0;
}

Footer::Footer(Header* head)
{
	header = head;
	magic = HEAP_NO_INITIALIZE;
}

bool Header_Lessthan_Predicate(void* a, void* b)
{
	return ( ((Header*)a)->size < ((Header*)b)->size );
}

Heap::Heap()
{
	printf("HeapManager created\n");
	start_address = end_address = max_address = 0;
	supervisor = readonly = false;
	manager = 0;
}

Heap::Heap(uint32 start, uint32 end, uint32 max, bool supervisor, bool readonly, MemoryManager* manager)
{
	printf("HeapManager created\n");
	this->supervisor = supervisor;
	this->readonly = readonly;

	this->manager = manager;

	if( ( (start & 0xFFF) != 0 ) || ( (end & 0xFFF) != 0 ) )	// start and end must be page aligned
	{
		PANIC("HEAP ERROR: start and end should be page aligned\n");
		return;
	}

	//index = OrderedArray( (void*) start, HEAP_INDEX_SIZE, Header_Lessthan_Predicate);

	start += sizeof(ordered_type) * HEAP_INDEX_SIZE;

	if( (start & 0xFFF) != 0)	 // page align if needed
	{
		start &= 0xFFFFF000;
		start += 0x1000;
	}

	start_address = start;
	end_address = end;
	max_address = max;

	Header* hole = (Header*) start;		// we start with one big hole in the kernel

	(*hole)(true, end_address - start);

	if(!index.Insert((void*) hole))
		PANIC("insertion error");
}

void Heap::Expand(uint32 new_size)
{
	if(new_size <= end_address - start_address)
		return;

	if( (new_size & 0xFFF) != 0)
	{
		new_size &= 0xFFFFF000;
		new_size += 0x1000;
	}

	if(start_address + new_size > max_address)
		return;

	uint32 old_size = end_address - start_address;
	uint32 i = old_size;

	while(i < new_size)
	{
		//manager->frameManager.AllocFrame(manager->GetPage(start_address + i, true), supervisor, readonly);
		i += 0x1000;
	}

	end_address = start_address + new_size;
}


uint32 Heap::Contract(uint32 new_size)
{
	if(new_size >= end_address - start_address)		// we need to shrink the heap so the new size needs to be obviously less
		return 0;

	if( (new_size & 0xFFF) != 0)					// page align the new size
	{
		new_size &= 0xFFFFF000;
		new_size += 0x1000;
	}

	if(new_size < HEAP_MIN_SIZE)
		new_size = HEAP_MIN_SIZE;

	uint32 old_size = end_address - start_address;
	uint32 i = old_size - 0x1000;

	while(new_size < i)
	{
		//manager->frameManager.FreeFrame(manager->GetPage(start_address + i, false));
		i -= 0x1000;
	}

	end_address = start_address + new_size;
	return new_size;
}

void* Heap::alloc(uint32 size, bool page_align)
{
	uint32 new_size = size + sizeof(Header) + sizeof(Footer);		// make sure we take into account the size of the header - footer

	int32 iter = FindSmallestHole(new_size, page_align);			// find the smallest hole where we fit

	if(iter == -1)
	{
		PANIC("PANIC: Heap is full");
		// stuff
		return 0;
	}

	Header* header = (Header*) index[iter];							// header for the hole
	uint32 hole_size = header->size;
	index.RemoveItem(iter);											// remove this header as it is a blobk now, not a hole

	char* loc = (char*) header;
	loc += sizeof(Header);											// user space starts

	if(page_align)
	{
		uint32 prev_loc = (uint32)loc, new_loc;		// to be used as uin32 location for the calculations only (so as not to always cast)

		new_size += 0x1000 - (uint32)loc % 0x1000;
		loc += 0x1000 - (uint32)loc % 0x1000;

		new_loc = (uint32)loc;

		// we attempt to insert a hole !!->behind<-!! the location to
		// save space as page-align eats alot of room
		// we need to insert a new Header to our normal page aligned room

		if(new_loc - prev_loc - sizeof(Header) > sizeof(Footer))
		{
			// we use the same current header as we re-adjust the size of this new BEHIND hole and make the insertion
			header->size = new_loc - sizeof(Header) - (uint32)header;
			index.Insert((void*)header);

			Footer* footer = (Footer*)(((char*)header) + header->size - sizeof(Footer));
																				// a footer is needed so as free can find the header
			footer->magic = HEAP_MAGIC;											// as we are in front of this header
			footer->header = header;

			Header* current = (Header*)(new_loc - sizeof(Header));
			header = current;
		}
	}

	// new_size may be modified to account for the hole being asked as page-aligned.

	if(hole_size - new_size > sizeof(Header) + sizeof(Footer))		// if there is enough room, split the hole into two
	{
		Header* split_hole_header = (Header*)(loc + size + sizeof(Footer));
		(*split_hole_header)(true, hole_size - new_size);

		Footer* footer = (Footer*)( (char*)split_hole_header + split_hole_header->size - sizeof(Header));
		footer->magic = HEAP_MAGIC;
		footer->header = split_hole_header;

		index.Insert((void*)split_hole_header);
	}

	Footer* foot = (Footer*)(loc + size);	// location is just after the header and size is the user size without the headers - footers
	foot->magic = HEAP_MAGIC;
	foot->header = header;

	(*header)(false, size + sizeof(Header) + sizeof(Footer));

	return (void*)loc;
}

void Heap::free(void* p)
{
	char* loc = (char*)p;
	Header* user_header = (Header*)(loc - sizeof(Header));

	ASSERT(user_header->magic == HEAP_MAGIC);

	Header* header_to_insert = user_header;
	header_to_insert->is_hole = true;

	bool remove_prev_header = false, remove_front_header = false;

	Footer* prev_footer = (Footer*)(loc - sizeof(Header) - sizeof(Footer));
	if(prev_footer->magic == HEAP_MAGIC)	// this is actually a footer pointing to a header (check made for when we llok at the first block)
	{
		Header* prev_header = prev_footer->header;
		if(prev_header->is_hole == true)	// if the previous is a hole combine the two
		{
			//combine left

			header_to_insert = prev_header;
			header_to_insert->size += user_header->size;
			remove_prev_header = true;
		}
	}

	Header* next_header = (Header*)(loc + user_header->size - sizeof(Header));
	if(next_header->magic == HEAP_MAGIC)		// check for header existance (if we llok at the last header)
	{
		if(next_header->is_hole == true)
		{
			// combine right

			header_to_insert->size += next_header->size;
			remove_front_header = true;
		}
	}

	if(remove_prev_header && prev_footer->magic == HEAP_MAGIC)
		index.RemoveItem(index.GetIndex((void*)prev_footer->header));

	if(remove_front_header && next_header->magic == HEAP_MAGIC)
		index.RemoveItem(index.GetIndex((void*)next_header));

	Footer* foot = (Footer*)((char*)header_to_insert + header_to_insert->size - sizeof(Footer));
	foot->magic = HEAP_MAGIC;
	foot->header = header_to_insert;

	index.Insert((void*)header_to_insert);
}

int32 Heap::FindSmallestHole(uint32 size, bool page_align)
{
	uint32 iter = 0;

	while(iter < index.GetSize())
	{
		Header* header = (Header*)index[iter];

		// handle specially the case where the user has requested page aligned memory
		if(page_align)
		{
			uint32 location = (uint32)header;
			int32 offset = 0;

			if( ( (location + sizeof(Header)) & 0x00000FFF ) != 0)	// the location the user will see (not our header) is not page aligned
				offset = 0x1000 - (location + sizeof(Header)) % 0x1000;

			int32 hole_size = (int32)header->size - offset;			// we need offset memory more

			if(hole_size >= (int32)size)	// enough memory found
				break;
		}
		else if(header->size >= size)
			break;

		iter++;
	}

	if(iter == index.GetSize())
		return -1;
	else
		return iter;
}
