#ifndef THREAD_EXCEPTION_H_25072017
#define THREAD_EXCEPTION_H_25072017

#include "types.h"

extern struct thread_control_block;

struct thread_exception
{
	uint8 exception_number;
	uint32 data[2];
	struct thread_control_block* target_thread;
};

//process_exception process_exception_create(uint8 number, uint8* data, uint8 data_count, uint32 target_tid);
//bool process_exception_insert(PCB* process, process_exception& exception);
//process_exception process_exception_get(PCB* process);

void thread_exception_print(struct thread_exception* pe);


#endif