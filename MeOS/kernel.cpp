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

extern "C" uint8 canOutput = 1;
extern HBA_MEM_t* ab;

heap* kernel_heap = 0;

void GetMemoryStats()
{
	printfln("Max blocks: %u", pmmngr_get_block_count());
	printfln("Available blocks: %u", pmmngr_get_free_block_count());
	printfln("Used blocks: %u", pmmngr_get_block_use_count());

	printfln("");
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
		key = getch();
		//printf("input");

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

		if (false/*ahci_read(0, sector, 0, 1, (VOID PTR)addr) != AHCIResult::AHCI_NO_ERROR*/)
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
	else
		printfln("Unknown command: %s.", cmd);

	return false;
}

void Run()
{
	char cmd[30];
	//SetMinWritable(strlen("cmd>"));

	while (true)
	{
		printf("cmd>");
		get_cmd(cmd, 28);

		if (run_cmd(cmd) == true)
			printfln("Unfortunately cannot exit.");
	}
}

void test1()
{
	printfln("Hello everyone");
	sleep(3000);
	printfln("Thread 1");
	while (true);
}

void test2()
{
	printfln("hello from the other side");
	sleep(400);
	printfln("Thread 2");
	while (true);
}

void idle()
{
	while (true) _asm pause;
}

int kmain(multiboot_info* boot_info, uint32 memory_map_len)
{
	uint32 kernel_size_bytes;
	uint32 memory_map_length = memory_map_len;

	_asm mov dword ptr[kernel_size_bytes], edx

	__asm cli
	init_descriptor_tables();
	__asm sti

	SetColor(DARK_BLUE, WHITE);
	ClearScreen();

	init_pit_timer(50, timer_callback);
	init_serial();

	printf("Welcome to ME Operating System!\n");

	uint32 memoryKB = 1024 + boot_info->m_memoryLo + boot_info->m_memoryHi * 64;
	serial_printf("Memory detected: %h KB %h MB\n", memoryKB, memoryKB / 1024);

	serial_printf("Kernel size: %u bytes\n", kernel_size_bytes);

	serial_printf("Boot device: %h\n", boot_info->m_bootDevice);

	bios_memory_region* region = (bios_memory_region*)0x500;

	pmmngr_init(memoryKB, 0xC0000000 + kernel_size_bytes);

	for (uint32 i = 0; i < memory_map_length; i++)
	{
		if (region[i].type > 4)
			break;

		if (i > 0 && region[i].startLo == 0)
			break;

		serial_printf("region %i: start: 0x%x %x length (bytes): 0x%x %x type: %i (%s)\n", i,
			region[i].startHi, region[i].startLo,
			region[i].sizeHi, region[i].sizeLo,
			region[i].type, strMemoryTypes[region[i].type - 1]);

		if (region[i].type == 1 && region[i].startLo >= 0x100000)	// make available only if region is above 1MB
			pmmngr_free_region(&physical_memory_region(region[i].startLo, region[i].sizeLo));
	}

	pmmngr_reserve_region(&physical_memory_region(0x100000, kernel_size_bytes + 4096));

	GetMemoryStats();

	vmmngr_initialize();
	pmmngr_paging_enable(true);

	HBA_MEM_t* abar = PCIFindAHCI();
	serial_printf("Found abar at: %h\n", abar);

	serial_printf("Virtual manager initialize\n");

	kernel_heap = heap_create(0x300000, 0x2000);		// initialize the heap

	fsysSimpleInitialize();
	init_keyboard();

	uint32 ahci_base = 0x200000;
	init_ahci(abar, ahci_base + ahci_base % 1024);	// ahci must be 1K aligned. (otherwise... crash)

	//ahci_send_identify(0, (VOID PTR)addr);
	//printfln("\nIdentification sector count: %u", *(uint32*)(buf + 120));

	init_device_manager();

	/*PDEVICE screen = mngr_device_add(0);
	mngr_device_add_std_info(screen, "SCREEN", 1, screen_control_function, 0);

	screen->device_control(0, BLACK, BLUE);
	screen->device_control(1, "Hello world!\n");
	screen->device_control(0, BLUE, WHITE);
	screen->device_control(1, "Hello_world!\n");*/

	ClearScreen();
	init_multitasking();
	//uint32 id = process_create("TestDLL.exe");
	//process_create("TestDLL2.exe");

	vmmngr_alloc_page(0x700000 - 4096);
	task_create((uint32)test1, 0x700000);		// create test1 task

	vmmngr_alloc_page(0x850000 - 4096);
	task_create((uint32)Run, 0x850000);			// create Run task

	vmmngr_alloc_page(0x600000 - 4096);
	task_create((uint32)idle, 0x600000);		// create idle task

	vmmngr_alloc_page(0x750000 - 4096);
	task_create((uint32)test2, 0x750000);		// create test2 task

	print_ready_queue();

	task* t = &ready_queue.head->data;

	_asm cli
	start();

	task_exeute(*t);
	///////////////////////////////////////
	//Run();

	_asm cli

	SetColor(DARK_RED, WHITE);
	ClearScreen();
	SetCursor(0, 13);
	PrintCentered("## RED SCREEN OF DEATH ##");
	SetCursor(81, 25);

	return 0xDEADBABA;
}