//////#include "Simple_fs.h"
//////#include "print_utility.h"
//////
//////// TO BE USED ONLY FOR fsys AND keyboard input TEST PURPOSES
//////#include "AHCI.h"
//////
///////* ----- THIS MODULE IS DEPRECATED AND WILL BE REMOVED */
//////
//////// private data
//////
//////FILESYSTEM _fsysSimple;
//////
//////// this function contains deprecated calls that are hard removed. Do not expect this function to work. Do not call
//////FILE fsysSimpleDirectory(const char* dir_name)
//////{
//////	char buf[512];
//////	//if (ahci_read(0, 0, 0, 1, buf) != AHCI_NO_ERROR)		deprecated
//////		DEBUG("FS boot: Could not read");
//////	// take bytes 6,7
//////	uint32 volSize = *(uint32*)(buf + 2);
//////	uint32 firstIndexSector = *(uint32*)(buf + 6);
//////	uint32 sectorsToRead = volSize - firstIndexSector;
//////
//////	//printfln("Reading %u sectors of indices at: %u", sectorsToRead, firstIndexSector);
//////
//////	for (uint32 i = 0; i < sectorsToRead; i++)
//////	{
//////		//if (ahci_read(0, firstIndexSector, 0, 1, buf) != AHCI_NO_ERROR)		deprecated
//////			DEBUG("FS dirs: Could not read");
//////
//////		for (int ind = 7; ind >= 0; ind--)
//////		{
//////			MSFSEntry* entry = (MSFSEntry*)(buf + sizeof(MSFSEntry) * ind);
//////			if (strcmp(dir_name, entry->name) == 0)
//////			{
//////				FILE file;
//////				file.flags = FS_FILE;
//////				file.file_length = entry->size;
//////				file.file_start = entry->first_sector;
//////				file.eof = 0;
//////				file.position = 0;
//////				strcpy(file.name, entry->name);
//////
//////				return file;
//////			}
//////		}
//////	}
//////
//////	FILE invl_file;
//////	invl_file.flags = FS_INVALID;
//////	invl_file.deviceID = DEVICE_INVALID;
//////	return invl_file;
//////}
//////
//////void fsysSimpleMount()
//////{
//////	//serial_printf("Attempt to mount\n");
//////}
//////
//////void fsysSimpleRead(PFILE file, uint8* buffer, uint32 length)
//////{
//////	//if (ahci_read(0, file->file_start + file->position / 512, 0, ceil_division(length, 512), buffer) != AHCIResult::AHCI_NO_ERROR)		deprecated
//////		DEBUG("Could not read");
//////
//////	file->position += length;
//////	if (file->position >= file->file_length)
//////		file->eof = 1;
//////}
//////
//////void fsysSimpleClose(PFILE file)
//////{
//////	//serial_printf("Attempt to close file: %h\n", file);
//////}
//////
//////FILE fsysSimpleOpen(const char* filename)
//////{
//////	//serial_printf("Attempt to open filename: %s\n", filename);
//////
//////	FILE invl_file;
//////	invl_file.flags = FS_INVALID;
//////	invl_file.deviceID = DEVICE_INVALID;
//////	return invl_file;
//////}
//////
//////void fsysSimpleInitialize()
//////{
//////	// setup Simple file system
//////	strcpy(_fsysSimple.name, "MeSFS");
//////
//////	_fsysSimple.Mount = fsysSimpleMount;
//////	_fsysSimple.Directory = fsysSimpleDirectory;
//////	_fsysSimple.Open = fsysSimpleOpen;
//////	_fsysSimple.Read = fsysSimpleRead;
//////	_fsysSimple.Close = fsysSimpleClose;
//////
//////	// register volume at device 0
//////	volRegisterFileSystem(&_fsysSimple, 0);
//////}
//////
//////// this function contains deprecated calls that are hard removed. Do not expect this function to work. Do not call
//////list<vfs_node*> simple_fs_mount(mass_storage_info* info)
//////{
//////	list<vfs_node*> l;
//////	list_init(&l);
//////
//////	char* buf = 0;	// (char*)mass_storage_read(info, 0, 0, 1, 0);  deprecated
//////	//replaces: info->entry_point(0, info, 0, 0, 1);
//////
//////	uint32 first_index_sector = *(uint32*)(buf + 6);
//////	uint32 index_count = *(uint32*)(buf + 10);
//////
//////	vfs_node** nodes = (vfs_node**)malloc(sizeof(vfs_node) * index_count);
//////	uint32* parents = (uint32*)malloc(sizeof(uint32) * index_count);
//////
//////	uint32 sectors_to_read = ceil_division(index_count, 8);
//////	uint32 count = 0;
//////	for (int i = 0; i < sectors_to_read; i++)
//////	{
//////		char* buffer = 0;// (char*)mass_storage_read(info, info->volume_size - i - 1, 0, 1, 0);		deprecated
//////		//replaces: info->entry_point(0, info, info->volume_size - i - 1, 0, 1);
//////
//////		for (int j = 0; j < 8; j++)
//////		{
//////			MSFSEntry* entry = (MSFSEntry*)(buffer + 512 - j * sizeof(MSFSEntry));
//////			if (entry->first_sector == 0)
//////				continue;
//////
//////			nodes[count] = vfs_create_node(entry->name, true, VFS_FILE | VFS_READ | VFS_WRITE, entry->size, sizeof(uint32), NULL, NULL, NULL);
//////			*(uint32*)nodes[count]->deep_md = entry->first_sector;
//////			parents[count] = entry->parent_index;
//////			count++;
//////		}
//////	}
//////
//////	for (uint32 i = 0; i < index_count; i++)
//////	{
//////		if (parents[i] == 0)
//////			list_insert_back(&l, nodes[i]);
//////		else
//////			list_insert_back(&nodes[parents[i] - 1]->children, nodes[i]);
//////	}
//////
//////	free(nodes);
//////	free(parents);
//////
//////	return l;
//////}