#ifndef MMNGR_VIRTUAL_H_130516
#define MMNGR_VIRTUAL_H_130516

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include "isr.h"
#include "SerialDebugger.h"

#include "vmmngr_pte.h"
#include "vmmngr_pde.h"

	typedef uint32 virtual_addr;

#define PAGES_PER_TABLE 1024	// intel arch definitions
#define TABLES_PER_DIR	1024
#define PAGE_SIZE 4096

#define DEFAULT_FLAGS I86_PDE_PRESENT | I86_PDE_WRITABLE	// default flags for page tables and pages.

	// definitions for entry extraction based on virtual address. (see virtual address format)

#define PAGE_DIR_INDEX(x)			( ((x) >> 22) & 0x3ff )		// Get the 10 upper bits of x
#define PAGE_TABLE_INDEX(x)			( ((x) >> 12) & 0x3ff )		// Get the 10 "middle" bits of x
#define PAGE_GET_PHYSICAL_ADDR(x)	( (*x) & ~0xfff )			// Physical address is 4KB aligned, so return all bits except the 12 first

	void page_fault(registers_struct* regs);
	// page table definition
	struct ptable
	{
		pt_entry entries[PAGES_PER_TABLE];
	};

	// page directory definition
	struct pdirectory
	{
		pd_entry entries[TABLES_PER_DIR];
	};

	// USEFUL DATA CHAIN cr3 -> pdirectory -> (PAGE_DIR_INDEX(v_addr)) -> pd_entry -> (PAGE_GET_PHYSICAL_ADDR(pd_entry)) -> ptable
	//					 ptable -> (PAGE_TABLE_INDEX(v_addr)) -> pt_entry -> (PAGE_GET_PHYSICAL_ADDR(pt_entry)) -> physical address

	// definitions usage can be replaced by below functions

	// INTERFACE

	// maps the virtual address given to the physical address given
	void vmmngr_map_page(pdirectory* dir, physical_addr phys, virtual_addr virt, uint32 flags);

	// initializes the virtual memory manager
	void vmmngr_initialize();

	// allocates a virtual page filling the entry with the necessary data
	bool vmmngr_alloc_page(virtual_addr base);

	// frees a virtual page
	void vmmngr_free_page(pt_entry* entry);

	// switch page directory
	bool vmmngr_switch_directory(pdirectory* dir, physical_addr pdbr);

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

	// print a directory structure (for debug purposes)
	void vmmngr_print(pdirectory* dir);

	// returns the physical address associated with this virtual address
	physical_addr vmmngr_get_phys_addr(virtual_addr addr);

	// returns true if the page given by the virtual address is present IN RAM
	bool vmmngr_is_page_present(virtual_addr addr);

	// creates a page table for the dir address space
	bool vmmngr_create_table(pdirectory* dir, virtual_addr addr, uint32 flags);

	// creates a new address space
	pdirectory* vmmngr_create_address_space();

	// maps the kernel pages to the directory given
	void vmmngr_map_kernel_space(pdirectory* pdir);

	void vmmngr_switch_to_kernel_directory();

#ifdef __cplusplus
}
#endif

#endif