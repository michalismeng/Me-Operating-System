#include "FAT32_fs.h"

vfs_result fat_fs_read(int fd, vfs_node* file, uint32 start, uint32 count, virtual_addr address);
vfs_result fat_fs_open(vfs_node* node);
vfs_result fat_fs_write(int fd, vfs_node* file, uint32 start, uint32 count, virtual_addr address);
vfs_result fat_fs_sync(int fd, vfs_node* file, uint32 start_page, uint32 end_page);

static fs_operations fat_fs_operations =
{
	fat_fs_read,		// read
	fat_fs_write,		// write
	fat_fs_open,		// open
	NULL,				// close
	fat_fs_sync			// sync
						// lookup
						// ioctl?
};

vfs_result fat_fs_read_to_cache(int fd, vfs_node* file, uint32 page, virtual_addr* _cache)
{
	vfs_node* mount_point = file->tag;
	vfs_node* device = file->tag->tag;

	virtual_addr cache = page_cache_get_buffer(fd, page);
	uint32 error;

	if (cache == 0)
	{
		cache = page_cache_reserve_buffer(fd, page);
		if (cache == 0)
			return VFS_CACHE_FULL;

		vmmngr_alloc_page(cache);

		if ((error = fat_fs_data_transfer(mount_point, (mass_storage_info*)device->deep_md, file, page, cache, true)) != VFS_OK)
		{
			page_cache_release_buffer(fd, page);
			return error;
		}
	}

	*_cache = cache;

	return VFS_OK;
}

//TODO: Do more testing with larger files...
vfs_result fat_fs_read(int fd, vfs_node* file, uint32 start, uint32 count, virtual_addr address)
{
	if (!file->tag->tag)
		return VFS_ERROR::VFS_INVALID_NODE_STRUCTURE;

	// TODO: filesystem read permission

	uint32 start_pg = start / PAGE_SIZE;
	uint32 current_pg = start_pg;
	uint32 read = 0;

	virtual_addr cache;
	uint32 error;

	if (error = fat_fs_read_to_cache(fd, file, current_pg, &cache))
		return error;

	// copy perhaps partial data to user buffer
	memcpy((void*)address, (void*)(cache + start % PAGE_SIZE), min(count, PAGE_SIZE - start % PAGE_SIZE));
	read += min(count, PAGE_SIZE - start % PAGE_SIZE);
	current_pg++;

	// retrieve and read foreach intermediate page
	for (uint32 pg = 1; pg < count / PAGE_SIZE; pg++, current_pg++)
	{
		if (error = fat_fs_read_to_cache(fd, file, current_pg, &cache))
			return error;

		read += 4096;
		memcpy((void*)(address + read), (void*)cache, 4096);
	}

	if (read == count)
		return VFS_OK;

	// now read the last page
	if (error = fat_fs_read_to_cache(fd, file, current_pg, &cache))
		return error;

	memcpy((void*)(address + read), (void*)cache, count - read);

	return VFS_OK;
}

//TODO: Do more testing with larger files...
vfs_result fat_fs_write(int fd, vfs_node* file, uint32 start, uint32 count, virtual_addr address)
{
	if (!file->tag->tag)
		return VFS_ERROR::VFS_INVALID_NODE_STRUCTURE;

	// TODO: filesystem write permission

	uint32 start_pg = start / PAGE_SIZE;
	uint32 current_pg = start_pg;
	uint32 read = 0;

	virtual_addr cache;
	uint32 error;

	// ensure page cache is read
	if (error = fat_fs_read_to_cache(fd, file, current_pg, &cache))
		return error;

	// copy perhaps partial data from user buffer to cache buffer
	memcpy((void*)(cache + start % PAGE_SIZE), (void*)address, min(count, PAGE_SIZE - start % PAGE_SIZE));
	read += min(count, PAGE_SIZE - start % PAGE_SIZE);
	current_pg++;

	// retrieve and read foreach intermediate page
	for (uint32 pg = 1; pg < count / PAGE_SIZE; pg++, current_pg++)
	{
		if (error = fat_fs_read_to_cache(fd, file, current_pg, &cache))
			return error;

		read += 4096;
		memcpy((void*)cache, (void*)(address + read), 4096);
	}

	if (read == count)
		return VFS_OK;

	// now read the last page
	if (error = fat_fs_read_to_cache(fd, file, current_pg, &cache))
		return error;

	memcpy((void*)cache, (void*)(address + read), count - read);

	return VFS_OK;
}

