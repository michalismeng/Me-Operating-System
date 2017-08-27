#ifndef SIMPLE_FS_H_26072016
#define SIMPLE_FS_H_26072016

/* ----- THIS MODULE IS DEPRECATED AND WILL BE REMOVED */

#include "vfs.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include "utility.h"
#include "SerialDebugger.h"
#include "fsys.h"
#include "cstring.h"
#include "MassStorageDefinitions.h"

	FILE fsysSimpleDirectory(const char* dir_name);
	void fsysSimpleMount();
	void fsysSimpleRead(PFILE file, uint8* buffer, uint32 length);
	void fsysSimpleClose(PFILE file);
	FILE fsysSimpleOpen(const char* filename);

	void fsysSimpleInitialize();

	list<vfs_node*> simple_fs_mount(mass_storage_info* info);

#pragma pack(push, 1)

	struct MSFSEntry
	{
		DWORD first_sector;
		WORD size;
		DWORD date_created;
		DWORD last_date_modified;
		DWORD parent_index;
		WORD flags;
		char name[44];
	};

#pragma pack(pop, 1)

#ifdef __cplusplus
}
#endif

#endif
