#ifndef MMNGR_VIRTUAL_H_130516
#define MMNGR_VIRTUAL_H_130516

#include "types.h"
#include "kalloc.h"

typedef uint32 virtual_addr;
typedef uint32 physical_addr;

#define PAGES_PER_TABLE 1024	// intel arch definitions
#define TABLES_PER_DIR	1024
#define PAGE_SIZE 4096

#define PAGE_FRAME 0xFFFFF000
#define DEFAULT_FLAGS 3		// default flags for page tables and pages. (present, writable and kernel)

// definitions for entry extraction based on virtual address. (see virtual address format)

#define PAGE_DIR_INDEX(x)			( ((x) >> 22) & 0x3ff )		// Get the 10 upper bits of x
#define PAGE_TABLE_INDEX(x)			( ((x) >> 12) & 0x3ff )		// Get the 10 "middle" bits of x
#define PAGE_GET_PHYSICAL_ADDR(x)	( (*x) & ~0xfff )			// Physical address is 4KB aligned, so return all bits except the 12 first

// page table definition
typedef struct _ptable
{
	uint32 entries[PAGES_PER_TABLE];
}ptable;

// page directory definition
typedef struct _pdirectory
{
	uint32 entries[TABLES_PER_DIR];
}pdirectory;

// INTERFACE

// maps the virtual address given to the physical address given
void vmmngr_map_page(pdirectory* dir, physical_addr phys, virtual_addr virt, uint32 flags);

// initializes the virtual memory manager
void vmmngr_initialize();

// switch page directory
bool vmmngr_switch_directory(pdirectory* dir, physical_addr pdbr);

// returns entry of ptable p based on addr
uint32* vmmngr_ptable_lookup_entry(ptable* p, virtual_addr addr);

// returns entry of pdirectory p based on addr
uint32* vmmngr_pdirectory_lookup_entry(pdirectory* p, virtual_addr addr);

// creates a page table for the dir address space
bool vmmngr_create_table(pdirectory* dir, virtual_addr addr, uint32 flags);

// enables paging (or disables based on flag)
void vmmngr_paging_enable(bool flag);

#endif