vfs_result fat_fs_open(vfs_node* node)
{
	if (!node->tag->tag)
		return VFS_ERROR::VFS_INVALID_NODE_STRUCTURE;

	// TODO: filesystem open permission
	vfs_node* mount_point = node->tag;
	vfs_node* device = node->tag->tag;

	return fat_fs_load_file_layout((fat_mount_data*)mount_point->deep_md, (mass_storage_info*)device->deep_md, node);
}

vfs_result fat_fs_sync(int fd, vfs_node* file, uint32 page_start, uint32 page_end)
{
	vfs_node* mount_point = file->tag;
	vfs_node* device = file->tag->tag;

	// convention, sync the whole file
	if (page_start > page_end)
	{
		fat_file_layout* layout = (fat_file_layout*)file->deep_md;
		page_start = 0;
		page_end = layout->count - 1;
	}

	for (uint32 pg = page_start; pg <= page_end; pg++)
	{
		uint32 error;
		virtual_addr cache = page_cache_get_buffer(fd, pg);

		if (cache == 0)
			continue;

		if ((error = fat_fs_data_transfer(mount_point, (mass_storage_info*)device->deep_md, file, pg, cache, false)) != VFS_OK)
			return error;
	}

	return VFS_OK;
}

// returns the next cluster to read based on the current cluster and the first FAT
uint32 fat_fs_get_next_cluster(mass_storage_info* info, uint32 fat_lba, uint32 current_cluster)
{
	// each 512 byte sector has 128 fat entries. We need to find out which part of FAT to load
	uint32 fat_offset = current_cluster / 128;
	uint32* buffer = (uint32*)mass_storage_read(info, fat_lba + fat_offset, 0, 8, 0);		// read FAT portion(address does nothing)
		// replaces: info->entry_point(0, info, fat_lba + fat_offset, 0, 8);

	return buffer[current_cluster] & 0x0FFFFFFF;		// clear 4 top most bits as they are reserved (FAT28 as we implement)
}

// returns a compressed 8.3 (max 13 characters) with ALL spaces killed
void fat_fs_retrieve_short_name(fat_dir_entry_short* entry, char buffer[13])
{
	buffer[12] = 0;
	uint8 name_index = 0;

	for (uint8 i = 0; i < 8; i++)
		if (entry->name[i] != ' ')
			buffer[name_index++] = entry->name[i];

	buffer[name_index++] = '.';
	uint8 prev_index = name_index;

	for (uint8 i = 0; i < 3; i++)
		if (entry->extension[i] != ' ')
			buffer[name_index++] = entry->extension[i];

	if (name_index == prev_index)		// all three extension chars were spaces so delete . as this is (perhaps) a folder
		buffer[--name_index] = 0;
}

// TODO: FIX THAAAAT
void fat_fs_retrieve_long_name(fat_dir_entry_long* entry, char name[256])
{
	uint8 index = entry->order & ~0x40 - 1;		// all entry orders start at 1
	bool name_end = false;						// detects a null terminator

	for (uint8 i = 0; i < 5 && !name_end; i++)
	{
		if (entry->name1_5[i * 2] == 0)				// name is over so copy the last null termination and mark the end
			name_end = true;

		name[index * 13 + i] = entry->name1_5[i * 2];
	}

	for (uint8 i = 0; i < 6 && !name_end; i++)
	{
		if (entry->name6_11[i * 2] == 0)			// name is over so copy the last null termination and mark the end
			name_end = true;

		name[5 + index * 13 + i] = entry->name6_11[i * 2];
	}

	for (uint8 i = 0; i < 2 && !name_end; i++)
	{
		if (entry->name12_13[i * 2] == 0)			// name is over so copy the last null termination and mark the end
			name_end = true;

		name[index * 13 + i] = entry->name12_13[i * 2];
	}

	// if this is the last order entry and
	// the name length is a multiple of 13 then there is no null paddind so we need to add it ourselves
	if ((entry->order & 0x40) == 0x40 && !name_end)
		name[index * 13 + 13] = 0;
}

inline uint32 fat_fs_get_entry_cluster(fat_dir_entry_short* e)
{
	return e->cluster_low + ((uint32)e->cluster_high & 0x0FFFFFFF);
}

