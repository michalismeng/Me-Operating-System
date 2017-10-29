#include "page_cache.h"
#include "open_file_table.h"
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

// TODO: TO BE DELETED
_page_cache_file* page_cache_get_file(uint32 fd)
{
	/*for (uint32 i = 0; i < page_cache.cached_files.count; i++)
		if (page_cache.cached_files[i].gfd == fd)
			return &page_cache.cached_files[i];*/

	return 0;
}

// TODO: TO BE DELETED
_page_cache_file* page_cache_get_first_unused()
{
	// trick here.
	return page_cache_get_file(INVALID_FD);
}

// public functions

error_t page_cache_init(virtual_addr start, uint32 no_buffers)
{
	page_cache.cache_size = no_buffers * PAGE_CACHE_SIZE;		// size entries = size pages

	page_cache.cache = (_cache_cell*)start;

	// Here we assume that alloced bitmap fits into just one page buffer. 
	// TODO: Perhaps change this in the future.

	// auto reserve the last buffer as this is where the alloced bitmap lives.
	virtual_addr last_buffer = page_cache_get_last();

	alloced_bitmap = (uint8*)last_buffer;
	memset(alloced_bitmap, 0, PAGE_CACHE_SIZE);

	page_cache_index_reserve_buffer(page_cache_index_by_addr(last_buffer));

	return ERROR_OK;
}

virtual_addr page_cache_get_buffer(uint32 gfd, uint32 page)
{
	if (gfd >= gft_get_table()->count)
	{
		set_last_error(EBADF, PAGE_CACHE_OUT_OF_BOUNDS, EO_PAGE_CACHE);
		return 0;
	}

	if (gft_get_table()->data[gfd].file_node == 0)
	{
		set_last_error(EBADF, PAGE_CACHE_INVALID, EO_PAGE_CACHE);
		return 0;
	}

	auto temp = gft_get_table()->data[gfd].pages.head;
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
		set_last_error(ENOMEM, PAGE_CACHE_DEPLET, EO_PAGE_CACHE);
		return 0;
	}

	// reserve the found buffer
	page_cache_index_reserve_buffer(free_buf);
	virtual_addr address = page_cache_addr_by_index(free_buf);

	// virtual memory allocation is now done through the page fault handler (29/10/2017). But the bug below is important as a note.
	
	// Pages are not freed so always check to see if they are already present
	//if (vmmngr_is_page_present(address) == false)	// HUGE BUG. If page is present and an allocation happens the software is updated but the TLB still points to the previous entry. Now the vmmngr is updated to check already alloced pages.
	//if (vmmngr_alloc_page(address) != ERROR_OK)
		//return 0;
	// if page is present and page is re-allocated then vmmngr_flush_TLB_entry(address);

	return address;
}

void page_cache_release_anonymous(virtual_addr address)
{
	uint32 index = page_cache_index_by_addr(address);

	if (index >= page_cache_num_buffers())
		return;

	page_cache_index_release_buffer(index);
	//vmmngr_free_page_addr(buffer); 
	// The cache will eat up space until it reaches a lethal point. Then a special kernel thread will clean up.
}

virtual_addr page_cache_reserve_buffer(uint32 gfd, uint32 page)
{
	/*if (gfd >= page_cache.cached_files.count || page_cache.cached_files[gfd].gfd == INVALID_FD)
	{
		set_last_error(EBADF, PAGE_CACHE_INVALID, EO_PAGE_CACHE);
		return 0;
	}*/

	// check if page is already allocated
	virtual_addr address = page_cache_reserve_anonymous();
	
	if (address == 0)
	{
		set_last_error(ENOMEM, PAGE_CACHE_DEPLET, EO_PAGE_CACHE);
		return 0;
	}

	uint32 free_buf = page_cache_index_by_addr(address);

	// associate the buffer with the given gfd + page
	// assume gfd entry exists but file info doesn't

	_page_cache_file_info finfo = page_cache_file_info_create(page, free_buf);

	list_insert_back(&gft_get(gfd)->pages, finfo);	// TODO : Check for errors in this line

	return address;
}

