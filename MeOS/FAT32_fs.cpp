#include "FAT32_fs.h"
#include "print_utility.h"

#define STORAGE_INFO(mp) ((mass_storage_info*)mp->tag->deep_md)
#define MOUNT_DATA(mp) ((fat_mount_data*)mp->deep_md)
#define NODE_DATA(n) ((fat_node_data*)n->deep_md)
#define LAYOUT(n) ((fat_file_layout*)n->deep_md)

error_t fat_fs_open(vfs_node* node, uint32 capabilities);
size_t fat_fs_read(uint32 fd, vfs_node* file, uint32 start, size_t count, virtual_addr address);
size_t fat_fs_write(uint32 fd, vfs_node* file, uint32 start, size_t count, virtual_addr address);
error_t fat_fs_sync(uint32 fd, vfs_node* file, uint32 start_page, uint32 end_page);
error_t fat_fs_ioctl(vfs_node* node, uint32 command, ...);

bool fat_fs_write_by_page(vfs_node* mount_point, vfs_node* node, uint32 file_page, virtual_addr address);
size_t fat_node_write(uint32 fd, vfs_node* file, uint32 start, size_t count, virtual_addr address);
bool fat_fs_read_by_page(vfs_node* mount_point, vfs_node* node, uint32 file_page, virtual_addr address);

// file operations
static fs_operations fat_fs_operations =
{
	fat_fs_read,		// read
	fat_fs_write,		// write
	fat_fs_open,		// open
	NULL,				// close
	fat_fs_sync,		// sync
	NULL,				// lookup
	fat_fs_ioctl		// ioctl?
};

// mount point operations
static fs_operations fat_mount_operations =
{
	NULL,				// read
	fat_node_write,		// write
	NULL,				// open
	NULL,				// close
	fat_fs_sync,		// sync
	NULL,				// lookup
	NULL				// ioctl?
};

#pragma region Private Functions

VFS_ATTRIBUTES fat_to_vfs_attributes(uint32 fat_attrs)
{
	uint32 attrs = VFS_ATTRIBUTES::VFS_READ;

	if ((fat_attrs & FAT_READ_ONLY) != FAT_READ_ONLY)
		attrs |= VFS_ATTRIBUTES::VFS_WRITE;

	if ((fat_attrs & FAT_HIDDEN) == FAT_HIDDEN)
		attrs |= VFS_ATTRIBUTES::VFS_HIDDEN;

	if ((fat_attrs & FAT_DIRECTORY) == FAT_DIRECTORY)
		attrs |= VFS_ATTRIBUTES::VFS_DIRECTORY;
	else
		attrs |= VFS_ATTRIBUTES::VFS_FILE;

	return (VFS_ATTRIBUTES)attrs;
}

