#include "descriptor_tables.h"
#include "pit.h"
#include "timer.h"
#include "boot_info.h"
#include "mmngr_phys.h"
#include "mmngr_virtual.h"

#include "keyboard.h"
#include "SerialDebugger.h"
#include "Simple_fs.h"

#include "OrderedArray.h"

#include "PCI.h"
#include "AHCI.h"

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
	else if (strcmp(cmd, "reset") == 0)
		kybrd_reset_system();
	else if (strcmp(cmd, "read") == 0)
	{
		unsigned char buffer[512];
		printf("Enter the path of the file you want to read: ");
		get_cmd(cmd, 28);

		FILE file = volOpenFile(cmd);

		if ((file.flags & 3) == FS_INVALID)
			printfln("File could not be found.");
		else
		{
			volReadFile(&file, buffer, 512);
			volCloseFile(&file);
		}
	}
	else if (strcmp(cmd, "dis_output") == 0)
		canOutput = false;
	else if (strcmp(cmd, "en_output") == 0)
		canOutput = true;
	else if (strcmp(cmd, "dis_kybrd") == 0)
		kybrd_disable();
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
			break;
	}
}

void print(char* arr)
{
	for (int i = 0; i < 512; i++)
	{
		if (isprint(arr[i]))
			Printch(arr[i]);
	}
}

extern HBA_MEM_t* ab;
char buf[512];

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

	init_pit_timer(1000, timer_callback);
	init_serial();

	printf("Welcome to ME Operating System\n");

	uint32 memoryKB = 1024 + boot_info->m_memoryLo + boot_info->m_memoryHi * 64;
	serial_printf("Memory detected: %h KB %h MB\n", memoryKB, memoryKB / 1024);

	serial_printf("Kernel size: %u bytes\n", kernel_size_bytes);

	serial_printf("Boot device: %h\n", boot_info->m_bootDevice);

	bios_memory_region* region = (bios_memory_region*)0x500;

	HBA_MEM_t* abar = PCIFindAHCI();
	serial_printf("Found abar at: %h\n", abar);

	pmmngr_init(memoryKB, 0x100000 + kernel_size_bytes);

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

	serial_printf("Virtual manager initialize\n");

	/*OrderedArray oa(0xc0000000, 0x4000, Standard_LessThan_Predicate);

	oa.Insert(0);
	oa.Insert((ordered_type)10);
	oa.Insert((ordered_type)2);

	oa.Print();*/

	fsysSimpleInitialize();

	init_ahci(abar, 0x100000 + kernel_size_bytes + 4097);

	init_keyboard();

	ahci_read(0, 0, 0, 1, buf);
	print(buf);
	buf[511] = 0;
	ahci_write(0, 0, 0, 1, buf);

	Run();

	_asm cli

	SetColor(DARK_RED, WHITE);
	ClearScreen();
	SetCursor(0, 13);
	PrintCentered("## RED SCREEN OF DEATH ##");
	SetCursor(81, 25);

	return 0xDEADBABA;
}