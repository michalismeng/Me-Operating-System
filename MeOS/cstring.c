#include "cstring.h"

uint16 strlen(const char* ch)		// return the number of characters the string has, not including the trailing \0
{
	uint16 i = 0;
	while (ch[i] != '\0')
		i++;

	return i;
}

int8 strcmp(const char* s1, const char* s2)
{
	int8 res = 0;

	while (!(res = *s1 - *s2) && *s2)
		s1++, s2++;

	if (res < 0)
		res = -1;
	else if (res > 0)
		res = 1;

	return res;
}