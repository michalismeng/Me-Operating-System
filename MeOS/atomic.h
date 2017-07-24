#ifndef ATOMIC_H_24072017
#define ATOMIC_H_24072017

#include "types.h"

// CAS: if *dir == old_val => *dir = new_val and return true
//      if *dir != old_val => return false 
template<class T>
bool CAS(T* dir, T old_val, T new_val)
{
	bool result = false;
	_asm
	{
		mov eax, old_val
		mov edx, new_val
		mov ebx, dir

		cmpxchg[ebx], edx
		jnz end

		mov dword ptr[result], 1		; success of exchange

		end:
	}

	return result;
}

#endif