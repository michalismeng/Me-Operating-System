#include "net.h"

#ifdef		LITTLE_ENDIAN

uint32 htonl(uint32 host)
{
	uint32 res;

	_asm
	{
		mov eax, host
		bswap eax
		mov res, eax
	}

	return res;
}

uint16 htons(uint16 host)
{
	return ((host << 8) | (host >> 8));
}

uint32 ntohl(uint32 net)
{
	uint32 res;

	_asm
	{
		mov eax, net
		bswap eax
		mov res, eax
	}

	return res;
}

uint16 ntohs(uint16 net)
{
	return ((net << 8) | (net >> 8));
}

#else		// assume BIG_ENDIAN

uint32 htonl(uint32 host)
{
	return host;
}

uint16 htons(uint16 host)
{
	return host;
}

uint32 ntohl(uint32 net)
{
	return net;
}

uint16 ntohs(uint16 net)
{
	return net;
}

#endif