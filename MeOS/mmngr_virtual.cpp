#include "mmngr_virtual.h"
#include "thread_sched.h"

pdirectory*	current_directory = 0;		// current page directory
physical_addr current_pdbr = 0;			// current page directory base register

pdirectory* kernel_directory = 0;		// kernel page directory

extern TCB* current_thread;

void page_fault(registers_struct* regs)
{
	uint32 addr;
	_asm
	{
		mov eax, cr2
		mov dword ptr addr, eax
	}

	printfln("PAGE_FALUT: FAULTING ADDRESS: %h, FAULTING THREAD: %u", addr, current_thread->id);

	if (addr > 0xF0000000)		// auto allocate space for MMIO
	{
		if ((addr &(~0xfff)) == 0xf0404000)
			printfln("mapping abar");

		addr &= ~0xfff;
		vmmngr_map_page(vmmngr_get_directory(), addr, addr, DEFAULT_FLAGS);

		printfln("mapped: %h %h", addr, vmmngr_get_phys_addr(addr));
	}
	else
	{
		if (vm_contract_find_area(&thread_get_current()->parent->memory_contract, addr) == 0)
			PANIC("could not find address in memory contract");
		else
			vmmngr_alloc_page(addr);

		//printfln("ERROR CODE: %u", regs->err_code);
	}
}

void vmmngr_map_page(pdirectory* dir, physical_addr phys, virtual_addr virt, uint32 flags)
{
	// our goal is to get the pt_entry indicated by virt and set its frame to phys.

	pd_entry* e = vmmngr_pdirectory_lookup_entry(dir, virt);

	if (pd_entry_test_attrib(e, I86_PDE_PRESENT) == false)	// table is not present
		vmmngr_create_table(dir, virt, flags);

	// here we have a guaranteed working table (perhaps empty)

	ptable* table = (ptable*)pd_entry_get_frame(*e);
	pt_entry* page = vmmngr_ptable_lookup_entry(table, virt);	// we have the page

	
	*page = 0;												// delete possible previous information (pt_entry is just a uint32)
	*page |= flags;											// and reset
	pt_entry_set_frame(page, phys);
}

void vmmngr_initialize(uint32 kernel_pages)
{
	pdirectory* pdir = (pdirectory*)pmmngr_alloc_block();
	kernel_directory = pdir;
	printfln("alloced dir at: %h", pdir);

	memset(pdir, 0, sizeof(pdirectory));

	physical_addr phys = 0;		// page directory structure is allocated at the beginning (<1MB) (false)
								// so identity map the first 4MB to be sure we can point to them

	for (uint32 i = 0; i < 1024; i++, phys += 4096)
		vmmngr_map_page(pdir, phys, phys, DEFAULT_FLAGS);

	phys = 0x100000;
	virtual_addr virt = 0xC0000000;

	for (uint32 i = 0; i < kernel_pages; i++, virt += 4096, phys += 4096)
		vmmngr_map_page(pdir, phys, virt, DEFAULT_FLAGS);

	vmmngr_switch_directory(pdir, (physical_addr)&pdir->entries);
	register_interrupt_handler(14, page_fault);
}

bool vmmngr_alloc_page(virtual_addr base)
{
	return vmmngr_alloc_page_f(base, DEFAULT_FLAGS);
}

bool vmmngr_alloc_page_f(virtual_addr base, uint32 flags)
{
	//TODO: cater for memory mapped IO where (in the most simple case) an identity map must be done.
	//TODO: fix this function
	physical_addr addr = base;

	serial_printf("Mapping virtual address: %h - %h\n", base, base + 4095);

	if (vmmngr_is_page_present(base))
		addr = vmmngr_get_phys_addr(base);
	else
	{
		if (base < 0xF0000000)		// memory mapped IO above 3GB
			addr = (physical_addr)pmmngr_alloc_block();

		if (!addr)
			return false;
	}

	vmmngr_map_page(vmmngr_get_directory(), addr, base, DEFAULT_FLAGS);
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
	// if the page directory hasn't change do not flush cr3 as such an action is a performance hit
	if (pmmngr_get_PDBR() == pdbr)
		return true;

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

physical_addr vmmngr_get_phys_addr(virtual_addr addr)
{
	pd_entry* e = vmmngr_pdirectory_lookup_entry(vmmngr_get_directory(), addr);
	ptable* table = (ptable*)pd_entry_get_frame(*e);
	pt_entry* page = vmmngr_ptable_lookup_entry(table, addr);
	physical_addr p_addr = pt_entry_get_frame(*page);

	p_addr += (addr & 0xfff);		// add in-page offset
	return p_addr;
}

void vmmngr_free_page_addr(virtual_addr addr)
{
	pd_entry* e = vmmngr_pdirectory_lookup_entry(vmmngr_get_directory(), addr);
	ptable* table = (ptable*)pd_entry_get_frame(*e);
	pt_entry* page = vmmngr_ptable_lookup_entry(table, addr);

	vmmngr_free_page(page);
}

bool vmmngr_is_page_present(virtual_addr addr)
{
	pd_entry* e = vmmngr_pdirectory_lookup_entry(vmmngr_get_directory(), addr);
	if (e == 0 || pd_entry_is_present(*e) == false)
		return false;

	ptable* table = (ptable*)pd_entry_get_frame(*e);
	if (table == 0)
		return false;

	pt_entry* page = vmmngr_ptable_lookup_entry(table, addr);
	if (page == 0 || pt_entry_is_present(*page) == false)
		return false;

	return true;
}

bool vmmngr_create_table(pdirectory* dir, virtual_addr addr, uint32 flags)
{
	pd_entry* entry = vmmngr_pdirectory_lookup_entry(dir, addr);
	if (pd_entry_is_present(*entry) == false)
	{
		ptable* table = (ptable*)pmmngr_alloc_block();
		if (!table)
			return false;		// not enough memory!!

		memset(table, 0, sizeof(ptable));
		pd_entry_set_frame(entry, (physical_addr)table);
		*entry |= flags;

		return true;
	}

	return false;
}

pdirectory* vmmngr_create_address_space()
{
	pdirectory* dir = (pdirectory*)pmmngr_alloc_block();
	if (!dir)
		return 0;

	printfln("creating addr space at: %h", dir);

	memset(dir, 0, sizeof(pdirectory));
	return dir;
}

void vmmngr_map_kernel_space(pdirectory* pdir)
{
	if (!pdir)
		return;

	memcpy(pdir, kernel_directory, sizeof(pdirectory));
}

void vmmngr_switch_to_kernel_directory()
{
	vmmngr_switch_directory(kernel_directory, (physical_addr)kernel_directory);
}

uint32 vmmngr_get_page_size()
{
	return PAGE_SIZE;
}