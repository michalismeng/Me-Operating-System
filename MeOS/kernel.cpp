#include "descriptor_tables.h"
#include "pit.h"
#include "timer.h"
#include "boot_info.h"
#include "mmngr_phys.h"
#include "mmngr_virtual.h"

#include "keyboard.h"
#include "SerialDebugger.h"
#include "Simple_fs.h"

#include "PCI.h"
#include "AHCI.h"

#include "memory.h"
#include "mngr_device.h"

#include "process.h"
#include "mutex.h"
#include "spinlock.h"

#include "vfs.h"
#include "FAT32_fs.h"
#include "vm_contract.h"
#include "MassStorageDefinitions.h"

#include "page_cache.h"
#include "thread_sched.h"

extern "C" uint8 canOutput = 1;
extern HBA_MEM_t* ab;

heap* kernel_heap = 0;

void GetMemoryStats()
{
	serial_printf("Max blocks: %u\n", pmmngr_get_block_count());
	serial_printf("Available blocks: %u\n", pmmngr_get_free_block_count());
	serial_printf("Used blocks: %u\n", pmmngr_get_block_use_count());
}

void print(char* arr)
{
	for (int i = 0; i < 512; i++)
	{
		if (isprint(arr[i]))
			Printch(arr[i]);
		arr[i] = 0;
	}
}

KEYCODE getch()
{
	KEYCODE key = KEY_UNKNOWN;

	while (key == KEY_UNKNOWN)
		key = kybrd_get_last_key();

	kybrd_discard_last_key();
	return key;
}

void get_cmd(char* buff, int max_size)
{
	KEYCODE key = KEY_UNKNOWN;
	int i = 0;

	while (true)
	{
		key = (KEYCODE)getch();

		if (key == KEY_RETURN)
		{
			Printch(key);
			break;
		}

		if (key == KEY_BACKSPACE && i > 0)
		{
			buff[--i] = '\0';
			Printch(key);
		}

		if (isprint(key) && i < max_size)
		{
			char c = kybrd_key_to_ascii(key);

			buff[i++] = c;
			Printch(c);
		}
	}

	buff[i] = '\0';
}

bool run_cmd(char* cmd)
{
	if (strcmp(cmd, "exit") == 0)
		return true;
	else if (strcmp(cmd, "help") == 0)
		printfln("Help messages");
	else if (strcmp(cmd, "clear") == 0)
		ClearScreen();
	else if (strcmp(cmd, "reset") == 0)
		kybrd_reset_system();
	else if (strcmp(cmd, "read") == 0)
	{
		if (vmmngr_is_page_present(0x500000) == false)
			vmmngr_alloc_page(0x500000);

		printf("Enter the sector number to read from: ");
		get_cmd(cmd, 20);
		DWORD sector = atoui(cmd);

		physical_addr addr = vmmngr_get_phys_addr((virtual_addr)0x500000);

		if (ahci_read(0, sector, 0, 1, (VOID PTR)addr) != AHCIResult::AHCI_NO_ERROR)
			DEBUG("AHCI ERROR");
		else
			print((char*)0x500000);
	}
	else if (strcmp(cmd, "dis_output") == 0)
		canOutput = false;
	else if (strcmp(cmd, "en_output") == 0)
		canOutput = true;
	else if (strcmp(cmd, "dis_kybrd") == 0)
		kybrd_disable();
	else if (strcmp(cmd, "caps") == 0)
		ahci_print_caps();
	else if (strcmp(cmd, "memstats") == 0)
		GetMemoryStats();
	else
		printfln("Unknown command: %s.", cmd);

	return false;
}

void Run()
{
	char cmd[30];
	SetMinWritable(strlen("cmd>"));

	while (true)
	{
		printf("cmd>");
		get_cmd(cmd, 28);

		if (run_cmd(cmd) == true)
			printfln("Unfortunately cannot exit.");
	}
}

int a = 0;
mutex m;
spinlock s;

void test1()
{
	for (int i = 0; i < 10000000; i++)
	{
		//mutex_acquire(&m);
		spinlock_acquire(&s);

		__asm
		{
			mov eax, dword ptr[a]
			inc eax
			pause
			mov dword ptr[a], eax
		}

		spinlock_release(&s);

		//mutex_release(&m);
	}

	printfln("test1 a=%u", a);
	while (true);
}

void test2()
{
	printfln("starting test2");
	for (int i = 0; i < 10000000; i++)
	{
		//mutex_acquire(&m);
		spinlock_acquire(&s);

		__asm
		{
			mov eax, dword ptr[a]
			inc eax
			pause
			pause
			mov dword ptr[a], eax
		}

		spinlock_release(&s);

		//mutex_release(&m);
	}

	printfln("test2 a=%u", a);

	while (true);
}