FAT_DIR_ATTRIBUTES vfs_to_fat_attributes(uint32 vfs_attrs)
{
	uint32 attrs = 0;

	switch (vfs_attrs & 7)
	{
		//case VFS_FILE:		attrs |= FAT_ARCHIVE;	break; FAT_ARCHIVE is used for backup utilities to track changes in files. Not to mark files!
	case VFS_DIRECTORY: attrs |= FAT_DIRECTORY; break;
	case VFS_LINK:		attrs |= FAT_DIRECTORY; break;
	default:			attrs = 0;				break;
	}

	if ((vfs_attrs & VFS_WRITE) != VFS_WRITE)
		attrs |= FAT_READ_ONLY;

	if ((vfs_attrs & VFS_HIDDEN) == VFS_HIDDEN)
		attrs |= FAT_HIDDEN;

	return (FAT_DIR_ATTRIBUTES)attrs;
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

// generates a valid FAT32 name from a vfs node name
void fat_fs_generate_short_name(vfs_node* node, char name[12])
{
	if (node->name_length > 12)
		return;

	for (uint8 j = 0; j < 11; j++)
		name[j] = ' ';

	uint8 i = 0;
	uint8 dot_index = 0;
	for (; i < 8 && i < node->name_length; i++)
	{
		if (node->name[i] == '.')
		{
			dot_index = i;
			while (i < 8)
				name[i++] = ' ';

			break;
		}

		name[i] = node->name[i];
	}

	if (dot_index == 0)
		return;

	for (uint8 j = 0; j < 3 && dot_index + j + 1 < node->name_length; j++, i++)
		name[i] = node->name[dot_index + j + 1];
}

bool fat_fs_validate_83_name(char* name, uint32 length)
{
	if (length > 11)
		return false;

	/*0x2E, (this is the '.')*/
	uint8 bad_values[] = { 0x22, 0x2A, 0x2B, 0x2C,  0x2F, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x5B, 0x5C, 0x5D, 0x7C };

	for (uint32 i = 0; i < length; i++)
	{
		if (i != 0 && name[i] < 0x20)
			return false;

		if (i == 0 && name[i] < 0x20 && name[i] != 0x05)
			return false;

		for (uint8 j = 0; j < 16; j++)
			if (name[i] == bad_values[j])
				return false;
	}

	return true;
}

#pragma endregion


#pragma region VFS API Implementation

size_t fat_fs_read(uint32 fd, vfs_node* file, uint32 start, size_t count, virtual_addr address)
{
	//if (!file->tag->tag)
	//{
	//	set_last_error(EINVAL, FAT_BAD_NODE_STRUCTURE, EO_MASS_STORAGE_FS);
	//	return 0;		// read 0 bytes
	//}

	if (start % FAT_FORMAT_PAGE_SIZE != 0 || count % FAT_FORMAT_PAGE_SIZE != 0)
	{
		set_last_error(EINVAL, FAT_BAD_ALIGN, EO_MASS_STORAGE_FS);
		return INVALID_IO;
	}

	uint32 start_pg = start / FAT_FORMAT_PAGE_SIZE;
	vfs_node* mount_point = file->tag;
	virtual_addr cache;
	uint32 offset = start % FAT_FORMAT_PAGE_SIZE;


	for (uint32 pg = 0; pg < ceil_division(count, FAT_FORMAT_PAGE_SIZE); pg++)
	{
		if (fat_fs_read_by_page(mount_point, file, start_pg + pg, address + FAT_FORMAT_PAGE_SIZE * pg) == false)
			return INVALID_IO;
	}

	return count;
}

size_t fat_fs_write(uint32 fd, vfs_node* file, uint32 start, size_t count, virtual_addr address)
{
	//if (!file->tag->tag)
	//{
	//	set_last_error(EINVAL, FAT_BAD_NODE_STRUCTURE, EO_MASS_STORAGE_FS);
	//	return 0;	// written zero bytes
	//}

	if (start % FAT_FORMAT_PAGE_SIZE != 0 || count % FAT_FORMAT_PAGE_SIZE != 0)
	{
		set_last_error(EINVAL, FAT_BAD_ALIGN, EO_MASS_STORAGE_FS);
		return INVALID_IO;
	}

	uint32 start_pg = start / FAT_FORMAT_PAGE_SIZE;
	vfs_node* mount_point = file->tag;
	virtual_addr cache;
	uint32 offset = start % FAT_FORMAT_PAGE_SIZE;

	for (uint32 pg = 0; pg < ceil_division(count, FAT_FORMAT_PAGE_SIZE); pg++)
	{
		if (fat_fs_write_by_page(mount_point, file, start_pg + pg, address + FAT_FORMAT_PAGE_SIZE * pg) == false)
			return INVALID_IO;
	}

	return count;
}

error_t fat_fs_open(vfs_node* node, uint32 capabilities)
{
	if (!node->tag->tag)
		return set_last_error(EINVAL, FAT_BAD_NODE_STRUCTURE, EO_MASS_STORAGE_FS);

	// TODO: filesystem open permission
	vfs_node* mount_point = node->tag;

	// load the file layout only if the layout has not been loaded. else it is cached so return success.
	if (NODE_DATA(node)->layout_loaded == false)
	{
		NODE_DATA(node)->layout_loaded = true;
		return fat_fs_load_file_layout((fat_mount_data*)mount_point->deep_md, node);
	}

	return ERROR_OK;
}

// TODO: When a page is outside the layout of a file (the file has been extended due to new data), reserve a new sector for it.
error_t fat_fs_sync(uint32 fd, vfs_node* file, uint32 page_start, uint32 page_end)
{
	vfs_node* mount_point;
	vfs_node* device;

	if ((file->attributes & 0x7) == VFS_FILE || (file->attributes & 0x7) == VFS_DIRECTORY)
	{
		mount_point = file->tag;
		device = file->tag->tag;
	}
	else if ((file->attributes & 0x7) == VFS_MOUNT_PT)
	{
		mount_point = file;
		device = file->tag;
	}
	else
		return set_last_error(EINVAL, FAT_BAD_NODE_STRUCTURE, EO_MASS_STORAGE_FS);

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

		serial_printf("cache: %h\n", cache);

		if (fat_fs_write_by_page(mount_point, file, pg, cache) == false)
		{
			serial_printf("AN ERROR OCCURED\N");
			return ERROR_OCCUR;
		}
	}

	return ERROR_OK;
}

error_t fat_fs_ioctl(vfs_node* node, uint32 command, ...)
{
	if (command == 0)	// invalidate
	{
		// mount point data is the file descriptor for the root directory "file"
		vfs_node* mount = node->tag;
		mount->fs_ops->fs_write(MOUNT_DATA(mount)->fd, node, 0, 0, 0);
	}
	
	return ERROR_OK;
}

#pragma endregion

#pragma region Sector Access Functions