vfs_node* fat_fs_read_long_directory(mass_storage_info* info, uint32 fat_lba, uint32 cluster_lba,
	uint32 current_cluster, fat_dir_entry_long* buffer_base, uint8& index)
{
	// we take the base buffer so that index can be used.
	bool over = false;
	char name[256] = { 0 };

	uint32 offset = current_cluster;

	while (true)
	{
		for (uint8 i = index; i < 16; i++)
		{
			fat_dir_entry_long* entry = buffer_base + i;

			if ((entry->attributes & FAT_LFN) != FAT_LFN)		// this is a short entry so we are finished
			{
				index = i;

				fat_dir_entry_short* s_entry = (fat_dir_entry_short*)entry;

				//return vfs_create_node(name, true,  );
			}

			fat_fs_retrieve_long_name(entry, name);
		}

		offset = fat_fs_get_next_cluster(info, fat_lba, offset);
		if (offset >= FAT_EOF)		// test for end of file marker
			break;

		buffer_base = (fat_dir_entry_long*)mass_storage_read(info, cluster_lba + (offset - 2) * 8, 0, 8, 0);
		// replaces: info->entry_point(0, info, cluster_lba + (offset - 2) * 8, 0, 8);
	}

	return 0;
}

// starting at current_cluster reads directories and files. Perhaps they will span more than one cluster.
list<vfs_node*> fat_fs_read_directory(vfs_node* mount_point, mass_storage_info* info, uint32 fat_lba, uint32 cluster_lba, uint32 current_cluster)
{
	list<vfs_node*> l;
	list_init(&l);

	// used to follow the cluster chain for directories
	uint32 offset = current_cluster;

	bool working_on_lfn = false;

	while (true)
	{
		fat_dir_entry_short* entry = (fat_dir_entry_short*)mass_storage_read(info, cluster_lba + (offset - 2) * 8, 0, 8, 0);
		// replaces: info->entry_point(0, info, cluster_lba + (offset - 2) * 8, 0, 8);

		// read all directory entries of this cluster. There are 16 entries as each is 32 bytes long
		for (uint8 i = 0; i < 16; i++)
		{
			if (entry[i].name[0] == 0)			// end
			{
				working_on_lfn = false;
				break;
			}

			if (entry[i].name[0] == 0xE5)		//unused entry
			{
				working_on_lfn = false;
				continue;
			}

			if ((entry[i].attributes & FAT_VOLUME_ID) == FAT_VOLUME_ID)		// volume id
			{
				working_on_lfn = false;
				continue;
			}

			char name[13] = { 0 };
			fat_fs_retrieve_short_name(entry + i, name);

			uint32 attrs = 0;
			list<vfs_node*> children;
			list_init(&children);

			if ((entry[i].attributes & FAT_DIRECTORY) == FAT_DIRECTORY)
			{
				// prevent infinite loop as '.' represents this directory
				if (name[0] != '.')
				{
					attrs |= VFS_ATTRIBUTES::VFS_DIRECTORY;

					// This is a directory. It contains files and directories so read them all
					uint32 clus = fat_fs_get_entry_cluster(entry + i);
					children = fat_fs_read_directory(mount_point, info, fat_lba, cluster_lba, clus);		// TODO: rethink this with buffers
					mass_storage_read(info, cluster_lba + current_cluster - 2, 0, 8, 0);	// restore entry context.
					//replaces: info->entry_point(0, info, cluster_lba + current_cluster - 2, 0, 8);
				}
				else  // dot and dotdot directories are registered as links
				{
					// TODO: set the tag for these two dot and dot dot links
					attrs |= VFS_ATTRIBUTES::VFS_LINK;
				}
			}
			else
				attrs |= VFS_ATTRIBUTES::VFS_FILE;

			// TODO: Add vfs hidden
			if ((entry[i].attributes & FAT_HIDDEN) == FAT_HIDDEN)
				attrs |= VFS_ATTRIBUTES::VFS_HIDDEN;

			if ((entry[i].attributes & FAT_READ_ONLY) == FAT_READ_ONLY)
				attrs |= VFS_ATTRIBUTES::VFS_READ;
			else
			{
				attrs |= VFS_ATTRIBUTES::VFS_READ;
				attrs |= VFS_ATTRIBUTES::VFS_WRITE;
			}

			auto node = vfs_create_node(name, true, attrs, entry[i].file_size, sizeof(fat_file_layout), mount_point, NULL);

			// setup layout list and add the starting cluster
			fat_file_layout* layout = (fat_file_layout*)node->deep_md;

			// LIST IMPLEMENTATION
			//list_init(layout);
			//list_insert_back(layout, fat_fs_get_entry_cluster(entry + i));

			vector_init(layout, ceil_division(entry[i].file_size, 4096));
			vector_insert_back(layout, fat_fs_get_entry_cluster(entry + i));

			node->children = children;
			list_insert_back(&l, node);
		}

		offset = fat_fs_get_next_cluster(info, fat_lba, offset);
		if (offset >= FAT_EOF)		// test for end of file marker
			break;
	}

	return l;
}

