#ifndef SIMPLE_FS_H_26072016
#define SIMPLE_FS_H_26072016

#include "types.h"
#include "utility.h"
#include "AHCI.h"

void fsysSimpleFind(const char* dir_name, uint32 port, uint32* file_length, uint32* file_start);
void fsysSimpleRead(uint32 start, uint32 size, uint8* buffer);

#pragma pack(push, 1)

typedef struct MSFSEntry
{
	DWORD first_sector;
	WORD size;
	DWORD date_created;
	DWORD last_date_modified;
	DWORD parent_index;
	WORD flags;
	char name[44];
}MSFSEntry_t;

#pragma pack(pop, 1)

#endif
