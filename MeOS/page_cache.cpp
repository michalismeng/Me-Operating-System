#include "page_cache.h"
#include "print_utility.h"

// private data
_page_cache page_cache;			// the global page cache
uint8* alloced_bitmap;			// bitmap containing the buffers allocated

// private functions

uint32 page_cache_num_buffers()
{
	return page_cache.cache_size / PAGE_CACHE_SIZE;
}

uint32 page_cache_index_by_addr(virtual_addr address)
{
	return (address - (uint32)page_cache.cache) / PAGE_CACHE_SIZE;
}

virtual_addr page_cache_addr_by_index(uint32 index)
{
	return (virtual_addr)(page_cache.cache) + index * PAGE_CACHE_SIZE;
}

virtual_addr page_cache_get_last()
{
	return (virtual_addr)(page_cache.cache + page_cache_num_buffers() - 1);
}

// returns the first free buffer index
uint32 page_cache_index_free_buffer()
{
	uint32 buffers = page_cache_num_buffers();

	for (uint32 i = 0; i < page_cache_num_buffers(); i++)
	{
		// TODO: optimize to use one bit per allocation
		if (alloced_bitmap[i] == 0)
			return i;
	}

	// not found. return invalid index == number of buffers.
	return buffers;
}

// reserves an unallocated buffer using its index to retrieve it.
void page_cache_index_reserve_buffer(uint32 index)
{
	alloced_bitmap[index] = 1;	// TODO: optimize as above
}

// releases the allocated buffer indexed by index
void page_cache_index_release_buffer(uint32 index)
{
	alloced_bitmap[index] = 0;	// TODO: optimize as above
}

// create a page_cache_file_info struct
_page_cache_file_info page_cache_file_info_create(uint32 page, uint32 buffer_index)
{
	_page_cache_file_info finfo;
	finfo.page = page;
	finfo.buffer_index = buffer_index;

	return finfo;
}

_page_cache_file* page_cache_get_file(uint32 fd)
{
	for (uint32 i = 0; i < page_cache.cached_files.count; i++)
		if (page_cache.cached_files[i].gfd == fd)
			return &page_cache.cached_files[i];

	return 0;
}

_page_cache_file* page_cache_get_first_unused()
{
	// trick here.
	return page_cache_get_file(-1);
}

// public functions

void page_cache_init(virtual_addr start, uint32 no_buffers, uint32 initial_file_count)
{
	page_cache.cache_size = no_buffers * PAGE_CACHE_SIZE;		// size entries = size pages

	page_cache.cache = (_cache_cell*)start;
	vector_init(&page_cache.cached_files, initial_file_count);		// TODO: If(error == error_occur)

	// Here we assume that alloced bitmap fits into just one page buffer. 
	// TODO: Perhaps change this in the future.
	// auto reserve the last buffer as this is where the alloced bitmap lives.

	virtual_addr last_buffer = page_cache_get_last();

	alloced_bitmap = (uint8*)last_buffer;
	memset(alloced_bitmap, 0, PAGE_CACHE_SIZE);

	page_cache_index_reserve_buffer(page_cache_index_by_addr(last_buffer));
}

virtual_addr page_cache_get_buffer(uint32 gfd, uint32 page)
{
	_page_cache_file* file = &page_cache.cached_files[gfd];

	auto temp = file->pages.head;
	while (temp != 0)
	{
		if (temp->data.page == page)
			break;
		temp = temp->next;
	}

	// page not found. No buffer is allocated. Return failure.
	if (temp == 0)
		return 0;

	return (virtual_addr)(page_cache.cache + temp->data.buffer_index);
}

virtual_addr page_cache_reserve_anonymous()
{
	// find the first free buffer index
	uint32 free_buf = page_cache_index_free_buffer();

	// could not find free buffer. Die!
	if (free_buf >= page_cache_num_buffers())
	{
		DEBUG("Could not find empty page cache buffer");
		return 0;
	}

	// reserve the found buffer
	page_cache_index_reserve_buffer(free_buf);
	virtual_addr address = page_cache_addr_by_index(free_buf);
	
	// Pages are not freed so always check to see if they are already present
	//if (vmmngr_is_page_present(address) == false)	// HUGE BUG. If page is present and an allocation happens the software is updated but the TLB still points to the previous entry. Now the vmmngr is updated to check already alloced pages.
	vmmngr_alloc_page(address);

	// if page is present and page is re-allocated then vmmngr_flush_TLB_entry(address);

	return address;
}

