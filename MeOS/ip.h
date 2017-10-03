#ifndef IP_H_18092017
#define IP_H_18092017

#include "types.h"
#include "net.h"
#include "sock_buf.h"

enum IPV4_FLAGS
{
	IPV4_DONT_FRAGMENT = 2,
	IPV4_MORE_FRAGMENTS = 1
};

#pragma pack(push, 1)

struct ipv4
{
	uint8 ihl : 4;				// ip header length (including data field)
	uint8 ver : 4;				// ip version (always 4 for IPv4)

	uint8 ecn : 2;				// explicit congestion notification
	uint8 dscp : 6;				// type of Service

	uint16 len;					// total header and data length
	uint16 id;					// packet identifier (for fragmentation purposes)

	uint16 frag_flags;			// fragment offset and flags (13 and 3 bits respectively)
	
	uint8 ttl;					// packet time to live
	uint8 protocol;				// packet protocol
	uint16 csum;				// checksum

	uint8 src_ip[4];			// source ip
	uint8 dest_ip[4];			// destinatnion ip

	void* opt_data[];			// ip options and data
};

#pragma pop(pop, 1)

// compute ipv4 checksum
uint16 ipv4_checksum(ipv4* header);

// creates an ip packet given the variable sized options and returns it
ipv4* ipv4_create(sock_buf* buffer, uint8 ecn, uint8 dscp, uint16 id, uint16 frag_offset, uint8 flags, uint8 ttl,
					uint8 protocol, uint8* src_ip, uint8* dest_ip, uint8* options, uint8 options_len, uint16 data_len);

// returns the address of the ip header to start placing data
//void* ipv4_get_data_addr(ipv4* header);

void ipv4_send(sock_buf* buffer);
void ipv4_recv(sock_buf* buffer);


#endif