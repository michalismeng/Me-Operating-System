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

#include "dl_list.h"

#include "VBEDefinitions.h"
#include "screen_gfx.h"
#include "print_utility.h"

extern "C" uint8 canOutput = 1;

heap* kernel_heap = 0;
HBA_MEM_t* _abar;

void GetMemoryStats()
{
	serial_printf("Max blocks: %u\n", pmmngr_get_block_count());
	serial_printf("Available blocks: %u\n", pmmngr_get_free_block_count());
	serial_printf("Used blocks: %u\n", pmmngr_get_block_use_count());
}

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
	printfln("executing test 1");

	for (int i = 0; i < 100000; i++)
	{
		//if (!queue_lf_insert(&q, (short)i))
		//	fail_insert++;
	}

	printfln("fail insert: %u", fail_insert);

	//thread_sleep(thread_get_current(), 1000);
	printfln("queue:");
	//for (int i = 0; i < 10; i++)
	//	printf("%u ", q.buffer[i]);
	while (true);
}

void test2()
{
	printfln("executing test 2");
	for (int i = 0; i < 100000; i++)
	{
		//if (!queue_lf_remove(&q))
		//	fail_remove++;
	}

	printfln("fail remove: %u", fail_remove);

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

TCB* create_test_process(int fd);

void keyboard_fancy_function()
{
	int fd;
	if (open_file("dev/keyboard", &fd) != VFS_OK)
	{
		printfln("error occured: %u", get_last_error());
	}
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
				vfs_print_all();
			}
			else if (c == KEYCODE::KEY_P)
			{
				int ___fd;
				if (open_file("sdc_mount/TEXT.TXT", &___fd) != VFS_OK)
					PANIC("Could not open text file");

				if (mmap(0x700000, lft_get(&process_get_current()->lft, ___fd)->gfd, 2, 4096, MMAP_PRIVATE, PROT_READ | PROT_WRITE) == MAP_FAILED)
					PANIC("Could not map file");

				char* character = (char*)0x700000;

				printfln("printing text: ");
				for (int i = 0; i < 20; i++)
					printf("%c", character[i]);

				printfln(".End");
			}
			else if (c == KEYCODE::KEY_L)
			{
				clear_screen();
				int test_proc;
				if (open_file("sdc_mount/TEST.EXE", &test_proc) != VFS_OK)
					PANIC("could not open test.exe");
				INT_OFF;
				TCB* t = create_test_process(test_proc);
				thread_insert(t);
				INT_ON;

			}
			else if (c == KEYCODE::KEY_S)
			{
				uint32 colors[2] = { 0xFFFFFF, 0xFF0000 };
				uint8 ind = 0;
				while (true)
				{
					read_file(fd, 0, 1, (virtual_addr)&c);
					draw_char(c);
					if (c == 'r')
						draw_rectangle(make_point(100, 100), make_point(400, 400), colors[ind++ % 2]);
				}
			}
			else if (c == KEYCODE::KEY_H)
			{
				/*printfln("sleeping thread %u at %u", thread_test_time->id, millis());
				thread_sleep(thread_test_time, 2000);*/

				/*printfln("sleeping me %u at %u", thread_get_current()->id, millis());
				thread_sleep(thread_get_current(), 3000);*/

				printfln("blocking thread %u at %u", thread_test_time->id, millis());
				ClearScreen();

				thread_block(thread_test_time);
				scheduler_print_queues();
				printfln("hello");
			}
		}
	}
}

char* ___buffer;
_pipe pipe;
int fd[2];

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

