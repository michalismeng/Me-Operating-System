#include "SerialDebugger.h"

// private data

bool _serial_port_found = false;

void serial_print(const char* str);

void init_serial()
{
	volatile uint8 value = 87;					// random value to check against

	// Scratch register test
	outportb(PORT + 7, value);

	if (inportb(PORT + 7) != value)
		return;

	_serial_port_found = true;

	outportb(PORT + 1, 0x00);	 // Disable Interrupt
	outportb(PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
	outportb(PORT + 0, 12);		 // Set divisor to 12 (lo byte) 9600 baud
	outportb(PORT + 1, 0x00);    //                  (hi byte)
	outportb(PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
	outportb(PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
	outportb(PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

int is_transmit_empty()
{
	return inportb(PORT + 5) & 0x20;
}

int is_receive_ready()
{
	return inportb(PORT + 5) & 0x1;
}

uint8 serial_read()
{
	if (!_serial_port_found)
		return 0;

	while (is_receive_ready() == 0);

	return inportb(PORT);
}

void serial_printch(char a)
{
	if (!_serial_port_found)
		return;

	while (is_transmit_empty() == 0);

	outportb(PORT, a);
}

void serial_print(const char* str)
{
	if (!_serial_port_found)
		return;

	int i = 0;
	while (str[i] != 0)
	{
		serial_printch(str[i]);
		i++;
	}
}

void serial_printf(char* fmt, ...)
{
	if (!_serial_port_found)
		return;

	va_list arg_start;
	va_start(arg_start, fmt);

	uint16 length = strlen(fmt);

	uint32 val;
	int32 ival;
	uint8 cval;
	uint64 lval;
	error_t err;
	char* ptr;
	char buffer[20];

	for (uint16 i = 0; i < length; i++)
	{
		switch (fmt[i])
		{
		case '%':

			switch (fmt[i + 1])
			{
			case 'u':
				val = va_arg(arg_start, uint32);
				uitoa(val, buffer, 10);
				serial_print(buffer);
				break;
			case 'i':
				ival = va_arg(arg_start, int32);
				itoa(ival, buffer, 10);
				serial_print(buffer);
				break;
			case 'h':
				val = va_arg(arg_start, uint32);
				uitoa(val, buffer, 16);
				serial_print("0x");
				serial_print(buffer);
				break;
			case 'x':
				val = va_arg(arg_start, uint32);
				uitoa(val, buffer, 16);
				serial_print(buffer);
				break;
			case 's':
				ptr = va_arg(arg_start, char*);
				serial_print(ptr);
				break;
			case 'c':
				cval = va_arg(arg_start, uint32);
				serial_printch(cval);
				break;
			case 'l':
				lval = va_arg(arg_start, uint64);
				uitoa(lval, buffer, 10);
				serial_print(buffer);
				break;
			case 'b':
				val = va_arg(arg_start, uint32);
				uitoa(val, buffer, 2);
				serial_print(buffer);
				break;
			// display error in human readable form
			case 'e':
				err = va_arg(arg_start, error_t);
				uitoa(err & 0xFF, buffer, 10);				// get linux code
				serial_print("base: ");
				serial_print(buffer);
				serial_print(" ext: ");
				uitoa((err >> 8) & 0xFFFF, buffer, 10);		// get extended code
				serial_print(buffer);
				serial_print(" ");
				serial_print(ERROR_ORIGIN_STR[err >> 24]);
			default:
				break;
			}

			i++;
			break;

		default:
			serial_printch(fmt[i]); break;
		}
	}

	va_end(arg_start);
}

void serial_dump_mem(char* base, uint32 length)
{
	for (uint32 i = 0; i < length; i++)
	{
		serial_printf("%x ", base[i]);
	}
}