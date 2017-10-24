#include "types.h"
#include "boot_info.h"
#include "descriptor_tables.h"
#include "pit.h"
#include "timer.h"
#include "AHCI.h"
#include "Simple_fs.h"
#include "PE_Definitions.h"
#include "kalloc.h"
#include "mmngr_virtual.h"

/* 
	This project contains pretty much duplicate code from the main kernel project. 
	This is an easy way to load the main kernel (AHCI driver) and setup paging (mmngr_virtual).
*/

struct kernel_info
{
	uint32 kernel_size;
	gdt_entry_t* gdt_base;
	isr_t* isr_handlers;
	idt_entry_t* idt_base;
};

#define KRN_LDR_BASE 0x10000			// this executable's base address
#define KRN_LDR_LIMIT 0x80000			// max address of guaranteed free conventional memory
#define KERNEL_BASE 0x100000			// our kernel's base address.		Both are physical
#define KERNEL_BASE_VIRT 0xC0000000

void execute_kernel(struct multiboot_info* boot_info, struct kernel_info* k_info)
{
	IMAGE_DOS_HEADER* pImage = (IMAGE_DOS_HEADER*)KERNEL_BASE_VIRT;
	IMAGE_NT_HEADERS* pHeaders = (IMAGE_NT_HEADERS*)(KERNEL_BASE_VIRT + pImage->e_lfanew);

	uint32 base = pHeaders->OptionalHeader.ImageBase;
	uint32 entryPoint = pHeaders->OptionalHeader.AddressOfEntryPoint;

	void(*entryFunction) (struct multiboot_info*, uint32) = (entryPoint + base);
	entryFunction(boot_info, k_info);
}

int ldr_main(struct multiboot_info* boot_info, uint32 krnldr_size_bytes)
{
	SetColor(MakeColor(DARK_BLUE, WHITE));
	ClearScreen();

	if (krnldr_size_bytes > 40 KB)
		PANIC("Kernel Loader is too large");

	init_kallocations(KRN_LDR_BASE + krnldr_size_bytes, KRN_LDR_LIMIT);

	Print("Initializing descriptor tables.");

	INT_OFF;
	init_isr();
	init_descriptor_tables();
	init_pic();
	INT_ON;

	init_pit_timer(50, timer_callback);

	struct kernel_info* k_info = kalloc(sizeof(struct kernel_info));

	//setup AHCI
	HBA_MEM_t* abar = PCIFindAHCI();

	// initialize basic virtual memory
	vmmngr_initialize();

	uint32 ahci_base = kalloc_get_ptr() + 1024 - (uint32)kalloc_get_ptr() % 1024;
	init_ahci(abar, ahci_base);

	uint32 start, _length, position = 0;
	fsysSimpleFind("MeOs.exe", 1, &_length, &start);

	if (start == (uint32)-1 && _length == 0)
		PANIC("Kernel module could not be found!");

	while (position <= _length)
	{
		fsysSimpleRead(start + position / 512, 4096, KERNEL_BASE + position);
		position += 4096;
	}

	// after all the loading is done... enable paging
	vmmngr_paging_enable(true);

	k_info->kernel_size = _length;
	k_info->isr_handlers = interrupt_handlers;
	k_info->gdt_base = gdt_entries;
	k_info->idt_base = idt_entries;

	printfln("Executing kernel\0");
	execute_kernel(boot_info, k_info);

	ClearScreen();
	_asm cli
	_asm hlt
}