uint32 entry;
void enter_user_mode(uint32 stack, uint32 entry)
{
	uint32 kernel_esp;
	_asm mov [kernel_esp], esp

	set_tss(0x10, kernel_esp);
	serial_printf("entering user mode %h, kernel stack: %h %h\n", entry, kernel_esp, stack);

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
void kernel_setup_process()
{
	if (mmap(1 GB - 28 KB, 0, 0, 32 KB, MMAP_PRIVATE | MMAP_ANONYMOUS | MMAP_USER, PROT_READ | PROT_WRITE) == MAP_FAILED)
		PANIC("Cannot map stack");

	serial_printf("spurious x = ");
	uint32 x = *(uint32*)(0x401000);
	x = *(uint32*)(0x401000);
	x = *(uint32*)(0x402000);
	x = *(uint32*)(0x403000);
	x = *(uint32*)(1 GB - 8 KB);
	x = *(uint32*)(1 GB - 12 KB);
	x = *(uint32*)(1 GB - 4 KB); 
	vmmngr_alloc_page_f(1 GB - 8 KB, DEFAULT_FLAGS | I86_PTE_USER);
	vmmngr_alloc_page_f(1 GB - 12 KB, DEFAULT_FLAGS | I86_PTE_USER);

	pd_entry* e = vmmngr_pdirectory_lookup_entry(vmmngr_get_directory(), 0xE0000000);
	ptable* table = (ptable*)pd_entry_get_frame(*e);
	pt_entry* page = vmmngr_ptable_lookup_entry(table, 0xE0000000);	
	serial_printf("0xE00000 is user? :%u", (*page) & I86_PTE_USER);

	enter_user_mode(1 GB - 8 KB, (uint32)entry);


	for (;;);
}

TCB* create_test_process(int fd)
{
	PCB* proc = process_create(process_get_current(), 0, 3 MB, 0xFFFFE000);

	for (uint32 i = 0; i < 1; i++)
	{
		uint32 error = read_file(fd, i, PAGE_SIZE, (virtual_addr)___buffer);
		if (error != PAGE_SIZE)
			printfln("read error: %u", error);
	}

	if (!validate_PE_image(___buffer))
	{
		DEBUG("Could not load PE image. Corrupt image or data.");
		return 0;
	}

	IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)___buffer;
	IMAGE_NT_HEADERS* nt_header = (IMAGE_NT_HEADERS*)(dos_header->e_lfanew + (uint32)___buffer);
	IMAGE_SECTION_HEADER* section = (IMAGE_SECTION_HEADER*)((char*)&nt_header->OptionalHeader + nt_header->FileHeader.SizeOfOptionalHeader);

	serial_printf("section name\n");
	for (int x = 0; x < nt_header->FileHeader.NumberOfSections; x++)
	{
		for (int i = 0; i < 8; i++)
			serial_printf("%c", section[x].Name[i]);
		serial_printf("\n");

		serial_printf("mmaping fd: %u to %h, size of raw: %h\n", lft_get(&process_get_current()->lft, fd)->gfd,
			section[x].VirtualAddress + nt_header->OptionalHeader.ImageBase, section[x].SizeOfRawData);

		if (mmap_p(proc, section[x].VirtualAddress + nt_header->OptionalHeader.ImageBase, 
			lft_get(&process_get_current()->lft, fd)->gfd, section->PointerToRawData, 4096,
			MMAP_PRIVATE | MMAP_USER, PROT_NONE | PROT_READ | PROT_WRITE) == MAP_FAILED)
		{
			serial_printf("address: %h size %h", section[x].VirtualAddress + nt_header->OptionalHeader.ImageBase, section[x].SizeOfRawData);
			PANIC("Could not map for process");
		}

	}

	serial_printf("kernel stack: %h\n\n", pmmngr_get_next_align((uint32)___buffer) + 4096 * 2);
	entry = nt_header->OptionalHeader.AddressOfEntryPoint + nt_header->OptionalHeader.ImageBase;
	TCB* thread = thread_create(proc, (uint32)kernel_setup_process, pmmngr_get_next_align((uint32)___buffer) + 4096 * 2, 4 KB, 3);

	return thread;
}

multiboot_info* boot_info;

