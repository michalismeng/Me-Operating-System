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
#include "queue_lf.h"

extern "C" uint8 canOutput = 1;

heap* kernel_heap = 0;
HBA_MEM_t* _abar;

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

semaphore sem;

queue_lf<short int> q;
uint32 fail_insert = 0;
uint32 fail_remove = 0;

void test1()
{
	printfln("executing test 1");

	for (int i = 0; i < 100000; i++)
	{
		if (!queue_lf_insert(&q, (short)i))
			fail_insert++;
	}

	printfln("fail insert: %u", fail_insert);

	thread_sleep(thread_get_current(), 1000);
	printfln("queue:");
	for (int i = 0; i < 10; i++)
		printf("%u ", q.buffer[i]);
	while (true);
}

void test2()
{
	printfln("executing test 2");
	for (int i = 0; i < 100000; i++)
	{
		if (!queue_lf_remove(&q))
			fail_remove++;
	}

	printfln("fail remove: %u", fail_remove);

	while (true);
}

extern "C" void loadFile(uint32 addr)
{
	int fd;
	if (open_file("sdc_mount/FOLDER/NEW_DIR/MIC.TXT", &fd) != VFS_OK)
		PANIC("Cannot open fd");

	if (read_file(fd, 0, 20, addr) != 20)
		PANIC("Could not read");

	for (int i = 0; i < 1000000; i++);	// lengthy procedure
	printfln("End load");
}

void test_print_time()
{
	printfln("Executing %s", __FUNCTION__);
	while (true)
	{
		INT_OFF;
		uint16 x = cursorX, y = cursorY;

		SetPointer(0, SCREEN_HEIGHT - 2);
		printf("test thread t=%u m=%u", get_ticks(), millis());
		SetPointer(x, y);
		INT_ON;
	}
}

void idle()
{
	while (true) _asm pause;
}

TCB* thread_test_time;

void keyboard_fancy_function()
{
	while (true)
	{
		KEYCODE c = getch();

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
			vfs_print_all();
		}
		else if (c == KEYCODE::KEY_P)
		{
			uint32 port = 0x3f8;
			printfln("reading port 0x3f8 %h %h", *((uint32*)port), inportb(port));

			outportb(PORT + 7, 0x30);   
			printfln("scratch pad reg: %h", inportb(PORT + 7));
		}
		else if (c == KEYCODE::KEY_L)
		{
			printfln("printing: ");
			char* buffer = (char*)0x700000;
			for (int i = 0; i < 20; i++)
				printf("%c", buffer[i]);

			printfln(".End");
		}
		else if (c == KEYCODE::KEY_S)
		{
			extern bool _serial_port_found;
			_serial_port_found = true;
			uint32 memoryKB = 256 KB;
			serial_printf("Memory detected: %h KB %h MB\n", memoryKB, memoryKB / 1024);

			while (true)
			{
				char c = serial_read();
				printf("%c", c);
			}
		}
		else if (c == KEYCODE::KEY_H)
		{
			printfln("sleeping thread 3 at %u", millis());
			thread_sleep(thread_test_time, 2000);

			printfln("sleeping me at %u", millis());
			thread_sleep(thread_get_current(), 3000);

			printfln("blocking thread 3 at %u", millis());
			thread_block(thread_test_time);
		}
	}
}

char* ___buffer;
_pipe pipe;
int fd[2];

