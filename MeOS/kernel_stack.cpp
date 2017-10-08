#include "kernel_stack.h"
#include "page_cache.h"

virtual_addr kernel_stack_reserve()
{
	virtual_addr address = page_cache_reserve_anonymous();
	if (address == 0)
		return 0;

	return address + PAGE_SIZE;		// increment by page size as this is the correct esp location
}

error_t kernel_stack_release(virtual_addr address)
{
	page_cache_release_anonymous(address - PAGE_SIZE);
	return ERROR_OK;
}