void test_print_time()
{
	printfln("Executing %s", __FUNCTION__);
	while (true)
	{
		INT_OFF;
		uint16 x = cursorX, y = cursorY;

		SetCursor(0, SCREEN_HEIGHT - 2);
		printf("t=%u m=%u", get_ticks(), millis());
		SetCursor(x, y);
		INT_ON;
	}
}

void idle()
{
	printfln("idle executing");
	while (true) _asm pause;
}

struct kernel_info
{
	uint32 kernel_size;
	gdt_entry_t* gdt_base;
	isr_t* isr_handlers;
	idt_entry_t* idt_base;
};

void proc_init_thread()
{
	INT_OFF;

	printfln("executing %s", __FUNCTION__);
	// start setting up heaps, drivers and everything needed.
	vm_area a = vm_area_create(3 GB + 10 MB, 3 GB + 11 MB, VM_AREA_WRITE, -1);
	if (!vm_contract_add_area(&thread_get_current()->parent->memory_contract, &a))
		PANIC("could not add area");

	// create a 16KB heap
	kernel_heap = heap_create(a.start_addr, 16 KB);
	printfln("heap start: %h %h", kernel_heap->start_address, kernel_heap);

	mutex_init(&m);
	spinlock_init(&s);

	thread_insert(thread_create(thread_get_current()->parent, (uint32)test1, 3 GB + 10 MB + 512 KB, 4 KB, 1));
	thread_insert(thread_create(thread_get_current()->parent, (uint32)test2, 3 GB + 10 MB + 508 KB, 4 KB, 1));
	TCB* thread = thread_create(thread_get_current()->parent, (uint32)test_print_time, 3 GB + 10 MB + 504 KB, 4 KB, 1);
	thread_insert(thread);
	ClearScreen();
	INT_ON;

	while (true)
		_asm pause
}

