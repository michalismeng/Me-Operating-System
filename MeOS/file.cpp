#include "file.h"
#include "thread_sched.h"

uint32 open_file(char* path, int* fd, uint32 flags)
{
	*fd = INVALID_FD;
	vfs_node* node = 0;

	// vfs find the node requested
	uint32 error = vfs_root_lookup(path, &node);
	if (error)
		return error;

	node->flags = flags;

	return open_file_by_node(node, fd);
}

uint32 open_file_by_node(vfs_node* node, int* local_fd)
{
	*local_fd = INVALID_FD;

	lfe entry = create_lfe(FILE_READ);
	*local_fd = lft_insert(&process_get_current()->lft, entry, node);

	uint32 global_fd = lft_get(&process_get_current()->lft, *local_fd)->gfd;

	if (gft_get(global_fd)->open_count > 1)
		page_cache_register_file(global_fd);
		
	return vfs_open_file(node);
}

uint32 read_file(int fd, uint32 start, uint32 count, virtual_addr buffer)
{
	uint32 global_fd = lft_get(&process_get_current()->lft, fd)->gfd;

	gfe* entry = gft_get(global_fd);
	if (!entry || gfe_is_invalid(entry))
		return -1;

	return vfs_read_file(global_fd, entry->file_node, start, count, buffer);
}

uint32 write_file(int fd, uint32 start, uint32 count, virtual_addr buffer)
{
	uint32 global_fd = lft_get(&process_get_current()->lft, fd)->gfd;

	gfe* entry = gft_get(global_fd);
	if (!entry || gfe_is_invalid(entry))
		return -1;

	return vfs_write_file(global_fd, entry->file_node, start, count, buffer);
}

uint32 sync_file(int fd, uint32 start_page, uint32 end_page)
{
	uint32 global_fd = lft_get(&process_get_current()->lft, fd)->gfd;

	gfe* entry = gft_get(global_fd);
	if (!entry || gfe_is_invalid(entry))
		return -1;

	return vfs_sync(global_fd, entry->file_node, start_page, end_page);
}