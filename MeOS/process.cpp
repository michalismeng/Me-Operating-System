#include "process.h"

// private data and functions

task* current_task = 0;
queue<task> ready_queue;

static uint32 lastID = 0;

void task_switch()
{
	if (ready_queue.count == 1)
		return;

	if (ready_queue.count != 0)
	{
		queue_insert(&ready_queue, *current_task);
		queue_remove(&ready_queue);
		current_task = queue_peek(&ready_queue);
	}
}

// public functions

__declspec(naked) void scheduler_interrupt()
{
	ticks++;
	__asm
	{
		cmp current_task, 0
		je no_tasks

		pushad

		push ds
		push es
		push fs
		push gs

		; save current esp to current_task

		mov eax, dword ptr[current_task]
		mov[eax], esp

		call task_switch

		; restore previous esp from the now changed current task pointer

		mov eax, dword ptr[current_task]
		mov esp, [eax]

		; restore registers saved at the time current task was preempted.These are not the segments above as esp has changed.

		pop gs
		pop fs
		pop es
		pop ds

		popad

		no_tasks : mov al, 20h
				   out 20h, al
				   iretd
	}
}

void init_multitasking()
{
	queue_init(&ready_queue);
}

void task_setup_stack(task* t, uint32 entry, uint32 esp)
{
	trap_frame* f;

	esp -= sizeof(trap_frame);		// prepare esp for manual data push
	f = (trap_frame*)esp;

	/* manual setup of the task's stack */
	f->flags = 0x202;		// IF set along with some ?reserved? bit
	f->cs = 0x8;
	f->ds = 0x10;
	f->eax = 0;
	f->ebp = 0;
	f->ebx = 0;
	f->ecx = 0;
	f->edi = 0;
	f->edx = 0;
	f->eip = entry;
	f->es = 0x10;
	f->esi = 0;
	f->esp = 0;
	f->fs = 0x10;
	f->gs = 0x10;

	t->ss = 0x10;
	t->esp = esp;
}

uint32 process_create(char* app_name)
{
	task t;

	FILE file = fsysSimpleDirectory(app_name);
	if (file.flags == FS_INVALID)
	{
		DEBUG("File was not found.");
		return 0;
	}

	uint8 buf[512];
	fsysSimpleRead(&file, buf, 512);

	/* validation of PE image */
	if (!validate_PE_image(buf))
	{
		DEBUG("Could not load PE image. Corrupt image or data.");
		return 0;
	}

	IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)buf;
	IMAGE_NT_HEADERS* nt_header = (IMAGE_NT_HEADERS*)(dos_header->e_lfanew + (uint32)buf);

	/* address space creation goes here */

	pdirectory* address_space = vmmngr_get_directory();
	if (!address_space)
		return 0;

	/* kernel mapping to new address space goes here */

	t.id = ++lastID;
	t.page_dir = address_space;
	t.priority = 1;
	t.state = TASK_READY;
	t.parent = 0;
	t.stack_base = 0;
	t.stack_limit = (void*)4096;
	t.image_base = nt_header->OptionalHeader.ImageBase;
	t.image_size = nt_header->OptionalHeader.SizeOfImage;

	/* copy image to memory */
	uint8* memory = (uint8*)pmmngr_alloc_block();
	memset(memory, 0, 4096);
	memcpy(memory, buf, 512);

	uint32 i = 1;
	for (; i <= t.image_size / 512; i++)
	{
		if (file.eof == true)
			break;
		fsysSimpleRead(&file, memory + 512 * i, 512);
	}

	printfln("image base: %h, size: %u", nt_header->OptionalHeader.ImageBase, nt_header->OptionalHeader.SizeOfImage);
	vmmngr_map_page(t.page_dir, (physical_addr)memory, nt_header->OptionalHeader.ImageBase, DEFAULT_FLAGS);

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

		vmmngr_map_page(t.page_dir, (physical_addr)cur, nt_header->OptionalHeader.ImageBase + i * 4096, DEFAULT_FLAGS);
		i++;
	}

	void* stack = (void*)(nt_header->OptionalHeader.ImageBase + nt_header->OptionalHeader.SizeOfImage + 4096);
	void* stack_phys = pmmngr_alloc_block();

	vmmngr_map_page(t.page_dir, (physical_addr)stack_phys, (virtual_addr)stack, DEFAULT_FLAGS);

	t.stack_base = stack;
	t.stack_limit = (void*)((uint32)t.stack_base + 4096);

	task_setup_stack(&t, nt_header->OptionalHeader.AddressOfEntryPoint + nt_header->OptionalHeader.ImageBase,
		(uint32)t.stack_base);

	queue_insert(&ready_queue, t);
	return t.id;
}

uint32 task_create(uint32 entry, uint32 esp)
{
	task t;
	t.id = ++lastID;
	t.image_base = entry;
	t.image_size = 0;
	t.page_dir = vmmngr_get_directory();

	task_setup_stack(&t, entry, esp);
	queue_insert(&ready_queue, t);

	return t.id;
}

void task_exeute(task t)
{
	__asm
	{
		mov esp, t.esp
		pop gs
		pop fs
		pop es
		pop ds
		popad
		iretd
	}
}

void start()
{
	current_task = queue_peek(&ready_queue);
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

void print_ready_queue()
{
	if (ready_queue.count == 0)
		return;

	queue_node<task>* ptr = ready_queue.head;
	printfln("ready queue count: %u", ready_queue.count);
	while (ptr != 0)
	{
		printfln("Task: %u with address space at: %h and esp: %h", ptr->data.id, ptr->data.page_dir, ptr->data.esp);
		ptr = ptr->next;
	}
}