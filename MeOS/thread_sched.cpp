#include "thread_sched.h"

TCB* current_thread = 0;
_thread_sched scheduler;
list<PCB*> procs;

extern volatile uint32 ticks;
extern "C" uint32 frequency;

// all thread scheduling methods should never request heap memory, in order for the locking mechanisms to work
// only thread insert should allocate memory
// every other action is just a node move between the queues

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

void scheduler_decrease_sleep_time()
{
	list_node<TCB*>* ptr = SLEEP_QUEUE.head;
	list_node<TCB*>* prev = 0;
	uint32 elapsed = 1000 / frequency;

	while (ptr != 0)
	{
		TCB* thread = ptr->data;
		if (elapsed > ptr->data->sleep_time)
		{
			list_node<TCB*>* node;
			ptr = ptr->next;

			//if (prev == 0)
			//	list_remove_front(&SLEEP_QUEUE);
			//else
			//	list_remove(&SLEEP_QUEUE, prev); // and delete

			if (prev == 0)
				node = list_remove_front_node(&SLEEP_QUEUE);
			else
				node = list_remove_node(&SLEEP_QUEUE, prev);

			thread->state = THREAD_READY;
			thread->sleep_time = 0;
			//list_insert_back(&READY_QUEUE(thread->base_priority), thread);
			list_insert_back_node(&READY_QUEUE(thread->base_priority), node);
		}
		else
		{
			thread->sleep_time -= elapsed;
			prev = ptr;
			ptr = ptr->next;
		}
	}
}

