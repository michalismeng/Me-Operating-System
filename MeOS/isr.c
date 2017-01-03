#include "isr.h"

isr_t* interrupt_handlers;

void __cdecl isr_handler(registers_t regs)
{
	if (interrupt_handlers[regs.int_no] != 0)
	{
		isr_t handler = interrupt_handlers[regs.int_no];
		handler(&regs);
	}
	else
		printf("Unhandled exception %u", regs.int_no);
}

void __cdecl irq_handler(registers_t regs)
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
}

void register_interrupt_handler(uint8 n, isr_t handler)
{
	interrupt_handlers[n] = handler;
}