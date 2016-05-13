#include "system.h"

uint8 __cdecl inportb(uint16 _port)
{
	uint8 rv;

	__asm
	{
		mov dx, word ptr [_port]
		in al, dx

		mov byte ptr [rv], al
	}

	return rv;
}

uint16 __cdecl inportw(uint16 _port)
{
	uint16 rv;
	__asm
	{
		mov dx, word ptr [_port]

		in ax, dx
		mov word ptr [rv], ax
	}
	return rv;
}

uint32 inportl(uint16 _port)
{
	uint32 rv;
	__asm 
	{
		mov dx, word ptr[_port]

		in eax, dx
		mov dword ptr[rv], eax
	};
	return rv;
}

void __cdecl outportb(uint16 _port, uint8 _data)
{
	__asm
	{
		mov dx, word ptr [_port]
		mov al, byte ptr [_data]

		out dx, al
	}
}

void __cdecl outportw(uint16 _port, uint16 _data)
{
	__asm
	{
		mov dx, word ptr [_port]
		mov ax, word ptr [_data]

		out dx, ax
	}
}

void outportl(uint16 _port, uint32 _data)
{
	__asm
	{
		mov dx, word ptr[_port]
		mov eax, dword ptr[_data]

		out dx, ax
	}
}