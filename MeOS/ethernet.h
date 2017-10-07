#ifndef ETHERNET_H_15092017
#define ETHERNET_H_15092017

#include "net.h"
#include "sock_buf.h"

enum ETH_TYPE
{
	ETH_TYPE_ARP	= 0x0806,			// arp data type
	ETH_TYPE_IPv4	= 0x0800,			// ipv4 data type
	ETH_TYPE_IPv6	= 0x86DD			// ipv6 data type
};

#pragma pack(push, 1)

struct eth_header
{
	uint8 dest_mac[6];
	uint8 src_mac[6];
	uint16 eth_type;
	void* eth_data[];
};

#pragma pack(pop, 1)

void eth_print(eth_header* header);
void eth_serial_print(eth_header* header);
bool eth_cmp_mac(uint8* mac1, uint8* mac2);

static uint8 mac_broadcast[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

// creates an ethernet packet at the given address and returns the created header
eth_header* eth_create(sock_buf* buffer, uint8* dest_mac, uint8* src_mac, uint16 eth_type);

void eth_send(sock_buf* eth);

void eth_recv(sock_buf* eth);

#endif