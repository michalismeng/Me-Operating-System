#include "test_AHCI.h"
#include "../file.h"
#include "../thread_sched.h"
#include "../process.h"
#include "../kernel_stack.h"

uint8 buf[4096] = { 1 };

uint8 test_buf1[4096] = { 1 };
uint8 test_buf2[4096] = { 1 };

uint32 fd = 1;

spinlock sp;

void test_ahci_read_second_thread()
{
	serial_printf("executing 2\n");

	if (read_file(fd, 6, 8, (virtual_addr)test_buf2) != 8)
		PANIC("Could not read sdc\n");

	while (true)
		if (spinlock_try_acquire(&sp))
			break;

	serial_printf("read 2\n");

	/*for (int i = 0; i < 512; i++)
		serial_printf("%c", test_buf2[i]);

	serial_printf("\n");*/

	for (int i = 0; i < 512; i++)
	{
		if (test_buf2[i] != buf[512 + i])
		{
			serial_printf("read 2 not equal at: %u\n", i);

			for (int i = 0; i < 512; i++)
				serial_printf("%c", test_buf2[i]);
			serial_printf("\n");
			break;
		}
	}
		

	serial_printf("\n");


	spinlock_release(&sp);


	while (true);
}

bool test_ahci_read()
{
	if (open_file("dev/sdc", &fd, VFS_CAP_READ) != ERROR_OK)
		FAIL("Could not open sdc: %e\n");

	if (read_file(fd, 0, 1, (virtual_addr)buf) != 1)
		FAIL("Could not read sdc: %e\n");

	if (read_file(fd, 6, 1, (virtual_addr)(buf + 512)) != 1)
		FAIL("Could not read sdc: %e\n");

	serial_printf("read stuff:\n");
	spinlock_init(&sp);

	//for (int i = 0; i < 1024; i++)
	//	serial_printf("%c", buf[i]);

	virtual_addr stack = kernel_stack_reserve();
	if (stack == 0)
		FAIL("Could not allocate stack: %e\n");

	serial_printf("testing two reads\n\n");

	TCB* thread = thread_create(process_get_current(), (uint32)test_ahci_read_second_thread, stack, 4096, 3, 0);
	INT_OFF;
	thread_insert(thread);
	INT_ON;

	serial_printf("executing 1\n");


	if (read_file(fd, 0, 8, (virtual_addr)test_buf1) != 8)
		FAIL("Could not read sdc: %e\n");

	while (true)
		if (spinlock_try_acquire(&sp))
			break;

	serial_printf("read 1\n");

	/*for (int i = 0; i < 512; i++)
		serial_printf("%c", test_buf1[i]);

	serial_printf("\n");*/

	for (int i = 0; i < 512; i++)
	{
		if (test_buf1[i] != buf[i])
		{
			serial_printf("read 1 not equal at: %u\n", i);
			for (int i = 0; i < 512; i++)
				serial_printf("%c", test_buf1[i]);

			serial_printf("\n");
			break;
		}		
	}
		

	serial_printf("\n");

	spinlock_release(&sp);

	while (true);

	RET_SUCCESS;
}
