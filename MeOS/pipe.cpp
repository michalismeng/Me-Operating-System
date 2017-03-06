#include "pipe.h"

static fs_operations pipe_operations =
{
	pipe_vfs_read,		// read
	pipe_vfs_write,		// write
	pipe_vfs_open		// open
						// close
						// lookup
						// ioctl?
};

_pipe create_pipe(char* buf, uint32 size)
{
	_pipe pipe;

	pipe.buffer = buf;
	pipe.size = size;
	pipe.read_pos = pipe.write_pos = 0;

	semaphore_init(&pipe.lock_messages, 0);
	semaphore_init(&pipe.lock_slots, size);

	return pipe;
}

void create_vfs_pipe(char* buf, uint32 size, uint32 fd[2])
{
	vfs_node* n = vfs_create_device("pipe", sizeof(_pipe), NULL, &pipe_operations);
	*(_pipe*)n->deep_md = create_pipe(buf, size);

	vfs_open_file(n);

	fd[0] = gft_insert_s(create_gfe(n));
	fd[1] = fd[0];
}

vfs_result pipe_vfs_open(vfs_node* node)
{
	return VFS_OK;
}

vfs_result pipe_vfs_read(vfs_node* file, uint32 page, virtual_addr address)
{
	_pipe* p = (_pipe*)file->deep_md;
	char read = pipe_read(p);
	*(char*)(address) = read;

	return VFS_ERROR::VFS_OK;
}

vfs_result pipe_vfs_write(vfs_node* file, uint32 page, virtual_addr address)
{
	_pipe* p = (_pipe*)file->deep_md;
	char write = *(char*)(address);
	pipe_write(p, write);

	return VFS_ERROR::VFS_OK;
}

char pipe_read(_pipe* pipe)
{
	semaphore_wait(&pipe->lock_messages);		// wait for some message to arrive

	char result = pipe->buffer[pipe->read_pos];
	pipe->read_pos = (pipe->read_pos + 1) % pipe->size;

	semaphore_signal(&pipe->lock_slots);		// inform lock mechanism that a message has been removed

	return result;
}

void pipe_write(_pipe* pipe, char element)
{
	semaphore_wait(&pipe->lock_slots);			// wait for some slot to become available

	pipe->buffer[pipe->write_pos] = element;
	pipe->write_pos = (pipe->write_pos + 1) % pipe->size;

	semaphore_signal(&pipe->lock_messages);		// inform lock mechanism that a message has been inseted
}