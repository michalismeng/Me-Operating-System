#include "descriptor_tables.h"
#include "pit.h"
#include "timer.h"
#include "boot_info.h"
#include "mmngr_phys.h"
#include "mmngr_virtual.h"

#include "keyboard.h"
#include "SerialDebugger.h"

#include "PCI.h"
#include "AHCI.h"

#include "memory.h"

#include "process.h"
#include "mutex.h"
#include "spinlock.h"
#include "semaphore.h"

#include "vfs.h"
#include "FAT32_fs.h"
#include "vm_contract.h"
#include "MassStorageDefinitions.h"

#include "page_cache.h"
#include "thread_sched.h"

#include "pipe.h"
#include "file.h"
#include "Debugger.h"

#include "atomic.h"
#include "queue_spsc.h"
#include "queue_mpmc.h"

#include "dl_list.h"

#include "VBEDefinitions.h"
#include "screen_gfx.h"
#include "print_utility.h"
#include "ethernet.h"
#include "arp.h"
#include "ip.h"
#include "udp.h"
#include "icmp.h"

#include "critlock.h"
#include "net.h"
#include "sock_buf.h"

#include "kernel_stack.h"

#include "test_dev.h"

#include "test/test_Fat32.h"
#include "test/test_open_file_table.h"
#include "test/test_page_cache.h"


extern "C" uint8 canOutput = 1;
extern "C" int _fltused = 1;

heap* kernel_heap = 0;
HBA_MEM_t* _abar;

KEYCODE getch()
{
	KEYCODE key = KEY_UNKNOWN;

	while (key == KEY_UNKNOWN)
		key = kybrd_get_last_key();

	kybrd_discard_last_key();
	return key;
}

int a = 0;
mutex m;
spinlock s;

semaphore sem;
uint32 fail_insert = 0;
uint32 fail_remove = 0;

void test1()
{
	serial_printf("executing test 1\n");

	for (int i = 0; i < 10000000; i++)
	{
		critlock_acquire();

		_asm
		{
			mov eax, a
			inc eax
			pause
			pause
			pause
			pause
			pause
			mov a, eax
		}

		critlock_release();

		//if (!queue_lf_insert(&q, (short)i))
		//	fail_insert++;
	}



	//printfln("fail insert: %u", fail_insert);

	//thread_sleep(thread_get_current(), 1000);
	//printfln("queue:");
	//for (int i = 0; i < 10; i++)
	//	printf("%u ", q.buffer[i]);

	serial_printf("test 1 finished: %u\n", a);
	while (true);
}

void test2()
{
	serial_printf("executing test 2");

	for (int i = 0; i < 10000000; i++)
	{
		critlock_acquire();

		_asm
		{
			mov eax, a
			inc eax
			mov a, eax
		}

		critlock_release();

		//if (!queue_lf_insert(&q, (short)i))
		//	fail_insert++;
	}

	/*char* x = (char*)0x800000;

	for (int i = 0; i < 5; i++)
		x[i] = 'b';

	serial_printf("end of test2\n");*/


	serial_printf("test 2 finished: %u\n", a);

	while (true);
}

void test_print_time()
{
	printfln("Executing %s", __FUNCTION__);
	while (true)
	{
		INT_OFF;
		point cursor = get_cursor();

		//SetPointer(0, SCREEN_HEIGHT - 3);
		set_cursor(0, get_chars_vertical() - 2);
		printf("main thread t=%u m=%u", get_ticks(), millis());
		//SetPointer(x, y);
		set_cursor(cursor.x, cursor.y);
		INT_ON;
		thread_current_yield();
	}
}

void idle()
{
	while (true) _asm pause;
}

TCB* thread_test_time;

TCB* create_test_process(uint32 fd);

extern char* ___buffer;
char __temp[4096] = {'a', 'b', 'c', 'd', 0};

