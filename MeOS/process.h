#ifndef PROCESS_H_28082016
#define PROCESS_H_28082016

#include "queue.h"
#include "Simple_fs.h"		// to be removed

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include "utility.h"
#include "mmngr_virtual.h"
#include "PE_Definitions.h"

#define THREAD_SAVE_STATE \
_asm	pushad \
_asm	push ds \
_asm	push es \
_asm	push fs \
_asm	push gs \
_asm	mov eax, dword ptr[current_thread] \
_asm	mov[eax], esp \
_asm	mov esp, 0x90000

	extern volatile uint32 ticks;

	enum THREAD_STATE {
		THREAD_NONE,
		THREAD_SLEEP,			// task resides in the sleep queue until its count-down timer reaches zero. It is then enqueued in the ready queue.
		THREAD_READY,			// task resides in the ready queue where it waits to be scheduled to run.
		THREAD_RUNNING,			// task does not reside in any queue as it is currently running.
		THREAD_BLOCK			// task resides in the block queue as it has requested blocking I/O service and waits for it to finish.
	};

#pragma pack(push, 1)
	// represents the stack right before the thread is loaded. Must have this exact form.
	struct trap_frame
	{
		/* pushed by isr. */
		uint32 gs;
		uint32 fs;
		uint32 es;
		uint32 ds;
		/* pushed by pusha. */
		uint32 eax;
		uint32 ebx;
		uint32 ecx;
		uint32 edx;
		uint32 esi;
		uint32 edi;
		uint32 esp;
		uint32 ebp;
		/* pushed by cpu. */
		uint32 eip;
		uint32 cs;
		uint32 flags;
	};

#pragma pack(pop, 1)

	typedef struct process_control_block PCB;

	typedef struct thread_control_block
	{
		uint32 esp;
		uint32 ss;

		PCB* parent;			// parent process that created this thread.
		uint32 sleep_time;		// time in millis of thread sleep. Used for the sleep function

		uint32 id;				// thread unique id
		uint32 priority;		// thread priority

		void* stack_base;		// address of the base of the thread's stack
		void* stack_limit;		// end address of the thread's stack incremented by 1. stack_base <= valid_stack < stack_limit

		THREAD_STATE state;		// the current state of the thread
	}TCB;

	typedef struct process_control_block
	{
		pdirectory* page_dir;				// address space of the process
		process_control_block* parent;		// parent PCB that created us. PCB 0 has null parent
		uint32 id;							// unique process id

		uint32 image_base;					// base of the image this task is running
		uint32 image_size;					// size of the image this task is running

		queue<TCB> threads;					// child threads of the process
	}PCB;

	extern queue<TCB*> ready_queue;
	extern uint32 frequency;

	void scheduler_interrupt();

	void init_multitasking();
	void multitasking_start();

	uint32 process_create(char* app_name);
	uint32 thread_create(PCB* parent, uint32 entry, uint32 esp);
	void thread_execute(TCB t);
	void thread_switch();

	void thread_current_block();
	void thread_current_sleep(uint32 time);
	void thread_notify(uint32 id);

	TCB* thread_get_current();

	bool validate_PE_image(void* image);
	void print_ready_queue();

#ifdef __cplusplus
}
#endif

#endif
