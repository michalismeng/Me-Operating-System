#ifndef PROCESS_H_28082016
#define PROCESS_H_28082016

#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include "utility.h"
#include "mmngr_virtual.h"
#include "PE_Definitions.h"

#include "Simple_fs.h"		// to be removed

	extern volatile uint32 ticks;

	enum TASK_STATE {
		TASK_NONE,
		TASK_SLEEP,			// task resides in the sleep queue until its count-down timer reaches zero. It is then enqueued in the ready queue.
		TASK_READY,			// task resides in the ready queue where it waits to be scheduled to run.
		TASK_RUNNING,		// task does not reside in any queue as it is currently running.
		TASK_BLOCK			// task resides in the block queue as it has requested blocking I/O service and waits for it to finish.
	};

#pragma pack(push, 1)
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

	struct task
	{
		uint32 esp;
		uint32 ss;

		task* parent;			// parent task that created this task. Task 0 has a null parent
		task* next;				// sibling task that is next to this task

		uint32 id;				// task unique id
		uint32 priority;		// task priority

		void* stack_base;		// address of the base of the task's stack
		void* stack_limit;		// end address of the task's stack incremented by 1. stack_base <= valid_stack < stack_limit

		TASK_STATE state;		// the current state of the task
		pdirectory* page_dir;	// address space of the task

		uint32 image_base;		// base of the image this task is running
		uint32 image_size;		// size of the image this task is running
	};

	extern queue<task> ready_queue;

#pragma pack(pop, 1)

	void scheduler_interrupt();

	void init_multitasking();

	uint32 process_create(char* app_name);
	uint32 task_create(uint32 entry, uint32 esp);
	void task_exeute(task t);

	void start();

	bool validate_PE_image(void* image);

	void print_ready_queue();

	void task_switch();

#ifdef __cplusplus
}
#endif

#endif
