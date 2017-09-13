#ifndef VM_AREA_H_25102016
#define VM_AREA_H_25102016

// contains heavy c++ code

#include "error.h"
#include "utility.h"
#include "mmngr_virtual.h"
#include "memory_definitions.h"

enum VM_AREA_ERROR
{
	VM_AREA_NONE,
	VM_AREA_BAD_BOUNDS
};

// defines a process virtual area. (This is the bare bones of a process - kernel memory contract)
struct vm_area
{
	uint32 start_addr;			// area inclusive start address
	uint32 end_addr;			// area exclusive end address (last valid address + 1)
	uint32 flags;				// area flags used to determine page fault action
	uint32 fd;					// the global file descriptor connected with this area
	uint32 offset;				// the offset withing the file of the mapping

	bool operator< (const vm_area& other);		// necessary c++ functions for keeping order
	bool operator> (const vm_area& other);
};

// initializes a vm_area for use
void vm_area_init(vm_area* area);

// creates a vm area
vm_area vm_area_create(uint32 start, uint32 end, uint32 flags, uint32 fd, uint32 offset);

// sets the boundaries of the vm_area. Arguments must be page-aligned
bool vm_area_set_bounds(vm_area* area, uint32 start, uint32 length);

// returns whether _ext(erior) intersects with _int(erior) vm_area
bool vm_area_intersects(vm_area* _ext, vm_area* _int);

// debug prints a vm_area
void vm_area_print(vm_area* area);

// checks if the area has valid address
bool vm_area_is_ok(vm_area* area);

// returns the last valid full-page address of the area
uint32 vm_area_get_end_address(vm_area* area);

// returns the first valid full-page address of the area
uint32 vm_area_get_start_address(vm_area* area);

// returns the actual (not full-page) length of the area
uint32 vm_area_get_length(vm_area* area);

// returns whether area can be removed from a contract.
bool vm_area_is_removable(vm_area* area);

// return whether the area grows downwards
bool vm_area_grows_down(vm_area* area);

#endif