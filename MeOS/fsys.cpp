#include "fsys.h"

// private data and definitions

#define DEVICE_MAX 26					// each device gets an aphabet letter number
PFILESYSTEM _FileSystems[DEVICE_MAX] = { 0 };

FILE volOpenFile(const char* filename)
{
	if (filename)
	{
		char device = 'a';
		const char* stripped_fname = filename;

		if (filename[1] == ':')		// spotted form c:/dir/file so change default device
		{
			device = filename[0];
			stripped_fname += 2;
		}

		if (_FileSystems[device - 'a'])
		{
			FILE file = _FileSystems[device - 'a']->Open(stripped_fname);
			file.deviceID = device;
			return file;
		}
	}

	serial_printf("Failed to open %s\n", filename);

	FILE invl_file;
	invl_file.flags = FS_INVALID;
	invl_file.deviceID = DEVICE_INVALID;

	return invl_file;
}

void volReadFile(PFILE file, uint8* buffer, uint32 length)
{
	// TODO: Mark that if file is invalid file->deviceID is invalid and unpredictable
	if (file)
	{
		if (_FileSystems[file->deviceID - 'a'])
			_FileSystems[file->deviceID - 'a']->Read(file, buffer, length);
		else
			buffer = 0;
	}
	else
		buffer = 0;
}

void volCloseFile(PFILE file)
{
	if (file)
		if (_FileSystems[file->deviceID - 'a'])
			_FileSystems[file->deviceID - 'a']->Close(file);
}

void volRegisterFileSystem(PFILESYSTEM fsys, unsigned int deviceID)
{
	if (fsys && deviceID < DEVICE_MAX)
	{
		_FileSystems[deviceID] = fsys;
		serial_printf("Registered filesystem: %s, at device: %u\n", fsys->name, deviceID);
	}
}

void volUnregisterFileSystem(PFILESYSTEM fsys)
{
	// more than one file systems may be registered so do not break on first found
	for (int i = 0; i < DEVICE_MAX; i++)
		if (_FileSystems[i] == fsys)
			_FileSystems[i] = 0;
}

void volUnregisterFileSystemByID(unsigned int deviceID)
{
	if (deviceID < DEVICE_MAX)
		_FileSystems[deviceID] = 0;
}