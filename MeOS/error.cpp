#include "error.h"

uint32 error_create(uint32 value)
{
	return value;  // TODO: further adjustments to globalify errors
}

void set_last_error(uint32 error)
{
	*thread_get_error(thread_get_current()) = error;
}

uint32 get_last_error()
{
	return *thread_get_error(thread_get_current());
}