#include "process.h"
#include "descriptor_tables.h"
#include "thread_sched.h"

// private data and functions

static uint32 lastID = 0;

void thread_setup_stack(TCB* t, uint32 entry, uint32 esp, uint32 stack_size)
{
	if (esp % vmmngr_get_page_size() != 0)
		PANIC("stack must be page-aligned");

	trap_frame* f;
	esp -= sizeof(trap_frame);		// prepare esp for manual data push
	esp -= 4;						// this goes for the thread error variable
	f = (trap_frame*)esp;

	/* manual setup of the thread's stack */
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

	// init the first 4 bytes of the stack -- the thread last error
	*((char*)f + sizeof(trap_frame)) = 0;

	t->ss = 0x10;
	t->esp = esp;
}

// public functions

uint32 process_create_s(char* app_name)
{
	PCB* p;
	TCB* t;

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

	pdirectory* address_space = vmmngr_get_directory();// vmmngr_create_address_space();
													   //if (!address_space)
													   //	return 0;

													   /* kernel mapping to new address space goes here */
													   //vmmngr_map_kernel_space(address_space);
													   //vmmngr_switch_directory(address_space, (physical_addr)address_space);

	//queue_insert(&process_queue, PCB());

	//p = &process_queue.tail->data;

	//queue_insert(&p->threads, TCB());

	p->id = ++lastID;
	p->page_dir = address_space;
	p->parent = 0;
	p->image_base = nt_header->OptionalHeader.ImageBase;
	p->image_size = nt_header->OptionalHeader.SizeOfImage;

	t = &p->threads.tail->data;

	t->id = ++lastID;
	//t->priority = 1;
	t->state = THREAD_READY;
	t->stack_base = 0;
	t->stack_limit = (void*)4096;
	t->parent = p;

	/* copy image to memory */
	uint8* memory = (uint8*)pmmngr_alloc_block();
	memset(memory, 0, 4096);
	memcpy(memory, buf, 512);

	uint32 i = 1;
	for (; i <= p->image_size / 512; i++)
	{
		if (file.eof == true)
			break;
		fsysSimpleRead(&file, memory + 512 * i, 512);
	}

	printfln("image base: %h, size: %u entry at: %h", nt_header->OptionalHeader.ImageBase, nt_header->OptionalHeader.SizeOfImage, nt_header->OptionalHeader.AddressOfEntryPoint + nt_header->OptionalHeader.ImageBase);
	vmmngr_map_page(p->page_dir, (physical_addr)memory, nt_header->OptionalHeader.ImageBase, DEFAULT_FLAGS);

	i = 1;
	while (file.eof != true)
	{
		uint8* cur = (uint8*)pmmngr_alloc_block();
		vmmngr_map_page(p->page_dir, (physical_addr)cur, nt_header->OptionalHeader.ImageBase + i * 4096, DEFAULT_FLAGS);

		for (uint32 cur_block = 0; cur_block < 8; cur_block++)
		{
			if (file.eof == true)
				break;

			fsysSimpleRead(&file, cur + 512 * cur_block, 512);
		}

		i++;
	}

	void* stack = (void*)(nt_header->OptionalHeader.ImageBase + nt_header->OptionalHeader.SizeOfImage + 4096);
	void* stack_phys = pmmngr_alloc_block();

	vmmngr_map_page(p->page_dir, (physical_addr)stack_phys, (virtual_addr)stack, DEFAULT_FLAGS);

	t->stack_base = stack;
	t->stack_limit = (void*)((uint32)t->stack_base + 4096);

	pdirectory* old_dir = vmmngr_get_directory();

	vmmngr_switch_directory(address_space, (physical_addr)address_space);

	thread_setup_stack(t, nt_header->OptionalHeader.AddressOfEntryPoint + nt_header->OptionalHeader.ImageBase,
		(uint32)t->stack_base, 4096);

	vmmngr_switch_directory(old_dir, (physical_addr)old_dir);

	//queue_insert(&ready_queue, t);
	return p->id;
}

PCB* process_create(PCB* parent, pdirectory* pdir, uint32 low_address, uint32 high_address)
{
	PCB* proc = (PCB*)malloc(sizeof(PCB));

	proc->id = 0;
	proc->parent = parent;

	if (pdir == 0)
		proc->page_dir = vmmngr_create_address_space();
	else
		proc->page_dir = pdir;

	queue_init(&proc->threads);
	vm_contract_init(&proc->memory_contract, low_address, high_address);

	return proc;
}

TCB* thread_create(PCB* parent, uint32 entry, uint32 esp, uint32 stack_size, uint32 priority)
{
	printfln("creating thread at: %h with id: %u", esp, lastID + 1);
	TCB* t;
	queue_insert(&parent->threads, TCB());
	t = &parent->threads.tail->data;

	t->id = ++lastID;
	t->parent = parent;
	t->stack_base = (void*)esp;
	t->state = THREAD_STATE::THREAD_READY;
	t->base_priority = priority;

	// TODO: Replace the directory switches by a simple kernel page map
	pdirectory* old_dir = vmmngr_get_directory();
	vmmngr_switch_directory(parent->page_dir, (physical_addr)parent->page_dir);

	thread_setup_stack(t, entry, esp, stack_size);

	vmmngr_switch_directory(old_dir, (physical_addr)old_dir);

	return t;
}

uint32* thread_get_error(TCB* thread)
{
	return (uint32*)((char*)thread->stack_base - 4);
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