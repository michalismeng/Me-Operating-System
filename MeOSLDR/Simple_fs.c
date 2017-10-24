#include "Simple_fs.h"

// private functions

int8 strcmp(const char* s1, const char* s2)
{
	int8 res = 0;

	while (!(res = *s1 - *s2) && *s2)
		s1++, s2++;

	if (res < 0)
		res = -1;
	else if (res > 0)
		res = 1;

	return res;
}

// public functions

void fsysSimpleFind(const char* dir_name, uint32 port, uint32* file_length, uint32* file_start)
{
	// set error values
	*file_length = (uint32)-1;
	*file_start = 0;
	////////////////////////

	char buf[512];
	if (ahci_read(1, 0, 0, 1, buf) != AHCI_NO_ERROR)
		PANIC("FS boot: Could not read");

	// take bytes 6,7
	uint32 volSize = *(uint32*)(buf + 2);
	uint32 firstIndexSector = *(uint32*)(buf + 6);
	uint32 sectorsToRead = volSize - firstIndexSector;

	for (uint32 i = 0; i < sectorsToRead; i++)
	{
		if (ahci_read(port, firstIndexSector, 0, 1, buf) != AHCI_NO_ERROR)
			PANIC("FS dirs: Could not read");

		for (int ind = 7; ind >= 0; ind--)
		{
			MSFSEntry_t* entry = (MSFSEntry_t*)(buf + sizeof(MSFSEntry_t) * ind);
			if (strcmp(dir_name, entry->name) == 0)
			{
				*file_length = entry->size * 512;
				*file_start = entry->first_sector;
				return;
			}
		}
	}
}
void fsysSimpleRead(uint32 start, uint32 size, uint8* buffer)
{
	if (ahci_read(1, start, 0, ceil_division(size, 512), buffer) != AHCI_NO_ERROR)
		PANIC("Kernel Could not read");
}