int kmain(multiboot_info* boot_info, kernel_info* k_info)
{
	init_descriptor_tables(k_info->gdt_base, k_info->isr_handlers, k_info->idt_base);

	SetColor(DARK_BLUE, WHITE);
	ClearScreen();

	INT_OFF;
	init_pit_timer(50, timer_callback);
	idt_set_gate(32, (uint32)scheduler_interrupt, 0x08, 0x8E);
	INT_ON;

	init_serial();

	printf("Welcome to ME Operating System\n");

	uint32 memoryKB = 1024 + boot_info->m_memoryLo + boot_info->m_memoryHi * 64;
	pmmngr_init(memoryKB, 0xC0000000 + k_info->kernel_size);

	serial_printf("Memory detected: %h KB %h MB\n", memoryKB, memoryKB / 1024);
	serial_printf("Kernel size: %u bytes\n", k_info->kernel_size);
	serial_printf("Boot device: %h\n", boot_info->m_bootDevice);

	bios_memory_region* region = (bios_memory_region*)boot_info->m_mmap_addr;

	for (uint32 i = 0; i < boot_info->m_mmap_length; i++)
	{
		if (region[i].type > 4)
			break;

		if (i > 0 && region[i].startLo == 0)
			break;

		printf("region %i: start: 0x%x %x length (bytes): 0x%x %x type: %i (%s)\n", i,
			region[i].startHi, region[i].startLo,
			region[i].sizeHi, region[i].sizeLo,
			region[i].type, strMemoryTypes[region[i].type - 1]);

		if (region[i].type == 1 && region[i].startLo >= 0x100000)	// make available only if region is above 1MB
			pmmngr_free_region(&physical_memory_region(region[i].startLo, region[i].sizeLo));
	}

	pmmngr_reserve_region(&physical_memory_region(0x100000, pmmngr_get_next_align(k_info->kernel_size + 1)));

	GetMemoryStats();

	vmmngr_initialize();
	pmmngr_paging_enable(true);

	HBA_MEM_t* abar = PCIFindAHCI();
	serial_printf("Found abar at: %h\n", abar);

	serial_printf("Virtual manager initialize\n");

	//kernel_heap = heap_create(0x300000, 0x4000);		// initialize the heap. Should be done in threading

	fsysSimpleInitialize();
	init_keyboard();

	//test_function(15);
	//PANIC("HERE");

	//init_vfs();

	//uint32 ahci_base = 0x200000;
	//init_ahci(abar, ahci_base + ahci_base % 1024);	// ahci must be 1K aligned. (otherwise... crash). Should be done in threading

	/*ClearScreen();

	auto disk = vfs_find_child(vfs_get_dev(), "sdc");
	auto hierarchy = fat_fs_mount("sdc_mount", (mass_storage_info*)disk->deep_md);

	list_insert_back(&vfs_get_root()->children, hierarchy);

	vfs_print_all();

	vfs_node* n = vfs_find_node("sdc_mount/MIC.TXT");

	fat_fs_load_file_layout((fat_mount_data*)hierarchy->deep_md, (mass_storage_info*)disk->deep_md, n);
	fat_file_layout layout = *(fat_file_layout*)n->deep_md;

	auto l_node = layout.head;
	printf("clusters for %s: ", n->shallow_md.name);

	while (l_node != 0)
	{
		printf("%u ", l_node->data);
		l_node = l_node->next;
	}

	page_cache_init(1000);

	_page_cache_file file = page_cache_file_create(n, vfs_find_node("sdc_mount"), vfs_find_child(vfs_get_dev(), "sdc"));

	virtual_addr r = page_cache_read(&file, 0);
	print((char*)r);

	while (true);		// block multi-tasking for VFS establishment*/

	ClearScreen();

	/*vm_area area;
	vm_contract c;

	vm_contract_init(&c, 0, 4096 * 100);
	area = vm_area_create(0x1000, 0x2000, VM_AREA_WRITE, -1);
	vm_contract_add_area(&c, &area);

	vm_area_set_bounds(&area, 0x4000, 0x1000);
	area.flags |= VM_AREA_GROWS_DOWN;
	vm_contract_add_area(&c, &area);

	vm_contract_print(&c);
	printfln("address: %h", vm_contract_get_area_for_length(&c, 0x1000));

	vm_area* _p = vm_contract_find_area(&c, 0x4500);
	if (_p == 0)
		printfln("area not found");
	else
		printfln("expansion result: %u", vm_contract_expand_area(&c, _p, 0x1000));

	vm_contract_print(&c);

	while (true);*/

	/*kernel_heap = heap_create(0x300000, 16 KB);

	//mutex_init(&m);
	printfln("starting multitasking");
	init_thread_scheduler();

	PCB* pr = process_create(0, vmmngr_get_directory(), 3 GB, 3 GB + 512 MB);

	extern queue<PCB> process_queue;

	PCB* p = &process_queue.head->data;

	/*uint32 phys = (uint32)pmmngr_alloc_block();
	vmmngr_map_page(p->page_dir, phys, 0x700000 - 4096, DEFAULT_FLAGS);

	phys = (uint32)pmmngr_alloc_block();
	vmmngr_map_page(p->page_dir, phys, 0x600000 - 4096, DEFAULT_FLAGS);

	phys = (uint32)pmmngr_alloc_block();
	vmmngr_map_page(p->page_dir, phys, 0x800000 - 4096, DEFAULT_FLAGS);

	thread_insert(thread_create(p, (uint32)test1, 3 GB + 16 KB, 4096));
	thread_insert(thread_create(p, (uint32)test2, 3 GB + 12 KB, 4096));		// create test2 task
	thread_insert(thread_create(p, (uint32)idle, 3 GB + 8 KB, 4096));		// create idle task

	//print_ready_queue();

	TCB* th = ready_queue.head->data;

	INT_OFF;
	multitasking_start();
	INT_ON;

	thread_execute(*th);

	while (true);*/

	// create a minimal multitasking environment to begin working with
	//init_multitasking();

	virtual_addr space = pmmngr_get_next_align(0xC0000000 + k_info->kernel_size + 4096);
	printfln("allocating 4KB at %h", space);

	if (!vmmngr_alloc_page(space) || !vmmngr_alloc_page(space + 4096))
		PANIC("cannot allocate for mini heap");

	// setup a dummy kernel heap for process 0 initialization
	kernel_heap = heap_create(space, 0x2000);
	printfln("heap start: %h %h", kernel_heap->start_address, kernel_heap);

	// create process 0 and its only thread
	PCB* proc = process_create(0, vmmngr_get_directory(), 3 GB, 3 GB + 512 MB);
	uint32 thread_stack = (uint32)malloc(4050);		// allocate enough space for page aligned stack
	// just for this thread, space is not malloc
	TCB* t = thread_create(proc, (uint32)proc_init_thread, pmmngr_get_next_align(thread_stack + 4096), 4096, 1);	// page align stack
	thread_insert(t);

	INT_OFF;
	scheduler_start();
	// From here and below nothing is executed.
	// Control is passed to the process 0 thread.
	INT_ON;
	while (true);

	///////////////////////////////////////

	//Run();

	SetColor(DARK_RED, WHITE);
	ClearScreen();
	SetCursor(0, 13);
	PrintCentered("## RED SCREEN OF DEATH ##");
	SetCursor(81, 25);

	return 0xDEADBABA;
}