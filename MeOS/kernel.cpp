#include "descriptor_tables.h"
#include "pit.h"
#include "timer.h"
#include "boot_info.h"
#include "mmngr_phys.h"
#include "mmngr_virtual.h"

#include "keyboard.h"

#include "OrderedArray.h"

extern "C" uint8 canOutput = 1;

void GetMemoryStats()
{
	printfln("Max blocks: %u", pmmngr_get_block_count());
	printfln("Available blocks: %u", pmmngr_get_free_block_count());
	printfln("Used blocks: %u", pmmngr_get_block_use_count());

	printfln("");
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

	printf("cmd>");
	SetMinWritable(strlen("cmd>"));

	while (true)
	{
		key = getch();

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
	else
		printfln("Unknown command: %s", cmd);
}

void Run()
{
	char cmd[30];

	while (true)
	{
		get_cmd(cmd, 28);

		if (run_cmd(cmd) == true)
			break;
	}
}

int kmain(multiboot_info* boot_info, uint32 memory_map_len)
{
	uint32 kernel_size_bytes;
	uint32 memory_map_length = memory_map_len;

	_asm mov dword ptr[kernel_size_bytes], edx

	__asm cli
	init_descriptor_tables();
	__asm sti

	init_pit_timer(1000, timer_callback);

	SetColor(DARK_BLUE, WHITE);
	ClearScreen();

	printf("Welcome to ME Operating System\n");

	uint32 memoryKB = 1024 + boot_info->m_memoryLo + boot_info->m_memoryHi * 64;
	printf("Memory detected: %h KB %h MB\n", memoryKB, memoryKB / 1024);

	printf("Kernel size: %u bytes\n", kernel_size_bytes);

	printf("Boot device: %h\n", boot_info->m_bootDevice);

	bios_memory_region* region = (bios_memory_region*)0x500;

	pmmngr_init(memoryKB, 0x100000 + kernel_size_bytes);

	for (uint32 i = 0; i < memory_map_length; i++)
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

	pmmngr_reserve_region(&physical_memory_region(0x100000, kernel_size_bytes + 4096));

	GetMemoryStats();

	vmmngr_initialize();
	pmmngr_paging_enable(true);

	printfln("Virtual manager initialized");

	/*OrderedArray oa(0xc0000000, 0x4000, Standard_LessThan_Predicate);

	oa.Insert(0);
	oa.Insert((ordered_type)10);
	oa.Insert((ordered_type)2);

	oa.Print();*/

	init_keyboard();

	Run();

	_asm cli

	SetColor(DARK_RED, WHITE);
	ClearScreen();
	SetCursor(0, 13);
	PrintCentered("## RED SCREEN OF DEATH ##");
	SetCursor(81, 25);

	return 0xDEADBABA;
}