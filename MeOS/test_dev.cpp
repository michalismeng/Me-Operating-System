#include "test_dev.h"

size_t test_dev_read(uint32 fd, vfs_node* file, uint32 start, size_t count, virtual_addr address);
size_t test_dev_write(uint32 fd, vfs_node* file, uint32 start, size_t count, virtual_addr address);

static fs_operations test_dev_ops = 
{
	test_dev_read,		// read
	test_dev_write,		// write
	NULL,				// open
	NULL,				// close
	NULL,				// sync
	NULL,				// lookup
	NULL				// ioctl
};

uint8 buffer[4 * 4096] = { 0 };

size_t test_dev_read(uint32 fd, vfs_node* file, uint32 start, size_t count, virtual_addr address)
{
	serial_printf("-----test dev reading-----\n");
	memcpy((void*)address, buffer + start, count);
	return count;
}

size_t test_dev_write(uint32 fd, vfs_node* file, uint32 start, size_t count, virtual_addr address)
{
	serial_printf("-----test dev writing-----\n");
	serial_printf("start: %u\n", start);
	memcpy(buffer + start, (void*)address, count);
	return count;
}

void init_test_dev()
{
	vfs_node* dev = vfs_create_device("test_dev", VFS_CAP_CACHE | VFS_CAP_READ | VFS_CAP_WRITE, 0, 0, &test_dev_ops);

	dev->file_length = 4 * 4096;

	for (int i = 0; i < 4 * 4096; i++)
		buffer[i] = i;
}
