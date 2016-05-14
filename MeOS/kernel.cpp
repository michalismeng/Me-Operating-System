/*#include "include/types.h"
#include "include/screen.h"
#include "include/utility.h"
#include "include/descriptor_tables.h"
#include "include/pit.h"
#include "include/kb.h"
#include "include/parallel.h"
#include "include/PCI.h"
#include "include/timer.h"
#include "include/AHCI.h"
#include "CPPEnv.h"
#include "include/MemoryManager.h"
#include "include/OrderedArray.h"
#include "include/HeapManager.h"
#include "include/vector.h"
#include "include/string.h"
#include "include/MemoryManager.h"
#include "include/Memory.h"

uint8 canOutput = 1;					// controls whether Printch can output to the screen
volatile uint8 endOfCommand = 0;		// end of user command

char* buffer = (char*)0x500000;
char* b2 = (char*)0x600000;
uint32 index = 0;

uint8 send_to_parallel = 0;
uint32 modulo = 200;
Memory mem;
//uint8 status = 1;

AHCI_Controller ahci;

void ProccessText();
void __cxa_finalize(void* f);

/*uint32 GetWord(char* src, char* dest, uint32 src_start, uint8* no_char)
{
	while(!isalnum(src[src_start]))
	{
		if(src[src_start] == 0)
			break;
		src_start++;
	}

	uint32 dest_start = 0;

	while(isalnum(src[src_start])) dest[dest_start++] = src[src_start++];

	dest[dest_start] = 0;

	if(dest_start == 0)
		*no_char = 1;
	else
		*no_char = 0;

	return src_start;
}

void lpt_callback(registers_t regs)
{
	printf("\nreceived lpt!!!\n");
}

void kb_callback(registers_t regs)
{
	if((inportb(0x64) & 0x1) == 0)
	{
		printf("ret\n");
		return;
	}

	uint8 c = read_char();

	if(c == 0)  // break code.... release key.... do nothing
		return;

	if(c == 255)
	{
		send_to_parallel = !send_to_parallel;
		return;
	}

	if(c == '\b')
	{
		buffer[--index] = 0;
		return;
	}

	if(c == '\n')
	{
		buffer[index] = 0;
		printf("received %s\n", buffer);
		endOfCommand = true;
		index = 0;

		return;
	}

	buffer[index] = c;
	index++;
}

#define MI		329
#define SOL		391
#define NTO		261
#define RE		293
#define FA		349

void ProccessCommand()
{
	uint8 no_char = 0;
	uint32 end = GetWord(buffer, b2, 0, &no_char);

	if(strEqual(b2, "freq"))
	{
		end = GetWord(buffer, b2, end, &no_char);
		uint16 back = atoui(b2);

		printf("param: %u\n", back);

		modulo = 200 / back;

		if(back == 0 || modulo == 0)
		{
			modulo = 1;
			SetColor(DARK_BLUE, RED);
			printf("!!! division by zero !!!");
			SetColor(DARK_BLUE, WHITE);
			return;
		}

		ClearLine(SCREEN_HEIGHT - 1, SCREEN_HEIGHT - 1);
		//tick = 0;
	}
	else if(strEqual(b2, "echo"))
	{
		end = GetWord(buffer, b2, end, &no_char);

		printf("%s\n", b2);
	}
	else if(strEqual(b2, "clear"))
		ClearScreen();
	else if(strEqual(b2, "color"))
	{
		end = GetWord(buffer, b2, end, &no_char);
		uint8 back = atoui(b2);

		GetWord(buffer, b2, end, &no_char);
		uint8 front = atoui(b2);

		SetColor(back, front);
	}
	else if(strEqual(b2, "out"))
	{
		end = GetWord(buffer, b2, end, &no_char);
		uint16 port = atoui(b2);

		end = GetWord(buffer, b2, end, &no_char);
		uint8 data = atoui(b2);

		outportb(port, data);
	}
	else if(strEqual(b2, "in"))
	{
		end = GetWord(buffer, b2, end, &no_char);
		uint16 port = atoui(b2);

		uint8 data = inportb(port);

		printf("%h\n", data);
	}
	else if(strEqual(b2, "movb"))
	{
		end = GetWord(buffer, b2, end, &no_char);
		uint8* addr = (uint8*)atoui(b2);

		end = GetWord(buffer, b2, end, &no_char);
		uint8 data = atoui(b2);

		*addr = data;
	}
	else if(strEqual(b2, "mov"))
	{
		end = GetWord(buffer, b2, end, &no_char);
		uint16* addr = (uint16*)atoui(b2);

		end = GetWord(buffer, b2, end, &no_char);
		uint16 data = atoui(b2);

		*addr = data;
	}
	else if(strEqual(b2, "read"))
	{
		end = GetWord(buffer, b2, end, &no_char);
		uint16* addr = (uint16*)atoui(b2);

		printf("%h\n", *addr);
	}
	else if(strEqual(b2, "play"))
	{
		end = GetWord(buffer, b2, end, &no_char);
		uint32 freq = (uint32)atoui(b2);
		play_sound(freq);
	}
	else if(strEqual(b2, "playt"))
	{
		end = GetWord(buffer, b2, end, &no_char);
		uint32 freq = (uint32)atoui(b2);

		end = GetWord(buffer, b2, end, &no_char);
		uint32 time = (uint32)atoui(b2);

		beep(freq, time);
	}
	else if(strEqual(b2, "stop"))
		stop_sound();
	else if(strEqual(b2, "serial"))
	{
		end = GetWord(buffer, b2, end, &no_char);
		uint8 data = (uint8)atoui(b2);

		outportb(0x3f8, data);
	}
	else if(strEqual(b2, "xmas"))
	{

		uint32 freq[] = {MI, MI, MI, MI, MI, MI, MI, SOL, NTO, RE, MI, FA, FA, FA, FA, FA, MI, MI, MI, RE, RE, MI, RE, SOL};
		uint32 time[] = {2,  2,  4,  2,  2,  4,  2,  2,   2,   2,  8,  2,  2,  3,  1,  2,  2,  4,  2,  2,  2,  2,  4,  4};
		uint32 mult = 125;

		play_pattern(freq, time, 24 ,mult);
	}
	else if(strEqual(b2, "ahci"))
	{
		ahci.print_caps();
	}
	else if(strEqual(b2, "printheap"))
		mem.kheap.Print();
	else
		Print("Unknown command!\n");

	buffer[0] = 0;
}*/

