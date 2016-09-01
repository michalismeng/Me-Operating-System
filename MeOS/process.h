#ifndef PROCESS_H_28082016
#define PROCESS_H_28082016

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include "utility.h"
#include "mmngr_virtual.h"
#include "PE_Definitions.h"

#include "Simple_fs.h"		// to be removed

	enum PROCESS_STATE { PROCESS_SLEEP, PROCESS_ACTIVE };
	typedef PROCESS_STATE THREAD_STATE;

#define MAX_THREADS 5

	struct process;

	struct trapFrame
	{
		uint32 esp;
		uint32 ebp;
		uint32 eip;
		uint32 edi;
		uint32 esi;
		uint32 eax;
		uint32 ebx;
		uint32 ecx;
		uint32 edx;
		uint32 flags;
	};

	struct thread
	{
		process* parent;
		void* initialStack;
		void* stackLimit;
		void* kernelStack;
		uint32 priority;
		THREAD_STATE state;

		trapFrame frame;

		uint32 imageBase;
		uint32 imageSize;
	};

	struct process
	{
		uint32 id;
		uint32 priority;
		pdirectory* page_dir;
		PROCESS_STATE state;

		uint32 thread_count;
		thread threads[5];		// TODO: Make linked list
	};

	uint32 process_create(char* app_name);
	process* process_get_current();
	void process_execute();

	bool validate_PE_image(void* image);

#ifdef __cplusplus
}
#endif

#endif
