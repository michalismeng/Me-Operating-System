#include "MassStorageDefinitions.h"

void* mass_storage_read(mass_storage_info* info, uint32 start_low, uint32 start_high, uint32 count, physical_addr address)
{
	return info->entry_point(MASS_STORAGE_READ, info, start_low, start_high, count, address);
}