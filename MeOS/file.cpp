#include "file.h"
#include "thread_sched.h"
#include "print_utility.h"

error_t open_file(char* path, uint32* fd, uint32 flags)
{
	*fd = INVALID_FD;
	vfs_node* node = 0;

	// vfs find the node requested
	if (vfs_root_lookup(path, &node) != ERROR_OK)
		return ERROR_OCCUR;

	node->flags = flags;

	return open_file_by_node(node, fd);
}

error_t open_file_by_node(vfs_node* node, uint32* local_fd)
{
	*local_fd = INVALID_FD;

	lfe entry = create_lfe(FILE_READ);
	*local_fd = lft_insert(&process_get_current()->lft, entry, node);

	uint32 global_fd = lft_get(&process_get_current()->lft, *local_fd)->gfd;

	printfln("file: %s open count: %u", gft_get(global_fd)->file_node->name, gft_get(global_fd)->open_count);
	if (gft_get(global_fd)->open_count == 1)
		page_cache_register_file(global_fd);
		
	return vfs_open_file(node);
}

size_t read_file(uint32 fd, uint32 start, size_t count, virtual_addr buffer)
{
	uint32 global_fd = lft_get(&process_get_current()->lft, fd)->gfd;

	gfe* entry = gft_get(global_fd);
	if (!entry || gfe_is_invalid(entry))
	{
		set_last_error(EBADF, FILE_GFD_NOT_FOUND, EO_FILE_INTERFACE);
		return 0;
	}

	return vfs_read_file(global_fd, entry->file_node, start, count, buffer);
}

size_t write_file(uint32 fd, uint32 start, size_t count, virtual_addr buffer)
{
	uint32 global_fd = lft_get(&process_get_current()->lft, fd)->gfd;

	gfe* entry = gft_get(global_fd);
	if (!entry || gfe_is_invalid(entry))	
	{
		set_last_error(EBADF, FILE_GFD_NOT_FOUND, EO_FILE_INTERFACE);
		return 0;
	}

	return vfs_write_file(global_fd, entry->file_node, start, count, buffer);
}

error_t sync_file(uint32 fd, uint32 start_page, uint32 end_page)
{
	uint32 global_fd = lft_get(&process_get_current()->lft, fd)->gfd;

	gfe* entry = gft_get(global_fd);
	if (!entry || gfe_is_invalid(entry))
	{
		set_last_error(EBADF, FILE_GFD_NOT_FOUND, EO_FILE_INTERFACE);
		return ERROR_OCCUR;
	}

	return vfs_sync(global_fd, entry->file_node, start_page, end_page);
}