#include "thread_exception.h"
#include "process.h"
#include "queue_lf.h"
#include "print_utility.h"

//process_exception process_exception_create(uint8 number, uint8* data, uint8 data_count, uint32 target_tid)
//{
//	process_exception exception;
//	exception.exception_number = number;
//	exception.target_thread = target_tid;
//
//	for (int i = 0; i < 3; i++)
//		exception.data[i] = 0;
//
//	for (int i = 0; i < data_count; i++)
//		exception.data[i] = data[i];
//
//	return exception;
//}
//
//bool process_exception_insert(PCB* process, process_exception& exception)
//{
//	return queue_lf_insert(&process->exceptions, exception);
//}
//
//process_exception process_exception_get(PCB* process)
//{
//	process_exception exception = queue_lf_peek(&process->exceptions);
//	//if(queue_lf_remove())
//	return exception;
//}

void thread_exception_print(thread_exception* pe)
{
	serial_printf("exception no: %u, target thread: %u\n", pe->exception_number, pe->target_thread->id);
}
