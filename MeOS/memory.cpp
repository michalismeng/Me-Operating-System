#include "memory.h"
#include "spinlock.h"
#include "thread_sched.h"

extern heap* kernel_heap;
spinlock kernel_heap_lock = 0;

void* malloc(uint32 size)
{
	spinlock_acquire(&kernel_heap_lock);
	void* addr = heap_alloc(kernel_heap, size);
	spinlock_release(&kernel_heap_lock);

	return addr;
}

void* calloc(uint32 size)
{
	void* ptr = malloc(size);
	memset(ptr, 0, size);
	return ptr;
}

void free(void* ptr)
{
	spinlock_acquire(&kernel_heap_lock);
	heap_free(kernel_heap, ptr);
	spinlock_release(&kernel_heap_lock);
}

void* realloc(void* ptr, uint32 new_size)
{
	spinlock_acquire(&kernel_heap_lock);
	void* addr = heap_realloc(kernel_heap, ptr, new_size);
	spinlock_release(&kernel_heap_lock);

	return addr;
}

virtual_addr vfs_mmap_p(void* _proc, virtual_addr pref, uint32 gfd, uint32 offset, uint32 length, uint32 flags, uint32 prot)
{
	PCB* proc = (PCB*)_proc;

	if (prot > 0xF)		// protection flags failed
		return MAP_FAILED;

	if (!CHK_BIT(flags, MMAP_SHARED) && !CHK_BIT(flags, MMAP_PRIVATE))
		return MAP_FAILED;

	vm_area area = vm_area_create(pref, pref + length, flags | prot, gfd, offset);

	if (area.flags == MMAP_INVALID)
		return MAP_FAILED;

	spinlock_acquire(&proc->contract_spinlock);

	if (!vm_contract_add_area(&proc->memory_contract, &area))	// TODO: Check flag for obligatory preffered addr load
		return MAP_FAILED;

	spinlock_release(&proc->contract_spinlock);

	// from current_process
	// from local_table get global fd through
	// increase the open_count
	return area.start_addr;
}

virtual_addr vfs_mmap(virtual_addr pref, uint32 gfd, uint32 offset, uint32 length, uint32 flags, uint32 prot)
{
	if (prot > 0xF)		// protection flags failed
		return MAP_FAILED;

	if (!CHK_BIT(flags, MMAP_SHARED) && !CHK_BIT(flags, MMAP_PRIVATE))
		return MAP_FAILED;

	vm_area area = vm_area_create(pref, pref + length, flags | prot, gfd, offset);

	if (area.flags == MMAP_INVALID)
		return MAP_FAILED;

	spinlock_acquire(&process_get_current()->contract_spinlock);

	if (!vm_contract_add_area(&process_get_current()->memory_contract, &area))	// TODO: Check flag for obligatory preffered addr load
		return MAP_FAILED;

	spinlock_release(&process_get_current()->contract_spinlock);

	// from current_process
	// from local_table get global fd through
	// increase the open_count
	return area.start_addr;
}

#ifdef __cplusplus

void* operator new(uint32 size)
{
	return  malloc(size);
}

void* operator new[](uint32 size)
{
	return  malloc(size);
}

void operator delete(void* ptr)
{
	free(ptr);
}

void operator delete(void* ptr, unsigned int)
{
	free(ptr);
}

void operator delete[](void* ptr)
{
	free(ptr);
}

#endif