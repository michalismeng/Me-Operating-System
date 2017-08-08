#ifndef PROCESS_H_28082016
#define PROCESS_H_28082016

#include "queue.h"
#include "Simple_fs.h"		// to be removed

#include "vm_contract.h"
#include "open_file_table.h"

#include "queue_lf.h"
#include "thread_exception.h"

#include "Debugger.h"
#include "spinlock.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include "utility.h"
#include "mmngr_virtual.h"
#include "PE_Definitions.h"

	// setups interrupt frame for the thread (flags - cs - eip) based on the function's return eip
	// assumes that function is naked
#define THREAD_INTERRUPT_FRAME \
_asm	pushfd	\
_asm	push cs	\
_asm	push eax \
_asm	mov eax, dword ptr[esp + 12]	\
_asm	xchg eax, dword ptr[esp + 4]	\
_asm	xchg eax, dword ptr[esp + 8]	\
_asm	xchg eax, dword ptr[esp + 12]	\
_asm	pop eax	\

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

	enum THREAD_ATTRIBUTE {
		THREAD_ATTR_NONE,
		THREAD_KERNEL = 1,					// thread is solely kernel => it does not have a user counter-part
		THREAD_NONPREEMPT = 1 << 1,			// thread is non pre-emptible. This applies only to kernel threads. Preemption state may change during thread execution
		THREAD_UNINTERRUPTIBLE = 1 << 2		// thread is uninterruptible on SIGNALS ! Interrupts affect this thread. (uninterruptible is not cli)
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

		PCB* parent;								// parent process that created this thread.
		uint32 sleep_time;							// time in millis of thread sleep. Used for the sleep function

		uint32 id;									// thread unique id

		int32 plus_priority;						// priority gained due to different factors such as waiting in the queues
		int32 base_priority;						// base priority given at the thread creation time

		void* stack_base;							// address of the base of the thread's stack
		void* stack_limit;							// end address of the thread's stack incremented by 1. stack_base <= valid_stack < stack_limit

		THREAD_STATE state;							// the current state of the thread
		THREAD_ATTRIBUTE attribute;					// thread's extra attribute info

		queue_lf<thread_exception> exceptions;		// thread exception queue to be consumed and served by the kernel
		uint32 exception_lock;						// lock for the exception consumption (to be used with CAS)
	}TCB;

	typedef struct process_control_block
	{
		pdirectory* page_dir;					// address space of the process
		process_control_block* parent;			// parent PCB that created us. PCB 0 has null parent
		uint32 id;								// unique process id

												// TODO: Perhaps these members will be erased
		uint32 image_base;						// base of the image this task is running
		uint32 image_size;						// size of the image this task is running

		local_file_table lft;					// local open file table

		vm_contract memory_contract;			// virtual memory layout
		spinlock contract_spinlock;				// virtual memory contract spinlock used for reading and writing

		queue<TCB> threads;						// child threads of the process
	}PCB;

	uint32 process_create_s(char* app_name);
	PCB* process_create(PCB* parent, pdirectory* pdir, uint32 low_address, uint32 high_address);
	TCB* thread_create(PCB* parent, uint32 entry, uint32 esp, uint32 stack_size, uint32 priority);

	int32 thread_get_priority(TCB* thread);

	uint32* thread_get_error(TCB* thread);

	bool validate_PE_image(void* image);

#ifdef __cplusplus
}
#endif

#endif
