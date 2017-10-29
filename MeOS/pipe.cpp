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

// TODO: FIX THAT
void create_vfs_pipe(char* buf, uint32 size, uint32 fd[2])
{
	vfs_node* n = vfs_create_device("pipe", DEVICE_DEFAULT_CAPS, sizeof(_pipe), NULL, &pipe_operations);
	*(_pipe*)n->deep_md = create_pipe(buf, size);

	vfs_open_file(n, 0);

	fd[0] = gft_insert_s(create_gfe(n));
	fd[1] = fd[0];
}

error_t pipe_vfs_open(vfs_node* node, uint32 capabilities)
{
	return ERROR_OK;
}

size_t pipe_vfs_read(uint32 fd, vfs_node* file, uint32 start, size_t count, virtual_addr address)
{
	_pipe* p = (_pipe*)file->deep_md;
	char* buffer = (char*)address;

	for (uint32 i = 0; i < count; i++)
		buffer[i] = pipe_read(p);

	return ERROR_OK;
}

size_t pipe_vfs_write(uint32 fd, vfs_node* file, uint32 start, size_t count, virtual_addr address)
{
	_pipe* p = (_pipe*)file->deep_md;
	char* buffer = (char*)address;

	for (uint32 i = 0; i < count; i++)
		pipe_write(p, buffer[i]);

	return ERROR_OK;
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