#include "Simple_fs.h"

// TO BE USED ONLY FOR fsys AND keyboard input TEST PURPOSES
#include "AHCI.h"

// private data

FILESYSTEM _fsysSimple;

FILE fsysSimpleDirectory(const char* dir_name)
{
	char buf[512];
	if (ahci_read(0, 0, 0, 1, buf) != AHCI_NO_ERROR)
		DEBUG("FS boot: Could not read");
	// take bytes 6,7
	uint32 volSize = *(uint32*)(buf + 2);
	uint32 firstIndexSector = *(uint32*)(buf + 6);
	uint32 sectorsToRead = volSize - firstIndexSector;

	//printfln("Reading %u sectors of indices at: %u", sectorsToRead, firstIndexSector);

	for (uint32 i = 0; i < sectorsToRead; i++)
	{
		if (ahci_read(1, firstIndexSector, 0, 1, buf) != AHCI_NO_ERROR)
			DEBUG("FS dirs: Could not read");

		for (int ind = 7; ind >= 0; ind--)
		{
			MSFSEntry* entry = (MSFSEntry*)(buf + sizeof(MSFSEntry) * ind);
			if (strcmp(dir_name, entry->name) == 0)
			{
				FILE file;
				file.flags = FS_FILE;
				file.file_length = entry->size;
				file.file_start = entry->first_sector;
				file.eof = 0;
				file.position = 0;
				strcpy(file.name, entry->name);

				return file;
			}
		}
	}

	FILE invl_file;
	invl_file.flags = FS_INVALID;
	invl_file.deviceID = DEVICE_INVALID;
	return invl_file;
}

void fsysSimpleMount()
{
	serial_printf("Attempt to mount\n");
}

void fsysSimpleRead(PFILE file, uint8* buffer, uint32 length)
{
	if (ahci_read(1, file->file_start + file->position / 512, 0, ceil_division(length, 512), buffer) != AHCIResult::AHCI_NO_ERROR)
		DEBUG("Could not read");

	file->position += length;
	if (file->position >= file->file_length)
		file->eof = 1;
}

void fsysSimpleClose(PFILE file)
{
	serial_printf("Attempt to close file: %h\n", file);
}

FILE fsysSimpleOpen(const char* filename)
{
	serial_printf("Attempt to open filename: %s\n", filename);

	FILE invl_file;
	invl_file.flags = FS_INVALID;
	invl_file.deviceID = DEVICE_INVALID;
	return invl_file;
}

void fsysSimpleInitialize()
{
	// setup Simple file system
	strcpy(_fsysSimple.name, "MeSFS");

	_fsysSimple.Mount = fsysSimpleMount;
	_fsysSimple.Directory = fsysSimpleDirectory;
	_fsysSimple.Open = fsysSimpleOpen;
	_fsysSimple.Read = fsysSimpleRead;
	_fsysSimple.Close = fsysSimpleClose;

	// register volume at device 0
	volRegisterFileSystem(&_fsysSimple, 0);
}