/*"C" int kmain(/*struct multiboot* mboot_ptr)
{
	init_descriptor_tables();
	asm("sti");

	SetColor(DARK_BLUE, WHITE);
	ClearScreen();

	init_pit_timer(1000, timer_callback);	// one millisecond

	PrintCentered("Welcome to Megis Operating System\n");
	PCIFindAHCI();

	//mem = Memory();
	//mem.manager = MemoryManager(0x1000000);
	//mem.manager.InitializePaging();
	//mem.kheap = Heap(HEAP_START, HEAP_START + HEAP_INITIAL_SIZE, 0x1000000, false, false, &mem.manager);

	//ClearScreen();

	//beep();

	ahci = AHCI_Controller();
	ahci.port_rebase(0);

	//ahci.HardReset();
	//ahci.print_caps();

	WORD* b = (WORD PTR) 0x400000;

	memset(b, 0, 512);

	ahci.SendIdentify(0, b);

	uint32 sectors = *(uint32*)(b + 60);

	printf("sectors: %h => %u MB", sectors, sectors / 2048);

	return 0;

	//printf("\n");

	//uint16* ptr = (uint16*)0x400;
	//printf("\nWe have a serial port at %h!!\n", *ptr);

	//string str = "Hello_world";

	//for(uint32 i = 0; i < str.size(); i++)
	//	printf("%c", str[i]);

	//printf("--------END OF DE ALLOCATION----------");

	//printf("paging enabled\n");

	// pseudo-setup parallel port
	uint16* addr = (uint16*)(0x408);

	if(*addr != 0)
	{
		printf("LPT port found...%h\n", *addr);
		send_to_parallel = 1;
		init_parallel_port(lpt_callback);
	}
	else
	{
		printf("No LPT port found\n");
		send_to_parallel = 0;
	}

	//if(WriteAHCI(&abar->ports[0], 1, 0, 1, buf) == 0) { printf("Error writing ahci\n"); return -1; }  ALWAYS DISABLE ON REAL HARDWARE

	//printf("Megis returns");

	/*init_keyboard(kb_callback);

	outportb(0x3f8+3, 0x80);
	outportb(0x3f8, 0xc);

	outportb(0x3f8 + 1, 0);
	outportb(0x3f8+3, 0x3);
	outportb(0x3f8 + 2, 0xc7);
	outportb(0x3f8 + 4, 0xb);

	//memset(buffer, 0, 0xFFFF);
	//memset(b2, 0, 0xFFFF);

	SetMinWritable(7);

	while(true)
	{
		Print("meOS> ");

		while(endOfCommand == false);

		ProccessCommand();

		endOfCommand = false;		// start new command
	}

	mem.manager.KillPaging();		// just put these to prevent variables from being destroyed as optimisation (arghhh c++)
	mem.kheap.Print();
	return 0xDEADBABA;
}*/


#include "descriptor_tables.h"
#include "pit.h"
#include "timer.h"
#include "boot_info.h"
#include "mmngr_phys.h"
#include "mmngr_virtual.h"

extern "C" uint8 canOutput = 1;

void GetMemoryStats()
{
	printfln("Max blocks: %u", pmmngr_get_block_count());
	printfln("Available blocks: %u", pmmngr_get_free_block_count());
	printfln("Used blocks: %u", pmmngr_get_block_use_count());

	printfln("");
}

int kmain(multiboot_info* boot_info)
{
	uint32 kernel_size_sectors;
	_asm mov dword ptr [kernel_size_sectors], edx

	__asm cli
	init_descriptor_tables();
	__asm sti

	init_pit_timer(1000, timer_callback);

	SetColor(DARK_BLUE, WHITE);
	ClearScreen();

	//canOutput = 0;

	printf("Welcome to ME Operating System\n");

	uint32 memoryKB = 1024 + boot_info->m_memoryLo + boot_info->m_memoryHi * 64;
	printf("Memory detected: %h KB %h MB\n", memoryKB, memoryKB / 1024);

	printf("Kernel size: %u bytes\n", kernel_size_sectors);

	printf("Boot device: %h\n", boot_info->m_bootDevice);

	memory_region* region = (memory_region*)0x500;

	pmmngr_init(memoryKB, 0x100000 + kernel_size_sectors * 512);

	for (int i = 0; i < 15; i++)
	{
		if (region[i].type > 4)
			break;

		if (i > 0 && region[i].startLo == 0)
			break;

		printf("region %i: start: 0x%x%x length (bytes): 0x%x%x type: %i (%s)\n", i,
			region[i].startHi, region[i].startLo,
			region[i].sizeHi, region[i].sizeLo,
			region[i].type, strMemoryTypes[region[i].type - 1]);

		if (region[i].type == 0)	// make available
			pmmngr_init_region(region[i].startLo, region[i].sizeLo);
	}

	pmmngr_deinit_region(0x100000, kernel_size_sectors * 512);

	canOutput = 1;

	GetMemoryStats();

	while (true);

	return 0xDEADBABA;
}