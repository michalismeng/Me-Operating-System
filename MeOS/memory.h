#ifndef MEMORY_H_16082016
#define MEMORY_H_16082016

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include "utility.h"
#include "mmngr_heap.h"

	// allocates size contiguous memory
	void* malloc(uint32 size);

	// allocates size contiguous memory and initializes to zero
	void* calloc(uint32 size);

	// frees an allocated memory
	void free(void* ptr);

	// re-allocates the given memory to now hold 'new_size' bytes
	void* realloc(void* ptr, uint32 new_size);

#ifdef __cplusplus
}
#endif

#endif