// reads a 4KB data region starting at the given linear block address. This is the lowest level data exchange function.
bool fat_fs_read_by_lba(vfs_node* mount_point, uint32 lba, virtual_addr address)
{
	if (mount_point == 0 || (mount_point->attributes & 7) != VFS_ATTRIBUTES::VFS_MOUNT_PT || mount_point->tag == 0 ||
		MOUNT_DATA(mount_point) == 0 || STORAGE_INFO(mount_point) == 0)
	{
		set_last_error(EINVAL, FAT_BAD_ARGUMENTS, EO_MASS_STORAGE_FS);
		return false;
	}

	// because this is a mass storage data exchange, the fd represents the high lba address to read (for now it is zero).
	// the count is 8 sectors !!
	if (vfs_read_file(0, mount_point->tag, lba, 8, address) != 8)
		return false;

	return true;
}

// writes a 4KB data region starting at the given linear block address. This is the lowest level data exchange function.
bool fat_fs_write_by_lba(vfs_node* mount_point, uint32 lba, virtual_addr address)
{
	if (mount_point == 0 || (mount_point->attributes & 7) != VFS_ATTRIBUTES::VFS_MOUNT_PT ||
		MOUNT_DATA(mount_point) == 0 || STORAGE_INFO(mount_point) == 0)
	{
		set_last_error(EINVAL, FAT_BAD_ARGUMENTS, EO_MASS_STORAGE_FS);
		return false;
	}
	
	if(vfs_write_file(0, mount_point->tag, lba, 8, address) != 8)
		return false;

	return true;
}

// reads a 4KB data region that corresponds to the given data cluster, starting at the volume's cluster lba. 
bool fat_fs_read_by_data_cluster(vfs_node* mount_point, uint32 cluster, virtual_addr address)
{
	if (mount_point == 0 || (mount_point->attributes & 7) != VFS_ATTRIBUTES::VFS_MOUNT_PT || MOUNT_DATA(mount_point) == 0)
	{
		set_last_error(EINVAL, FAT_BAD_ARGUMENTS, EO_MASS_STORAGE_FS);
		return false;
	}

	return fat_fs_read_by_lba(mount_point, MOUNT_DATA(mount_point)->cluster_lba + (cluster - 2) * 8, address);
}

// writes a 4KB data region that corresponds to the given data cluster, starting at the volume's cluster lba. 
bool fat_fs_write_by_data_cluster(vfs_node* mount_point, uint32 cluster, virtual_addr address)
{
	if (mount_point == 0 || (mount_point->attributes & 7) != VFS_ATTRIBUTES::VFS_MOUNT_PT || MOUNT_DATA(mount_point) == 0)
	{
		set_last_error(EINVAL, FAT_BAD_ARGUMENTS, EO_MASS_STORAGE_FS);
		return false;
	}

	return fat_fs_write_by_lba(mount_point, MOUNT_DATA(mount_point)->cluster_lba + (cluster - 2) * 8, address);
}

// reads a 4KB data region that corresponds to the given node file_page data cluster.
bool fat_fs_read_by_page(vfs_node* mount_point, vfs_node* node, uint32 file_page, virtual_addr address)
{
	fat_file_layout* layout = LAYOUT(node);

	if (layout == 0 || file_page >= layout->count)
	{
		set_last_error(EINVAL, FAT_BAD_LAYOUT, EO_MASS_STORAGE_FS);
		return false;
	}

	return fat_fs_read_by_data_cluster(mount_point, vector_at(layout, file_page), address);
}

// write a 4KB data region that corresponds to the given node file_page data cluster.
bool fat_fs_write_by_page(vfs_node* mount_point, vfs_node* node, uint32 file_page, virtual_addr address)
{
	fat_file_layout* layout = LAYOUT(node);

	if (layout == 0 || file_page >= layout->count)
	{
		set_last_error(EINVAL, FAT_BAD_LAYOUT, EO_MASS_STORAGE_FS);
		return false;
	}

	return fat_fs_write_by_data_cluster(mount_point, vector_at(layout, file_page), address);
}

#pragma endregion


/* Read short entry and FAT values */

// given a buffer of FAT entries returns the FAT value at the given index
// actually returning the next data cluster in the chain to read
uint32 fat_fs_read_fat_value(virtual_addr buffer, uint32 index)
{
	return ((uint32*)buffer)[index] & 0x0FFFFFFF;
}

// returns the entry's first data cluster, by combining the low and high values.
uint32 fat_fs_get_entry_data_cluster(fat_dir_entry_short* e)
{
	return e->cluster_low + ((uint32)e->cluster_high & 0x0FFFFFFF);
}

/*************************************************************************************/


