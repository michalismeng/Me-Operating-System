#include "file.h"

uint32 open_file(char* path, int* fd)
{
	*fd = INVALID_FD;
	vfs_node* node = 0;

	// vfs find the node requested
	uint32 error = vfs_root_lookup(path, &node);
	if (error)
		return error;

	return open_file_by_node(node, fd);
}

uint32 open_file_by_node(vfs_node* node, int* fd)
{
	*fd = INVALID_FD;
	gfe entry = create_gfe(node);
	*fd = gft_insert_s(entry);

	page_cache_register_file(*fd);

	return vfs_open_file(node);
}

uint32 read_file(int fd, uint32 start, uint32 count, virtual_addr buffer)
{
	gfe* entry = gft_get(fd);
	if (!entry || gfe_is_invalid(entry))
		return -1;

	return vfs_read_file(fd, entry->file_node, start, count, buffer);
}

uint32 write_file(int fd, uint32 start, uint32 count, virtual_addr buffer)
{
	gfe* entry = gft_get(fd);
	if (!entry || gfe_is_invalid(entry))
		return -1;

	return vfs_write_file(fd, entry->file_node, start, count, buffer);
}

uint32 sync_file(int fd, uint32 start_page, uint32 end_page)
{
	gfe* entry = gft_get(fd);
	if (!entry || gfe_is_invalid(entry))
		return -1;

	return vfs_sync(fd, entry->file_node, start_page, end_page);
}