#ifndef FSYS_H_2607_2016
#define FSYS_H_2607_2016

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include "utility.h"
#include "SerialDebugger.h"

	// FILE flags.
	// TODO: Add more like hidden, password-protected...
	enum FILE_FLAGS
	{
		FS_INVALID = 0,
		FS_DIRECTORY,
		FS_FILE,
		FS_LINK
	};

#define DEVICE_INVALID 27				// invalid device mark

	// FILE definition
	typedef struct _FILE
	{
		char name[64];
		uint32 flags;
		uint32 file_length;
		uint32 file_start;
		bool eof;
		uint32 position;
		uint32 deviceID;
	}FILE, *PFILE;

	// filesystem interface
	typedef struct _FILE_SYSTEM
	{
		char name[8];

		// Function pointers to different file actions

		FILE(*Directory) (const char* dir_name);
		void(*Mount) ();
		void(*Read) (PFILE file, uint8* buffer, uint32 length);
		void(*Close) (PFILE file);
		FILE(*Open) (const char* filename);
		/////////
	}FILESYSTEM, *PFILESYSTEM;

	// request filesystem to open filename
	FILE volOpenFile(const char* filename);

	// request filesystem to read an opened file
	void volReadFile(PFILE file, uint8* buffer, uint32 length);

	// TODO: Add write function

	// request filesystem to close an opened file
	void volCloseFile(PFILE file);

	// register a filesystem to be mounted on a device
	void volRegisterFileSystem(PFILESYSTEM fsys, unsigned int deviceID);

	// un-register a filesystem from a device
	void volUnregisterFileSystem(PFILESYSTEM fsys);

	//un-register a filesystem from a device by its ID
	void volUnregisterFileSystemByID(unsigned int deviceID);

#ifdef __cplusplus
}
#endif

#endif
