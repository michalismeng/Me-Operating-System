#include "error.h"
#include "thread_sched.h"

// private functions

uint32 error_create(uint8 base_code, uint16 extended_code, uint8 code_origin)
{
	uint32 value = (((uint32)code_origin) << 24) | (((uint32)extended_code) << 8) | (base_code);		// properly format the error value
	return value;
}

const char* ERROR_ORIGIN_STR[] =
{
	"NO ERRORS",						// one dummy entry to match the above enum
	"VFS",
	"VMMNGR",
	"PMMNGR",
	"HMMNGR",
	"MEMORY",
	"FS",
	"MASS STORAGE FS",
	"MASS STORAGE DEV",
	"FILE INTERFACE",
	"VM AREA",
	"VM CONTRACT",
	"OPEN FILE TBL",
	"PAGE CACHE"
};

const char* BASE_ERROR_STR[] =
{
	"Unknown error"			  ,
	"Operation not permitted" ,
	"No such file or director",
	"No such process"		  ,
	"Interrupted system call" ,
	"Input / output error"	  ,
	"Device not configured"	  ,
	"Argument list too long"  ,
	"Exec format error"		  ,
	"Bad file descriptor"	  ,
	"No child processes"	  ,
};

// public functions

uint32 set_last_error(uint8 base_code, uint16 extended_code, uint8 code_origin)
{
	uint32 error = error_create(base_code, extended_code, code_origin);
	*thread_get_error(thread_get_current()) = error;

	return error;
}

uint32 get_last_error()
{
	uint32 temp = *thread_get_error(thread_get_current());
	clear_last_error();
	return temp;
}

uint32 get_raw_error()
{
	return *thread_get_error(thread_get_current());
}

void clear_last_error()
{
	*thread_get_error(thread_get_current()) = 0;
}