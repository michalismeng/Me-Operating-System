#ifndef FILE_H_17032017
#define FILE_H_17032017

#include "types.h"
#include "vfs.h"
#include "open_file_table.h"
#include "page_cache.h"

/* Defines the standard file io API */

// opens a file and associates a global file descriptor with it
uint32 open_file(char* path, int* fd);

// opens a file indicated by the given node and associates a global file descriptor with it
uint32 open_file_by_node(vfs_node* node, int* fd);

// reads the file, given its global file descriptor, to the given buffer
uint32 read_file(int fd, uint32 start, uint32 count, virtual_addr buffer);

// writes to the file, given its global file descriptor, from the given buffer
uint32 write_file(int fd, uint32 start, uint32 count, virtual_addr buffer);

// syncs the file, given its global file descriptor
uint32 sync_file(int fd, uint32 start_page, uint32 end_page);

#endif