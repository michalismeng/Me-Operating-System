#ifndef MASS_STORAGE_HEADERS_01102016
#define MASS_STORAGE_HEADERS_01102016

// this file contains the interface needed by anyone who wishes to send standard commands to a Mass Storage Device

#include "types.h"
#include "mmngr_phys.h"

enum MASS_STORAGE_COMMANDS
{
	MASS_STORAGE_READ = 0,
	MASS_STORAGE_WRITE = 1
};

struct mass_storage_info;

typedef int(*mass_read)(mass_storage_info* info, uint32 start_low, uint32 start_high, uint32 count, physical_addr address);
typedef int(*mass_write)(mass_storage_info* info, uint32 start_low, uint32 start_high, uint32 count, physical_addr address);

struct mass_storage_info
{
	uint32 volume_size;								// volume size in sectors
	uint16 sector_size;								// size of sector in bytes
	char serial_number[20];							// volume serial number
	uint32 vendor_number;							// volume vendor identifier
	void* (*entry_point)(uint32 command, ...);		// pointer to driver entry point

	//mass_read read;
	//mass_write write;
};

extern "C" void* mass_storage_read(mass_storage_info* info, uint32 start_low, uint32 start_high, uint32 count, physical_addr address);

#endif