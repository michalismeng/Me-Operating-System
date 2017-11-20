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

int8 strcmp_insensitive(const char* s1, const char* s2)
{
	int8 res = 0;

	while (!(res = tolower(*s1) - tolower(*s2)) && *s2)
		s1++, s2++;

	if (res < 0)
		res = -1;
	else if (res > 0)
		res = 1;

	return res;
}

void strcpy(char* destination, const char* source)
{
	char* s = source;

	while (*destination = *s)
		destination++, s++;
}

uint8 strcpy_s(char* destination, uint16 destsz, const char* source)
{
	uint16 len = strlen(source);

	// TODO: More error-checking.
	if (len > destsz)
		return 1;

	strcpy(destination, source);
	return 0;
}

char* strchr(const char* str, int c)
{
	while (*str != 0)
		if (*str++ == c)
			return str - 1;

	return 0;		// entry not found
}

char* strupper(char* str)
{
	char* temp = str;

	while (*str != 0)
	{
		*str = toupper(*str);
		str++;
	}

	return temp;
}