size_t fat_node_write(uint32 fd, vfs_node* node, uint32 start, size_t count, virtual_addr address)
{
	uint32 page = NODE_DATA(node)->metadata_cluster;
	uint32 offset = NODE_DATA(node)->metadata_index;

	fat_dir_entry_short* entry = (fat_dir_entry_short*)(page_cache_get_buffer(fd, page) + offset);
	entry->file_size = node->file_length;

	char buffer[12];
	fat_fs_generate_short_name(node, buffer);
	memcpy(entry->name, buffer, 11);

	return sizeof(fat_dir_entry_short);
}

// returns the next cluster to read based on the current cluster and the first FAT or zero on failure
uint32 fat_fs_find_next_cluster(vfs_node* mount_point, uint32 current_cluster)
{
	// each 4KB sector has 128 fat entries. We need to find out which part of FAT to load
	uint32 fat_offset = current_cluster / 128;
	uint32 address;

	if (!(address = page_cache_reserve_anonymous()))
		return ERROR_OCCUR;

	// Load desired FAT cluster
	if(fat_fs_read_by_lba(mount_point, MOUNT_DATA(mount_point)->fat_lba + fat_offset, address) == false)
	{
		DEBUG("error reading smoething went wrong");
		page_cache_release_anonymous(address);
		return 0;
	}

	// Get the next data cluster to read based on the current data cluster. (Follow the chain)
	uint32 res = fat_fs_read_fat_value(address, current_cluster);
	page_cache_release_anonymous(address);

	return res;
}

// returns the first free cluster and marks it with the next_cluster value or zero on failure
uint32 fat_fs_reserve_first_cluster(vfs_node* mount_point, uint32 next_cluster)
{
	uint32 address;	
	if (!(address = page_cache_reserve_anonymous()))
		return 0;

	uint32 fat_lba_index = 0;
	// TODO: Find a true limit. Perhaps index < first cluster LBA?
	while (true)
	{
		// Read the FAT cluster indicated by fat_lba_index
		if(fat_fs_read_by_lba(mount_point, MOUNT_DATA(mount_point)->fat_lba + fat_lba_index * 8, address) == false)
		{
			page_cache_release_anonymous(address);
			return 0;
		}

		// lopp through all 128 entries per 4KB clusters
		for (uint32 i = 0; i < 128; i++)
		{
			if (fat_fs_read_fat_value(address, i) == 0)		// this is a free cluster
			{
				((uint32*)address)[i] = next_cluster & 0x0FFFFFFF;	// reserve the cluster with the value given

				// write back the reuslts
				if (fat_fs_write_by_lba(mount_point, MOUNT_DATA(mount_point)->fat_lba + fat_lba_index * 8, address) == false)
				{
					page_cache_release_anonymous(address);
					return 0;
				}

				page_cache_release_anonymous(address);
				return fat_lba_index * 128 + i;
			}
		}

		fat_lba_index++;
	}

	// we should never reach here
	DEBUG("ERROR");
	set_last_error(ENOMEM, FAT_NO_CLUSTERS, EO_MASS_STORAGE_FS);
	return 0;
}

// marks the given cluster with the given value and returns its previous value or zero on failure
uint32 fat_fs_mark_cluster(vfs_node* mount_point, uint32 fat_index, uint32 value)
{
	// each 4KB sector has 128 fat entries. We need to find out which part of FAT to load
	uint32 fat_offset = fat_index / 128;
	uint32 address;
	if (!(address = page_cache_reserve_anonymous()))
		return ERROR_OCCUR;

	// Load desired FAT cluster
	if (fat_fs_read_by_lba(mount_point, MOUNT_DATA(mount_point)->fat_lba + fat_offset, address) == false)
	{
		page_cache_release_anonymous(address);
		return 0;
	}

	uint32 last_value = fat_fs_read_fat_value(address, fat_index % 128);
	((uint32*)address)[fat_index % 128] = value & 0x0FFFFFFF;

	if (fat_fs_write_by_lba(mount_point, MOUNT_DATA(mount_point)->fat_lba + fat_offset, address) == false)
	{
		page_cache_release_anonymous(address);
		return 0;
	}

	page_cache_release_anonymous(address);
	return last_value;
}

