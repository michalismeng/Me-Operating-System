#include "file.h"
#include "thread_sched.h"
#include "print_utility.h"
#include "atomic.h"
#include "critlock.h"

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
	if (count > MAX_IO)
	{
		set_last_error(EINVAL, FILE_BIG_REQUEST, EO_FILE_INTERFACE);
		return INVALID_IO;
	}

	// TODO: Lock local entry
	lfe* local_entry = lft_get(&process_get_current()->lft, fd);
	if (local_entry == 0)
		return INVALID_IO;

	uint32 global_fd = local_entry->gfd;
	return read_file_global(global_fd, start, count, buffer, local_entry->flags);
}

size_t read_file_global(uint32 gfd, uint32 start, size_t count, virtual_addr buffer, uint32 capabilities)
{
	gfe* entry = gft_get(gfd);
	if (!entry || gfe_is_invalid(entry))
	{
		set_last_error(EBADF, FILE_GFD_NOT_FOUND, EO_FILE_INTERFACE);
		return INVALID_IO;
	}

	// check read capabilities
	if (!CHK_BIT(capabilities, VFS_CAP_READ))
	{
		set_last_error(EACCES, FILE_READ_ACCESS_DENIED, EO_FILE_INTERFACE);
		return INVALID_IO;
	}

	if (CHK_BIT(capabilities, VFS_CAP_CACHE))
	{
		// initiate caching and do 4KB chunks of data
		// segment the 'count' bytes into 4KB regions => read them to the page cache => copy to user buffer

		if (start % PAGE_CACHE_SIZE != 0)
		{
			set_last_error(EINVAL, FILE_UNALIGED_ADDRESS, EO_FILE_INTERFACE);
			return INVALID_IO;
		}

		uint32 total_pages = ceil_division(count, PAGE_CACHE_SIZE);
		size_t bytes_read = 0;

		for (uint32 i = 0; i < total_pages; i++)
		{
			uint32 page = start / PAGE_CACHE_SIZE + i;

			// do not read past the end of file
			/*if (page * PAGE_CACHE_SIZE >= entry->file_node->file_length)
				break;*/

			virtual_addr cache = page_cache_get_buffer(gfd, page);

			// if the area hasn't been read => read it
			if (cache == 0)
			{
				cache = page_cache_reserve_buffer(gfd, page);

				if (cache == 0)
					return bytes_read;

				if (vfs_read_file(gfd, entry->file_node, start + i * PAGE_CACHE_SIZE, PAGE_CACHE_SIZE, cache) == INVALID_IO)
				{
					page_cache_release_buffer(gfd, page);
					return bytes_read;
				}
			}

			// convention: when address is -1 do not copy from the cache
			if(buffer != -1)
				memcpy((uint8*)buffer + i * PAGE_CACHE_SIZE, (void*)cache, min(count - bytes_read, PAGE_CACHE_SIZE));

			/*if (count - bytes_read < PAGE_CACHE_SIZE)
			{
				serial_printf("zeroing cache for addr: %h", buffer + i * PAGE_CACHE_SIZE);
				memset((char*)cache + count - bytes_read, 0, count - bytes_read);

			}*/

			bytes_read += min(count - bytes_read, PAGE_CACHE_SIZE);
		}

		return bytes_read;
	}
	else
	{
		// the user requires immediate read to his buffer
		// it is up to the driver to check count validity or do segmented data loading (in specific manageable chunks)
		return vfs_read_file(gfd, entry->file_node, start, count, buffer);
	}

	return INVALID_IO;
}

