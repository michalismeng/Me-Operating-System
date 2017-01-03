#ifndef PAGE_CACHE_H_15112016
#define PAGE_CACHE_H_15112016

#include "types.h"
#include "utility.h"
#include "vfs.h"
#include "vector.h"
#include "mmngr_virtual.h"

struct _page_cache_file_info
{
	int file_page_offset;				// the in-file page offset (used to determine where in the file we point)
	int page_cache_index;				// index in the page cache structure of this file
};

struct _page_cache_file
{
	vfs_node* file_node;					// actual file description node
	vfs_node* file_mount;					// file mount point. Used for file access
	vfs_node* disk_dev;						// disk device where the file is stored
	vector<_page_cache_file_info> pages;	// file pages cached
};

// represents a page in the page cache
struct _cache_cell
{
	char array[4096];						// 4KB array.
};

struct _page_cache
{
	vector<_page_cache_file> cached_files;		// all the files cached descriptors
	_cache_cell* cache;							// cached data
	uint32 cache_size;							// page cache size
};

// create a page_cache_file_info struct
_page_cache_file_info page_cache_file_info_create(int file_offset, int cache_index);

// create a page_cache_file struct
_page_cache_file page_cache_file_create(vfs_node* file, vfs_node* mount, vfs_node* disk);

// given a in-file page returns the cache index where this page is read or -1 if not found
int page_cache_file_get_cache_index(_page_cache_file* file, int file_page);

// initialize the page cache
void page_cache_init(uint32 size);

// returns a ?virtual? address where the requested file contents are loaded
virtual_addr page_cache_read(_page_cache_file* file, int file_page);

_page_cache* page_cache_get();

#endif