// starting at current_cluster recursively reads directories and files. Perhaps they will span more than one cluster.
list<vfs_node*> fat_fs_read_directory(vfs_node* mount_point, uint32 current_cluster, vfs_node* parent)
{
	list<vfs_node*> l;
	list_init(&l);

	// used to follow the cluster chain for directories
	uint32 offset = current_cluster;

	while (offset < FAT_EOF)
	{
		virtual_addr cache;
		if (!(cache = page_cache_reserve_anonymous()))
		{
			serial_printf("cache problem: %h %h", offset, cache);
			debugf("");
			return list<vfs_node*>();
		}

		fat_dir_entry_short* entry = (fat_dir_entry_short*)cache;

		if (fat_fs_read_by_data_cluster(mount_point, offset, cache) == false)
		{
			serial_printf("read problem %h reading: %h at %h", get_last_error(), offset, cache);
			debugf("");
			return list<vfs_node*>();
		}

		//vector_insert_back(&MOUNT_DATA(mount_point)->layout, (uint32)offset);	//TODO: What is that??

		// read all directory entries of this cluster. There are 128 entries as each is 32 bytes long
		for (uint8 i = 0; i < 128; i++)
		{
			if (entry[i].name[0] == 0)			// end
				break;

			if (entry[i].name[0] == 0xE5)		//unused entry
				continue;

			if ((entry[i].attributes & FAT_VOLUME_ID) == FAT_VOLUME_ID)		// volume id
				continue;

			char name[13] = { 0 };
			fat_fs_retrieve_short_name(entry + i, name);

			list<vfs_node*> children;
			list_init(&children);

			uint32 attrs = fat_to_vfs_attributes(entry[i].attributes);

			auto node = vfs_create_node(name, true, attrs, VFS_CAP_READ | VFS_CAP_WRITE | VFS_CAP_CACHE, entry[i].file_size, sizeof(fat_node_data), mount_point, parent, &fat_fs_operations);
			NODE_DATA(node)->metadata_cluster = offset;
			NODE_DATA(node)->metadata_index = i;
			NODE_DATA(node)->layout_loaded = false;		// even though the first entry is inserted in the layout, the whole layout is not loaded and a file open is expected.

			if (name[0] != '.' && (entry[i].attributes & FAT_DIRECTORY) == FAT_DIRECTORY)
			{
				// This is a directory (and not a recursive directory . or ..). 
				// It contains files and directories so read them all
				uint32 clus = fat_fs_get_entry_data_cluster(entry + i);
				children = fat_fs_read_directory(mount_point, clus, node);
			}

			// setup layout list and add the starting cluster
			fat_file_layout* layout = &NODE_DATA(node)->layout;

			if (vector_init(layout, ceil_division(entry[i].file_size, 4096)) != ERROR_OK)
				return list<vfs_node*>();

			if(vector_insert_back(layout, fat_fs_get_entry_data_cluster(entry + i)) != ERROR_OK)
				return list<vfs_node*>();

			node->children = children;
			list_insert_back(&l, node);
		}

		uint32 temp_offset = offset;
		offset = fat_fs_find_next_cluster(mount_point, offset);						// first find the next offset (requires the page still be locked)
		page_cache_release_anonymous(cache);
	}

	return l;
}

vfs_node* fat_fs_mount(char* mount_name, vfs_node* dev_node)
{
	// read the whole root directory (all clusters) and load all folders and files

	// hard code partition code...
	// TODO: move this part at partition/filesystem identification

	virtual_addr cache = page_cache_reserve_anonymous();
	if (!cache)
		return 0;

	// get the primary partition offset (read the first sector into the 8-sector-cache reserved)
	if (vfs_read_file(0, dev_node, 0, 1, cache) != 1)
		return 0;

	uint32 partiton_offset = ((fat_mbr*)cache)->primary_partition.lba_offset;

	// get the volume id of the primary partition
	if (vfs_read_file(0, dev_node, partiton_offset, 1, cache) != 1)
		return 0;

	fat_volume_id* volume = (fat_volume_id*)cache;

	// gather important data
	uint32 fat_lba = partiton_offset + volume->reserved_sector_count;
	uint32 cluster_lba = fat_lba + volume->number_FATs * volume->extended.sectors_per_FAT;
	uint32 root_dir_first_cluster = volume->extended.root_cluster_lba;

	page_cache_release_anonymous(cache);

	// create the vfs mount point node and get the mount data pointer
	vfs_node* mount_point = vfs_create_node(mount_name, true, VFS_MOUNT_PT, VFS_CAP_READ | VFS_CAP_WRITE, 0, sizeof(fat_mount_data), dev_node, NULL, &fat_mount_operations);
	fat_mount_data* mount_data = (fat_mount_data*)mount_point->deep_md;
	if (vector_init(&mount_data->layout, 1) != ERROR_OK)
		return 0;

	// create the mount point file (root directory)

	//MOUNT_DATA(mount_point)->fd = gft_insert_s(create_gfe(mount_point));
	/*if (page_cache_register_file(MOUNT_DATA(mount_point)->fd) != ERROR_OK)
		return 0;*/

	// load the data at the mount point
	mount_data->cluster_lba = cluster_lba;
	mount_data->fat_lba = fat_lba;
	mount_data->partition_offset = partiton_offset;
	mount_data->root_dir_first_cluster = root_dir_first_cluster;

	// read root directory along with each sub directory
	mount_point->children = fat_fs_read_directory(mount_point, root_dir_first_cluster, mount_point);

	return mount_point;
}

