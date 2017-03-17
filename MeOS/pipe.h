#ifndef PIPE_H_08012017
#define PIPE_H_08012017

#include "types.h"
#include "utility.h"
#include "vfs.h"
#include "open_file_table.h"
#include "semaphore.h"

struct _pipe
{
	char* buffer;
	uint32 size;
	uint32 read_pos;
	uint32 write_pos;

	semaphore lock_slots;		// how many buffer places are empty
	semaphore lock_messages;	// how many buffer places are filled
};

_pipe create_pipe(char* buf, uint32 size);
void create_vfs_pipe(char* buf, uint32 size, uint32 fd[2]);

void pipe_close();

vfs_result pipe_vfs_open(vfs_node* node);
uint32 pipe_vfs_read(int fd, vfs_node* file, uint32 start, uint32 count, virtual_addr address);
uint32 pipe_vfs_write(int fd, vfs_node* file, uint32 start, uint32 count, virtual_addr address);

char pipe_read(_pipe* pipe);
void pipe_write(_pipe* pipe, char element);

#endif