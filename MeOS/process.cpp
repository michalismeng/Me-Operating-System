#include "process.h"

// private data

process current_process = { 0,0,0,PROCESS_ACTIVE,0 };

// public functions

uint32 process_create(char* app_name)
{
	process* proc;
	thread* main_thread;

	FILE file = fsysSimpleDirectory(app_name);
	if (file.flags == FS_INVALID)
		return 0;

	uint8 buf[512];
	fsysSimpleRead(&file, buf, 512);

	/* validation of PE image */
	if (!validate_PE_image(buf))
	{
		DEBUG("Could not load PE image");
		return 0;
	}

	IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)buf;
	IMAGE_NT_HEADERS* nt_header = (IMAGE_NT_HEADERS*)(dos_header->e_lfanew + (uint32)buf);

	/* address space creation goes here */

	pdirectory* address_space = vmmngr_get_directory();
	if (!address_space)
		return 0;

	/* kernel mapping to new address space goes here */

	proc = process_get_current();
	proc->id = 1;
	proc->page_dir = address_space;
	proc->priority = 1;
	proc->state = PROCESS_ACTIVE;
	proc->thread_count = 1;

	main_thread = &proc->threads[0];
	main_thread->kernelStack = 0;
	main_thread->parent = proc;
	main_thread->state = THREAD_STATE::PROCESS_ACTIVE;
	main_thread->initialStack = 0;
	main_thread->stackLimit = (void*)((uint32)main_thread->initialStack + 4096);
	main_thread->imageBase = nt_header->OptionalHeader.ImageBase;
	main_thread->imageSize = nt_header->OptionalHeader.SizeOfImage;

	memset(&main_thread->frame, 0, sizeof(trapFrame));

	main_thread->frame.eip = nt_header->OptionalHeader.AddressOfEntryPoint + nt_header->OptionalHeader.ImageBase;
	main_thread->frame.flags = 0x200;

	/* copy image to memory */
	uint8* memory = (uint8*)pmmngr_alloc_block();
	memset(memory, 0, 4096);
	memcpy(memory, buf, 512);

	uint32 i = 1;
	for (; i <= main_thread->imageSize / 512; i++)
	{
		if (file.eof == true)
			break;
		fsysSimpleRead(&file, memory + 512 * i, 512);
	}

	printfln("image base: %h, size: %u", nt_header->OptionalHeader.ImageBase, nt_header->OptionalHeader.SizeOfImage);
	vmmngr_map_page(proc->page_dir, (physical_addr)memory, nt_header->OptionalHeader.ImageBase, DEFAULT_FLAGS);

	i = 1;
	while (file.eof != true)
	{
		uint8* cur = (uint8*)pmmngr_alloc_block();
		for (uint32 cur_block = 0; cur_block < 8; cur_block++)
		{
			if (file.eof == true)
				break;

			fsysSimpleRead(&file, cur + 512 * cur_block, 512);
		}

		vmmngr_map_page(proc->page_dir, (physical_addr)cur, nt_header->OptionalHeader.ImageBase + i * 4096, DEFAULT_FLAGS);
		i++;
	}

	void* stack = (void*)(nt_header->OptionalHeader.ImageBase + nt_header->OptionalHeader.SizeOfImage + 4096);
	void* stack_phys = pmmngr_alloc_block();

	vmmngr_map_page(proc->page_dir, (physical_addr)stack_phys, (virtual_addr)stack, DEFAULT_FLAGS);

	main_thread->initialStack = stack;
	main_thread->frame.esp = (uint32)main_thread->initialStack;
	main_thread->frame.ebp = main_thread->frame.esp;

	return proc->id;
}

process* process_get_current()
{
	return &current_process;
}

void process_execute()
{
	process* proc = process_get_current();

	uint32 entryPoint = proc->threads[0].frame.eip;
	uint32 procStack = proc->threads[0].frame.ebp;

	__asm cli
	pmmngr_load_PDBR((physical_addr)proc->page_dir);

	__asm
	{
		; change to user mode selectors

		; create a interrupt return stack frame
		push 0x10; to be changed to user mode selector
		push dword ptr procStack
		push 0x200
		push 0x8
		push dword ptr entryPoint

		iretd
	}
}

bool validate_PE_image(void* image)
{
	IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)image;

	/* make sure program is valid */
	if (dos_header->e_lfanew == 0 || dos_header->e_magic != IMAGE_DOS_SIGNATURE)
		return false;

	IMAGE_NT_HEADERS* nt_headers = (IMAGE_NT_HEADERS*)(dos_header->e_lfanew + (uint32)image);

	/* make sure program header is valid */
	if (nt_headers->Signature != IMAGE_NT_SIGNATURE)
		return false;

	/* make sure executable is only for i386 cpu */
	if (nt_headers->FileHeader.Machine != IMAGE_FILE_MACHINE_I386)
		return false;

	/* make sure this is an executable image and that it is built for 32-bit arch */
	if (nt_headers->FileHeader.Characteristics & (IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_32BIT_MACHINE) == 0)
		return false;

	/* make sure the image base is between 4MB and 2GB as this is the user land */
	if (nt_headers->OptionalHeader.ImageBase < 4 MB || nt_headers->OptionalHeader.ImageBase >= 2 GB)
		return false;

	return true;
}