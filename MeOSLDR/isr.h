#ifndef ISR_H
#define ISR_H

#include "system.h"
#include "types.h"
#include "utility.h"
#include "pic.h"
#ifdef MINI
#include "kalloc.h"
#endif

#define ISR_HANDLERS 256

typedef struct registers_struct
{
	uint32 ds;
	uint32 edi, esi, ebp, esp, ebx, edx, ecx, eax;
	uint32 int_no, err_code;
	uint32 eip, cs, eflags, useresp, ss;
} registers_t;

void init_isr();

typedef void(*isr_t)(registers_t* regs);
void register_interrupt_handler(uint8 n, isr_t handler);

extern isr_t* interrupt_handlers;

#endif