error_t fat_fs_load_file_layout(fat_mount_data* mount_info, vfs_node* node)
{
	fat_file_layout* layout = (fat_file_layout*)node->deep_md;

	while (true)
	{
		uint32 next_cluster = fat_fs_find_next_cluster(node->tag, vector_at(layout, layout->count - 1));	
		// TODO: Consider speed up by one time cache reservation.
				
		if (next_cluster >= FAT_EOF)
			break;

		if (vector_insert_back(layout, next_cluster) != ERROR_OK)
			return ERROR_OCCUR;
	}

	return ERROR_OK;
}

error_t fat_fs_create_short_entry_from_node(fat_dir_entry_short* entry, vfs_node* node)
{
	if (LAYOUT(node)->count == 0)
	{
		DEBUG("cannot create fat entry from bad node");
		set_last_error(EINVAL, FAT_BAD_NODE_STRUCTURE, EO_MASS_STORAGE_FS);
		return ERROR_OCCUR;
	}

	uint32 first_cluster = vector_at(LAYOUT(node), 0);

	entry->attributes = vfs_to_fat_attributes(node->attributes);
	fat_fs_generate_short_name(node, (char*)entry->name);
	entry->file_size = 0;

	// TODO: Fix dates
	entry->created_date = 8225;
	entry->created_time = 2082;
	entry->created_time_10 = 0;

	entry->last_accessed_date = 8225;
	entry->last_modified_date = 8225;
	entry->last_modified_time = 2082;
	///////////////////////////////////

	entry->cluster_low = (uint16)first_cluster;
	entry->cluster_high = (uint16)(first_cluster >> 16) & 0x0FFF;

	entry->resv0 = 0;
	return ERROR_OK;
}

// initializes a directory with the '.' and '..' entries as the first entries in the buffer.
void fat_fs_initialize_directory(uint32 parent_cluster, fat_dir_entry_short* new_dir, char* buffer)
{
	fat_dir_entry_short* dot = (fat_dir_entry_short*)buffer;

	memcpy(dot, new_dir, sizeof(fat_dir_entry_short));		// greedy copy of new_dir data over dot
	memcpy((char*)dot->name, ".          ", 11);			// fix dot name

	printfln("new_dir attributes: %h", new_dir->attributes);
	printfln("dot attributes: %h", dot->attributes);

	fat_dir_entry_short* dotdot = dot + 1;

	memcpy(dotdot, new_dir, sizeof(fat_dir_entry_short));
	memcpy((char*)dotdot->name, "..         ", 11);			

	dotdot->cluster_low = (uint16)parent_cluster;
	dotdot->cluster_high = (uint16)((parent_cluster >> 16) & 0x00FF);
}

// ioctl functions

// TODO: Do some testing...
// Delete a file or directory that is empty of sub-content
error_t fat_fs_delete_node(vfs_node* mount_point, vfs_node* node)
{
	if ((node->attributes & 7) == VFS_DIRECTORY && node->children.count > 2)		// directory has children, return failure.
	{
		set_last_error(EINVAL, VFS_BAD_ARGUMENTS, EO_MASS_STORAGE_FS);
		return ERROR_OCCUR;
	}

	uint32 cache;
	if (!(cache = page_cache_reserve_anonymous()))
		return ERROR_OCCUR;

	uint32 cluster = NODE_DATA(node)->metadata_cluster;		// file metadata cluster
	uint32 index = NODE_DATA(node)->metadata_index;

	// read the metadata cluster
	if (fat_fs_read_by_data_cluster(mount_point, cluster, cache) == false)
	{
		DEBUG("Could not read cluster");
		page_cache_release_anonymous(cache);
		return ERROR_OCCUR;
	}

	fat_dir_entry_short* entry = ((fat_dir_entry_short*)cache) + index;

	uint32 value = 0xE5;
	if (index < 127 && (entry + 1)->name[0] == 0)		// if the next entry is mark as last entry then mark this one as the last.
		value = 0;

	entry->name[0] = value;		// mark the file as deleted

	if (fat_fs_write_by_data_cluster(mount_point, cluster, cache) == false)
	{
		DEBUG("Could not write cluster");
		page_cache_release_anonymous(cache);
		return ERROR_OCCUR;
	}

	// release the cache buffer as it is not used any longer.
	page_cache_release_anonymous(cache);

	// delete FAT chain

	uint32 next_cluster = vector_at(LAYOUT(node), 0);
	printfln("zero out cluster: %u", next_cluster);

	while (next_cluster < FAT_EOF)
	{
		printfln("zero out cluster: %u", next_cluster);
		next_cluster = fat_fs_mark_cluster(mount_point, next_cluster, 0);

		if (next_cluster == 0)
		{
			DEBUG("Delete file error in deleting chains");
			return ERROR_OCCUR;
		}
	}

	return ERROR_OK;
}

