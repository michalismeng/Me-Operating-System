#include "vm_area.h"

bool vm_area::operator<(const vm_area& other)
{
	return start_addr < other.start_addr;
}

bool vm_area::operator>(const vm_area& other)
{
	return start_addr > other.start_addr;
}

// checks that noth start and end addresses are page-aligned and that they follow the basic ineequality
bool vm_area_check_bounds(uint32 start, uint32 end)
{
	if (start % vmmngr_get_page_size() != 0 ||
		end % vmmngr_get_page_size() != 0 ||
		start >= end)
		return false;

	return true;
}

void vm_area_init(vm_area* area)
{
	area->start_addr = area->end_addr = 0;
	area->flags = VM_AREA_INVALID;

	//TODO: Change this to the upcoming FD_INVALID
	area->fd = (uint32)-1;
}

vm_area vm_area_create(uint32 start, uint32 end, uint32 flags, uint32 fd, uint32 offset)
{
	vm_area a;
	vm_area_init(&a);		// on purpose 'bad' initialization of vm_area

	// assert good arguments and fail if necessary
	if (vm_area_check_bounds(start, end) == false || (flags & VM_AREA_INVALID) == VM_AREA_INVALID)
	{
		PANIC("Bad area received");		// can be removed at release
		return a;
	}

	a.start_addr = start;
	a.end_addr = end;
	a.flags = flags;
	a.fd = fd;
	a.offset = offset;

	return a;
}

bool vm_area_set_bounds(vm_area* area, uint32 start, uint32 length)
{
	if (vm_area_check_bounds(start, start + length) == false)
		return false;

	area->start_addr = start;
	area->end_addr = start + length;

	return true;
}

bool vm_area_intersects(vm_area* _ext, vm_area* _int)
{
	// define the full page valid addresses (exactly)
	uint32 ext_start = vm_area_get_start_address(_ext);
	uint32 ext_end = vm_area_get_end_address(_ext);

	uint32 int_start = vm_area_get_start_address(_int);
	uint32 int_end = vm_area_get_end_address(_int);

	if (int_end < ext_start || int_start > ext_end)
		return false;

	return true;
}

void vm_area_print(vm_area* area)
{
	printfln("start: %h  page: %h  length: %u  page length: %u  page end: %h", area->start_addr, vm_area_get_start_address(area),
		vm_area_get_length(area), ceil_division(vm_area_get_end_address(area) - vm_area_get_start_address(area) + 1, PAGE_SIZE), vm_area_get_end_address(area));
}

bool vm_area_is_ok(vm_area* area)
{
	return ((area->flags & VM_AREA_INVALID) != VM_AREA_INVALID) && area->start_addr < area->end_addr;
}

uint32 vm_area_get_end_address(vm_area* area)
{
	return ceil_division(area->end_addr, PAGE_SIZE) * PAGE_SIZE - 1;
}

uint32 vm_area_get_start_address(vm_area * area)
{
	return (area->start_addr / PAGE_SIZE) * PAGE_SIZE;
}

uint32 vm_area_get_length(vm_area* area)
{
	return area->end_addr - area->start_addr;
}

bool vm_area_is_removable(vm_area* area)
{
	return ((area->flags & VM_AREA_NON_REMOVE) != VM_AREA_NON_REMOVE);
}

bool vm_area_grows_down(vm_area* area)
{
	return ((area->flags & VM_AREA_GROWS_DOWN) == VM_AREA_GROWS_DOWN);
}