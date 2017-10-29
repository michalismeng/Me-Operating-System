#include "test_page_cache.h"
#include "../thread_sched.h"

bool test_page_cache_reserve_anonymous()
{
	// RESERVE FIRST CACHE
	virtual_addr cache1 = page_cache_reserve_anonymous();
	if (cache1 == 0)
		FAIL("Page cache anonymous allocation error: %e\n");

	serial_printf("cache reserved at: %h\n", cache1);
	page_cache_print();

	// RESERVE SECOND CACHE
	virtual_addr cache2 = page_cache_reserve_anonymous();
	if (cache2 == 0)
		FAIL("Page cache anonymous allocation error: %e\n");

	serial_printf("cache reserved at: %h\n", cache1);
	page_cache_print();

	// RELEASE FIRST CACHE
	serial_printf("releasing the cache at: %h\n", cache1);

	page_cache_release_anonymous(cache1);
	page_cache_print();

	// RESERVE AGAIN FIRST CACHE
	cache1 = page_cache_reserve_anonymous();
	if (cache1 == 0)
		FAIL("Page cache anonymous allocation error: %e\n");
	
	serial_printf("cache reserved at: %h\n", cache1);
	page_cache_print();

	RET_SUCCESS;
}

bool test_page_cache_reserve()
{
	uint32 fd, gfd;
	if (open_file("dev/keyboard", &fd, VFS_CAP_READ) != ERROR_OK)
		FAIL("Could not open keyboard file: %e\n");

	serial_printf("keyboard file opened.\nAllocating page cache buffers...\n");

	gfd = process_get_current()->lft.entries[fd].gfd;

	// RESERVE THREE BUFFERS
	virtual_addr p1 = page_cache_reserve_buffer(gfd, 0);
	virtual_addr p2 = page_cache_reserve_buffer(gfd, 5);
	virtual_addr p3 = page_cache_reserve_buffer(gfd, 12);

	if (p1 == 0 || p2 == 0 || p3 == 0)
		FAIL("Could not reserve page cache buffer: %e\n");

	serial_printf("allocated page cache buffers: %h %h %h\n", p1, p2, p3);
	serial_printf("page cache: \n");
	page_cache_print();

	serial_printf("Releasing page cache buffer: %h at page 5\n", p2);

	// RELEASE THE SECOND BUFFER
	if (page_cache_release_buffer(gfd, 5) != ERROR_OK)
		FAIL("Could not release page cache buffer: %e\n");

	serial_printf("page cache: \n");
	page_cache_print();

	serial_printf("Releasing page cache buffers: %h %h at pages 0, 12\n", p1, p3);

	// RELEASE THE OTHER TWO BUFFERS
	if (page_cache_release_buffer(gfd, 0) != ERROR_OK || page_cache_release_buffer(gfd, 12) != ERROR_OK)
		FAIL("Could not release page cache buffer: %e\n");

	page_cache_print();

	// RELEASE A NON-EXISTING BUFFER
	//// uncomment to fail
	//if (page_cache_release_buffer(gfd, 0) != ERROR_OK)
	//	FAIL("Could not release page cache buffer: %e\n");

	RET_SUCCESS;
}

bool test_page_cache_find_buffer()
{
	uint32 fd, gfd;

	if(open_file("dev", &fd, VFS_CAP_READ) != ERROR_OK)
		FAIL("Could not open keyboard file: %e\n");

	gfd = gft_get_by_fd(fd);

	// RESERVE TWO BUFFERS
	virtual_addr p1 = page_cache_reserve_buffer(gfd, 0);
	virtual_addr p2 = page_cache_reserve_buffer(gfd, 1);

	if (p1 == 0 || p2 == 0)
		FAIL("Could not reserve page cache buffer: %e\n");

	serial_printf("allocated page cache buffers at: %h %h\n", p1, p2);

	// TRY TO FIND THREE BUFFERS, THE TWO ABOVE AND A NON-EXISTING ONE
	virtual_addr result1 = page_cache_get_buffer(gfd, 0);
	virtual_addr result2 = page_cache_get_buffer(gfd, 10);
	virtual_addr result3 = page_cache_get_buffer(gfd, 1);

	if (result1 == 0 || result3 == 0)
		FAIL("Could not get page cache buffer: %e\n");

	if (result2 != 0)
		FAIL("Got page cache buffer, but page was not registered.");

	serial_printf("Got page cache buffers at: %h %h %h\n", result1, result2, result3);

	RET_SUCCESS;
}