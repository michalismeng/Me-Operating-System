#include "test_Fat32.h"

bool test_FAT32_init()
{
	serial_printf("Starting FAT32 test cases.\n");

	vfs_node* disk = vfs_find_child(vfs_get_dev(), "sdc");
	if (!disk)
		FAIL("Could not find FAT32 volume sdc: %e\n");

	vfs_node* hierarchy = fat_fs_mount("sdc_mount", disk);

	if (hierarchy == 0)
		FAIL("Could not mount FAT32 drive: %e\n");

	vfs_add_child(vfs_get_root(), hierarchy);

	SUCCESS("sdc drive mounted succesfully\n");
}

bool test_FAT32_create_file()
{
	vfs_node* file, *mnt, *folder;

	if (vfs_lookup(vfs_get_root(), "sdc_mount", &mnt) != ERROR_OK)
		FAIL("Could not find FAT32 mount: %e\n");

	if (vfs_lookup(mnt, "FOLDER", &folder) != ERROR_OK)
		FAIL("Could not find FAT32 FOLDER: %e\n");

	if ((file = fat_fs_create_node(mnt, folder, "SOME2", VFS_READ | VFS_WRITE | VFS_DIRECTORY)) == 0)
		FAIL("Could not create test file some.txt: %e\n");

	serial_printf("some.txt FAT32 attributes:\n");
	serial_printf("data first sector: %u\n", ((fat_node_data*)file->deep_md)->layout[0]);
	serial_printf("metadata cluster: %u\n", ((fat_node_data*)file->deep_md)->metadata_cluster);
	serial_printf("metadata index: %u\n", ((fat_node_data*)file->deep_md)->metadata_index);

	SUCCESS("some.txt created succesfully");
}

bool test_FAT32_delete_file()
{
	return true;
}

bool test_FAT32_create_directory()
{
	return true;

}

bool test_FAT32_delete_directory()
{

	return true;
}

bool test_FAT32_delete_directory_with_files()
{
	return true;

}

bool test_FAT32_move_file()
{
	return true;

}

bool test_FAT32_move_directory()
{
	return true;

}