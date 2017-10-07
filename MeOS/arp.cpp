#include "arp.h"
#include "ethernet.h"
#include "memory.h"
#include "print_utility.h"
#include "i217.h"

#include "timer.h"

extern e1000* nic_dev;

uint8 my_ip[4] = { 192, 168, 1, 30 };

bool protocol_addr_equal(uint8* proto1, uint8* proto2, uint8 len)
{
	for (uint8 i = 0; i < len; i++)
		if (proto1[i] != proto2[i])
			return false;

	return true;
}

void arp_receive_ipv4(sock_buf* buffer)
{
	arp_header* arp = (arp_header*)buffer->data;
	arp_ipv4* arp4 = (arp_ipv4*)arp->data;

	bool merged = false;

	// setup buffer header addresses
	memcpy(buffer->src_addrs[1].addr, arp4->src_ip, 4);
	memcpy(buffer->dst_addrs[1].addr, arp4->dest_ip, 4);

	// check arp cache for <ipv4, src_ip>
	// if exists => merged = true + upddate the cache with src_mac
	printfln("arp proc: %u", millis());

	// check dest_ip with my own
	if (protocol_addr_equal(arp4->dest_ip, my_ip, 4))
	{
		// if not merged add into cache <ipv4, src_ip, src_mac>
		if (ntohs(arp->opcode) == ARP_REQ)		// if this is a request then a reply is needed
		{
			// swap src and dest hardware/protocol addressed and send arp_reply

			// reuse the same sock buffer to send the reply
			sock_buf_reset(buffer);
			eth_create(buffer, buffer->src_addrs[0].addr, nic_dev->mac, 0x0806);
			arp_create(buffer, HW_ETHER, PROTO_IPv4, 6, 4, ARP_REP, nic_dev->mac, buffer->dst_addrs[1].addr,
																	buffer->src_addrs[0].addr, buffer->src_addrs[1].addr);		

			/*printfln("arping to: %u.%u.%u.%u",
				buffer->src_addrs[1].addr[0], buffer->src_addrs[1].addr[1], buffer->src_addrs[1].addr[2], buffer->src_addrs[1].addr[3]);*/

			arp_send(buffer);
		}
	}	
}

// TODO: add errors
error_t arp_recv(sock_buf* buffer)
{
	arp_header* arp = (arp_header*)buffer->data;

	// check hardware supprt
	if (arp->hw_type != htons(HW_ETHER))
		return ERROR_OCCUR;

	uint16 prot_type = ntohs(arp->prot_type);

	// the arp receive functions require the arp header. so do not push sock buf
	// sock_buf_push(buffer, sizeof(arp_header));

	switch (prot_type)
	{
	case PROTO_IPv4:
		if (arp->prot_len != 4)		// validate protocol length
			return ERROR_OCCUR;

		arp_receive_ipv4(buffer);
		break;
	default:
		return ERROR_OK;
	}

	return ERROR_OK;
}

void arp_create(sock_buf* buffer, uint16 hw_type, uint16 prot_type, uint8 hw_len, uint8 prot_len, uint16 opcode, uint8* src_hw, uint8* src_prot, 
	uint8* dest_hw, uint8* dest_prot)
{
	arp_header* arp = (arp_header*)buffer->data;

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

	uint16 arp_size = sizeof(arp_header) + 2 * arp->hw_len + 2 * arp->prot_len;
	if(buffer)
	sock_buf_push(buffer, arp_size);
}

error_t arp_send(sock_buf* buffer)
{
	eth_send(buffer);
	return ERROR_OK;
}

error_t init_arp(uint32 layer)
{
	net_operations ops;
	ops.recv = arp_recv;
	ops.send = arp_send;

	return net_layer_register_proto(layer, net_protocol_create(0x0806, ops));
}
