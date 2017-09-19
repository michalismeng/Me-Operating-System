#ifndef UDP_H_19092017
#define UDP_H_19092017

#include "types.h"
#include "utility.h"

#pragma pack(push, 1)

struct udp_header
{
	uint16 src_port;			// udp source port (required if sender expects reply)
	uint16 dest_port;			// udp destination port
	uint16 len;					// total length of udp header and data
	uint16 csum;				// udp checksum over the ip header, udp header, udp data

	void* data[];				// upd data
};

#pragma pack(pop, 1)

uint16 udp_checksum(udp_header* header);

// creates a udp header and returns it
udp_header* udp_create(virtual_addr header, uint16 src_port, uint16 dest_port, uint16 data_len);

#endif