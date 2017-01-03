#include "FAT32_fs.h"

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
list<vfs_node*> fat_fs_read_directory(mass_storage_info* info, uint32 fat_lba, uint32 cluster_lba, uint32 current_cluster)
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
				attrs |= VFS_ATTRIBUTES::VFS_DIRECTORY;

				// prevent infinite loop as '.' represents this directory
				if (name[0] != '.')
				{
					// This is a directory. It contains files and directories so read them all
					uint32 clus = fat_fs_get_entry_cluster(entry + i);
					children = fat_fs_read_directory(info, fat_lba, cluster_lba, clus);		// TODO: rethink this with buffers
					mass_storage_read(info, cluster_lba + current_cluster - 2, 0, 8, 0);	// restore entry context.
					//replaces: info->entry_point(0, info, cluster_lba + current_cluster - 2, 0, 8);
				}
			}
			else
				attrs |= VFS_ATTRIBUTES::VFS_FILE;

			// TODO: Add vfs hidden
			//if((entry[i].attributes & HIDDEN) == HIDDEN)
			//attrs |= VFS_ATTRIBUTES::HIDDEN

			auto node = vfs_create_node(name, true, attrs, entry[i].file_size, sizeof(fat_file_layout));
			node->shallow_md.funcions.fs_read = fat_fs_load_file;

			// setup layout list and add the starting cluster
			fat_file_layout* layout = (fat_file_layout*)node->deep_md;
			list_init(layout);
			list_insert_back(layout, fat_fs_get_entry_cluster(entry + i));

			node->children = children;
			list_insert_back(&l, node);
		}

		offset = fat_fs_get_next_cluster(info, fat_lba, offset);
		if (offset >= FAT_EOF)		// test for end of file marker
			break;
	}

	return l;
}

vfs_node* fat_fs_mount(char* mount_name, mass_storage_info* info)
{
	// read the whole root directory (all clusters) and load all folders and files

	// hard code partition code...
	// TODO: move this part at partition/filesystem identification

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

	// read root directory along with each sub directory
	list<vfs_node*> hierarchy = fat_fs_read_directory(info, fat_lba, cluster_lba, root_dir_first_cluster);

	// create the vfs mount point node and get the mount data pointer
	// TODO: Change VFS_DIRECTORY to VFS_MOUNT_POINT
	vfs_node* mount_point = vfs_create_node(mount_name, true, VFS_DIRECTORY, 0, sizeof(fat_mount_data));
	fat_mount_data* mount_data = (fat_mount_data*)mount_point->deep_md;

	// set the mount point children to the list hierarchy just created
	mount_point->children = hierarchy;

	// load the data at the mount point
	mount_data->cluster_lba = cluster_lba;
	mount_data->fat_lba = fat_lba;
	mount_data->partition_offset = partiton_offset;
	mount_data->root_dir_first_cluster = root_dir_first_cluster;

	return mount_point;
}

void fat_fs_load_file_layout(fat_mount_data* mount_info, mass_storage_info* storgae_info, vfs_node* node)
{
	fat_file_layout* layout = (fat_file_layout*)node->deep_md;

	while (true)
	{
		uint32 next_cluster = fat_fs_get_next_cluster(storgae_info, mount_info->fat_lba, layout->tail->data);
		if (next_cluster >= FAT_EOF)
			break;

		list_insert_back(layout, next_cluster);
	}
}

int fat_fs_load_file(vfs_node* mount_point, mass_storage_info* storage_info, vfs_node* node,
	uint32 file_page, virtual_addr address)
{
	fat_file_layout* layout = (fat_file_layout*)node->deep_md;
	fat_mount_data* mount_info = (fat_mount_data*)mount_point->deep_md;

	if (layout->count == 0)
		return -1;

	auto l = layout->head;
	for (int i = 0; i < file_page; i++)
	{
		if (l == 0)
			return -1;
		l = l->next;
	}

	mass_storage_read(storage_info, mount_info->cluster_lba + (l->data - 2) * 8, 0, 8, vmmngr_get_phys_addr(address));
	return 0;
}