void test3()
{
	printfln("Test3 id: %u", thread_get_current()->id);
	char* message = "Hello from this pipe!\n";
	printfln("I will print through the pipe");

	for (int i = 0; i < strlen(message); i += 2)
	{
		//pipe_write(&pipe, message[i]);
		write_file(fd[0], 0, 2, (virtual_addr)(message + i));
		thread_sleep(thread_get_current(), 500);
	}

	while (true)
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

void create_test_process(int fd)
{
	for (uint32 i = 0; i < 1; i++)
	{
		uint32 error = read_file(fd, i, PAGE_SIZE, (virtual_addr)___buffer);
		if (error != 0)
			printfln("read error: %u", error);
	}

	if (!validate_PE_image(___buffer))
	{
		DEBUG("Could not load PE image. Corrupt image or data.");
		return;
	}

	IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)___buffer;
	IMAGE_NT_HEADERS* nt_header = (IMAGE_NT_HEADERS*)(dos_header->e_lfanew + (uint32)___buffer);
	IMAGE_SECTION_HEADER* section_0 = (IMAGE_SECTION_HEADER*)((char*)&nt_header->OptionalHeader + nt_header->FileHeader.SizeOfOptionalHeader);

	/* address space creation goes here. Create a new address space */
	pdirectory* address_space = vmmngr_get_directory();

	// here we ve got a PE.
	PCB* proc = process_create(thread_get_current()->parent, address_space, 4 MB, 2 GB);

	uint32 baseText = nt_header->OptionalHeader.BaseOfCode;
	uint32 sizeText = nt_header->OptionalHeader.SizeOfCode;
	uint32 baseData = nt_header->OptionalHeader.BaseOfData;
	uint32 sizeData = nt_header->OptionalHeader.SizeOfInitializedData;
	uint32 entry = nt_header->OptionalHeader.AddressOfEntryPoint + nt_header->OptionalHeader.ImageBase;
	uint32 imageBase = nt_header->OptionalHeader.ImageBase;

	/*vm_area code = vm_area_create(imageBase + baseText, pmmngr_get_next_align(imageBase + baseText + sizeText), VM_AREA_READ | VM_AREA_EXEC, fd);
	vm_area stack = vm_area_create(1 GB, 1 GB + 4 KB, VM_AREA_READ | VM_AREA_WRITE | VM_AREA_GROWS_DOWN, -1);
	vm_area data = vm_area_create(imageBase + baseData, pmmngr_get_next_align(imageBase + baseData + sizeData), VM_AREA_READ | VM_AREA_WRITE, -1);*/

	/*vm_contract_add_area(&proc->memory_contract, &code);
	vm_contract_add_area(&proc->memory_contract, &stack);
	vm_contract_add_area(&proc->memory_contract, &data);*/

	memcpy((void*)(imageBase + section_0->VirtualAddress), ___buffer + section_0->PointerToRawData, section_0->SizeOfRawData);
	section_0++;
	memcpy((void*)(imageBase + section_0->VirtualAddress), ___buffer + section_0->PointerToRawData, section_0->SizeOfRawData);

	uint32 address = vm_contract_get_area_for_length(&proc->memory_contract, 4096);

	//TCB* main = thread_create(proc, entry, stack.end_addr, 4096, 1);

	//thread_insert(main);
	printfln("thread creationg ended");
}

extern "C" int execute = false;