// searches for an empty entry in the given directory's clusters. 
// If found returns the cluster id and entry index and the cluster remains loaded in the given cache
bool fat_fs_find_empty_entry(vfs_node* mount_point, vfs_node* directory, virtual_addr cache, uint32* cluster, uint32* index)
{
	*cluster = 0;
	*index = 129;		// index ranges [0, 127], so set this error value

	auto layout = LAYOUT(directory);	// this must have been loaded...

										// foreach metadata cluster in the chain loaded
	for (uint32 i = 0; i < layout->count; i++)
	{
		// read the metadata cluster
		if (fat_fs_read_by_data_cluster(mount_point, vector_at(layout, i), cache) == false)
			return false;

		fat_dir_entry_short* file_entry = (fat_dir_entry_short*)cache;

		// loop through the metadata cluster's 128 entries
		for (uint32 j = 0; j < 127; j++)
		{
			if (file_entry[j].name[0] == 0xE5 || file_entry[j].name[0] == 0)		// this is a free entry
			{
				*cluster = vector_at(layout, i);
				*index = j;
				return true;
			}
		}
	}

	// no empty entry was found
	return false;
}

// move a node under the given directory which must be within the same filesystem
// TODO: eliminate bad cache claiming
error_t fat_fs_move_node(vfs_node* mount_point, vfs_node* node, vfs_node* directory)
{
	if (mount_point == 0 || node == 0 || directory == 0 ||
		(mount_point->attributes & 7) != VFS_MOUNT_PT || (directory->attributes & 7) != VFS_DIRECTORY ||
		MOUNT_DATA(mount_point) == 0)
	{
		set_last_error(EINVAL, FAT_BAD_NODE_STRUCTURE, EO_MASS_STORAGE_FS);
		return ERROR_OCCUR;
	}

	uint32 fd;
	// TODO: This may cause problems if the directory is being used elsewhere.
	if (open_file_by_node(directory, &fd, 0) != ERROR_OK)		// TODO: Close file
	{
		DEBUG("create file could not open directory file");
		return ERROR_OCCUR;
	}

	uint32 cache;
	/*if (!(cache = page_cache_reserve_buffer(fd, 0)))
		return ERROR_OCCUR;*/

	// changed the above t use anonymous cache
	if (!(cache = page_cache_reserve_anonymous()))
		return ERROR_OCCUR;

	/* read the node's metadata cluster */
	if (fat_fs_read_by_data_cluster(mount_point, NODE_DATA(node)->metadata_cluster, cache) == false)
	{
		page_cache_release_anonymous(cache);
		return ERROR_OCCUR;
	}

	/* copy entry to temporary storage */
	fat_dir_entry_short* entry_ptr = (fat_dir_entry_short*)cache + NODE_DATA(node)->metadata_index;
	fat_dir_entry_short entry = *entry_ptr;

	/* mark the node as deleted */

	// if the next entry is marked as last entry then mark this one as the last.
	if (NODE_DATA(node)->metadata_index < 127 && (entry_ptr + 1)->name[0] == 0)		
		entry_ptr->name[0] = 0x00;
	else
		entry_ptr->name[0] = 0xE5;

	/* write metadata cluster back to disk */
	if (fat_fs_write_by_data_cluster(mount_point, NODE_DATA(node)->metadata_cluster, cache) == false)
	{
		page_cache_release_anonymous(cache);
		return ERROR_OCCUR;
	}

	/* find empty metadata entry undes directory */
	uint32 metadata_cluster, metadata_index;

	if (fat_fs_find_empty_entry(mount_point, directory, cache, &metadata_cluster, &metadata_index))
	{
		/* copy data over to the empty entry and write results back to disk */
		*((fat_dir_entry_short*)cache + metadata_index) = entry;

		if (fat_fs_write_by_data_cluster(mount_point, metadata_cluster, cache) == false)
		{
			page_cache_release_anonymous(cache);
			return ERROR_OCCUR;
		}

		/* update node filesystem data */
		NODE_DATA(node)->metadata_cluster = metadata_cluster;
		NODE_DATA(node)->metadata_index = metadata_index;
	}
	else
	{
		/* reserve a new cluster for the directory */
		// TODO: Add new cluster to the chain and continue
	}

	page_cache_release_anonymous(cache);
	return ERROR_OK;
}

