#ifndef VM_AREA_H_25102016
#define VM_AREA_H_25102016

// contains heavy c++ code

#include "utility.h"
#include "mmngr_virtual.h"

enum VM_AREA_FLAGS
{
	VM_AREA_INVALID = 1 << 0,
	VM_AREA_READ = 1 << 1,
	VM_AREA_WRITE = 1 << 2,
	VM_AREA_EXEC = 1 << 3,
	VM_AREA_SHARED = 1 << 4,
	VM_AREA_PRIVATE = 1 << 5,
	VM_AREA_EXCLUSIVE = 1 << 6
};

// defines a process virtual area. (This is the bare bones of a process - kernel memory contract)
struct vm_area
{
	uint32 start_addr;			// area inclusive start address
	uint32 end_addr;			// area exclusive end address (last valid address + 1)
	uint32 flags;				// area flags used to determine page fault action
	uint32 fd;					// the file descriptor connected with this area

	bool operator< (const vm_area& other);		// necessary c++ functions for keeping order
	bool operator> (const vm_area& other);
};

// initializes a vm_area for use
void vm_area_init(vm_area* area);

// sets the boundaries of the vm_area. Arguments must be page-aligned
bool vm_area_set_bounds(vm_area* area, uint32 start, uint32 end);

// returns whether _ext(erior) contains _int(erior) vm_area
bool vm_area_intersects(vm_area* _ext, vm_area* _int);

#endif