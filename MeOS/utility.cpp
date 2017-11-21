#include "utility.h"
#include "SerialDebugger.h"
#include "file.h"

char hexes[] = "0123456789ABCDEF";
char alphabet[] = "abcdefghijklmnopqrstuvwxyz";

uint32 get_flags()
{
	_asm
	{
		pushfd
		pop eax
	}
}

void uitoa(uint32 val, char* buffer, uint8 base)
{
	uint16 i = 0;
	do
	{
		buffer[i] = hexes[val % base];
		i++;
	} while ((val /= base) != 0);

	buffer[i] = 0;

	//reverse

	uint16 length = strlen(buffer);
	i = 0;
	for (; i < length / 2; i++)
	{
		char temp = buffer[i];
		buffer[i] = buffer[length - 1 - i];
		buffer[length - 1 - i] = temp;
	}

	buffer[length] = 0;		// not required but to be sure. buffer[i] = 0 above is enough and is not affected by the reverse process.
}

void uitoalpha(uint32 val, char* buffer)
{
	uint16 i = 0;
	do
	{
		buffer[i] = alphabet[val % 26];
		i++;
	} while ((val /= 26) != 0);

	buffer[i] = 0;

	uint16 length = strlen(buffer);

	i = 0;
	for (; i < length / 2; i++)
	{
		char temp = buffer[i];
		buffer[i] = buffer[length - 1 - i];
		buffer[length - 1 - i] = temp;
	}

	//buffer[length] = 0;
}

void itoa(int32 val, char* buffer, uint8 base)
{
	if (base <= 1 || base >= 17)
		return;

	uint16 i = 0;
	int16 sign = 1;

	if (val < 0)
	{
		sign = -1;
		val = -val;
	}

	do
	{
		buffer[i] = hexes[val % base];
		i++;
	} while ((val /= base) != 0);

	if (base == 16)
	{
		buffer[i++] = 'x';
		buffer[i++] = '0';
	}

	if (sign == -1)
		buffer[i++] = '-';

	buffer[i] = 0;

	//reverse

	uint16 length = strlen(buffer);
	i = 0;
	for (; i < length / 2; i++)
	{
		char temp = buffer[i];
		buffer[i] = buffer[length - 1 - i];
		buffer[length - 1 - i] = temp;
	}

	buffer[length] = 0;		// not required but to be sure. buffer[i] = 0 above is enough and is not affected by the reverse process.
}

uint32 pow(uint32 base, uint32 exp)	// IMPROVEEEEE
{
	uint32 res = 1;;
	for (uint32 i = 0; i < exp; i++)
		res *= base;
	return res;
}

uint32 ceil_division(uint32 value, uint32 divisor)
{
	uint32 div = value / divisor;
	uint32 rem = value % divisor;

	if (rem != 0)
		div++;

	return div;
}

uint32 atoui(char* input)
{
	if (input[0] == '0' && tolower(input[1]) == 'x')	// hex
		return atoui_hex(input + 2, strlen(input) - 2);
	else
		return atoui_dec(input, strlen(input));
}

uint32 atoui_hex(char* input, uint16 length)
{
	uint32 res = 0;

	for (uint32 i = 0; i < length; i++)
	{
		uint32 mult = input[i] - '0';

		if (isalpha(input[i]))
			mult = 10 + tolower(input[i]) - 'a';	// convert

		res += mult * pow(16, length - 1 - i);
	}

	return res;
}

uint32 atoui_dec(char* input, uint16 length)
{
	uint32 res = 0;

	for (uint32 i = 0; i < length; i++)
		res += (input[i] - '0') * pow(10, length - 1 - i);

	return res;
}

void memset(void* base, uint8 val, uint32 length)
{
	char* ptr = (char*)base;

	uint32 index = length % sizeof(uint32);

	for (uint32 j = 0; j < index; j++)
	{
		*ptr = val;
		ptr++;
		length--;
	}

	uint32* new_ptr = (uint32*)ptr;
	length /= sizeof(uint32);

	//create the 32 bit value so that the last loop works
	uint32 val32 = val & 0xFF;
	val32 |= (val << 8) & 0xFF00;
	val32 |= (val << 16) & 0xFF0000;
	val32 |= (val << 24) & 0xFF000000;

	for (uint32 i = 0; i < length; i++)	// must be strictly less than length
		new_ptr[i] = val32;
}

void memcpy(void* dest, void* source, uint32 length)
{
	char* ptr_s = (char*)source;
	char* ptr_d = (char*)dest;

	uint32 index = length % sizeof(uint32);

	for (uint32 j = 0; j < index; j++)
	{
		*ptr_d = *ptr_s;
		ptr_s++;
		ptr_d++;
		length--;
	}

	uint32* new_ptr_s = (uint32*)ptr_s;
	uint32* new_ptr_d = (uint32*)ptr_d;
	length /= sizeof(uint32);

	for (uint32 i = 0; i < length; i++)	// must be strictly less than length
		new_ptr_d[i] = new_ptr_s[i];
}