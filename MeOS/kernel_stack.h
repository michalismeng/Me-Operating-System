#ifndef KERNEL_STACK_H_08102017
#define KERNEL_STACK_H_08102017

/* 
	Management for the kernel stacks

	The kernel stack is a 4KB memory area allocated per thread.
	Currently each kernel stack is part of the page cache memory utility
*/

#include "types.h"

// reserves a 4 KB region to be used as a kernel stack and returns the stack top.
virtual_addr kernel_stack_reserve();

// releases the allocated kernel stack, given its top address.
error_t kernel_stack_release(virtual_addr address);

#endif