void keyboard_fancy_function()
{
	uint32 fd;
	if (open_file("dev/keyboard", &fd, VFS_CAP_READ) != ERROR_OK)
		serial_printf("error occured: %u", get_last_error());
	else
	{
		while (true)
		{
			char c;
			read_file(fd, 0, 1, (virtual_addr)&c);

			if (c == KEYCODE::KEY_R)
			{
				printf("reseting system");
				sleep(500);
				printf(".");
				sleep(500);
				printf(".");
				sleep(500);
				printf(".");
				sleep(500);
				kybrd_reset_system();
			}
			else if (c == KEYCODE::KEY_V)
			{
				ClearScreen();
				printfln("printing vfs:");
				vfs_print_all();
			}
			else if (c == KEYCODE::KEY_P)
			{
				uint32 text_fd;
				if (open_file("sdc_mount/TEXT.TXT", &text_fd, VFS_CAP_READ | VFS_CAP_WRITE) != ERROR_OK)
					PANIC("could not open text.txt");

				if (vfs_mmap(0x500000, gft_get_by_fd(text_fd), 0, 4096, PROT_READ | PROT_WRITE, MMAP_PRIVATE) == MAP_FAILED)
					serial_printf("map error: %e\n", get_last_error());

				serial_printf("%s\n\n", 0x500000);
			}
			else if (c == KEYCODE::KEY_L)
			{
				clear_screen();
				uint32 test_proc;
				page_cache_print();

				/*for (int i = 0; i < 256; i++)
					printf("%c", i);
				printfln("");*/

				if (open_file("sdc_mount/TEXT.TXT", &test_proc, O_CACHE_ONLY) != VFS_OK)
					PANIC("could not open text.exe");

				if(vfs_mmap(0x800000, lft_get(&process_get_current()->lft, test_proc)->gfd, 0, 4096, PROT_READ | PROT_WRITE, MMAP_SHARED) == MAP_FAILED)
					PANIC("Could not map file");

				
				//printfln("page for text: %h", page_cache_get_buffer(lft_get(&process_get_current()->lft, test_proc)->gfd, 0));


				char* x = (char*)0x800000;

				for (int i = 0; i < 10; i++)
					printf("%c", x[i]);
				printfln("");

				for (int i = 0; i < 5; i++)
					x[i] = 'a';

				/*if (open_file("sdc_mount/TEST.EXE", &test_proc, 0) != VFS_OK)
					PANIC("could not open text.exe");

				TCB* thread = create_test_process(test_proc);


				INT_OFF;
				thread_insert(thread);
				INT_ON;*/

				/*;

				if (read_file(test_proc, 0, 4096, (virtual_addr)__temp) != 4096)
				{
					serial_printf("error %u", get_last_error());
					PANIC("could not read text.txt");
				}*/

				page_cache_print();

				/*for (int i = 0; i < 50; i++)
					printf("%c", __temp[i]);
				printfln(".End");*/

			}
			else if (c == KEYCODE::KEY_S)
			{
				uint32 colors[2] = { 0xFFFFFF, 0xFF0000 };
				uint8 ind = 0;

				while (true)
				{
					read_file(fd, 0, 1, (virtual_addr)&c);
					draw_char(kybrd_key_to_ascii((KEYCODE)c));
					if (c == 'r')
						draw_rectangle(make_point(100, 100), make_point(400, 400), colors[ind++ % 2]);
				}
			}
			else if (c == KEYCODE::KEY_H)
			{
				/*printfln("sleeping thread %u at %u", thread_test_time->id, millis());
				thread_sleep(thread_test_time, 2000);

				printfln("sleeping me %u at %u", thread_get_current()->id, millis());
				thread_sleep(thread_get_current(), 3000);

				printfln("blocking thread %u at %u", thread_test_time->id, millis());
				ClearScreen();

				thread_block(thread_test_time);*/
				scheduler_print_queues();
				printfln("this is a new build!");
			}
			else if (c == KEYCODE::KEY_D)
			{
				uint32 __fd;
				if (open_file("sdc_mount/TESTDLL.DLL", &__fd, 0) != ERROR_OK)
				{
					printfln("open file error: %e", get_last_error());
					return;
				}

				TCB* proc = create_test_process(__fd);
				INT_OFF;
				serial_printf("executing");
				thread_insert(proc);
				INT_ON;
				
			}
			else if (c == KEYCODE::KEY_N)
			{
				clear_screen();
				printfln("networking has been disabled... please enable adapter and remove this message.");
				return;

				serial_printf("sending dummy packet\n");

				extern e1000* nic_dev;

				uint8 pc_mac[6] = { 0x98, 0x90, 0x96, 0xAA, 0x62, 0x7F };
				uint8 dest_mac[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
				uint8 router_mac[6] = { 0x74, 0xa7, 0x8e, 0xeb, 0xe5, 0xca };
				uint8 zero_mac[6] = { 0, 0, 0, 0, 0, 0 };

				extern uint8 my_ip[4];
				uint8 google_ip[4] = { 172, 217, 22, 99 };
				uint8 pc_ip[4] = { 192, 168, 1, 11 };
				uint8 gateway[4] = { 192, 168, 1, 254 };
				uint8 zamanis_ip[4] = { 0, 0, 0, 0 };
				uint8 rafael_ip[4] = { 5, 55, 107, 232 };
				uint8 pipinis_ip[4] = { 94, 66, 17, 174 };

				uint8 hello[] = "Hello world!!";
				uint16 data_length = sizeof(hello);

				SKB sock_main;
				SKB* sock = &sock_main;

				if (sock_buf_init(sock, 200) != ERROR_OK)
					DEBUG("socket creation failed");

				memset(sock->head, 0, 200);

				eth_header* eth = eth_create(sock, pc_mac, nic_dev->mac, 0x800);

				ipv4* ip = ipv4_create(sock, 0, 0, 0, 0, 0, 128, 17, my_ip, pc_ip, 0, 0,
					data_length + sizeof(udp_header));

				udp_header* packet = udp_create(sock, 12345, 12345, data_length);
				sock_buf_put(sock, hello, data_length);

				udp_send(sock);

				if(sock_buf_release(sock))
					DEBUG("socket release failed");
			}
			else if (c == KEYCODE::KEY_B)
			{
				extern uint32 udp_recved;
				printfln("received packets: %u", udp_recved);
			}
			else if (c == KEYCODE::KEY_E)
			{
				serial_printf("error location: %h\n", thread_get_error(thread_get_current()));
				page_cache_print();
			}
		}
	}
}

char* ___buffer;
_pipe pipe;
uint32 fd[2];

void test3()
{
	//_asm int 0x80
	serial_printf("Test3 id: %u", thread_get_current()->id);
	char* message = "Hello from this pipe!\n";
	serial_printf("I will print through the pipe");

	//for (int i = 0; i < strlen(message); i += 2)
	//{
	//	//pipe_write(&pipe, message[i]);
	//	write_file(fd[0], 0, 2, (virtual_addr)(message + i));
	//	thread_sleep(thread_get_current(), 500);
	//}

	for (;;);
	{
		/*vfs_node* n = vfs_find_node("sdc_mount/MIC.TXT");
		vfs_open_file(n);

		vfs_read_file(n, 0, (virtual_addr)(___buffer + 4096));
		printfln("MIC DATA: %s", ___buffer + 4096);*/
	}
}

struct kernel_info
{
	uint32 kernel_size;
	gdt_entry_t* gdt_base;
	isr_t* isr_handlers;
	idt_entry_t* idt_base;
};

_declspec(naked)
void enter_user_mode(uint32 stack, uint32 entry)
{
	uint32 kernel_esp;
	_asm mov [kernel_esp], esp

	//set_tss(0x10, kernel_esp);
	serial_printf("entering user mode %h, kernel stack: %h %h\n", entry, kernel_esp, stack);
	_asm ret

	_asm {
		cli
		mov ax, 0x23				// user mode data selector is 0x20 (GDT entry 3).Also sets RPL to 3
		mov ds, ax
		mov es, ax
		mov fs, ax
		mov gs, ax

		push 0x23					// SS, notice it uses same selector as above
		push dword ptr[stack]
		pushfd						// push flags, move them to eax and OR with ths IF flags to enable interrupts
		pop eax
		or eax, 0x200
		push eax
		push 0x1B					// CS, user mode code selector is 0x18. With RPL 3 this is 0x1b
		push dword ptr[entry]		// EIP first

		iretd
	}
}

// sets up a process and initializes the first thread and its stacks (kernel + user)
void kernel_setup_process(uint32 stack, uint32 entry)
{
	/*if (vfs_mmap(1 GB - 28 KB, 0, 0, 32 KB, PROT_READ | PROT_WRITE, MMAP_PRIVATE | MMAP_ANONYMOUS | MMAP_USER) == MAP_FAILED)
		PANIC("Cannot map stack");

	vmmngr_alloc_page_f(1 GB - 8 KB, DEFAULT_FLAGS | I86_PTE_USER);
	vmmngr_alloc_page_f(1 GB - 12 KB, DEFAULT_FLAGS | I86_PTE_USER);*/

	// fix the stack	NOT NEEDED AS THE RETURN VALUE IS PLACED IN THE STACK
	//_asm sub ebp, 4				// compiler offsets arguments by 8 (due to ebp push and return value on the stack?). we have not the return value so fake it

	serial_printf("executing process: %u\n\n", process_get_current()->id);

	serial_printf("executed testdll with stack: %u\n", stack);

	typedef void(*fptr)();
	((fptr)entry)();

	//_asm call enter_user_mode

	serial_printf("end of execution\n");

	//enter_user_mode(1 GB - 8 KB, (uint32)entry);
	for (;;);
}

virtual_addr pe_get_export_function(IMAGE_EXPORT_DIRECTORY* export_directory, uint32 image_base, char* name)
{
	char** names = (char**)((uint32)export_directory->AddressOfNames + image_base);
	uint16* ordinals = (uint16*)((uint32)export_directory->AddressOfNameOrdinal + image_base);
	virtual_addr* addrs = (virtual_addr*)((uint32)export_directory->AddressOfFunctions + image_base);

	for (int i = 0; i < export_directory->NumberOfFunctions; i++)
	{
		char* func_name = names[i] + image_base;

		if (strcmp(func_name, name) == 0)
		{
			uint16 ordinal = ordinals[i];
			virtual_addr address = addrs[ordinal] + image_base;
			return address;
		}
	}

	return 0;
}

TCB* create_test_process(uint32 fd)
{
	PCB* proc = process_create(process_get_current(), 0, 3 MB, 0xFFFFE000);

	for (uint32 i = 0; i < 1; i++)
	{
		size_t read = read_file(fd, i, PAGE_SIZE, (virtual_addr)___buffer);
		if (read != PAGE_SIZE)
			printfln("read error: %e", get_last_error());
	}

	if (!validate_PE_image((void*)___buffer))
	{
		DEBUG("Could not load PE image. Corrupt image or data.");
		return 0;
	}

	IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)___buffer;
	IMAGE_NT_HEADERS* nt_header = (IMAGE_NT_HEADERS*)(dos_header->e_lfanew + (uint32)___buffer);
	IMAGE_SECTION_HEADER* section = (IMAGE_SECTION_HEADER*)((char*)&nt_header->OptionalHeader + nt_header->FileHeader.SizeOfOptionalHeader);

	IMAGE_DATA_DIRECTORY* DataDirectory = &nt_header->OptionalHeader.DataDirectory[0];
	IMAGE_EXPORT_DIRECTORY* exportDirectory = (IMAGE_EXPORT_DIRECTORY*)(DataDirectory->VirtualAddress + nt_header->OptionalHeader.ImageBase);

	serial_printf("section name\n");
	for (int x = 0; x < nt_header->FileHeader.NumberOfSections; x++)
	{
		for (int i = 0; i < 8; i++)
			serial_printf("%c", section[x].Name[i]);
		serial_printf("\n");

		serial_printf("mmaping fd: %u to %h, size of raw: %h, raw offset: %h\n", lft_get(&process_get_current()->lft, fd)->gfd,
			section[x].VirtualAddress + nt_header->OptionalHeader.ImageBase, section[x].SizeOfRawData, section[x].PointerToRawData);

		// TODO: Remember vfs_map_p prot and flags were reversed in the definition and this function is not cheched if working.
		if (vfs_mmap_p(proc, section[x].VirtualAddress + nt_header->OptionalHeader.ImageBase, 
			lft_get(&process_get_current()->lft, fd)->gfd, section[x].PointerToRawData, 4096,
			PROT_NONE | PROT_READ | PROT_WRITE, MMAP_PRIVATE | MMAP_USER) == MAP_FAILED)
		{
			serial_printf("address: %h size %h", section[x].VirtualAddress + nt_header->OptionalHeader.ImageBase, section[x].SizeOfRawData);
			PANIC("Could not map for process");
		}
	}

	/*typedef int(*add)(int, int);
	add func = (add)pe_get_export_function(exportDirectory, nt_header->OptionalHeader.ImageBase, "?add@@YAHHH@Z");

	serial_printf("add(10, 20) = %u\n\n", func(10, 20));*/

	uint32 entry = nt_header->OptionalHeader.AddressOfEntryPoint + nt_header->OptionalHeader.ImageBase;
	virtual_addr krnl_stack = kernel_stack_reserve();
	TCB* thread = 0;

	if (krnl_stack == 0)
		serial_printf("process creation failed: cannot allocate stack: %e\n", krnl_stack);
	else
		thread = thread_create(proc, (uint32)kernel_setup_process, krnl_stack, 4 KB, 3, 2, entry, 4096);

	return thread;
}

