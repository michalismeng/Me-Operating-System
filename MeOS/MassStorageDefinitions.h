#ifndef MASS_STORAGE_HEADERS_01102016
#define MASS_STORAGE_HEADERS_01102016

#include "types.h"

struct mass_storage_info
{
	uint32 volume_size;								// volume size in sectors
	uint16 sector_size;								// size of sector in bytes
	char serial_number[20];							// volume serial number
	uint32 vendor_number;							// volume vendor identifier
	void* (*entry_point)(uint32 command, ...);		// pointer to entry point
};

#endif