#include "error.h"

void set_last_error(uint32 error)
{
	*thread_get_error(thread_get_current()) = error;
}

uint32 get_last_error()
{
	return *thread_get_error(thread_get_current());
}