#include "arp.h"
#include "ethernet.h"
#include "memory.h"
#include "i217.h"

extern e1000* nic_dev;

uint8 my_ip[4] = { 192, 168, 1, 30 };

bool protocol_addr_equal(uint8* proto1, uint8* proto2, uint8 len)
{
	for (uint8 i = 0; i < len; i++)
		if (proto1[i] != proto2[i])
			return false;

	return true;
}

void arp_receive_ipv4(arp_header* arp)
{
	bool merged = false;
	arp_ipv4* arp4 = (arp_ipv4*)arp->data;

	// check arp cache for <ipv4, src_ip>
	// if exists => merged = true + upddate the cache with src_mac

	// check dest_ip with my own
	if (protocol_addr_equal(arp4->dest_ip, my_ip, 4))
	{
		// if not merged add into cache <ipv4, src_ip, src_mac>
		if (ntohs(arp->opcode) == ARP_REQ)		// if this is a request then a reply is needed
		{
			// swap src and dest hardware/protocol addressed
			// and send arp_reply
			/*uint32 arp_size = sizeof(arp_header) + 2 * arp->hw_len + 2 * arp->prot_len;
			virtual_addr buffer = (virtual_addr)calloc(sizeof(eth_header) + arp_size);
			eth_header* eth = eth_create(buffer, arp4->src_mac, nic_dev->mac, 0x0806);
			arp_create((virtual_addr)eth->eth_data, HW_ETHER, PROTO_IPv4, 6, 4, ARP_REP, nic_dev->mac, arp4->dest_ip, arp4->src_mac, arp4->src_ip);
			eth_send(eth, arp_size);
			free((void*)buffer);*/
		}
	}	
}

void arp_recv(arp_header* arp)
{
	// check hardware supprt
	if (arp->hw_type != htons(HW_ETHER))
		return;

	switch (ntohs(arp->prot_type))
	{
	case PROTO_IPv4:
		if (arp->prot_len != 4)		// validate protocol length
			return;

		arp_receive_ipv4(arp);
		break;
	default:
		return;
	}
}

void arp_create(virtual_addr header, uint16 hw_type, uint16 prot_type, uint8 hw_len, uint8 prot_len, uint16 opcode, uint8* src_hw, uint8* src_prot, 
	uint8* dest_hw, uint8* dest_prot)
{
	arp_header* arp = (arp_header*)header;
	arp->hw_type = htons(hw_type);
	arp->prot_type = htons(prot_type);
	arp->hw_len = hw_len;
	arp->prot_len = prot_len;
	arp->opcode = htons(opcode);

	uint8* ptr = (uint8*)arp->data;
	memcpy(ptr, src_hw, hw_len);
	ptr += hw_len;

	memcpy(ptr, src_prot, prot_len);
	ptr += prot_len;

	memcpy(ptr, dest_hw, hw_len);
	ptr += hw_len;

	memcpy(ptr, dest_prot, prot_len);
}

void arp_send(arp_header* arp)
{
	// total length of variable header is twice the hardware and protocol address lengths
	/*uint16 arp_size = sizeof(arp_header) + 2 * arp->hw_len + 2 * arp->prot_len;	
	eth_header* eth = eth_create((virtual_addr)arp - sizeof(eth_header), );
	eth_send(eth, arp_size);*/
}
