#include "FAT32_fs.h"

// returns the next cluster to read based on the current cluster and the first FAT
uint32 fat_fs_get_next_cluster(mass_storage_info* info, uint32 fat_lba, uint32 current_cluster)
{
	// each 512 byte sector has 128 fat entries. We need to find out which part of FAT to load
	uint32 fat_offset = current_cluster / 128;
	uint32* buffer = (uint32*)info->entry_point(0, info, fat_lba + fat_offset, 0, 8);	// read FAT portion

	return buffer[current_cluster] & 0x0FFFFFFF;		// clear 4 top most bits as they are reserved (FAT28 as we implement)
}

// returns a compressed 8.3 (max 13 characters) with ALL spaces killed
void fat_fs_retrive_short_name(char* name, char* extension, char buffer[13])
{
	buffer[12] = 0;
	uint8 name_index = 0;

	for (uint8 i = 0; i < 8; i++)
		if (name[i] != ' ')
			buffer[name_index++] = name[i];

	buffer[name_index++] = '.';
	uint8 prev_index = name_index;

	for (uint8 i = 0; i < 3; i++)
		if (extension[i] != ' ')
			buffer[name_index++] = extension[i];

	if (name_index == prev_index)		// all three extension chars were spaces so delete . as this is (perhaps) a folder
		buffer[--name_index] = 0;
}

inline uint32 fat_fs_get_entry_cluster(fat_dir_entry_short* e)
{
	return e->cluster_low + ((uint32)e->cluster_high & 0x0FFFFFFF);
}

//void fat_fs_create_long_entry()

// starting at current_cluster reads directories and files. Perhaps they will span more than one cluster.
list<vfs_node*> fat_fs_read_directory(mass_storage_info* info, uint32 fat_lba, uint32 cluster_lba, uint32 current_cluster)
{
	list<vfs_node*> l;
	list_init(&l);

	// used to follow the cluster chain for directories
	uint32 offset = current_cluster;

	while (true)
	{
		fat_dir_entry_short* entry = (fat_dir_entry_short*)info->entry_point(0, info, cluster_lba + (offset - 2) * 8, 0, 8);

		// read all directory entries of this cluster. There are 16 entries as each is 32 bytes long
		for (uint8 i = 0; i < 16; i++)
		{
			if (entry[i].name[0] == 0)			// end
				break;

			if (entry[i].name[0] == 0xE5)		//unused entry
				continue;

			if ((entry[i].attributes & LFN) == LFN)		// this is along file name directory/file
				printfln("long file name detected");

			if ((entry[i].attributes & FAT_VOLUME_ID) == FAT_VOLUME_ID)		// volume id
				continue;

			char name[13] = { 0 };			// fix 8.3 name
			fat_fs_retrive_short_name((char*)entry[i].name, (char*)entry[i].extension, name);

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
					info->entry_point(0, info, cluster_lba + current_cluster - 2, 0, 8);	// restore entry context.
				}
			}
			else
				attrs |= VFS_ATTRIBUTES::VFS_FILE;

			// TODO: Add vfs hidden
			//if((entry[i].attributes & HIDDEN) == HIDDEN)
			//attrs |= VFS_ATTRIBUTES::HIDDEN

			auto node = vfs_create_node(name, true, attrs, entry[i].file_size, sizeof(fat_file_layout));

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

list<vfs_node*> fat_fs_mount(mass_storage_info* info)
{
	// read the whole root directory (all clusters) and load all folders and files

	// hard code partition code...
	// TODO: move this part at partition/filesystem identification

	// get the primary partition offset
	fat_mbr* buffer = (fat_mbr*)info->entry_point(0, info, 0, 0, 1);
	uint32 partiton_offset = buffer->primary_partition.lba_offset;

	// get the volume id of the primary partition
	fat_volume_id* volume = (fat_volume_id*)info->entry_point(0, info, partiton_offset, 0, 1);

	// gather important data
	uint32 fat_lba = partiton_offset + volume->reserved_sector_count;
	uint32 cluster_lba = fat_lba + volume->number_FATs * volume->extended.sectors_per_FAT;
	uint32 root_dir_first_cluster = volume->extended.root_cluster_lba;

	// read root directory along with each sub directory
	return fat_fs_read_directory(info, fat_lba, cluster_lba, root_dir_first_cluster);
}

void fat_fs_load_file_layout(mass_storage_info* info, vfs_node* node)
{
	// TODO: Remove these and take the mount point holding this info
	fat_mbr* buffer = (fat_mbr*)info->entry_point(0, info, 0, 0, 1);
	uint32 partiton_offset = buffer->primary_partition.lba_offset;
	fat_volume_id* volume = (fat_volume_id*)info->entry_point(0, info, partiton_offset, 0, 1);
	uint32 fat_lba = partiton_offset + volume->reserved_sector_count;
	////////////////////

	fat_file_layout* layout = (fat_file_layout*)node->deep_md;

	while (true)
	{
		uint32 next_cluster = fat_fs_get_next_cluster(info, fat_lba, layout->tail->data);
		if (next_cluster >= FAT_EOF)
			break;

		list_insert_back(layout, next_cluster);
	}
}