vfs_node* fat_fs_mount(char* mount_name, vfs_node* dev_node)
{
	// read the whole root directory (all clusters) and load all folders and files

	// hard code partition code...
	// TODO: move this part at partition/filesystem identification

	mass_storage_info* info = (mass_storage_info*)dev_node->deep_md;

	// get the primary partition offset
	fat_mbr* buffer = (fat_mbr*)mass_storage_read(info, 0, 0, 1, 0);	//replaces: info->entry_point(0, info, 0, 0, 1);
	uint32 partiton_offset = buffer->primary_partition.lba_offset;

	// get the volume id of the primary partition
	fat_volume_id* volume = (fat_volume_id*)mass_storage_read(info, partiton_offset, 0, 1, 0);
	//replaces: info->entry_point(0, info, partiton_offset, 0, 1);

	// gather important data
	uint32 fat_lba = partiton_offset + volume->reserved_sector_count;
	uint32 cluster_lba = fat_lba + volume->number_FATs * volume->extended.sectors_per_FAT;
	uint32 root_dir_first_cluster = volume->extended.root_cluster_lba;

	// create the vfs mount point node and get the mount data pointer
	vfs_node* mount_point = vfs_create_node(mount_name, true, VFS_MOUNT_PT, 0, sizeof(fat_mount_data), dev_node, &fat_fs_operations);
	fat_mount_data* mount_data = (fat_mount_data*)mount_point->deep_md;

	// read root directory along with each sub directory
	list<vfs_node*> hierarchy = fat_fs_read_directory(mount_point, info, fat_lba, cluster_lba, root_dir_first_cluster);

	// set the mount point children to the list hierarchy just created
	mount_point->children = hierarchy;

	// load the data at the mount point
	mount_data->cluster_lba = cluster_lba;
	mount_data->fat_lba = fat_lba;
	mount_data->partition_offset = partiton_offset;
	mount_data->root_dir_first_cluster = root_dir_first_cluster;

	return mount_point;
}

vfs_result fat_fs_load_file_layout(fat_mount_data* mount_info, mass_storage_info* storgae_info, vfs_node* node)
{
	fat_file_layout* layout = (fat_file_layout*)node->deep_md;

	while (true)
	{
		//uint32 next_cluster = fat_fs_get_next_cluster(storgae_info, mount_info->fat_lba, layout->tail->data);  LIST IMPLEMENTATION
		uint32 next_cluster = fat_fs_get_next_cluster(storgae_info, mount_info->fat_lba, vector_at(layout, layout->count - 1));
		if (next_cluster >= FAT_EOF)
			break;

		//list_insert_back(layout, next_cluster);
		vector_insert_back(layout, next_cluster);
	}

	return VFS_ERROR::VFS_OK;
}

vfs_result fat_fs_data_transfer(vfs_node* mount_point, mass_storage_info* storage_info, vfs_node* node, uint32 file_page, virtual_addr address, bool read)
{
	fat_file_layout* layout = (fat_file_layout*)node->deep_md;
	fat_mount_data* mount_info = (fat_mount_data*)mount_point->deep_md;

	if (layout->count == 0 || file_page >= layout->count)
		return VFS_ERROR::VFS_PAGE_NOT_FOUND;

	//int result = storage_info->read(storage_info, mount_info->cluster_lba + (l->data - 2) * 8, 0, 8, vmmngr_get_phys_addr(address));  LIST IMPLEMENTATION
	int result;

	if (read)
		result = storage_info->read(storage_info, mount_info->cluster_lba + (vector_at(layout, file_page) - 2) * 8, 0, 8, vmmngr_get_phys_addr(address));
	else
		result = storage_info->write(storage_info, mount_info->cluster_lba + (vector_at(layout, file_page) - 2) * 8, 0, 8, vmmngr_get_phys_addr(address));

	if (result == 0)
		return VFS_ERROR::VFS_OK;

	return VFS_ERROR::VFS_READ_ERROR;
}