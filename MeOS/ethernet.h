#ifndef ETHERNET_H_15092017
#define ETHERNET_H_15092017

#include "types.h"
#include "utility.h"

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
eth_header* eth_create(virtual_addr header, uint8* dest_mac, uint8* src_mac, uint16 eth_type);

void eth_send(eth_header* eth, uint32 data_size);

#endif