multiboot_info* boot_info;

void proc_init_thread()
{
	INT_OFF;
	printfln("executing %s", __FUNCTION__);

	// start setting up heaps, drivers and everything needed.
	if (vfs_mmap(2 GB, INVALID_FD, 0, 1 GB + 12 MB, PROT_NONE | PROT_READ | PROT_WRITE, MMAP_PRIVATE | MMAP_ANONYMOUS) == MAP_FAILED)
		PANIC("Could not map kernel land");

	// memory map MMIO
	if (vfs_mmap(0xF0000000, INVALID_FD, 0, 0x0FFFE000, PROT_NONE | PROT_READ | PROT_WRITE, MMAP_PRIVATE | MMAP_ANONYMOUS | MMAP_IDENTITY_MAP) == MAP_FAILED)
		PANIC("Could not map MMIO");

	// write protect supervisor => cr0 bit 16 must be set to trigger page fault when kernel writes to read only page
	enable_write_protection();

	// create a 16KB heap
	kernel_heap = heap_create(3 GB + 11 MB, 16 KB);
	printfln("heap start: %h %h", kernel_heap->start_address, kernel_heap);

	// re-init this processe's local file table against the new heap
	init_local_file_table(&process_get_current()->lft, 10);

	___buffer = (char*)(3 GB + 10 MB + 10 KB);

	// TODO: standardize this!
	vbe_mode_info_block* vbe = (vbe_mode_info_block*)(0x2000);
	
	init_vfs();

	page_cache_init(2 GB, 20);
	init_global_file_table(16);

	_abar = PCIFindAHCI();

	uint32 ahci_base = 0x200000;
	init_ahci(_abar, ahci_base);

	

	/*init_net();
	init_arp(NETWORK_LAYER);*/
	//init_ipv4(NETWORK_LAYER);
	//init_icmp(TRANSPORT_LAYER);
	//init_udp(TRANSPORT_LAYER);

	//vfs_node* disk = vfs_find_child(vfs_get_dev(), "sdc");
	//if (disk)
	//{
	//	vfs_node* hierarchy = fat_fs_mount("sdc_mount", disk);
	//	vfs_node* folder = 0;
	//	vfs_add_child(vfs_get_root(), hierarchy);
	//}

	//vfs_print_all();

	//page_cache_print();
	//debugf(strupper("print tree..."));

	//ClearScreen();
	//print_vfs(hierarchy, 0);


	//if (vfs_lookup(hierarchy, "FOLDER", &folder) != 0)
	//	printfln("ERROR");
	//else
	//{
	//	if (vfs_open_file(folder) != VFS_OK)
	//		DEBUG("error");

	//	auto layout = &((fat_node_data*)folder->deep_md)->layout;
	//	printf("folder clusters ");
	//	for (int i = 0; i < layout->count; i++)
	//		printf("%u ", vector_at(layout, i));
	//}
	////while (true);
	//ClearScreen();

	//print_vfs(hierarchy, 0);

	//while (true);

	/*vfs_node* file;
	printfln("reading clusters for FOLDER/MIC.TXT");

	if (vfs_lookup(hierarchy, "FOLDER/NEW_DIR/MIC.TXT", &file) != 0)
		printfln("ERROR");
	else
	{
		uint32 err;
		int f_desc;*/
		//if (err = open_file("sdc_mount/FOLDER/MIC.TXT", &f_desc))
		//	printfln("Error opening mic.TXT: %u", err);

		/*printfln("creating new node");
		if ((file = fat_fs_create_node(hierarchy, folder, "some.txt", VFS_READ | VFS_WRITE)) == 0)
			printfln("error creating file %u", get_last_error());
		else
			printfln("some.txt file created");*/

		/*printfln("deleting node some.txt");
		if (vfs_lookup(hierarchy, "FOLDER/some.txt", &file) != 0)
			printfln("some.txt could not be found");

		if (fat_fs_delete_node(hierarchy, file) != VFS_OK)
			printfln("could not delete some.txt");

		debugf("press to show cache...");
		ClearScreen();
		page_cache_print();

		while (true);*/


		//vfs_node* mic, *dir;
		//if (vfs_lookup(hierarchy, "FOLDER/NEW_DIR/MIC.TXT", &mic) != 0 || vfs_lookup(hierarchy, "FOLDER", &dir) != 0)
		//	printfln("Error looking up names: %h %h", mic, dir);

		//printfln("moving mic.txt from new_dir to folder");
		//*if (fat_fs_move_node(hierarchy, mic, dir) != VFS_OK)
		//{
		//	printfln("error when moving file: %u", get_last_error());
		//}*/

		//debugf("press to print cache...");

		//page_cache_print();

		//while (true);

		//while (true);

		//if (err = open_file("sdc_mount/FONT.RAW", &f_desc))
		//	printfln("Error opening mic.TXT: %u", err);

		//if (err = read_file(f_desc, 0, 128, (virtual_addr)___buffer) != 128)
		//	printfln("Could not read file: %u", err);
		//else
		//{
		//	for (uint32 i = 0; i < 128; i++)
		//		if (((char*)___buffer)[i] != 0)
		//			printfln("this is not zero");
		//	//printfln("read:   %s", ___buffer);
		//}

		//while (true);

		/*fat_fs_load_file_layout((fat_mount_data*)hierarchy->deep_md, (mass_storage_info*)hierarchy->tag->deep_md, file);
		auto head = (fat_file_layout*)file->deep_md;
		printf("clusters: ");
		for (int i = 0; i < head->count; i++)
			printf("%u ", vector_at(head, i));*/
	//}

	/*printfln("folder first cluster: %u", ((fat_node_data*)folder->deep_md)->metadata_cluster);

	printf("mnt: ");
	for (int i = 0; i < ((fat_mount_data*)hierarchy->deep_md)->layout.count; i++)
		printf("%u ", vector_at(&((fat_mount_data*)hierarchy->deep_md)->layout, i));
	page_cache_print();*/

	//while (true);

	/*vfs_node* f;
	if(vfs_root_lookup("sdc_mount/FONT.RAW", &f) != VFS_OK)
		printfln("read error for FONT.RAW");

	printfln("mnt point for hierarchy: %s", vfs_get_mount_point(hierarchy)->name);
	printfln("mnt point for dev: %h", vfs_get_mount_point(vfs_get_dev()));
	printfln("mnt point for random node: %s", vfs_get_mount_point(f)->name);

	debugf("Press any key to see FAT structure...");
	ClearScreen();

	print_vfs(hierarchy, 0);*/

	/*printfln("deleting dire");
	if (vfs_lookup(hierarchy, "FOLDER/TEST.TXT", &file) == 0)
	{
		if (fat_fs_delete_file(hierarchy, file) != 0)
			printfln("ERROR");
	}
	else
		printfln("Could not locate DIRE");*/

	/*printfln("creating file NEW.TXT");
	if ((file = fat_fs_create_file(hierarchy, folder, "NEW.TXT", VFS_READ | VFS_WRITE)) == 0)
		printfln("error creating file");
	else
		printfln("NEW.txt file created");*/

	//list_insert_back(&folder->children, file);

	/*while (true);*/

	// initialize the page cache
	//page_cache_print();
	//debugf("");
	INT_OFF;

	uint32 error;
	vfs_node* n;


	init_keyboard();	

	//uint32 addr = 0x700000;
	//int fd;
	//if (open_file("sdc_mount/FOLDER/NEW_DIR/MIC.TXT", &fd) != VFS_OK)
	//	PANIC("Cannot open fd");

	//if (read_file(fd, 0, 20, addr) != 20)
	//	PANIC("Could not read");

	//for (int i = 0; i < 20; i++)serial_printf("%c", ((char*)addr)[i]);	// lengthy procedure
	//printfln("End load");

	//PANIC("");
#ifdef TEST_ENV
	if (test_open_file_table_open() == false)
	{
		serial_printf("test failed...\n");
		PANIC("");
	}

	if (test_vfs_open_file() == false)
	{
		serial_printf("vfs open file test failed...\n");
		PANIC("");
	}

	if (test_open_file() == false)
	{
		serial_printf("open file test failed...\n");
		PANIC("");
	}

	if (test_page_cache_reserve_anonymous() == false)
	{
		serial_printf("page cache reserve anonymous test failed...\n");
		PANIC("");
	}

	if (test_page_cache_reserve_and_release() == false)
	{
		serial_printf("page cache reserve test failed");
		PANIC("");
	}

	if (test_page_cache_find_buffer() == false)
	{
		serial_printf("page cache find buffer test failed");
		PANIC("");
	}

	init_test_dev();

	// do not run the two tests below simulatneously as they require pages not be cached
	/*if (test_read_file_cached() == false)
	{
		serial_printf("read cached file failed");
		PANIC("");
	}*/

	if (test_write_file_cached() == false)
	{
		serial_printf("write cached file failed");
		PANIC("");
	}

	if (test_sync_file() == false)
	{
		serial_printf("sync cached file failed");
		PANIC("");
	}

#endif

	//vfs_node* dev;
	//if (vfs_root_lookup("dev/sdc", &dev) != ERROR_OK)
	//	PANIC("Error opening");

	//virtual_addr cache = page_cache_reserve_anonymous();

	//*(uint32*)(cache) = 0;

	//// attention: if the address is not allocated (but exists only in the contract) then the driver fails (as it uses vmmngr_get_phys_addr)
	//if (vfs_read_file(0, dev, 0, 1, cache) != 1)
	//{
	//	serial_printf("Cannot read: %e\n", get_last_error());
	//	PANIC("");
	//}


	//fat_mbr* mbr = (fat_mbr*)cache;

	//serial_printf("signature: %h\n", mbr->boot_sign);
	//serial_printf("1st partition offset, size: %h, %h\n", mbr->primary_partition.lba_offset, mbr->primary_partition.size);
	//serial_printf("2nd partition offset: %h\n", mbr->secondary_partition.lba_offset);

	serial_printf("initializeing screen width mode: %h...\n", boot_info->m_vbe_mode_info);
	init_screen_gfx(vbe);

	/*set_foreground_color(0x00FFFFFF);
	set_background_color(0x000000FF);*/

	set_foreground_color(0x000FF00);
	set_background_color(0);

	init_print_utility();

	serial_printf("screen ready...\n");

	/*virtual_addr krnl_stack = kernel_stack_reserve();
	if (krnl_stack == 0)
	{
		serial_printf("kernel stack allocation failed: %e", get_last_error());
		PANIC("");
	}
	serial_printf("kernel stack top for kybd_fancy: %h", krnl_stack);

	TCB* c;
	thread_insert(c = thread_create(thread_get_current()->parent, (uint32)keyboard_fancy_function, krnl_stack, 4 KB, 3, 0));*/

	virtual_addr krnl_stack = kernel_stack_reserve();
	if (krnl_stack == 0)
	{
		serial_printf("kernel stack allocation failed: %e", get_last_error());
		PANIC("");
	}
	serial_printf("kernel stack top for idle: %h\n", krnl_stack);

	thread_insert(thread_create(thread_get_current()->parent, (uint32)idle, krnl_stack, 4 KB, 7, 0));
	/*thread_insert(thread_create(thread_get_current()->parent, (uint32)test1, 3 GB + 10 MB + 512 KB, 4 KB, 3, 0));
	thread_insert(thread_create(thread_get_current()->parent, (uint32)test2, 3 GB + 10 MB + 508 KB, 4 KB, 3, 0));*/
	/*TCB* thread = thread_create(thread_get_current()->parent, (uint32)test3, 3 GB + 10 MB + 500 KB, 4 KB, 1);
	thread_insert(thread); */
	//TCB* thread = thread_create(thread_get_current()->parent, (uint32)test_print_time, 3 GB + 10 MB + 504 KB, 4 KB, 3);
	//thread_insert(thread);
	thread_test_time = 0;//thread;
	//create_vfs_pipe(___buffer, 512, fd);

	// create new test process to run keyboard fancy function.
	//PCB* p = process_create(process_get_current(), 0, 0, 0xFFFFFFFF);
	krnl_stack = kernel_stack_reserve();
	if (krnl_stack == 0)
	{
		serial_printf("kernel stack allocation failed: %e", get_last_error());
		PANIC("");
	}
	thread_insert(thread_create(process_get_current(), (uint32)keyboard_fancy_function, krnl_stack, 4 KB, 3, 0));

	//serial_printf("int on\n");
	INT_ON;

	vfs_node* dev;
	if (vfs_root_lookup("dev/sdc", &dev) != ERROR_OK)
		PANIC("Error opening");

	virtual_addr cache = page_cache_reserve_anonymous();

	*(uint32*)(cache) = 0;

	// mount FAT

	if (dev)
	{
		vfs_node* hierarchy = fat_fs_mount("sdc_mount", dev);
		vfs_add_child(vfs_get_root(), hierarchy);
	}

	uint32 fd;
	if (open_file("sdc_mount/TEXT.TXT", &fd, VFS_CAP_READ | VFS_CAP_WRITE) == ERROR_OCCUR)
	{
		serial_printf("error: %e\n", get_last_error());
		PANIC("Could not open text");
	}

	if (read_file(fd, 0, 4096, cache) != 4096)
	{
		serial_printf("error occur: %e\n", get_last_error());
		PANIC("Could not read");
	}

	char* buf = (char*)cache;
	for (int i = 0; i < 40; i++)
		if (isprint(buf[i]))
			serial_printf("%c", buf[i]);	

	if (write_file(fd, 0, 4096, cache) != 4096)
	{
		serial_printf("error occur: %e\n", get_last_error());
		PANIC("Could not write");
	}

	serial_printf("\n\n");
	page_cache_print();
	serial_printf("\n\n");

	serial_printf("end of read\n");

	load_default_font();

	clear_screen();
	draw_rectangle({ 400 - 100, 300 - 100 }, { 100, 100 }, 0xFFFFFFFF);
	printfln("Welcome to Me Operating System");

	while (true)
	{
		INT_OFF;
		point cursor = get_cursor();

		set_cursor(0, get_chars_vertical() - 3);
		printf("main thread t=%u m=%u", get_ticks(), millis());
		set_cursor(cursor.x, cursor.y);
		INT_ON;
		thread_current_yield();
	}

	/*int fd;

	for (int i = 0; i < 10000; i++)
		___buffer[i] = 0;

	if (error = open_file("sdc_mount/BEST.TXT", &fd))
		printfln("open error code %u", error);

	if (read_file(fd, 25000, 10, (virtual_addr)___buffer) != 10)
		printfln("read error code %u", get_last_error());
	else
		printfln("buffer: %s", ___buffer);

	for (int i = 0; i < 5; i++)
		___buffer[i] = 'c';*/

	/*if (write_file(fd, 5, 5, (virtual_addr)___buffer) != 5)
		printfln("write error code %u", get_last_error());*/

	/*if (error = sync_file(fd, -1, 0))
		printfln("sync error: %u", error);*/

	/*if (error = hierarchy->fs_ops->fs_sync(hierarchy->data, hierarchy, -1, 0))
		printfln("sync error: %u", error);*/

	/*vfs_node* node = gft_get(fd)->file_node;

	printfln("node: %u %h", node->data >> 12, node->data & 0xFFF);

	node = vfs_find_node("sdc_mount/FOLDER");

	printfln("node: %u %h", node->data >> 12, node->data & 0xFFF);*/
	/*page_cache_print();
	debugf("");*/

	/*vfs_node* node = gft_get(fd)->file_node;
	node->name = "NEW.TXT";
	node->name_length = strlen("NEW.TXT");*/

	//node->file_length = 50;

	/*node->fs_ops->fs_ioctl(node, 0);

	if (error = hierarchy->fs_ops->fs_sync(hierarchy->data, hierarchy, -1, 0))
		printfln("sync error: %u", error);*/

		/*if (read_file(fd, 5000, 20, (virtual_addr)___buffer) != 20)
			printfln("read error code %u", get_last_error());
		else
			printfln("MIC DATA: %s.", ___buffer);*/

			/*if (write_file(fd, 10, 10, (virtual_addr)___buffer) != 10)
				printfln("write error code %u", get_last_error());

			if (error = sync_file(fd, -1, 0))
				printfln("sync error: %u", error);

			if (error = hierarchy->fs_ops->fs_sync(hierarchy->data, hierarchy, -1, 0))
				printfln("sync error: %u", error);*/

	/*page_cache_print();

	gft_print();

	printfln("end");

	while (true);*/

	ClearScreen();
	INT_ON;

	//while (true);

	/*while (true)
	{
		char c;
		uint32 error;
		if (error = read_file(fd, 0, 1, (virtual_addr)&c))
			printfln("error: %u", error);
		else
			printf("%c", c);
	}*/
}

