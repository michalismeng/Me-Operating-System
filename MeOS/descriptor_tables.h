#ifdef __cplusplus
extern "C" {
#endif

#ifndef DESCRIPTOR_TABLES_H
#define DESCRIPTOR_TABLES_H

#include "types.h"
#include "pic.h"
#include "isr.h"

#pragma pack(push, 1)

	struct gdt_entry_struct
	{
		uint16 limit_low;		// The lower 16 bits of the limit.
		uint16 base_low;		// The lower 16 bits of the base.
		uint8 base_middle;		// The next 8 bits of the base.
		uint8 access;			// Access flags, determine what ring this segment can be used in.
		uint8 granularity;
		uint8 base_high;		// The last 8 bits of the base.
	};

	struct gdt_ptr_struct
	{
		uint16 limit;			// the size of the gdt table MINUS one (this is the last valid index of our table)
		uint32 base;			// The address of the first gdt_entry_t struct.
	};

	struct idt_entry_struct
	{
		uint16 base_low;		// the lower 16 bits of the base of the addres to execute when the interrupt fires
		uint16 selector;		// the kernel segment selector
		uint8 always0;
		uint8 flags;			// flags
		uint16 base_high;		// the higher 16 bits of the address to execute
	};

	struct idt_ptr_struct
	{
		uint16 limit;			// the size of the idt table MINUS one again as gdt table
		uint32 base;			// the addres of the first idt_entry_t struct
	};

	struct tss_entry_struct
	{
		uint32 prevTss;
		uint32 esp0;
		uint32 ss0;
		uint32 esp1;
		uint32 ss1;
		uint32 esp2;
		uint32 ss2;
		uint32 cr3;
		uint32 eip;
		uint32 eflags;
		uint32 eax;
		uint32 ecx;
		uint32 edx;
		uint32 ebx;
		uint32 esp;
		uint32 ebp;
		uint32 esi;
		uint32 edi;
		uint32 es;
		uint32 cs;
		uint32 ss;
		uint32 ds;
		uint32 fs;
		uint32 gs;
		uint32 ldt;
		uint16 trap;
		uint16 iomap;
	};

#pragma pack(pop, 1)

	typedef struct gdt_entry_struct gdt_entry_t;
	typedef struct gdt_ptr_struct 	gdt_ptr_t;
	typedef struct idt_entry_struct	idt_entry_t;
	typedef struct idt_ptr_struct	idt_ptr_t;
	typedef struct tss_entry_struct tss_entry_struct_t;

	//extern gdt_entry_t*	gdt_entries;
	extern gdt_ptr_t 	gdt_ptr;

	extern idt_entry_t*	idt_entries;
	extern idt_ptr_t 	idt_ptr;

	// initializes GDT and IDT based on values fetched by the loader
	void init_descriptor_tables(gdt_entry_t* gdt_base, isr_t* isr_base, idt_entry_t* idt_base);

	// initializes GDTs
	void init_gdt();
	void gdt_set_gate(uint16 num, uint32 base, uint32 limit, uint8 access, uint8 gran);

	void init_tss();
	void set_tss(uint32 kernel_stack, uint32 kernel_esp);
	extern void flush_tss(uint16 selector);

	// calls asm code to load gdt tables
	extern void flush_gdt(uint32 addr);

	// initializes IDTs
	void init_idt();
	void idt_set_gate(uint8 num, uint32 base, uint16 selector, uint8 flags);

	// calls asm code to load idt tables
	extern void flush_idt(uint32 addr);

	// asm externals of interrupt handlers
	extern void isr0();
	extern void isr1();
	extern void isr2();
	extern void isr3();
	extern void isr4();
	extern void isr5();
	extern void isr6();
	extern void isr7();
	extern void isr8();
	extern void isr9();
	extern void isr10();
	extern void isr11();
	extern void isr12();
	extern void isr13();
	extern void isr14();
	extern void isr15();
	extern void isr16();
	extern void isr17();
	extern void isr18();
	extern void isr19();
	extern void isr20();
	extern void isr21();
	extern void isr22();
	extern void isr23();
	extern void isr24();
	extern void isr25();
	extern void isr26();
	extern void isr27();
	extern void isr28();
	extern void isr29();
	extern void isr30();
	extern void isr31();
	extern void isr128();  // scheduler interrupt
	///////////////////////////

	// asm externals of hardware interrupt handlers
	extern void irq0();
	extern void irq1();
	extern void irq2();
	extern void irq3();
	extern void irq4();
	extern void irq5();
	extern void irq6();
	extern void irq7();
	extern void irq8();
	extern void irq9();
	extern void irq10();
	extern void irq11();
	extern void irq12();
	extern void irq13();
	extern void irq14();
	extern void irq15();
	///////////////////////////

#endif

#ifdef __cplusplus
}
#endif
