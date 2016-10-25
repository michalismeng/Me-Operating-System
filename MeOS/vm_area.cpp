#include "vm_area.h"

bool vm_area::operator<(const vm_area& other)
{
	return start_addr < other.start_addr;
}

bool vm_area::operator>(const vm_area& other)
{
	return start_addr > other.start_addr;
}

void vm_area_init(vm_area* area)
{
	area->start_addr = area->end_addr = 0;
	area->flags = VM_AREA_INVALID;

	//TODO: Change this to the upcoming FD_INVALID
	area->fd = (uint32)-1;
}

bool vm_area_set_bounds(vm_area* area, uint32 start, uint32 end)
{
	if (start >= end || end - start < PAGE_SIZE || start % PAGE_SIZE != 0 || end % PAGE_SIZE != 0)
		return false;

	area->start_addr = start;
	area->end_addr = end;

	return true;
}

bool vm_area_intersects(vm_area* _ext, vm_area* _int)
{
	//if( (_ext->start_addr <= _int->start_addr && _ext->end_addr > _int->start_addr) ||
	//	)

	return false;
}