#ifndef PAGE_CACHE_H_15112016
#define PAGE_CACHE_H_15112016

#include "types.h"
#include "utility.h"
#include "vfs.h"
#include "vector.h"
#include "mmngr_virtual.h"

#define PAGE_CACHE_SIZE 4096

enum PAGE_CACHE_ERROR
{
	PAGE_CACHE_NONE,
	PAGE_CACHE_OUT_OF_BOUNDS,
	PAGE_CACHE_INVALID,				// Invalid file descriptor
	PAGE_CACHE_DEPLET,
	PAGE_CACHE_BAD_PAGES,
	PAGE_CACHE_PAGE_NOT_FOUND,
	PAGE_CACHE_FINFO_NOT_FOUND
};

struct _page_cache_file_info
{
	uint32 page;			// the in-file page offset (used to determine where in the file we point)
	uint32 buffer_index;	// index in the page cache structure of this file page
	bool dirty;				// set if the page is dirty => has been written to
};

//struct _page_cache_file
//{
//	uint32 gfd;
//	list<_page_cache_file_info> pages;		// cached page indices
//};

// represents a page in the page cache
struct _cache_cell
{
	char array[PAGE_CACHE_SIZE];				// 4KB array.
};

struct _page_cache
{
	//vector<_page_cache_file> cached_files;		// all the cached files descriptors
	_cache_cell* cache;							// cached data
	uint32 cache_size;							// page cache size
};

// initialize the page cache
error_t page_cache_init(virtual_addr start, uint32 no_buffers);

// returns the virtual address of the buffer assigned to the given page in the given file.
virtual_addr page_cache_get_buffer(uint32 gfd, uint32 page);

// reserves a buffer without associating it with any file descriptor. Returns its virtual address.
virtual_addr page_cache_reserve_anonymous();

// releases the buffer indicated by address.
void page_cache_release_anonymous(virtual_addr address);

// reserves a buffer and associates it with the given file descriptor and file page. Returns its virtual address.
virtual_addr page_cache_reserve_buffer(uint32 gfd, uint32 page);

// releases a buffer that is associated with the given file descriptor and page.
error_t page_cache_release_buffer(uint32 gfd, uint32 page);

// modifies the dirty flag for the given page
error_t page_cache_make_dirty(uint32 gfd, uint32 page, bool dirty);

// returns the dirty flag of the requested page
bool page_cache_is_page_dirty(uint32 gfd, uint32 page);

// registers a file for caching services using its global file descriptor. This is needed prior to any caching function call.
//error_t page_cache_register_file(uint32 gfd);

// removes a file from the caching services. After that, the file can no longer use the caching services.
//error_t page_cache_unregister_file(uint32 gfd);

void page_cache_print();

_page_cache* page_cache_get();

#endif