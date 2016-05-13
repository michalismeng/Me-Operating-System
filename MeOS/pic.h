#ifdef __cplusplus
extern "C" {
#endif

#ifndef PIC_H
#define PIC_H

#include "system.h"
#include "types.h"
#include "utility.h"

	// defines every initialization and command word that is needed for the pic

#define PIC_PRIMARY_COMMAND_PORT 0x20
#define PIC_PRIMARY_STATUS_PORT	 0x20
#define PIC_PRIMARY_IMR_PORT	 0x21
#define PIC_PRIMARY_DATA_PORT	 0x21

#define PIC_SLAVE_COMMAND_PORT	 0xA0
#define PIC_SLAVE_STATUS_PORT	 0xA0
#define PIC_SLAVE_IMR_PORT		 0xA1
#define PIC_SLAVE_DATA_PORT		 0xA1

	// initialization word 1 (ICW1) // sent to command port // base initialization

#define RECEIVE_ICW4			1 << 0
#define NO_CASCADE				1 << 1
#define LEVEL_TRIGGERED_MODE	1 << 3
#define INITIALIZATION  		1 << 4

	// initialization word 2 (ICW2) // sent to data port // address of the ISRs to execute

#define IRQ_0 					0x20  // base address for master pic (with 8 interrupt lines)
#define IRQ_8					0x28  // base address for slave  pic (with the rest 8 interrupt lines)

	// initialization word 3 (ICW3) // sent to data port // line for pic inter-communication

#define PRIMARY_IRQ_LINE_2		1 << 2  // 00000100
#define SLAVE_IRQ_LINE_2		1 << 1  // 010      binary notation used for the slave

	// initialization word 4 (ICW4) // send to data port

#define OPERATION_80x86			1 << 0
#define AUTO_EOI				1 << 1
#define BUFFER_MASTER			1 << 2	// used only with the next command
#define OPERATION_BUFFERED		1 << 3
#define FULLY_NESTED			1 << 4  // used in systems with many cascades. not supported for us

	// operating command word 2 (OCW2) // sent to command port

#define EOI 					1 << 5


void init_pic();

#endif

#ifdef __cplusplus
}
#endif