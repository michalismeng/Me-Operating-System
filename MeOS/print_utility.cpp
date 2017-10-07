#include "print_utility.h"
#include "file.h"

uint32 screen_fd;

char x[4096 * 5];

bool print_utility_initialized = false;

void init_print_utility()
{
	if (open_file("dev/screen", &screen_fd, 0) != ERROR_OK)
		PANIC("print utility could not be initialized");

	print_utility_initialized = true;
}

void printf_base(char* fmt, va_list arg_start)
{
	if (!print_utility_initialized)
		return;

	uint16 length = strlen(fmt);

	uint32 val;
	int32 ival;
	uint8 cval;
	uint64 lval;
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
				write_file(screen_fd, 0, 0, (virtual_addr)buffer);
				break;
			case 'i':
				ival = va_arg(arg_start, int32);
				itoa(ival, buffer, 10);
				write_file(screen_fd, 0, 0, (virtual_addr)buffer);
				break;
			case 'h':
				val = va_arg(arg_start, uint32);
				uitoa(val, buffer, 16);
				write_file(screen_fd, 0, 2, (virtual_addr)"0x");
				write_file(screen_fd, 0, 0, (virtual_addr)buffer);
				break;
			case 'x':
				val = va_arg(arg_start, uint32);
				uitoa(val, buffer, 16);
				write_file(screen_fd, 0, 0, (virtual_addr)buffer);
				break;
			case 's':
				ptr = va_arg(arg_start, char*);
				write_file(screen_fd, 0, 0, (virtual_addr)ptr);
				break;
			case 'c':
				cval = va_arg(arg_start, uint32);
				buffer[0] = cval;
				write_file(screen_fd, 0, 1, (virtual_addr)buffer);
				break;
			case 'l':
				lval = va_arg(arg_start, uint64);
				uitoa(lval, buffer, 10);
				write_file(screen_fd, 0, 0, (virtual_addr)buffer);
				break;
			case 'b':
				val = va_arg(arg_start, uint32);
				uitoa(val, buffer, 2);
				write_file(screen_fd, 0, 0, (virtual_addr)buffer);
			default:
				break;
			}

			i++;
			break;

		default:
			write_file(screen_fd, 0, 1, (virtual_addr)&fmt[i]);
			break;
		}
	}
}

void printf(char* fmt, ...)
{
	va_list l;
	va_start(l, fmt);	// spooky stack magic going on here

	printf_base(fmt, l);

	va_end(l);
}

// TODO: think about this bug. When return is not there printing crashes
void printfln(char* fmt, ...)
{
	va_list l;
	va_start(l, fmt);	// spooky stack magic going on here

	printf_base(fmt, l);

	va_end(l);

	printf("\n");
}

void PANIC(char* str)
{
	_asm cli
	serial_printf("\n%s", str);
	__asm
	{
		cli
		hlt
	}
}

void ASSERT(bool expr)
{
	if (!expr)
		PANIC("Assertion failed!!");
}

void DEBUG(char* str)
{
	//uint16 temp = color;
	//SetForegroundColor(RED);

	printf("\nDEBUG: %s\n", str);

	//SetColor(temp >> 8, temp);
}

void WARNING(char * str)
{
	//uint16 temp = color;
	//SetForegroundColor(DARK_GREEN);

	printf("\WARNING: %s\n", str);

	//SetColor(temp >> 8, temp);
}