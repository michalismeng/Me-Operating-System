#include "page_cache.h"

// private data
_page_cache page_cache;			// the global page cache

_page_cache_file_info page_cache_file_info_create(int file_offset, int cache_index)
{
	_page_cache_file_info info;
	info.file_page_offset = file_offset;
	info.page_cache_index = cache_index;

	return info;
}

_page_cache_file page_cache_file_create(vfs_node* file, vfs_node* mount, vfs_node* disk)
{
	_page_cache_file _file;
	_file.file_mount = mount;
	_file.file_node = file;
	_file.disk_dev = disk;
	vector_init(&_file.pages, 1);

	return _file;
}

int page_cache_file_get_cache_index(_page_cache_file* file, int file_page)
{
	for (uint32 i = 0; i < file->pages.count; i++)
		if (file->pages[i].file_page_offset == file_page)
			return file->pages[i].page_cache_index;

	return -1;
}

void page_cache_init(uint32 size)
{
	page_cache.cache_size = size;		// size entries = size pages

	// naive setup of memory for page cache
	uint32 page = 0x80000000;
	for (uint32 i = 0; i < size; i++, page += 4096)
		if (!vmmngr_alloc_page(page))
			PANIC("PAGE CACHE ERROR");

	page_cache.cache = (_cache_cell*)0x80000000;
	vector_init(&page_cache.cached_files, 1);
}

virtual_addr page_cache_read(_page_cache_file* file, int file_page)
{
	// test if file page has already been read.
	int index = page_cache_file_get_cache_index(file, file_page);

	if (index != -1)
		return (virtual_addr)&page_cache.cache[index];

	// else read the file
	file->file_node->shallow_md.funcions.fs_read(file->file_mount, (mass_storage_info*)file->disk_dev->deep_md, file->file_node,
		file_page, (virtual_addr)page_cache.cache);

	return (virtual_addr)page_cache.cache;
}

_page_cache* page_cache_get()
{
	return &page_cache;
}