// TODO: APPLY NEW FIX
error_t page_cache_release_buffer(uint32 gfd, uint32 page)
{
	//if (gfd >= page_cache.cached_files.count || page_cache.cached_files[gfd].gfd == INVALID_FD)		// kinda erroneous gfd
	//{
	//	set_last_error(EBADF, PAGE_CACHE_INVALID, EO_PAGE_CACHE);
	//	return ERROR_OCCUR;
	//}

	uint32 index = -1;

	// remove index from page list
	auto list = &gft_get_table()->data[gfd].pages;
	auto prev = list->head;

	if (prev == 0)
	{
		DEBUG("Page cache release got zero length page list");
		set_last_error(EINVAL, PAGE_CACHE_BAD_PAGES, EO_PAGE_CACHE);
		return ERROR_OCCUR;
	}

	if (prev->data.page == page)
	{
		index = prev->data.buffer_index;
		list_remove_front(list);					// TODO: Check this line for errors
	}
	else
	{
		while (prev->next != 0)
		{
			if (prev->next->data.page == page)
			{
				index = prev->next->data.buffer_index;
				list_remove(list, prev);			// TODO: Check this line for errors
				break;
			}
			prev = prev->next;
		}
	}
	
	if (index == -1)
	{
		DEBUG("Page not found to release");
		set_last_error(EINVAL, PAGE_CACHE_PAGE_NOT_FOUND, EO_PAGE_CACHE);
		return ERROR_OCCUR;
	}

	page_cache_release_anonymous(page_cache_addr_by_index(index));

	return ERROR_OK;
}

// TODO: TO BE DELETED
error_t page_cache_register_file(uint32 gfd)
{	
	// if the file doesnt exist then we must create an entry for it either add a new one, or consume an unused one.
	if (page_cache_get_file(gfd) == 0)
	{
		_page_cache_file* unused = page_cache_get_first_unused();
		if (unused == 0)		// page cache entries vector is full so extend it
		{
			_page_cache_file file;
			file.gfd = gfd;
			list_init(&file.pages);

			//if (vector_insert_back(&page_cache.cached_files, file) != ERROR_OK)
				//return ERROR_OCCUR;
		}
		else					// replace the empty entry found with the new one
		{
			unused->gfd = gfd;
			list_init(&unused->pages);
		}
	}

	return ERROR_OK;

	// if the file exists, we do nothing.
}

// TODO: TO BE DELETED
error_t page_cache_unregister_file(uint32 gfd)
{
	//if (gfd >= page_cache.cached_files.count || page_cache.cached_files[gfd].gfd == INVALID_FD)		// kinda erroneous gfd
	//{
	//	set_last_error(EBADF, PAGE_CACHE_INVALID, EO_PAGE_CACHE);
	//	return ERROR_OCCUR;
	//}

	//page_cache.cached_files[gfd].gfd = INVALID_FD;				// invalid global descriptor

	// release all buffers associated with this file
	//auto temp = page_cache.cached_files[gfd].pages.head;
	//while (temp != 0)
	//{
	//	page_cache_index_release_buffer(temp->data.buffer_index);
	//	temp = temp->next;
	//}

	//// TODO: Check errors for this line
	//list_clear(&page_cache.cached_files[gfd].pages);	// empty page list.	

	return ERROR_OK;
}

void page_cache_print()
{
	for (uint32 i = 0; i < gft_get_table()->count; i++)
	{
		if (gft_get_table()->data[i].pages.count > 0)
		{
			serial_printf("gfd %u %s (page, buf_ind):", i, gft_get_table()->data[i].file_node->name);
			for (auto temp = gft_get_table()->data[i].pages.head; temp != 0; temp = temp->next)
				serial_printf("(%u, %u)", temp->data.page, temp->data.buffer_index);

			serial_printf("\n");
		}
	}

	serial_printf("alloced: \n");
	
	for (uint32 i = 0; i < page_cache_num_buffers(); i++)
		if (alloced_bitmap[i])
			serial_printf("%u ", i);

	serial_printf("\n\n");
}

_page_cache* page_cache_get()
{
	return &page_cache;
}