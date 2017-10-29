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