void proc_init_thread()
{
	printfln("executing %s", __FUNCTION__);
	// start setting up heaps, drivers and everything needed.
	vm_area a = vm_area_create(3 GB + 10 MB, 3 GB + 12 MB, VM_AREA_WRITE, -1, 0);  // create a 1MB space
	if (!vm_contract_add_area(&thread_get_current()->parent->memory_contract, &a))
		PANIC("could not add area 3GB");

	a = vm_area_create(4 MB, 3 GB, VM_AREA_WRITE | VM_AREA_READ, -1, 0);  // create user process running space
	if (!vm_contract_add_area(&thread_get_current()->parent->memory_contract, &a))
		PANIC("could not add area 4MB");

	// create a 16KB heap
	kernel_heap = heap_create(3 GB + 11 MB, 16 KB);
	printfln("heap start: %h %h", kernel_heap->start_address, kernel_heap);

	___buffer = (char*)(3 GB + 10 MB + 10 KB);

	ClearScreen();

	INT_OFF;

	init_vfs();

	uint32 ahci_base = 0x300000;
	init_ahci(_abar, ahci_base);

	page_cache_init(2 GB, 20, 16);
	init_global_file_table(16);

	page_cache_print();

	vfs_node* disk = vfs_find_child(vfs_get_dev(), "sdc");
	vfs_node* hierarchy = fat_fs_mount("sdc_mount", disk);

	vfs_node* folder = 0;

	vfs_add_child(vfs_get_root(), hierarchy);

	vfs_print_all();

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

	/*int test_fd = 0;
	page_cache_register_file(test_fd);

	page_cache_reserve_buffer(test_fd, 0);
	page_cache_reserve_buffer(test_fd, 1);
	page_cache_reserve_buffer(test_fd, 2);

	page_cache_print();

	page_cache_register_file(1);

	page_cache_reserve_buffer(1, 10);
	page_cache_reserve_buffer(1, 11);
	page_cache_reserve_buffer(1, 12);

	page_cache_print();

	page_cache_release_buffer(test_fd, 1);

	page_cache_print();

	page_cache_unregister_file(test_fd);

	page_cache_reserve_buffer(1, 13);

	page_cache_print();*/

	//while (true);

	uint32 error;
	vfs_node* n;

	/*if (error = open_file("sdc_mount/FOLDER/TESTDLL.EXE", &fd[0]))
		printfln("Error opening TestDLL.exe: %u", error);

	INT_OFF;

	if (!error)
		create_test_process(fd[0]);*/

		//while (true);

		//mutex_init(&m);
		//spinlock_init(&s);
		//semaphore_init(&sem, 1);

	queue_lf_init(&q, 1000);

	thread_insert(thread_create(thread_get_current()->parent, (uint32)keyboard_fancy_function, 3 GB + 10 MB + 520 KB, 4 KB, 1));
	printfln("new insertion");
	thread_insert(thread_create(thread_get_current()->parent, (uint32)idle, 3 GB + 10 MB + 516 KB, 4 KB, 7));
	thread_insert(thread_create(thread_get_current()->parent, (uint32)test1, 3 GB + 10 MB + 512 KB, 4 KB, 1));
	thread_insert(thread_create(thread_get_current()->parent, (uint32)test2, 3 GB + 10 MB + 508 KB, 4 KB, 1));
	/*TCB* thread = thread_create(thread_get_current()->parent, (uint32)test3, 3 GB + 10 MB + 500 KB, 4 KB, 1);
	thread_insert(thread); */
	TCB* thread = thread_create(thread_get_current()->parent, (uint32)test_print_time, 3 GB + 10 MB + 504 KB, 4 KB, 1);
	thread_insert(thread);
	thread_test_time = thread;

	//create_vfs_pipe(___buffer, 512, fd);

	//ClearScreen();
	execute = 10;
	INT_ON;

	//*(char*)0x700000 = 0;


	printfln("here end");
	while (true)
	{
		INT_OFF;
		uint16 x = cursorX, y = cursorY;

		SetPointer(0, SCREEN_HEIGHT - 3);
		printf("main thread t=%u m=%u", get_ticks(), millis());
		SetPointer(x, y);
		INT_ON;
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

extern "C" void test_handle(registers_t* regs);

int kmain(multiboot_info* boot_info, kernel_info* k_info)
{
	SetColor(DARK_BLUE, WHITE);
	ClearScreen();

	INT_OFF;
	init_descriptor_tables(k_info->gdt_base, k_info->isr_handlers, k_info->idt_base);

	init_pit_timer(50, timer_callback);
	idt_set_gate(32, (uint32)scheduler_interrupt, 0x08, 0x8E);		// bypass the common interrupt handler to play with the stack
	INT_ON;

	printf("Welcome to ME Operating System\n");

	init_serial();

	uint32 memoryKB = 1024 + boot_info->m_memoryLo + boot_info->m_memoryHi * 64;
	pmmngr_init(memoryKB, 0xC0000000 + k_info->kernel_size);

	/*serial_printf("Memory detected: %h KB %h MB\n", memoryKB, memoryKB / 1024);
	serial_printf("Kernel size: %u bytes\n", k_info->kernel_size);
	serial_printf("Boot device: %h\n", boot_info->m_bootDevice);*/

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

	//GetMemoryStats();

	vmmngr_initialize(pmmngr_get_next_align(k_info->kernel_size + 1) / 4096);
	pmmngr_paging_enable(true);

	/*printfln("testing");
	int x = *(uint32*)(0xF0404000);

	printfln("found: %h", x);*/

	_abar = PCIFindAHCI();

	/*serial_printf("Found abar at: %h\n", _abar);

	serial_printf("Virtual manager initialize\n");*/

	fsysSimpleInitialize();
	init_keyboard();

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

	// create a minimal multihtreaded environment to work with

	virtual_addr space = pmmngr_get_next_align(0xC0000000 + k_info->kernel_size + 4096);
	printfln("allocating 4KB at %h", space);

	if (vmmngr_is_page_present(space))
		printfln("space: %h alloced", space);

	if (!vmmngr_alloc_page(space))
		PANIC("cannot allocate for mini heap");

	if(!vmmngr_alloc_page(space + 4096))
		PANIC("cannot allocate for mini heap");


	// setup a dummy kernel heap for process 0 initialization
	kernel_heap = heap_create(space, 0x2000);
	printfln("heap start: %h %h", kernel_heap->start_address, kernel_heap);

	// create process 0 and its only thread
	PCB* proc = process_create(0, vmmngr_get_directory(), 0, 4 GB - 4 KB);
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