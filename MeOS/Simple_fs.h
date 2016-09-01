#ifndef SIMPLE_FS_H_26072016
#define SIMPLE_FS_H_26072016

// this file contains only dummy implementations of the below functions, to validate command input and fsys funcionality
// to be developped

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include "utility.h"
#include "SerialDebugger.h"
#include "fsys.h"
#include "cstring.h"

	FILE fsysSimpleDirectory(const char* dir_name);
	void fsysSimpleMount();
	void fsysSimpleRead(PFILE file, uint8* buffer, uint32 length);
	void fsysSimpleClose(PFILE file);
	FILE fsysSimpleOpen(const char* filename);

	void fsysSimpleInitialize();

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