int kmain(multiboot_info* _boot_info, kernel_info* k_info)
{
	boot_info = _boot_info;

	INT_OFF;
	init_descriptor_tables(k_info->gdt_base, k_info->isr_handlers, k_info->idt_base);

	init_pit_timer(50, timer_callback);
	idt_set_gate(32, (uint32)scheduler_interrupt, 0x08, 0x8E);		// bypass the common interrupt handler to play with the stack
	INT_ON;

	init_serial();

	uint32 memoryKB = 1024 + boot_info->m_memoryLo + boot_info->m_memoryHi * 64;
	pmmngr_init(memoryKB, 0xC0000000 + k_info->kernel_size);

	bios_memory_region* region = (bios_memory_region*)boot_info->m_mmap_addr;

	for (uint32 i = 0; i < boot_info->m_mmap_length; i++)
	{
		if (region[i].type > 4)
			break;

		if (i > 0 && region[i].startLo == 0)
			break;

		/*printf("region %i: start: 0x%x %x length (bytes): 0x%x %x type: %i (%s)\n", i,
			region[i].startHi, region[i].startLo,
			region[i].sizeHi, region[i].sizeLo,
			region[i].type, strMemoryTypes[region[i].type - 1]);*/

		if (region[i].type == 1 && region[i].startLo >= 0x100000)	// make available only if region is above 1MB
			pmmngr_free_region(&physical_memory_region(region[i].startLo, region[i].sizeLo));
	}

	pmmngr_reserve_region(&physical_memory_region(0x100000, pmmngr_get_next_align(k_info->kernel_size + 1)));

	vmmngr_initialize(pmmngr_get_next_align(k_info->kernel_size + 1) / 4096);
	pmmngr_paging_enable(true);

	// create a minimal multihtreaded environment to work with

	virtual_addr space = pmmngr_get_next_align(0xC0000000 + k_info->kernel_size + 4096);

	if (vmmngr_is_page_present(space))
		printfln("space: %h alloced", space);

	if (vmmngr_alloc_page(space) != ERROR_OK)
		PANIC("cannot allocate for mini heap");

	if(vmmngr_alloc_page(space + 4096) != ERROR_OK)
		PANIC("cannot allocate for mini heap");

	// setup a dummy kernel heap for process 0 initialization
	kernel_heap = heap_create(space, 0x2000);
	printfln("heap start: %h %h", kernel_heap->start_address, kernel_heap);

	// create process 0 and its only thread
	PCB* proc = process_create(0, vmmngr_get_directory(), 0, 0xFFFFF000/*4 GB - 4 KB*/);	// 4 GB is 0 in DWORD
	uint32 thread_stack = (uint32)malloc(4050);		// allocate enough space for page aligned stack
	// just for this thread, space is not malloc
	TCB* t = thread_create(proc, (uint32)proc_init_thread, pmmngr_get_next_align(thread_stack + 4096), 4096, 3, 0);	// page align stack
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