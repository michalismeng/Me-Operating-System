#include "mmngr_virtual.h"

pdirectory*	current_directory = 0;		// current page directory
physical_addr current_pdbr = 0;			// current page directory base register

void page_fault(registers_struct regs)
{
	PANIC("page fault");
}

void vmmngr_map_page(physical_addr phys, virtual_addr virt)
{
	// our goal is to get the pt_entry indicated by virt and set its frame to phys.

	pdirectory* dir = vmmngr_get_directory();

	pd_entry* e = vmmngr_pdirectory_lookup_entry(dir, virt);

	if (pd_entry_test_attrib(e, I86_PDE_PRESENT) == false)	// table is not present
	{
		ptable* table = (ptable*)pmmngr_alloc_block();

		printfln("created table %i at %h", PAGE_DIR_INDEX(virt), table);

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

void vmmngr_initialize()		// identity map kernel
{
	pdirectory* pdir = (pdirectory*)pmmngr_alloc_block();
	vmmngr_switch_directory(pdir, (physical_addr)&pdir->entries);

	physical_addr phys = 0;		// page directory structure is allocated at the beginning (<1MB) (?)
								// so identity map the first 4MB to be sure we can point to them

	for (uint32 i = 0; i < 1024; i++, phys += 4096)
		vmmngr_map_page(phys, phys);

	register_interrupt_handler(14, page_fault);
}

bool vmmngr_alloc_page(virtual_addr base)
{
	//TODO: cater for memory mapped IO where (in the most simple case) an identity map must be done.
	//TODO: fix this function
	physical_addr addr = base;

	if (base < 0xF0000000)		// memory mapped IO above 3GB
		addr = (physical_addr)pmmngr_alloc_block();

	if (!addr)
		return false;

	vmmngr_map_page(addr, base);
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

bool vmmngr_switch_directory(pdirectory* dir, physical_addr pdbr)
{
	if (!dir)
		return false;

	current_directory = dir;
	current_pdbr = pdbr;
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
		invlpg addr; use assembly special instruction
		sti
	}
}

void vmmngr_ptable_clear(ptable* table)
{
	if (!table)
		return;

	memset(table, 0, sizeof(ptable));
	pmmngr_free_block(table);
}

pt_entry* vmmngr_ptable_lookup_entry(ptable* p, virtual_addr addr)
{
	if (p)
		return &p->entries[PAGE_TABLE_INDEX(addr)];

	return 0;
}

void vmmngr_pdirectory_clear(pdirectory* pdir)
{
	for (int i = 0; i < TABLES_PER_DIR; i++)
		vmmngr_ptable_clear((ptable*)pd_entry_get_frame(pdir->entries[i]));

	memset(pdir, 0, sizeof(pdirectory));
	pmmngr_free_block(pdir);
}

pd_entry* vmmngr_pdirectory_lookup_entry(pdirectory* p, virtual_addr addr)
{
	if (p)
		return &p->entries[PAGE_DIR_INDEX(addr)];

	return 0;
}

void vmmngr_print(pdirectory* dir)
{
	for (int i = 0; i < 1024; i++)
	{
		if (pd_entry_test_attrib(&dir->entries[i], I86_PDE_PRESENT) == false)
			continue;

		ptable* table = (ptable*)PAGE_GET_PHYSICAL_ADDR(&dir->entries[i]);

		printfln("table %i is present at %h", i, table);

		for (int j = 0; j < 1024; j++)
		{
			if (pt_entry_test_attrib(&table->entries[j], I86_PTE_PRESENT) == false)
				continue;

			//printf("page %i is present with frame: %h ", j, pt_entry_get_frame(table->entries[j]));
		}
	}
}