extern "C" __declspec(naked) void scheduler_interrupt()
{
	// already pushed due to interrupt flags, cs, eip
	__asm
	{
		cli

		// increase the time ticks
		push eax
		mov eax, dword ptr[ticks]
		inc eax
		mov dword ptr[ticks], eax
		pop eax

		// start threading
		cmp current_thread, 0
		je no_tasks

		THREAD_SAVE_STATE

		call scheduler_thread_switch
		call scheduler_decrease_sleep_time

		mov eax, dword ptr[current_thread]	// deref current_thread
		mov eax, [eax + 8]					// deref parent of thread
		mov eax, [eax]						// get page dir

		push eax
		push eax

		call vmmngr_switch_directory

		// restore previous esp from the now changed current thread pointer

		mov eax, dword ptr[current_thread]
		mov esp, [eax]

		// restore registers saved at the time current task was preempted.These are not the segments above as esp has changed.

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

// returns the first non empty queue, priority taken into account.
list<TCB*>* sched_get_first_non_empty()
{
	for (uint32 i = HIGHEST_PRIORITY; i < NUMBER_PRIORITIES; i++)
		if (scheduler.thread_queues[i].count > 0)
			return &scheduler.thread_queues[i];

	return 0;
}

// public functions

void init_thread_scheduler()
{
	for (uint32 i = 0; i < NUMBER_PRIORITIES; i++)
		list_init(&scheduler.thread_queues[i]);

	list_init(&scheduler.block_queue);
}

TCB* thread_get_current()
{
	return current_thread;
}

void scheduler_thread_switch()
{
	// assertions:
	// thread's state is saved on its stack
	// esp points to neutral stack

	// remove current thread from the executing state if it is in the running state. (See block thread removal below using this function)
	if (current_thread->state == THREAD_STATE::THREAD_RUNNING)
	{
		current_thread->state = THREAD_STATE::THREAD_READY;
		list_head_to_tail(&READY_QUEUE(current_thread->base_priority));
	}

	// find the first thread that can be run
	list<TCB*>* to_execute_list = sched_get_first_non_empty();
	if (to_execute_list == 0)		// nothing was found. Serious problem
		PANIC("Error scheduler execute list is empty");

	// setup thread to execute
	TCB* to_execute = LIST_PEEK(to_execute_list);

	if (to_execute->state != THREAD_STATE::THREAD_READY)
		PANIC("Received non ready thread to execute");

	to_execute->state = THREAD_STATE::THREAD_RUNNING;
	current_thread = to_execute;
	// all that remains is to switch context and jump to the thread's eip
}

void scheduler_start()
{
	list<TCB*>* queue = sched_get_first_non_empty();
	if (queue == 0)
		PANIC("No thread found to start");

	current_thread = LIST_PEEK(queue);
	current_thread->state = THREAD_STATE::THREAD_RUNNING;
	thread_execute(*current_thread);
}

void thread_insert(TCB* thread)
{
	//TODO: if current thread's priority is lees than the new one's, the current should be pre-empted (except if it is non preemptible)
	thread->state = THREAD_STATE::THREAD_READY;
	list_insert_back(&READY_QUEUE(thread->base_priority), thread);
	printfln("inserted");
}

// TODO: Search all the queues for the thread
TCB* thread_find(uint32 id)
{
	// we need a cli environment as we search a common data structure
	INT_OFF;

	list_node<TCB*>* temp = 0;

	for (uint32 i = HIGHEST_PRIORITY; i < NUMBER_PRIORITIES; i++)
	{
		temp = READY_QUEUE(i).head;

		while (temp != 0)
		{
			if (temp->data->id == id)
				break;

			temp = temp->next;
		}
	}

	INT_ON;

	if (temp == 0)
		return 0;

	return temp->data;
}

__declspec(naked) void thread_current_yield()
{
	THREAD_INTERRUPT_FRAME;		// save flags

	INT_OFF;					// mask interrupts. (we do not need to unmask as the previously saved flag state is not aware of this malicious change).

	THREAD_SAVE_STATE;			// save stack and change to neutral stack

	scheduler_thread_switch();
	thread_execute(*current_thread);
}

__declspec(naked) void thread_block(TCB* thread)
{
	// TODO: Perhaps we will need to disable interrupts throughout this function to control the reading of the thread's state
	_asm push ebp
	_asm mov ebp, esp	// create a new stack frame, but destroy it immediatelly before messing with th thread stack

	INT_OFF;  // cli to mess with common data structure

	if (thread->state == THREAD_STATE::THREAD_BLOCK)
		PANIC("Thread already blocked"); // return

	if (thread->state != THREAD_STATE::THREAD_RUNNING && thread->state != THREAD_STATE::THREAD_READY)
	{
		printfln("thread: %u", thread->state);
		PANIC("thread error: thread neither READY nor RUNNING");  // iretd. Thread is neither running nor ready-waiting
	}

	// if the thread is not running
	// we simply remove the thread and add it to the blocked queue
	if (thread->state != THREAD_STATE::THREAD_RUNNING)
	{
		thread->state = THREAD_BLOCK;

		if (list_remove(&scheduler.thread_queues[thread->base_priority], list_get_prev(&scheduler.thread_queues[thread->base_priority], thread)))
			list_insert_back(&scheduler.block_queue, thread);

		_asm pop ebp
	}
	else  // we need to firstly save its state at the stack
	{
		thread->state = THREAD_BLOCK;

		if (thread == current_thread)
		{
			list_remove_front(&scheduler.thread_queues[thread->base_priority]);
			list_insert_back(&scheduler.block_queue, thread);

			_asm pop ebp

			INT_ON;

			THREAD_INTERRUPT_FRAME;

			INT_OFF;

			THREAD_SAVE_STATE;

			scheduler_thread_switch();
			thread_execute(*current_thread);
		}
		else
			PANIC("Strange. Blocking a running thread from a running thread, they are not equal, UP system");  // there are two STATE == RUNNING threads
	}

	INT_ON;
	_asm ret
}

__declspec(naked) void thread_sleep(TCB* thread, uint32 sleep_time)
{
	_asm push ebp
	_asm mov ebp, esp

	INT_OFF;  // cli to mess with common data structure

	if (thread->state == THREAD_STATE::THREAD_SLEEP)
		PANIC("THREAD IS ALREADY SLEEPING");

	if (thread->state != THREAD_STATE::THREAD_RUNNING && thread->state != THREAD_STATE::THREAD_READY)
	{
		printfln("thread: %u", thread->state);
		PANIC("current thread error");  // iretd. Thread is neither running nor ready-waiting
	}

	thread->sleep_time = sleep_time;

	// if the thread is not running
	// we simply remove the thread and add it to the sleeping queue
	if (thread->state != THREAD_STATE::THREAD_RUNNING)
	{
		thread->state = THREAD_SLEEP;

		if (list_remove(&READY_QUEUE(thread->base_priority), list_get_prev(&READY_QUEUE(thread->base_priority), thread)))
			list_insert_back(&SLEEP_QUEUE, thread);

		_asm pop ebp
	}
	else  // we need to firstly save its state at the stack
	{
		if (thread == current_thread)
		{
			thread->state = THREAD_SLEEP;

			/*list_remove_front(&READY_QUEUE(thread->base_priority));
			list_insert_back(&SLEEP_QUEUE, thread);*/

			auto node = list_remove_front_node(&READY_QUEUE(thread->base_priority));
			list_insert_back_node(&SLEEP_QUEUE, node);

			_asm pop ebp  // fix the stack to create the interrupt frame

			INT_ON;

			THREAD_INTERRUPT_FRAME;

			INT_OFF;

			THREAD_SAVE_STATE;

			scheduler_thread_switch();
			thread_execute(*current_thread);
		}
		else
			PANIC("Strange. Sleeping a running thread from a running thread, they are not equal, UP system");  // there are two STATE == RUNNING threads
	}

	INT_ON;
	_asm ret
}

void thread_notify(TCB* thread)
{
	INT_OFF;

	// thread must be blocked
	if (thread->state != THREAD_STATE::THREAD_BLOCK)
	{
		INT_ON;
		return;
	}

	/*list_remove(&BLOCK_QUEUE, list_get_prev(&BLOCK_QUEUE, thread));
	thread->state = THREAD_STATE::THREAD_READY;
	list_insert_back(&READY_QUEUE(thread->base_priority), thread);*/

	auto node = list_remove_node(&BLOCK_QUEUE, list_get_prev(&BLOCK_QUEUE, thread));
	thread->state = THREAD_STATE::THREAD_READY;
	list_insert_back_node(&READY_QUEUE(thread->base_priority), node);

	INT_ON;
}

void scheduler_print_queue(list<TCB*>& queue)
{
	if (queue.count == 0)
		return;

	list_node<TCB*>* ptr = queue.head;

	while (ptr != 0)
	{
		printfln("Task: %h with address space at: %h and esp: %h, id %u", ptr->data->id, ptr->data->parent->page_dir, ptr->data->esp, ptr->data->id);
		ptr = ptr->next;
	}
}

void scheduler_print_queues()
{
	printfln("printing");
	for (int i = 0; i < NUMBER_PRIORITIES; i++)
	{
		if (scheduler.thread_queues[i].count == 0) continue;

		printfln("queue: %u", i);
		scheduler_print_queue(scheduler.thread_queues[i]);
	}
}