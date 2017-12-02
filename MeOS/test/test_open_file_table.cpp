#include "test_open_file_table.h"
#include "../vfs.h"
#include "../file.h"
#include "../thread_sched.h"

bool test_open_file_table_open()
{
	vfs_node* file;

	serial_printf("Starting open file table test\n");

	// OPEN KEYBOARD FILE TWICE
	if (vfs_root_lookup("dev/keyboard", &file) == ERROR_OCCUR)
		FAIL("Could not open test file: %e\n");

	gfe entry;
	entry = create_gfe(file);

	uint32 kybd_index = gft_insert_s(entry);

	if (kybd_index == -1)
		FAIL("Could not insert keyboard entry: %e\n");

	serial_printf("keyboard file inserted once\n");
	gft_print();

	kybd_index = gft_insert_s(entry);

	if (kybd_index == -1)
		FAIL("Could not insert keyboard entry: %e\n");

	serial_printf("keyboard file inserted once twice\n");
	gft_print();

	// OPEN ROOT FILE ONCE

	entry = create_gfe(vfs_get_root());
	uint32 root_index = gft_insert_s(entry);

	if (root_index == -1)
		FAIL("Could not insert root entry: %e\n");

	serial_printf("root file inserted once\n");
	gft_print();

	// CLOSE KEYBOARD FILE ONCE

	if (gfe_decrement_open_count(kybd_index) == ERROR_OCCUR)
		FAIL("Could not close keyboard once: %e\n");

	serial_printf("keyboard file removed once\n");
	gft_print();

	RET_SUCCESS
}

bool test_vfs_open_file()
{
	if (vfs_open_file(vfs_get_root(), VFS_CAP_READ) != ERROR_OK)
	{
		uint32 err = get_raw_error();

		FAIL("Could not open root file: %e\n");
	}

	serial_printf("root file opened\n");
	gft_print();

	RET_SUCCESS
}

bool test_open_file()
{
	// OPEN DEV TWICE
	uint32 fd, fd2;
	if (open_file("dev", &fd, VFS_CAP_READ) != ERROR_OK)
		FAIL("Could not open dev file: %e\n");

	serial_printf("dev file opened\n");
	gft_print();

	serial_printf("\n");

	lft_print(&process_get_current()->lft);


	if(open_file("dev", &fd2, VFS_CAP_READ) != ERROR_OK)
		FAIL("Could not open dev file: %e\n");

	serial_printf("dev file opened\n");
	gft_print();

	serial_printf("\n");

	lft_print(&process_get_current()->lft);

	RET_SUCCESS;
}

// test reading a keyboard character
// keyboard read requires threading to block
bool test_read_file()
{
	// OPEN KEYBOARD
	uint32 fd;
	if (open_file("dev/keyboard", &fd, VFS_CAP_READ) != ERROR_OK)
		FAIL("Could not open keyboard file: %e\n");

	serial_printf("gft: \n");
	gft_print();

	serial_printf("\nlft: \n");

	lft_print(&process_get_current()->lft);

	serial_printf("opened index is: %u\n", fd);
	serial_printf("reading 5 characters from keyboard\n");

	char c[6] = {0};

	if (read_file(fd, 0, 5, (virtual_addr)&c) != 5)
		FAIL("Keyboard read failed: %e\n");

	serial_printf("read: %s\n", c);

	RET_SUCCESS;
}

uint8 test_buffer_read[4096] = { 0 };
uint8 test_buffer_write[4096] = { 0 };

bool test_read_file_cached()
{
	serial_printf("page cache: \n");
	page_cache_print();

	uint32 fd;
	if (open_file("dev/test_dev", &fd, VFS_CAP_READ | VFS_CAP_CACHE) != ERROR_OK)
		FAIL("Could not open test device: %e\n");

	serial_printf("reading bytes 4096 + 512\n");

	if (read_file(fd, 4096, 512, (virtual_addr)test_buffer_read) != 512)
		FAIL("Could not read test device: %e\n");

	serial_printf("read 512 bytes\n");
	serial_printf("buffer[511] = %u\nbuffer[512] = %u\n", test_buffer_read[511], test_buffer_read[512]);

	serial_printf("page cache: \n");
	page_cache_print();

	serial_printf("reading once first 4KB\n");

	if (read_file(fd, 0, 4096, (virtual_addr)test_buffer_read) != 4096)
		FAIL("Could not read test device: %e\n");

	serial_printf("reading bytes 4096 + 512 * 2\n");

	page_cache_print();

	RET_SUCCESS;
}

bool test_write_file_cached()
{
	uint32 fd;
	if (open_file("dev/test_dev", &fd, VFS_CAP_WRITE | VFS_CAP_READ | VFS_CAP_CACHE) != ERROR_OK)
		FAIL("Could not open test device: %e\n");

	for (int i = 0; i < 512; i++)
		test_buffer_write[i] = 255 - i;

	serial_printf("writing 512 bytes at first 'page'\n");

	if (write_file(fd, 0, 512, (virtual_addr)test_buffer_write) != 512)
		FAIL("Could not write test dev: %e\n");

	serial_printf("reading 4096 bytes from first page (not fetched from actual driver)\n");

	if (read_file(fd, 0, 4096, (virtual_addr)test_buffer_read) != 4096)
		FAIL("Could not read test dev: %e\n");

	serial_printf("page cache: \n");
	page_cache_print();

	// expect buffer[513] = 0 even though actual data is different as we have written 512 zero-padded bytes first !
	// buffer[512] should equal 0 but test_dev already places a zero there and this cannot be checked.
	serial_printf("buffer[510] = %u\nbuffer[511] = %u\nbuffer[512] = %u\nbuffer[513] = %u\n", test_buffer_read[510], test_buffer_read[511], test_buffer_read[512], test_buffer_read[513]);

	serial_printf("old test dev file length: %u\n", gft_get(gft_get_by_fd(fd))->file_node->file_length);
	serial_printf("writing past the end of file\n");

	if (write_file(fd, 4 * 4096, 512, (virtual_addr)test_buffer_write) != 512)
		FAIL("Could not write test dev: %e\n");

	serial_printf("new test dev file length: %u\n", gft_get(gft_get_by_fd(fd))->file_node->file_length);

	RET_SUCCESS;
}

bool test_write_with_dirty()
{
	uint32 fd;
	if (open_file("dev/test_dev", &fd, VFS_CAP_WRITE | VFS_CAP_READ | VFS_CAP_CACHE) != ERROR_OK)
		FAIL("Could not open test device: %e\n");

	for (int i = 0; i < 512; i++)
		test_buffer_write[i] = 255 - i;

	serial_printf("writing 512 bytes at first 'page'\n");

	if (write_file(fd, 0, 512, (virtual_addr)test_buffer_write) != 512)
		FAIL("Could not write test dev: %e\n");

	if (read_file(fd, 4096, 512, (virtual_addr)test_buffer_read) != 512)
		FAIL("Could not read test dev: %e\n");

	// on sync expect to see only ONE test dev write (although there are two pages read)

	// uncomment and expect to see no test dev write
	// page_cache_make_dirty(gft_get_by_fd(fd), 0, false);

	RET_SUCCESS;
}

bool test_sync_file()
{
	uint32 fd;
	if (open_file("dev/test_dev", &fd, VFS_CAP_WRITE | VFS_CAP_READ | VFS_CAP_CACHE) != ERROR_OK)
		FAIL("Could not open test device: %e\n");

	serial_printf("syncing test device\n");

	if (sync_file(fd, 0, 0) != ERROR_OK)
		FAIL("Could not sync test device: %e\n");

	// expect to see test dev write

	RET_SUCCESS;
}