#include "process.h"

// private data and functions

TCB* current_thread = 0;

queue<TCB*> ready_queue;
list<TCB*> sleeping_queue;
list<TCB*> blocking_queue;

queue<PCB> process_queue;

static uint32 lastID = 0;

void thread_switch()
{
	// this must be an uninterruptable code section
	if (ready_queue.count == 1)
		return;

	if (ready_queue.count != 0)
	{
		queue_insert(&ready_queue, current_thread);
		queue_remove(&ready_queue);

		current_thread = queue_peek(&ready_queue);
		while (current_thread->state != THREAD_STATE::THREAD_READY)
		{
			queue_remove(&ready_queue);
			current_thread = queue_peek(&ready_queue);
		}

		if (current_thread == 0)
			DEBUG("thread null ERROR");
	}
}

void thread_current_block()
{
	__asm
	{
		mov eax, 1
		int 0x81
	}
}

void thread_current_sleep(uint32 time)
{
	__asm
	{
		mov eax, 3
		mov ebx, dword ptr[time]
		int 0x81
	}
}

void thread_notify(uint32 id)
{
	__asm
	{
		mov eax, 2
		mov ebx, dword ptr[id]
		int 0x81
	}
}

TCB* thread_get_current()
{
	return current_thread;
}

// decreases by timeslice the sleep time of all threads in the sleep_queue
void thread_dec_sleep_time()
{
	if (sleeping_queue.count == 0)
		return;

	list_node<TCB*>* ptr = sleeping_queue.head;
	list_node<TCB*>* prev = 0;

	while (ptr != 0)
	{
		auto thread = ptr->data;
		if (1000 / frequency > ptr->data->sleep_time)
		{
			if (prev == 0)
			{
				list_remove_front(&sleeping_queue);
				ptr = sleeping_queue.head;
				prev = 0;
			}
			else
			{
				ptr = ptr->next;
				list_remove(&sleeping_queue, prev);
			}

			thread->state = THREAD_READY;
			thread->sleep_time = 0;
			queue_insert(&ready_queue, thread);
		}
		else
		{
			thread->sleep_time -= 1000 / frequency;
			prev = ptr;
			ptr = ptr->next;
		}
	}
}

// public functions

extern "C" void timer_callback(registers_t* regs);

__declspec(naked) void scheduler_interrupt()
{
	__asm
	{
		cli
		pushad

		; push 0
		call timer_callback
		; add esp, 4

		; mov edx, [ticks]
		; inc edx
		; mov[ticks], edx

		cmp current_thread, 0
		je no_tasks

		push ds
		push es
		push fs
		push gs

		; save current esp to current_task

		mov eax, dword ptr[current_thread]
		mov[eax], esp

		// switch address space using a neutral stack

		mov esp, 0x90000
		call thread_switch
		call thread_dec_sleep_time

		mov eax, dword ptr[current_thread]	// deref current_thread
		mov eax, [eax + 8]					// deref parent of thread
		mov eax, [eax]						// get page dir

		push eax
		push eax

		call vmmngr_switch_directory

		; restore previous esp from the now changed current thread pointer

		mov eax, dword ptr[current_thread]
		mov esp, [eax]

		; restore registers saved at the time current task was preempted.These are not the segments above as esp has changed.

		pop gs
		pop fs
		pop es
		pop ds

		no_tasks :
		mov al, 20h
			out 20h, al

			popad
			iretd
	}
}

__declspec(naked) void dispatcher_thread_current_block()
{
	if (current_thread->state != THREAD_STATE::THREAD_RUNNING && current_thread->state != THREAD_STATE::THREAD_READY)
		PANIC("current thread error");	// iretd

	THREAD_SAVE_STATE;

	// TODO: remove blocked thread from ready queue inserted by thread switch
	current_thread->state = THREAD_BLOCK;
	list_insert_back(&blocking_queue, current_thread);		// insert thread in the blocking queue

	thread_switch();
	thread_execute(*current_thread);
}

__declspec(naked) void dispatcher_thread_current_sleep()
{
	if (current_thread->state != THREAD_STATE::THREAD_RUNNING && current_thread->state != THREAD_STATE::THREAD_READY)
		PANIC("current thread sleep error");	// iretd

	THREAD_SAVE_STATE;

	// set sleep time. We cannot use local variables as this is a naked function
	_asm
	{
		mov eax, dword ptr[current_thread]		// deref current_thread
		mov dword ptr[eax + 12], ebx			// deref sleep_time and store ebx(actual sleep time value)
	}

	current_thread->state = THREAD_SLEEP;
	list_insert_back(&sleeping_queue, current_thread);

	thread_switch();
	thread_execute(*current_thread);
}