bool fat_fs_initialize_node_cluster(vfs_node* mount_point, vfs_node* new_node, vfs_node* directory, fat_dir_entry_short* entry, virtual_addr cache)
{
	uint32 free_cluster = fat_fs_get_entry_data_cluster(entry);

	if (fat_fs_read_by_data_cluster(mount_point, free_cluster, cache) == false)
		return false;

	memset((void*)cache, 0, 4096);	// erase all data as specified in the documentation

	if ((new_node->attributes & 7) == VFS_DIRECTORY)
	{
		fat_fs_initialize_directory(vector_at(LAYOUT(directory), 0), entry, (char*)cache);

		vfs_add_child(directory, vfs_create_node(".          ", false, VFS_DIRECTORY | VFS_READ | VFS_WRITE, VFS_CAP_READ | VFS_CAP_WRITE, 0, 0, mount_point, directory, &fat_fs_operations));
		vfs_add_child(directory, vfs_create_node("..         ", false, VFS_DIRECTORY | VFS_READ | VFS_WRITE, VFS_CAP_READ | VFS_CAP_WRITE, 0, 0, mount_point, directory, &fat_fs_operations));
	}

	// finally write the first cluster data back to the disk
	if (fat_fs_write_by_data_cluster(mount_point, free_cluster, cache) == false)
		return false;

	return true;
}

// Create a new node under the given directory 
// TODO: eliminate bad cache claiming
vfs_node* fat_fs_create_node(vfs_node* mount_point, vfs_node* directory, char* name, uint32 vfs_attributes)
{
	if (mount_point == 0 || directory == 0 ||
		(mount_point->attributes & 7) != VFS_MOUNT_PT || (directory->attributes & 7) != VFS_DIRECTORY ||
		MOUNT_DATA(mount_point) == 0)
	{
		set_last_error(EINVAL, FAT_BAD_NODE_STRUCTURE, EO_MASS_STORAGE_FS);
		return 0;
	}

	if (fat_fs_validate_83_name(name, strlen(name)) == false)
	{
		set_last_error(EINVAL, FAT_INVALID_83_NAME, EO_MASS_STORAGE_FS);
		return 0;
	}

	uint32 cache;
	if (!(cache = page_cache_reserve_anonymous()))
		return 0;

	uint32 metadata_cluster, metadata_index;

	// try to find an empty entry under the directory clusters.
	if (fat_fs_find_empty_entry(mount_point, directory, cache, &metadata_cluster, &metadata_index))
	{
	file_creation:
		serial_printf("found empty entry at: %u %u\n", metadata_cluster, metadata_index);
		// when found, the cluster is already loaded into the cache
		fat_dir_entry_short* file_entry = (fat_dir_entry_short*)cache + metadata_index;

		uint32 free_cluster = fat_fs_reserve_first_cluster(mount_point, FAT_EOF);
		if (free_cluster == 0)
		{
			DEBUG("FAT32: create new file failed. No more empty clusters");
			page_cache_release_anonymous(cache);
			return 0;
		}

		// create the new node for the vfs tree
		vfs_node* new_node = vfs_create_node(name, true, vfs_attributes, NULL, 0, sizeof(fat_node_data), mount_point, directory, &fat_fs_operations);
		NODE_DATA(new_node)->metadata_cluster = metadata_cluster;
		NODE_DATA(new_node)->metadata_index = metadata_index;

		if (vector_init(LAYOUT(new_node), 1) != ERROR_OK)
			return 0;

		if (vector_insert_back(LAYOUT(new_node), free_cluster) != ERROR_OK)
			return 0;

		if (fat_fs_create_short_entry_from_node(file_entry, new_node) != 0)
		{
			page_cache_release_anonymous(cache);
			delete new_node;
			return 0;
		}
		////////////////////////////////////////////

		/* save the new entry metadata */
		if (fat_fs_write_by_data_cluster(mount_point, metadata_cluster, cache) == false)
		{
			page_cache_release_anonymous(cache);
			delete new_node;
			return 0;
		}

		/* The file's metadata stuff have been created. Now we deal with the file's first cluster */

		// save the created entry for further use as the cache is re-used
		fat_dir_entry_short temp_entry = *file_entry;

		if (fat_fs_initialize_node_cluster(mount_point, new_node, directory, &temp_entry, cache) == false)
		{
			page_cache_release_anonymous(cache);
			delete new_node;
			return 0;
		}

		page_cache_release_anonymous(cache);
		return new_node;
	}
	else
	{
		/* reserve a new cluster for the directory and run the algorithm again */

		// TODO: Test this!!
		uint32 free_cluster = fat_fs_reserve_first_cluster(mount_point, FAT_EOF);
		if (free_cluster == 0)
		{
			DEBUG("FAT32: create new file failed. No more empty clusters");
			page_cache_release_anonymous(cache);
			return 0;
		}

		/* Append the cluster to the FAT cluster-chain */
		fat_fs_mark_cluster(mount_point, vector_at(LAYOUT(directory), LAYOUT(directory)->count - 1), free_cluster);
		if (vector_insert_back(LAYOUT(directory), free_cluster) != ERROR_OK)
			return 0;

		// re-run the function but now we know where the empty entry is (in the free cluster)

		metadata_cluster = free_cluster;
		metadata_index = 0;

		goto file_creation;		// does this even work???
	}

	DEBUG("No empty cluster found!");
	return 0;
}