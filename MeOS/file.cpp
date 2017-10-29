#include "file.h"
#include "thread_sched.h"
#include "print_utility.h"

// private functions

inline bool file_validate_capabilities(uint32 base_caps, uint32 required_caps)
{
	return (base_caps & (required_caps & VFS_CAP_MAX)) == (required_caps & VFS_CAP_MAX);
}

// public functions

error_t open_file(char* path, uint32* fd, uint32 capabilities)
{
	*fd = INVALID_FD;
	vfs_node* node = 0;

	// vfs find the node requested
	if (vfs_root_lookup(path, &node) != ERROR_OK)
		return ERROR_OCCUR;

	node->flags = 0;//flags;

	return open_file_by_node(node, fd, capabilities);
}

error_t open_file_by_node(vfs_node* node, uint32* local_fd, uint32 capabilities)
{
	// validate the file capabilities
	if (file_validate_capabilities(node->capabilities, capabilities) == false)
	{
		serial_printf("access denied: %s %h %h\n", node->name, node->capabilities, capabilities);
		set_last_error(EACCES, FILE_CAPABILITIES_ERROR, EO_FILE_INTERFACE);
		return ERROR_OCCUR;
	}

	gfe entry = create_gfe(node);
	uint32 gfd = gft_insert_s(entry);

	if (gfd == INVALID_FD)
		return ERROR_OCCUR;

	// TODO: Consider locking the lft from this point
	lfe local_entry = create_lfe(gfd, capabilities);
	*local_fd = lft_insert(&process_get_current()->lft, local_entry);

	if (*local_fd == INVALID_FD)
	{
		// on failure we do not entirely delete the gfe (even if this is the first time to open) as it may be re-opened in the future
		// TODO: Perhaps this should be decrement_raw
		gfe_decrement_open_count(gfd);
		return ERROR_OCCUR;
	}
		
	return vfs_open_file(node, capabilities);
}

size_t read_file(uint32 fd, uint32 start, size_t count, virtual_addr buffer)
{
	// TODO: Lock local entry
	lfe* local_entry = lft_get(&process_get_current()->lft, fd);
	uint32 global_fd = local_entry->gfd;

	gfe* entry = gft_get(global_fd);
	if (!entry || gfe_is_invalid(entry))
	{
		set_last_error(EBADF, FILE_GFD_NOT_FOUND, EO_FILE_INTERFACE);
		return 0;
	}

	// check read capabilities
	if (file_validate_capabilities(VFS_CAP_READ, local_entry->flags) == false)
	{
		set_last_error(EACCES, FILE_READ_ACCESS_DENIED, EO_FILE_INTERFACE);
		return 0;
	}

	if (file_validate_capabilities(VFS_CAP_CACHE, local_entry->flags) == true)
	{
		// initiate caching and do 4KB chunks of data
		// segment the 'count' bytes into 4KB regions => read them to the page cache => copy to user buffer

		if ((count % PAGE_SIZE != 0) || (start % PAGE_SIZE != 0))
		{
			set_last_error(EINVAL, FILE_UNALIGED_ADDRESS, EO_FILE_INTERFACE);
			return 0;
		}

		uint32 total_pages = count / PAGE_SIZE;
		size_t bytes_read = 0;

		// TODO: Detect end of file
		for (uint32 i = 0; i < total_pages; i++)
		{
			uint32 page = start / PAGE_SIZE + i;

			virtual_addr cache = page_cache_get_buffer(global_fd, page);

			// if the area hasn't been read => read it
			if (cache == 0)
			{
				cache = page_cache_reserve_buffer(global_fd, page);

				if (vfs_read_file(global_fd, entry->file_node, start + i * PAGE_SIZE, PAGE_SIZE, cache) != PAGE_SIZE)
					return bytes_read;
			}

			bytes_read += PAGE_SIZE;
			memcpy((uint8*)buffer + i * PAGE_SIZE, (void*)cache, PAGE_SIZE);
		}

		return bytes_read;
	}
	else
	{
		// the user requires immediate read to his buffer
		// it is up to the driver to check count validity or do segmented data loading (in specific manageable chunks)
		return vfs_read_file(global_fd, entry->file_node, start, count, buffer);
	}

	return 0;
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