size_t write_file(uint32 fd, uint32 start, size_t count, virtual_addr buffer)
{
	if (count > MAX_IO)
	{
		set_last_error(EINVAL, FILE_BIG_REQUEST, EO_FILE_INTERFACE);
		return INVALID_IO;
	}

	lfe* local_entry = lft_get(&process_get_current()->lft, fd);
	if (local_entry == 0)
		return INVALID_IO;

	uint32 global_fd = local_entry->gfd;

	gfe* entry = gft_get(global_fd);
	if (!entry || gfe_is_invalid(entry))	
	{
		set_last_error(EBADF, FILE_GFD_NOT_FOUND, EO_FILE_INTERFACE);
		return INVALID_IO;
	}

	// check write capabilities
	if (CHK_BIT(local_entry->flags, VFS_CAP_WRITE) == false)
	{
		set_last_error(EACCES, FILE_WRITE_ACCESS_DENIED, EO_FILE_INTERFACE);
		return INVALID_IO;
	}

	if (CHK_BIT(local_entry->flags, VFS_CAP_CACHE))
	{
		if (start % PAGE_CACHE_SIZE != 0)
		{
			set_last_error(EINVAL, FILE_UNALIGED_ADDRESS, EO_FILE_INTERFACE);
			return INVALID_IO;
		}

		if (start / PAGE_CACHE_SIZE >= ceil_division(entry->file_node->file_length, PAGE_CACHE_SIZE) + 2)
		{
			set_last_error(EINVAL, FILE_FAR_START, EO_FILE_INTERFACE);
			return INVALID_IO;
		}

		uint32 total_pages = ceil_division(count, PAGE_CACHE_SIZE);
		size_t bytes_written = 0;

		for (uint32 i = 0; i < total_pages; i++)
		{
			uint32 page = start / PAGE_CACHE_SIZE + i;
			virtual_addr cache = page_cache_get_buffer(global_fd, page);

			// if the area hasn't been read => write with zero pad
			if (cache == 0)
			{
				cache = page_cache_reserve_buffer(global_fd, page);
				if (cache == 0)
					break;

				if(count - bytes_written < PAGE_CACHE_SIZE)
					memset((void*)cache, 0, PAGE_CACHE_SIZE - count % PAGE_CACHE_SIZE);
			}

			if(buffer != -1)
				memcpy((void*)cache, (uint8*)buffer + i * PAGE_CACHE_SIZE, PAGE_CACHE_SIZE);

			bytes_written += min(count - bytes_written, PAGE_CACHE_SIZE);

			//if (count - bytes_written >= PAGE_CACHE_SIZE)
			//{
			//	memcpy((void*)cache, (uint8*)buffer + i * PAGE_CACHE_SIZE, PAGE_CACHE_SIZE);
			//	bytes_written += PAGE_CACHE_SIZE;
			//}
			//else
			//{
			//	memcpy((void*)cache, (uint8*)buffer + i * PAGE_CACHE_SIZE, count - bytes_written);
			//	bytes_written += count - bytes_written;
			//}

			// TODO: make page dirty

		}

		uint32 length = entry->file_node->file_length;

		// adjust file length
		if (start + bytes_written > length)
			while(CAS<uint32>(&entry->file_node->file_length, length, start + bytes_written) == false)
				length = entry->file_node->file_length;

		return bytes_written;
	}
	else
		return vfs_write_file(global_fd, entry->file_node, start, count, buffer);

	return INVALID_IO;
}

error_t sync_file(uint32 fd, uint32 start_page, uint32 end_page)
{
	lfe* local_entry = lft_get(&process_get_current()->lft, fd);
	if (local_entry == 0)
		return ERROR_OCCUR;

	uint32 global_fd = local_entry->gfd;

	gfe* entry = gft_get(global_fd);
	if (!entry || gfe_is_invalid(entry))
	{
		set_last_error(EBADF, FILE_GFD_NOT_FOUND, EO_FILE_INTERFACE);
		return ERROR_OCCUR;
	}

	// check cache capabilities. This is a cache-mode only function
	if (CHK_BIT(local_entry->flags, VFS_CAP_WRITE | VFS_CAP_CACHE) == false)
	{
		set_last_error(EACCES, FILE_CAPABILITIES_ERROR, EO_FILE_INTERFACE);
		return INVALID_IO;
	}

	// convention to sync the whole file
	if (end_page < start_page)
	{
		start_page = 0;
		end_page = ceil_division(entry->file_node->file_length, PAGE_CACHE_SIZE);
	}

	for (uint32 pg = start_page; pg <= end_page; pg++)
	{
		// TODO: Consider speed-up using the page cache list primitive (to avoid searching each time from the beginning of the page list)
		virtual_addr cache = page_cache_get_buffer(global_fd, pg);

		// assume page not loaded
		if (cache == 0)
			continue;

		// TODO: Check if page is not dirty

		// write the page to the hardware
		if (vfs_write_file(global_fd, entry->file_node, start_page * PAGE_CACHE_SIZE, PAGE_CACHE_SIZE, cache) != PAGE_CACHE_SIZE)
			return ERROR_OCCUR;

		// TODO: Remove page dirtyness
	}

	return ERROR_OK;
}