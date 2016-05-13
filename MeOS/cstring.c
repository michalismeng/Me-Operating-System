#include "cstring.h"

uint16 strlength(const char* ch)		// return the number of characters the string has, not including the trailing \0
{
	uint16 i = 0;
	while (ch[i] != '\0')
		i++;

	return i;
}

uint8 strEqual(char* s1, char* s2)
{
	uint8 result = 1;
	uint16 size = strlength(s1);

	if (size != strlength(s2))
		result = 0;
	else
	{
		uint16 i = 0;
		for (i; i < size; i++)
		{
			if (s1[i] != s2[i])
			{
				result = 0;
				break;
			}
		}
	}

	return result;
}