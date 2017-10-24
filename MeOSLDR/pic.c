#include "pic.h"

void init_pic()
{
	// first command

	uint8 cw = RECEIVE_ICW4 | INITIALIZATION;

	outportb(PIC_PRIMARY_COMMAND_PORT, cw);
	outportb(PIC_SLAVE_COMMAND_PORT, cw);

	// second command

	outportb(PIC_PRIMARY_DATA_PORT, IRQ_0);
	outportb(PIC_SLAVE_DATA_PORT, IRQ_8);

	// third command

	outportb(PIC_PRIMARY_DATA_PORT, PRIMARY_IRQ_LINE_2);
	outportb(PIC_SLAVE_DATA_PORT, SLAVE_IRQ_LINE_2);

	//fourth command

	cw = OPERATION_80x86;

	outportb(PIC_PRIMARY_DATA_PORT, cw);
	outportb(PIC_SLAVE_DATA_PORT, cw);

	// null the data ports

	outportb(PIC_PRIMARY_DATA_PORT, 0x0);
	outportb(PIC_SLAVE_DATA_PORT, 0x0);
}

