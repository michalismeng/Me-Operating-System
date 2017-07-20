#ifndef PAGE_CACHE_H_15112016
#define PAGE_CACHE_H_15112016

#include "types.h"
#include "utility.h"
#include "vfs.h"
#include "vector.h"
#include "mmngr_virtual.h"

#define PAGE_CACHE_SIZE 4096

struct _page_cache_file_info
{
	uint32 page;			// the in-file page offset (used to determine where in the file we point)
	uint32 buffer_index;	// index in the page cache structure of this file page
};

struct _page_cache_file
{
	int gfd;
	list<_page_cache_file_info> pages;		// cached page indices
};

// represents a page in the page cache
struct _cache_cell
{
	char array[4096];						// 4KB array.
};

struct _page_cache
{
	vector<_page_cache_file> cached_files;		// all the cached files descriptors
	_cache_cell* cache;							// cached data
	uint32 cache_size;							// page cache size
};

// initialize the page cache
void page_cache_init(virtual_addr start, uint32 no_buffers, uint32 initial_file_count);

// returns the virtual address of the buffer assigned to the given page in the given file.
virtual_addr page_cache_get_buffer(int gfd, uint32 page);

// reserves a buffer without associating it with any file descriptor. Returns its virtual address.
virtual_addr page_cache_reserve_anonymous();

// releases the buffer indicated by address.
void page_cache_release_anonymous(virtual_addr address);

// reserves a buffer and associates it with the given file descriptor and file page. Returns its virtual address.
virtual_addr page_cache_reserve_buffer(int gfd, uint32 page);

// releases a buffer that is associated with the given file descriptor and page.
void page_cache_release_buffer(int gfd, uint32 page);

// registers a file for caching services using its global file descriptor. This is needed prior to any caching function call.
void page_cache_register_file(int gfd);

// removes a file from the caching services. After that, the file can no longer use the caching services.
void page_cache_unregister_file(int gfd);

void page_cache_print();

_page_cache* page_cache_get();

#endif