void dispatcher_thread_notify(uint32 id)
{
	if (blocking_queue.count == 0)
		return;

	list_node<TCB*>* ptr = blocking_queue.head;
	list_node<TCB*>* prev = 0;
	TCB* thread = 0;

	while (ptr != 0)
	{
		if (ptr->data->id == id)
		{
			thread = ptr->data;
			list_remove(&blocking_queue, prev);
		}
		prev = ptr;
		ptr = ptr->next;
	}

	if (thread == 0)
		return;

	thread->state = THREAD_STATE::THREAD_READY;
	queue_insert(&ready_queue, thread);
}

extern "C" __declspec(naked) void process_dispatcher()
{
	__asm
	{
		cmp eax, 1
		je dispatcher_thread_current_block

		cmp eax, 3										// ebx stores the amount of milliseconds to sleep
		je dispatcher_thread_current_sleep				// put the currently executing thread to sleep

		cmp eax, 2
		jne _next

		push ebx
		call dispatcher_thread_notify					// thread id to awake is loaded into ebx register
		add esp, 4

		_next :
		iretd
	}
}

void init_multitasking()
{
	queue_init(&ready_queue);
	list_init(&sleeping_queue);
	list_init(&blocking_queue);
}

void thread_setup_stack(TCB* t, uint32 entry, uint32 esp)
{
	printfln("creating thread esp: %h", esp);
	trap_frame* f;
	esp -= sizeof(trap_frame);		// prepare esp for manual data push
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

	t->ss = 0x10;
	t->esp = esp;
}

uint32 process_create(char* app_name)
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

	pdirectory* address_space = vmmngr_create_address_space();
	if (!address_space)
		return 0;

	/* kernel mapping to new address space goes here */
	vmmngr_map_kernel_space(address_space);
	//vmmngr_switch_directory(address_space, (physical_addr)address_space);

	queue_insert(&process_queue, PCB());

	p = &process_queue.tail->data;

	queue_insert(&p->threads, TCB());

	p->id = ++lastID;
	p->page_dir = address_space;
	p->parent = 0;
	p->image_base = nt_header->OptionalHeader.ImageBase;
	p->image_size = nt_header->OptionalHeader.SizeOfImage;

	t = &p->threads.tail->data;

	t->id = ++lastID;
	t->priority = 1;
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
		(uint32)t->stack_base);

	vmmngr_switch_directory(old_dir, (physical_addr)old_dir);

	queue_insert(&ready_queue, t);
	return p->id;
}

uint32 thread_create(PCB* parent, uint32 entry, uint32 esp)
{
	TCB* t;
	queue_insert(&parent->threads, TCB());
	t = &parent->threads.tail->data;

	t->id = ++lastID;
	t->parent = parent;
	t->stack_base = (void*)esp;
	t->state = THREAD_STATE::THREAD_READY;

	pdirectory* old_dir = vmmngr_get_directory();
	vmmngr_switch_directory(parent->page_dir, (physical_addr)parent->page_dir);

	thread_setup_stack(t, entry, esp);

	vmmngr_switch_directory(old_dir, (physical_addr)old_dir);
	queue_insert(&ready_queue, t);

	return t->id;
}

void thread_execute(TCB t)
{
	__asm
	{
		// get the new address space
		mov eax, t.parent
		mov eax, [eax]

		push eax
		push eax

		call vmmngr_switch_directory

		; make a good stack
		add esp, 8

		// restore stack where all thread data where saved
		mov esp, t.esp

		// and pop them
		pop gs
		pop fs
		pop es
		pop ds

		popad

		iretd
	}
}

void multitasking_start()
{
	current_thread = queue_peek(&ready_queue);
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

	queue_node<TCB*>* ptr = ready_queue.head;
	printfln("ready queue count: %u", ready_queue.count);
	while (ptr != 0)
	{
		printfln("Task: %h with address space at: %h and esp: %h", ptr->data->id, ptr->data->parent->page_dir, ptr->data->esp);
		ptr = ptr->next;
	}
}