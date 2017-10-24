#include "mmngr_virtual.h"

pdirectory*	current_directory = 0;		// current page directory
physical_addr current_pdbr = 0;			// current page directory base register

pdirectory* kernel_directory = 0;		// kernel page directory

void vmmngr_map_page(pdirectory* dir, physical_addr phys, virtual_addr virt, uint32 flags)
{
	// our goal is to get the pt_entry indicated by virt and set its frame to phys.

	uint32* e = vmmngr_pdirectory_lookup_entry(dir, virt);

	// here we have a guaranteed working table because of our init function

	ptable* table = *e & PAGE_FRAME;
	uint32* page = vmmngr_ptable_lookup_entry(table, virt);	// we have the page

	*page = DEFAULT_FLAGS | phys;							// reset
}

void vmmngr_initialize()
{
	pdirectory* pdir = kalloc_a(sizeof(pdirectory), 4096);
	if (pdir == 0)
		PANIC("virtual page directory allocation failed");

	kernel_directory = pdir;
	memset(pdir, 0, sizeof(pdirectory));

	physical_addr phys = 0;			//identity map the first 4MB to be sure we can point to them
	vmmngr_create_table(pdir, phys, DEFAULT_FLAGS);

	for (uint32 i = 0; i < 1024; i++, phys += 4096)
		vmmngr_map_page(pdir, phys, phys, DEFAULT_FLAGS);

	phys = 0x100000;
	virtual_addr virt = 0xC0000000;
	vmmngr_create_table(pdir, virt, DEFAULT_FLAGS);

	for (uint32 i = 0; i < 1024; i++, virt += 4096, phys += 4096)
		vmmngr_map_page(pdir, phys, virt, DEFAULT_FLAGS);

	vmmngr_switch_directory(pdir, (physical_addr)&pdir->entries);
}

bool vmmngr_switch_directory(pdirectory* dir, physical_addr pdbr)
{
	if (!dir)
		return false;

	_asm
	{
		pushad
		mov	eax, dword ptr[dir]
		mov	cr3, eax
		popad
	}

	return true;
}

uint32* vmmngr_ptable_lookup_entry(ptable* p, virtual_addr addr)
{
	if (p)
		return &p->entries[PAGE_TABLE_INDEX(addr)];

	return 0;
}

uint32* vmmngr_pdirectory_lookup_entry(pdirectory* p, virtual_addr addr)
{
	if (p)
		return &p->entries[PAGE_DIR_INDEX(addr)];

	return 0;
}

bool vmmngr_create_table(pdirectory* dir, virtual_addr addr, uint32 flags)
{
	uint32* entry = vmmngr_pdirectory_lookup_entry(dir, addr);

	if ((*entry & 1) != 1)
	{
		ptable* table = kalloc_a(sizeof(ptable), 4096);

		if (!table)
			return false;		// not enough memory!!

		memset(table, 0, sizeof(ptable));

		*entry = (uint32)table | flags;
		return true;
	}

	return false;
}

void vmmngr_paging_enable(bool flag)
{
	_asm
	{
		mov eax, cr0
		cmp byte ptr[flag], 1
		jne disable

		enable :
		or eax, 0x80000000	// set bit 31 of cr0 register
			jmp done

			disable :
		and eax, 0x7FFFFFFF	// unset bit 31

			done :
			mov cr0, eax
	}
}