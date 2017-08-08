#ifndef ISR_H
#define ISR_H

#include "system.h"
#include "types.h"
#include "utility.h"
#include "pic.h"
#include "thread_exception.h"

#ifdef __cplusplus
extern "C" {
#endif

	struct registers_struct
	{
		uint32 ds;
		uint32 edi, esi, ebp, esp, ebx, edx, ecx, eax;
		uint32 int_no, err_code;
		uint32 eip, cs, eflags, useresp, ss;
	};

	typedef struct registers_struct registers_t;
	typedef void(*isr_t)(registers_t* regs);

	// register raw interrupt handler that is executed on cpu interrupt
	void register_interrupt_handler(uint8 n, isr_t handler);

#ifdef __cplusplus
}
#endif

typedef void(*isr_bottom_t)(struct thread_exception pe);

// register bottom interrupt handler that is executed in the deferred context
void register_bottom_interrupt_handler(uint8 n, isr_bottom_t handler);



#endif