void proc_init_thread()
{
	printfln("executing %s", __FUNCTION__);

	// start setting up heaps, drivers and everything needed.
	if (mmap(2 GB, INVALID_FD, 0, 1 GB + 12 MB, MMAP_PRIVATE | MMAP_ANONYMOUS, PROT_NONE | PROT_READ | PROT_WRITE) == MAP_FAILED)
		PANIC("Could not map kernel land");

	// memory map MMIO
	if (mmap(0xF0000000, INVALID_FD, 0, 0x0FFFE000, MMAP_PRIVATE | MMAP_ANONYMOUS | MMAP_IDENTITY_MAP, PROT_NONE | PROT_READ | PROT_WRITE) == MAP_FAILED)
		PANIC("Could not map MMIO");

	// write protect supervisor => cr0 bit 16 must be set to trigger page fault when kernel writes on read only page
	enable_write_protection();

	// create a 16KB heap
	kernel_heap = heap_create(3 GB + 11 MB, 16 KB);
	printfln("heap start: %h %h", kernel_heap->start_address, kernel_heap);

	/*if (mmap(4 MB, INVALID_FD, 0, 3 GB - 4 MB, VM_AREA_WRITE, 0) == MAP_FAILED)
		PANIC("Could not map user land");*/

	___buffer = (char*)(3 GB + 10 MB + 10 KB);

	// standardize this!
	vbe_mode_info_block* vbe = (vbe_mode_info_block*)(0x2000);

	//ClearScreen();

	/*dl_list<int> test;
	dl_list_init(&test);

	auto x1 = new dl_list_node<int>{ 1, 0, 0 }, x2 = new dl_list_node<int>{ 2, 0, 0 }, x3 = new dl_list_node<int>{ 3, 0, 0 }, x4 = new dl_list_node<int>{ 4, 0, 0 };

	dl_list_insert_back_node(&test, x1);
	dl_list_insert_back_node(&test, x2);
	dl_list_insert_back_node(&test, x3);
	dl_list_insert_back_node(&test, x4);

	dl_list_remove_node(&test, x1);

	printfln("data: %u", dl_list_find_node(&test, 3)->data);

	for (auto temp = test.head; temp != 0; temp = temp->next)
		printf("%u ", temp->data);
	printfln("");

	PANIC("");*/
	init_vfs();

	_abar = PCIFindAHCI();

	uint32 ahci_base = 0x300000;
	init_ahci(_abar, ahci_base);

	page_cache_init(2 GB, 20, 16);
	init_global_file_table(16);

	page_cache_print();

	vfs_node* disk = vfs_find_child(vfs_get_dev(), "sdc");
	if (disk)
	{
		vfs_node* hierarchy = fat_fs_mount("sdc_mount", disk);
		vfs_node* folder = 0;
		vfs_add_child(vfs_get_root(), hierarchy);
	}

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

		//while (true);

		//mutex_init(&m);
		//spinlock_init(&s);
		//semaphore_init(&sem, 1);


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

	serial_printf("initializeing screen width mode: %h...\n", boot_info->m_vbe_mode_info);
	init_screen_gfx(vbe);

	set_foreground_color(0x00FFFFFF);
	set_background_color(0x000000FF);

	init_print_utility();

	printfln("hello");
	printfln("hello %u", 3);

	serial_printf("screen ready...\n");

	

	TCB* c;
	thread_insert(c = thread_create(thread_get_current()->parent, (uint32)keyboard_fancy_function, 3 GB + 10 MB + 520 KB, 4 KB, 3));
	//////thread_insert(thread_create(thread_get_current()->parent, (uint32)idle, 3 GB + 10 MB + 516 KB, 4 KB, 7));
	//////thread_insert(thread_create(thread_get_current()->parent, (uint32)test1, 3 GB + 10 MB + 512 KB, 4 KB, 3));
	//////thread_insert(thread_create(thread_get_current()->parent, (uint32)test2, 3 GB + 10 MB + 508 KB, 4 KB, 3));
	///////*TCB* thread = thread_create(thread_get_current()->parent, (uint32)test3, 3 GB + 10 MB + 500 KB, 4 KB, 1);
	//////thread_insert(thread); */
	//////TCB* thread = thread_create(thread_get_current()->parent, (uint32)test_print_time, 3 GB + 10 MB + 504 KB, 4 KB, 3);
	//////thread_insert(thread);
	//////thread_test_time = thread;
	ClearScreen();
	//create_vfs_pipe(___buffer, 512, fd);

	// create new test process to run keyboard fancy function.
	//PCB* p = process_create(process_get_current(), 0, 0, 0xFFFFFFFF);
	//thread_insert(c = thread_create(p, (uint32)keyboard_fancy_function, 3 GB + 10 MB + 520 KB, 4 KB, 3));

	//ClearScreen();
	clear_screen();

	INT_ON;


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

extern "C" void test_handle(registers_t* regs);

extern "C" int _fltused = 1;

//extern "C" long __declspec (naked) _ftol2_sse() {
//
//	int a;
//	_asm	fistp[a]
//		_asm	mov	ebx, a
//	_asm	ret
//}
//
//extern "C" float __declspec(naked) _CIcos() {
//	_asm fcos
//};
//
//extern "C" float __declspec(naked) _CIsin() {
//	_asm fsin
//};
//
//extern "C" float __declspec(naked) _CIsqrt() {
//	_asm fsqrt
//};

int kmain(multiboot_info* _boot_info, kernel_info* k_info)
{
	boot_info = _boot_info;

	printf("hello");

	//printfln("LFA: %h",_boot_info->m_vbe_control_info);

	INT_OFF;
	init_descriptor_tables(k_info->gdt_base, k_info->isr_handlers, k_info->idt_base);

	init_pit_timer(50, timer_callback);
	idt_set_gate(32, (uint32)scheduler_interrupt, 0x08, 0x8E);		// bypass the common interrupt handler to play with the stack
	INT_ON;

	//printf("Welcome to ME Operating System\n");

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

		/*printf("region %i: start: 0x%x %x length (bytes): 0x%x %x type: %i (%s)\n", i,
			region[i].startHi, region[i].startLo,
			region[i].sizeHi, region[i].sizeLo,
			region[i].type, strMemoryTypes[region[i].type - 1]);*/

		if (region[i].type == 1 && region[i].startLo >= 0x100000)	// make available only if region is above 1MB
			pmmngr_free_region(&physical_memory_region(region[i].startLo, region[i].sizeLo));
	}

	pmmngr_reserve_region(&physical_memory_region(0x100000, pmmngr_get_next_align(k_info->kernel_size + 1)));

	//GetMemoryStats();

	vmmngr_initialize(pmmngr_get_next_align(k_info->kernel_size + 1) / 4096);
	pmmngr_paging_enable(true);

	fsysSimpleInitialize();

	//ClearScreen();

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
	PCB* proc = process_create(0, vmmngr_get_directory(), 0, 0xFFFFF000/*4 GB - 4 KB*/);	// 4 GB is 0 in DWORD
	uint32 thread_stack = (uint32)malloc(4050);		// allocate enough space for page aligned stack
	// just for this thread, space is not malloc
	TCB* t = thread_create(proc, (uint32)proc_init_thread, pmmngr_get_next_align(thread_stack + 4096), 4096, 3);	// page align stack
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