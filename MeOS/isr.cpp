#include "isr.h"

#include "thread_sched.h"
#include "atomic.h"

isr_t* interrupt_handlers;
isr_bottom_t bottom_interrupt_handlers[256] = { 0 };

extern "C" void __cdecl isr_handler(registers_t regs)
{
	printfln("Executing softwqre interrupt: %h", regs.int_no);
	if (interrupt_handlers[regs.int_no] != 0)
	{
		isr_t handler = interrupt_handlers[regs.int_no];
		handler(&regs);
	}
	else
		printf("Unhandled exception %u", regs.int_no);

	// handle thread exceptions, in the defered context
	INT_ON;

	if (CAS<uint32>(&thread_get_current()->exception_lock, 0, 1))
	{
		auto exceptions = &thread_get_current()->exceptions;

		while (!queue_lf_is_empty(exceptions))
		{
			thread_exception te = queue_lf_peek(exceptions);
			queue_lf_remove(exceptions);

			// run bottom half for pe
			if (bottom_interrupt_handlers[regs.int_no] != 0)
			{
				isr_bottom_t handler = bottom_interrupt_handlers[regs.int_no];
				handler(te);
			}
		}
		thread_get_current()->exception_lock = 0;		// reset the lock
	}

	// return to process execution
}

extern "C" void __cdecl irq_handler(registers_t regs)
{
	if (interrupt_handlers[regs.int_no] != 0)
	{
		isr_t handler = interrupt_handlers[regs.int_no];
		handler(&regs);
	}
	else
		printf("Hardware interrupt: %u", regs.int_no);

	if (regs.int_no >= 40)						// irq from slave
		outportb(PIC_SLAVE_COMMAND_PORT, EOI);	// send EOI to slave

	outportb(PIC_PRIMARY_COMMAND_PORT, EOI);	// send EOI to master

	// Interrupt ends here... Just sti	
}

void register_interrupt_handler(uint8 n, isr_t handler)
{
	interrupt_handlers[n] = handler;
}

void register_bottom_interrupt_handler(uint8 n, isr_bottom_t handler)
{
	bottom_interrupt_handlers[n] = handler;
}
