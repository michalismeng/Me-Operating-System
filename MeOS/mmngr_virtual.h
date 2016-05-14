#ifndef MMNGR_VIRTUAL_H_130516
#define MMNGR_VIRTUAL_H_130516

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

#include "vmmngr_pte.h"
#include "vmmngr_pde.h"

typedef uint32 virtual_addr;

#define PAGES_PER_TABLE 1024	// intel arch definitions
#define TABLES_PER_DIR	1024

	// definitions for entry extraction based on virtual address. (see virtual address format)

#define PAGE_DIR_INDEX(x)			( ((x) >> 22) & 0x3ff )		// Get the 10 upper bits of x 
#define PAGE_TABLE_INDEX(x)			( ((x) >> 12) & 0x3ff )		// Get the 10 "middle" bits of x
#define PAGE_GET_PHYSICAL_ADDR(x)	( *x & ~0xfff )				// Physical address is 4KB aligned, so return all bits except the 12 first

// page table definition
typedef struct ptable_t
{
	pt_entry entries[PAGES_PER_TABLE];
}ptable;

// page directory definition
typedef struct pdirectory_t
{
	pd_entry entries[TABLES_PER_DIR];
}pdirectory;

// USEFUL DATA CHAIN cr3 -> pdirectory -> (PAGE_DIR_INDEX(v_addr)) -> pd_entry -> (PAGE_GET_PHYSICAL_ADDR(pd_entry)) -> ptable
//					 ptable -> (PAGE_TABLE_INDEX(v_addr)) -> pt_entry -> (PAGE_GET_PHYSICAL_ADDR(pt_entry)) -> physical address

// definitions usage can be replaced by below functions

// INTERFACE

// maps the virtual address given to the physical address given
void vmmngr_map_page(physical_addr phys, virtual_addr virt);

// initializes the virtual memory manager
void vmmngr_initialize();

// allocates a virtual page filling the entry with the necessary data
bool vmmngr_alloc_page(pt_entry* entry);

// frees a virtual page
void vmmngr_free_page(pt_entry* entry);

// switch page directory
bool vmmngr_switch_directory(pdirectory* dir);

// get the current page directory
pdirectory* vmmngr_get_directory();

// flush a cached virtual address
void vmmngr_flush_TLB_entry(virtual_addr addr);

// clear a page table
void vmmngr_ptable_clear(ptable* table);

// returns entry of ptable p based on addr
pt_entry* vmmngr_ptable_lookup_entry(ptable* p, virtual_addr addr);

// clear a page directory
void vmmngr_pdirectory_clear(pdirectory* pdir);

// returns entry of pdirectory p based on addr
pd_entry* vmmngr_pdirectory_lookup_entry(pdirectory* p, virtual_addr addr);


#ifdef __cplusplus
}
#endif

#endif