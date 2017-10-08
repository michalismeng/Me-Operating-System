#include "memory.h"
#include "spinlock.h"
#include "thread_sched.h"
#include "print_utility.h"
#include "critlock.h"

extern heap* kernel_heap;
spinlock kernel_heap_lock = 0;

void* malloc(uint32 size)
{
	/*if (kernel_heap_lock)
		PANIC("HEAP ACQUIRED");*/

	spinlock_acquire(&kernel_heap_lock);
	//critlock_acquire();
	void* addr = heap_alloc(kernel_heap, size);
	//critlock_release();
	spinlock_release(&kernel_heap_lock);

	return addr;
}

void* calloc(uint32 size)
{
	void* ptr = malloc(size);
	memset(ptr, 0, size);
	return ptr;
}

error_t free(void* ptr)
{
	spinlock_acquire(&kernel_heap_lock);
	//critlock_acquire();
	error_t res = heap_free(kernel_heap, ptr);
	//critlock_release();
	spinlock_release(&kernel_heap_lock);

	return res;
}

void* realloc(void* ptr, uint32 new_size)
{
	spinlock_acquire(&kernel_heap_lock);
	//critlock_acquire();
	void* addr = heap_realloc(kernel_heap, ptr, new_size);
	//critlock_release();
	spinlock_release(&kernel_heap_lock);

	return addr;
}

virtual_addr vfs_mmap_p(void* _proc, virtual_addr pref, uint32 gfd, uint32 offset, uint32 length, uint32 prot, uint32 flags)
{
	PCB* proc = (PCB*)_proc;

	if (prot > 0xF)		// protection flags failed
	{
		set_last_error(EINVAL, лелоRу_BAD_PROTECTION, EO_MEMORY);
		return MAP_FAILED;
	}

	if (!CHK_BIT(flags, MMAP_SHARED) && !CHK_BIT(flags, MMAP_PRIVATE))
	{
		set_last_error(EINVAL, MEMORY_BAD_FLAGS, EO_MEMORY);
		return MAP_FAILED;
	}

	vm_area area = vm_area_create(pref, pref + length, flags | prot, gfd, offset);

	if (area.flags == MMAP_INVALID)
		return MAP_FAILED;

	spinlock_acquire(&proc->contract_spinlock);

	if (vm_contract_add_area(&proc->memory_contract, &area) != ERROR_OK)	// TODO: Check flag for obligatory prefered addr load
		return MAP_FAILED;

	spinlock_release(&proc->contract_spinlock);
	return area.start_addr;
}

virtual_addr vfs_mmap(virtual_addr pref, uint32 gfd, uint32 offset, uint32 length, uint32 prot, uint32 flags)
{
	return vfs_mmap_p(process_get_current(), pref, gfd, offset, length, prot, flags);
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