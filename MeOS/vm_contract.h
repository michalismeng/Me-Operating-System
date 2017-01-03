#ifndef VM_CONTRACT_H_26102016
#define VM_CONTRACT_H_26102016

// contains c++ code

#include "utility.h"
#include "system.h"
#include "vm_area.h"
#include "ordered_vector.h"

struct vm_contract
{
	ordered_vector<vm_area> contract;		// vitrual memory areas of the process - kenrel contract
	uint32 lowest_addr;						// lowest valid address for this space
	uint32 highest_addr;					// highest valid address for this space
};

// initializes a virtual memory contract. low is inclusive, high is exclusive
void vm_contract_init(vm_contract* c, uint32 low, uint32 high);

// inserts a new memory area that does not overlap with any other
bool vm_contract_add_area(vm_contract* c, vm_area* new_area);

// removes an area
void vm_contract_remove_area(vm_contract* c, vm_area* area);

// expands the given vm_area by 'length' if there is enough space, based on the GROWS_DOWN flag
bool vm_contract_expand_area(vm_contract* c, vm_area* area, uint32 length);

// returns the vm_area that contains the 'address'
vm_area* vm_contract_find_area(vm_contract* c, uint32 address);

// returns a starting address that has 'length' length available. Caller constructs vm_area
uint32 vm_contract_get_area_for_length(vm_contract* c, uint32 length);

// debug print the address contract
void vm_contract_print(vm_contract* c);

#endif