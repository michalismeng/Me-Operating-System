#include "isr.h"

isr_t* interrupt_handlers;
extern int execute;
void loadFile(uint32 addr);

void __cdecl isr_handler(registers_t regs)
{
	printfln("Executing softwqre interrupt: %h", regs.int_no);
	if (interrupt_handlers[regs.int_no] != 0)
	{
		isr_t handler = interrupt_handlers[regs.int_no];
		handler(&regs);
	}
	else
		printf("Unhandled exception %u", regs.int_no);

	_asm sti

	if (regs.int_no == 14 && execute)
	{
		uint32 addr;
		_asm
		{
			mov eax, cr2
			mov dword ptr addr, eax
		}
		printfln("loading file to addr: %h", addr);
		loadFile(addr);
	}
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

	// Interrupt ends here... Just sti

	//if (execute)
	//{
	//	//_asm sti
	//	printfln("pausing execution of:");
	//	while (true);
	//}
}

void register_interrupt_handler(uint8 n, isr_t handler)
{
	interrupt_handlers[n] = handler;
}