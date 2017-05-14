#ifndef FAT32_FS_H_01102016
#define FAT32_FS_H_01102016

// TODO: The whole FAT driver needs checking with bigger files. Operations are not granted to work for any input file given.

#include "types.h"
#include "vfs.h"
#include "MassStorageDefinitions.h"
#include "vector.h"
#include "page_cache.h"
#include "open_file_table.h"
#include "file.h"
#include "error.h"

#define FAT_EOF	0x0FFFFFF8
#define GFD_FAT_SPECIAL 0

#pragma pack(push, 1)

struct fat_partition_entry
{
	uint8 current_state;		// active - inactive state
	uint8 start_head;			// partition start head
	uint16 start_cylsect;		// partition start cylinder and sector
	uint8 type;					// partition type
	uint8 end_head;				// partition end head
	uint16 end_cylsect;			// partition end cylinder and sector
	uint32 lba_offset;			// sectors between MBR and partition start
	uint32 size;				// size of partition in sectors
};

struct fat_mbr
{
	uint8 boot_code[446];
	fat_partition_entry primary_partition;
	// TODO: to be changed... i don't know if type is correct
	fat_partition_entry secondary_partition;
	fat_partition_entry other_partition;
	fat_partition_entry another_partition;
	////////////
	uint16 boot_sign;
};

struct fat_ext_volume_id
{
	uint32 sectors_per_FAT;
	uint16 extended_flags;
	uint16 fat_version;
	uint32 root_cluster_lba;
	uint16 fat_info;
	uint16 backup_BS_sector;
	uint8 reserved_0[12];
	uint8 drive_number;
	uint8 reserved_1;
	uint8 boot_signature;
	uint32 volume_id;
	uint8 volume_label[11];
	uint8 fat_type_label[8];
};

struct fat_volume_id
{
	uint8 boot_jmp[3];
	uint8 oem_name[8];
	uint16 bytes_per_sector;
	uint8 sectors_per_cluster;
	uint16 reserved_sector_count;
	uint8 number_FATs;
	uint16 root_entry_count;
	uint16 total_sectors_16;
	uint8 media_type;
	uint16 table_size_16;
	uint16 sectors_per_track;
	uint16 head_side_count;
	uint32 hidden_sector_count;
	uint32 total_sectors_32;

	fat_ext_volume_id extended;
};

struct fat_dir_entry_short
{
	uint8 name[8];				// 8.3 name
	uint8 extension[3];			// 8.3 extension
	uint8 attributes;
	uint8 resv0;
	uint8 created_time_10;		// created time in thenths of a second
	uint16 created_time;		// created time following special format
	uint16 created_date;		// created date following special format
	uint16 last_accessed_date;
	uint16 cluster_high;		// first data cluster high bits (mask 4 top bits) for this file
	uint16 last_modified_time;
	uint16 last_modified_date;
	uint16 cluster_low;			// first data cluster low bits for this file
	uint32 file_size;			// file size
};

struct fat_dir_entry_long
{
	uint8 order;				// the order in which this long name entry appears
	uint8 name1_5[10];			// first 5 characters each a unicode one
	uint8 attributes;			// attributes same as for short entry but need to be FAT_LFN
	uint8 type;
	uint8 checksum;
	uint8 name6_11[12];			// following 6 characters
	uint16 clustoer_low;		// cluster low always zero for lfn
	uint8 name12_13[4];			// last 2 characters
};

enum FAT_DIR_ATTRIBUTES
{
	FAT_READ_ONLY = 1,
	FAT_HIDDEN = 2,
	FAT_SYSTEM = 4,
	FAT_VOLUME_ID = 8,
	FAT_DIRECTORY = 16,
	FAT_ARCHIVE = 32,
	FAT_LFN = FAT_READ_ONLY | FAT_HIDDEN | FAT_SYSTEM | FAT_VOLUME_ID		// LONG FILE NAME
};

#pragma pack(pop, 1)

enum FAT_IOCTL_COMMANDS
{
	FAT_CREATE_FILE,
	FAT_DELETE_FILE,
	FAT_COPY_FILE,
	FAT_CUT_FILE
};

typedef vector<uint32> fat_file_layout;

struct fat_node_data
{
	fat_file_layout layout;		// the file layout on the FAT image.
	uint32 metadata_cluster;	// the cluster where this file's metadata are located.
	uint32 metadata_index;		// the index in the cluster (0-127) where this file's metadata are located.
};

struct fat_mount_data
{
	fat_file_layout layout;
	uint32 partition_offset;			// start of the FAT partition where the volume ID is located
	uint32 fat_lba;						// linear block addr of the FAT for this volume.
	uint32 cluster_lba;					// linear block addr of the first data cluster.
	uint32 root_dir_first_cluster;		// root directory first cluster.
	uint32 fd;							// file descriptor used when caching general FAT clusters in page cache.
};

// mount the FAT32 filesystem using the 'mount_name'.
// returns the pointer to the mount file head
vfs_node* fat_fs_mount(char* mount_name, vfs_node* dev_node);

// loads the file's, pointed by 'node', cluster chain
vfs_result fat_fs_load_file_layout(fat_mount_data* mount_info, vfs_node* node);


uint32 fat_fs_find_next_cluster(vfs_node* mount_point, uint32 current_cluster);

// reserves the first free cluster assigning it 'next_cluster' value and returns its index.
uint32 fat_fs_reserve_first_cluster(vfs_node* mount_point, uint32 next_cluster);

VFS_ATTRIBUTES fat_to_vfs_attributes(uint32 attrs);
FAT_DIR_ATTRIBUTES vfs_to_fat_attributes(uint32 attrs);

void fat_fs_generate_short_name(vfs_node* node, char name[12]);

vfs_node* fat_fs_create_file(vfs_node* mount_point, vfs_node* directory, char* name, uint32 vfs_attributes);

vfs_result fat_fs_delete_file(vfs_node* mount_point, vfs_node* node);

#endif