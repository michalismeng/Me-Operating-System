#ifndef VMMNGR_PDE_H_160516
#define VMMNGR_PDE_H_160516

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include "mmngr_phys.h"

	// define utilities for a page directory entry (pde)

	enum PAGE_PDE_FLAGS
	{
		I86_PDE_PRESENT = 1,
		I86_PDE_WRITABLE = 2,
		I86_PDE_USER = 4,
		I86_PDE_PWT = 8,
		I86_PDE_PCD = 0x10,
		I86_PDE_ACCESSED = 0x20,
		I86_PDE_DIRTY = 0x40,
		I86_PDE_4MB = 0x80,
		I86_PDE_CPU_GLOBAL = 0x100,
		I86_PDE_LV4_GLOBAL = 0x200,
		I86_PDE_FRAME = 0x7FFFF000
	};

	typedef uint32 pd_entry;

	// INTERFACE

	// sets a flag in the page table entry
	extern void pd_entry_add_attrib(pd_entry* entry, uint32 attrib);

	// clears a flag in the page table entry
	extern void pd_entry_del_attrib(pd_entry* entry, uint32 attrib);

	// checks if the given attribute is set
	extern bool pd_entry_test_attrib(pd_entry* entry, uint32 attrib);

	// sets a frame to page table
	extern void pd_entry_set_frame(pd_entry* entry, physical_addr addr);

	// test if page is present
	extern bool pd_entry_is_present(pd_entry entry);

	// test if directory is user mode
	extern bool pd_entry_is_user(pd_entry entry);

	// test if directory contains 4mb pages
	extern bool pd_entry_is_4mb(pd_entry entry);

	// test if page is writable
	extern bool pd_entry_is_writable(pd_entry entry);

	// get page table entry frame address
	extern physical_addr pd_entry_get_frame(pd_entry entry);

#ifdef __cplusplus
}
#endif

#endif