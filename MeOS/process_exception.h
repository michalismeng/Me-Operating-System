#ifndef PROCESS_EXCEPTION_H_25072017
#define PROCESS_EXCEPTION_H_25072017

#include "types.h"

extern struct thread_control_block;

struct process_exception
{
	uint8 exception_number;
	uint32 data[2];
	struct thread_control_block* target_thread;
};

//process_exception process_exception_create(uint8 number, uint8* data, uint8 data_count, uint32 target_tid);
//bool process_exception_insert(PCB* process, process_exception& exception);
//process_exception process_exception_get(PCB* process);

void process_exception_print(struct process_exception* pe);


#endif