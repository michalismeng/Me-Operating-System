#ifndef KALLOC_H_20112016
#define KALLOC_H_20112016

// defines full-greedy routines for claiming memory without ever returning it

#include "types.h"

// initializes kallocation service with a limiting address
void init_kallocations(uint8* base, uint8* _max_addr);

// claim space that is never de-allocated
void* kalloc(uint32 size);

// claim space that is align-byte aligned and never de-allocated
void* kalloc_a(uint32 size, uint32 align);

// returns the current alloced pointer
uint8* kalloc_get_ptr();

#endif