#ifndef MEMORY_H_16082016
#define MEMORY_H_16082016

#include "mmngr_heap.h"
#include "vm_area.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include "utility.h"

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

// maps the portion of the file described by fd (local table) into the calling processes memory
// pref if not nulled is the preffered address to load the start-length file data
virtual_addr mmap(virtual_addr pref, uint32 gfd, uint32 offset, uint32 length, uint32 flags, uint32 prot);

#endif
