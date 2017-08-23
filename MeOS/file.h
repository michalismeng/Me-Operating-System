#ifndef FILE_H_17032017
#define FILE_H_17032017

#include "types.h"
#include "vfs.h"
#include "open_file_table.h"
#include "page_cache.h"

#define INVALID_FD -1

enum FILE_OPEN_FLAGS
{
	O_NOCACHE	 = 1,			// when set, the driver does not use the page cache. Every read and write offset and count must be aligned as specified by the driver.
	O_CACHE_ONLY = 2			// when set, any data transfered from a mass storage media to the page cache, remain there and are not copied to the requested address (address is ignored).
};

	/* Defines the standard file io API */

	// opens a file and associates a local file descriptor with it
	uint32 open_file(char* path, int* fd, uint32 flags);

	// opens a file indicated by the given node and associates a local and a global file descriptor with it
	uint32 open_file_by_node(vfs_node* node, int* fd);

	// reads the file, given its global file descriptor, to the given buffer
	uint32 read_file(int fd, uint32 start, uint32 count, virtual_addr buffer);

	// writes to the file, given its global file descriptor, from the given buffer
	uint32 write_file(int fd, uint32 start, uint32 count, virtual_addr buffer);

	// syncs the file, given its global file descriptor
	uint32 sync_file(int fd, uint32 start_page, uint32 end_page);

#endif