void page_cache_release_anonymous(virtual_addr address)
{
	uint32 index = page_cache_index_by_addr(address);
	page_cache_index_release_buffer(index);
	//vmmngr_free_page_addr(buffer);  // TODO: Decide if we need to include this cleaning line. Perhaps the cache will eat up space until it reaches a lethal point
}

virtual_addr page_cache_reserve_buffer(uint32 gfd, uint32 page)
{
	virtual_addr address = page_cache_reserve_anonymous();
	
	if (address == 0)
		return 0;

	uint32 free_buf = page_cache_index_by_addr(address);

	// associate the buffer with the given gfd + page
	// assume gfd entry exists but file info doesn't

	_page_cache_file_info finfo = page_cache_file_info_create(page, free_buf);
	list_insert_back(&page_cache.cached_files[gfd].pages, finfo);
	// TODO: This function gives a buffer even to a non-registered gfd. We first need to assert that cached_files[gfd] is allocated
	printfln("cached file gfd: %u", page_cache.cached_files[gfd].gfd);

	return address;
}

// TODO: Replace common functionality with release anonymous
void page_cache_release_buffer(uint32 gfd, uint32 page)
{
	virtual_addr buffer = page_cache_get_buffer(gfd, page);
	uint32 index = page_cache_index_by_addr(buffer);

	// remove index from page list
	auto list = &page_cache.cached_files[gfd].pages;
	auto prev = list->head;

	if (prev == 0)
	{
		DEBUG("Page cache release got zero length page list");
		return;
	}

	bool found = false;

	if (prev->data.page == page)
	{
		list_remove_front(list);
		found = true;
	}
	else
	{
		while (prev->next != 0)
		{
			if (prev->next->data.page == page)
			{
				list_remove(list, prev);
				found = true;
				break;
			}
			prev = prev->next;
		}
	}
	
	if (!found)
	{
		DEBUG("Pag enot found to release");
		return;
	}
	// release from page cache
	page_cache_index_release_buffer(index);
	//vmmngr_free_page_addr(buffer);  // TODO: Decide if we need to include this cleaning line. Perhaps the cache will eat up space until it reaches a lethal point
}

void page_cache_register_file(uint32 gfd)
{
	// TODO: This is true only if no descriptor is removed and then replaced
	// this is a new descriptor
	
	// if the file exists, we do nothing.
	// if the file doesnt exist then we must create an entry for it either add a new one, or consume an unused one.
	if (page_cache_get_file(gfd) == 0)
	{
		_page_cache_file* unused = page_cache_get_first_unused();
		if (unused == 0)		// page cache entries vector is full so extend it
		{
			_page_cache_file file;
			file.gfd = gfd;
			list_init(&file.pages);

			vector_insert_back(&page_cache.cached_files, file);
		}
		else					// replace the empty entry found with the new one
		{
			unused->gfd = gfd;
			list_init(&unused->pages);
		}

	}

	//if (gfd >= page_cache.cached_files.count)
	//{
	//	_page_cache_file file;

	//	file.gfd = gfd;
	//	list_init(&file.pages);

	//	vector_insert_back(&page_cache.cached_files, file);
	//}
	//else		// descriptor already existed so overwrite it.
	//{
	//	page_cache.cached_files[gfd].gfd = gfd;
	//	list_clear(&page_cache.cached_files[gfd].pages);		// the list should be clear, but ensure it.
	//	list_init(&page_cache.cached_files[gfd].pages);			// and re-init our list.
	//}
}

void page_cache_unregister_file(uint32 gfd)
{
	if (gfd >= page_cache.cached_files.count)		// kinda erroneous gfd
	{
		DEBUG("Page cache unregister got invalid gfd");
		return;
	}

	page_cache.cached_files[gfd].gfd = -1;				// invalid global descriptor

	// release all buffers associated with this file
	auto temp = page_cache.cached_files[gfd].pages.head;
	while (temp != 0)
	{
		page_cache_index_release_buffer(temp->data.buffer_index);
		temp = temp->next;
	}

	list_clear(&page_cache.cached_files[gfd].pages);	// empty page list.
}

void page_cache_print()
{
	for (uint32 i = 0; i < page_cache.cached_files.count; i++)
	{
		printf("gfd %u:", page_cache.cached_files[i].gfd);
		for (auto temp = page_cache.cached_files[i].pages.head; temp != 0; temp = temp->next)
			printf("%u ", temp->data);

		printfln("");
	}

	printfln("alloced: ");
	
	for (uint32 i = 0; i < page_cache_num_buffers(); i++)
		if (alloced_bitmap[i])
			printf("%u ", i);

	printfln("\n");
}

_page_cache* page_cache_get()
{
	return &page_cache;
}