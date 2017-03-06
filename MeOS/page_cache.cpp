#include "page_cache.h"

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
	return index * PAGE_CACHE_SIZE;
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

// public functions

void page_cache_init(virtual_addr start, uint32 no_buffers)
{
	page_cache.cache_size = no_buffers * PAGE_CACHE_SIZE;		// size entries = size pages

	//// naive setup of memory for page cache
	//uint32 page = 0x80000000;
	//for (uint32 i = 0; i < size; i++, page += 4096)
	//	if (!vmmngr_alloc_page(page))
	//		PANIC("PAGE CACHE ERROR");

	page_cache.cache = (_cache_cell*)start;
	vector_init(&page_cache.cached_files, 1);

	// Here we assume that alloced bitmap fits into just one page buffer. Perhaps change this in the future.
	// auto reserve the last buffer as this is where the alloced bitmap lives.

	virtual_addr last_buffer = page_cache_get_last();

	alloced_bitmap = (uint8*)last_buffer;
	memset(alloced_bitmap, 0, PAGE_CACHE_SIZE);

	page_cache_index_reserve_buffer(page_cache_index_by_addr(last_buffer));
}

virtual_addr page_cache_get_buffer(int gfd, uint32 page)
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

virtual_addr page_cache_reserve_buffer(int gfd, uint32 page)
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

	// associate the buffer with the given gfd + page
	// assume gfd entry exists but file info doesn't

	_page_cache_file_info finfo = page_cache_file_info_create(page, free_buf);
	list_insert_back(&page_cache.cached_files[gfd].pages, finfo);

	return page_cache_addr_by_index(free_buf);
}

void page_cache_release_buffer(int gfd, uint32 page)
{
	virtual_addr buffer = page_cache_get_buffer(gfd, page);
	uint32 index = page_cache_index_by_addr(buffer);

	// remove index from page list
	auto list = page_cache.cached_files[gfd].pages;
	auto prev = list.head;

	if (prev == 0)
	{
		DEBUG("Page cache release got zero length page list");
		return;
	}

	if (prev->data.page == page)
		list_remove_front(&list);

	while (prev->next != 0)
	{
		if (prev->next->data.page == page)
		{
			list_remove(&list, prev);
			break;
		}
		prev = prev->next;
	}

	// release from page cache
	page_cache_index_release_buffer(index);
}

void page_cache_register_file(int gfd)
{
	// this is a new descriptor
	if (gfd >= page_cache.cached_files.count)
	{
		_page_cache_file file;

		file.gfd = gfd;
		list_init(&file.pages);

		vector_insert_back(&page_cache.cached_files, file);
	}
	else		// descriptor already existed so overwrite it.
	{
		page_cache.cached_files[gfd].gfd = gfd;
		list_clear(&page_cache.cached_files[gfd].pages);		// the list should be clear, but ensure it.
		list_init(&page_cache.cached_files[gfd].pages);			// and re-init our list.
	}
}

void page_cache_unregister_file(int gfd)
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
		printfln("gfd: %u", page_cache.cached_files[i].gfd);

	printf("alloced: ");
	for (uint32 i = 0; i < page_cache_num_buffers(); i++)
		if (alloced_bitmap[i])
			printf("%u ", i);

	printfln("\n");
}

_page_cache* page_cache_get()
{
	return &page_cache;
}