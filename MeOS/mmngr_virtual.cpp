#include "mmngr_virtual.h"

pdirectory*	current_directory = 0;		// current page directory
physical_addr current_pdbr = 0;			// current page directory base register

void vmmngr_map_page(physical_addr phys, virtual_addr virt)
{
	// our goal is to get the pt_entry indicated by virt and set its frame to phys.

	pdirectory* dir = vmmngr_get_directory();

	pd_entry* e = vmmngr_pdirectory_lookup_entry(dir, virt);

	if (pd_entry_test_attrib(e, I86_PDE_PRESENT) == false)	// table is not present
	{
		ptable* table = (ptable*)pmmngr_alloc_block();

		if (!table)
			return;		// not enough memory

		memset(table, 0, sizeof(table));			// reset the whole table (page pointers)

		pd_entry_add_attrib(e, I86_PDE_PRESENT);	// set this table present in the directory (table pointers)
		pd_entry_add_attrib(e, I86_PDE_WRITABLE);
		pd_entry_set_frame(e, (physical_addr)table);
	}

	// here we have a guaranteed working table (perhaps empty)

	ptable* table = (ptable*)pd_entry_get_frame(*e);
	pt_entry* page = vmmngr_ptable_lookup_entry(table, virt);	// we have the page

	*page = 0;												// delete possible previous information (pt_entry is just a uint32)
	pt_entry_add_attrib(page, I86_PTE_PRESENT);				// and reset
	pt_entry_add_attrib(page, I86_PTE_WRITABLE);
	pt_entry_set_frame(page, phys);
}	

void vmmngr_initialize()
{
}

bool vmmngr_alloc_page(pt_entry* entry)
{
	physical_addr addr = (physical_addr)pmmngr_alloc_block();

	if (!addr)
		return false;

	pt_entry_add_attrib(entry, I86_PTE_PRESENT);
	pt_entry_add_attrib(entry, I86_PTE_WRITABLE);
	pt_entry_set_frame(entry, addr);

	return true;
}

void vmmngr_free_page(pt_entry* entry)
{
	if (!entry)
		return;

	void* addr = (void*)pt_entry_get_frame(*entry);

	if (addr)
		pmmngr_free_block(addr);

	pt_entry_del_attrib(entry, I86_PTE_PRESENT);
	pt_entry_del_attrib(entry, I86_PTE_WRITABLE);
}

bool vmmngr_switch_directory(pdirectory* dir)
{
	if (!dir)
		return false;

	current_directory = dir;
	pmmngr_load_PDBR(current_pdbr);
	return true;
}

pdirectory* vmmngr_get_directory()
{
	return current_directory;
}

void vmmngr_flush_TLB_entry(virtual_addr addr)
{
	_asm
	{
		cli
		invlpg addr		; use assembly special instruction
		sti
	}
}

void vmmngr_ptable_clear(ptable* table)
{
}

pt_entry* vmmngr_ptable_lookup_entry(ptable* p, virtual_addr addr)
{
	if (p)
		return &p->entries[PAGE_TABLE_INDEX(addr)];

	return 0;
}

void vmmngr_pdirectory_clear(pdirectory* pdir)
{
}

pd_entry* vmmngr_pdirectory_lookup_entry(pdirectory* p, virtual_addr addr)
{
	if (p)
		return &p->entries[PAGE_DIR_INDEX(addr)];

	return 0;
}
