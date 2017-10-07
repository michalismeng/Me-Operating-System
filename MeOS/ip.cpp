#include "ip.h"
#include "print_utility.h"
#include "ethernet.h"
#include "udp.h"

uint8 data[20] = { 0x45, 0x00, 0x00, 0x30, 0x44, 0x22, 0x40, 0x00, 0x80, 0x06,
					0x00, 0x00, 0x8C, 0x7C, 0x19, 0xAC, 0xAE, 0x24, 0x1E, 0x2B };

uint16 ipv4_checksum(ipv4* header)
{
	uint16* ptr = (uint16*)header;
	uint8 len = 2 * header->ihl;
	uint32 sum = 0;

	for (uint8 i = 0; i < len; i++)
		sum += ptr[i];

	uint16 sum16 = (sum >> 16) + (sum & 0xffff);
	return ~sum16;
}

ipv4* ipv4_create(sock_buf* buffer, uint8 ecn, uint8 dscp, uint16 id, uint16 frag_offset,
	uint8 flags, uint8 ttl, uint8 protocol, uint8* src_ip, uint8* dest_ip, uint8* options, uint8 options_len, uint16 data_len)
{
	ipv4* ip = (ipv4*)buffer->data;

	ip->ver = 4;
	ip->ihl = (sizeof(ipv4) + options_len) / 4;

	ip->ecn = ecn;
	ip->dscp = dscp;
	ip->len = htons(sizeof(ipv4) + options_len + data_len);
	ip->id = htons(id);
	ip->frag_flags = htons((flags << 13) | (frag_offset & ~(0xE000)));

	ip->ttl = ttl;
	ip->protocol = protocol;
	
	memcpy(ip->src_ip, src_ip, 4);
	memcpy(ip->dest_ip, dest_ip, 4);
	memcpy(ip->opt_data, options, options_len);

	ip->csum = ipv4_checksum(ip);

	sock_buf_push(buffer, ip->ihl * 4);

	return ip; 
}

error_t ipv4_send(sock_buf* buffer)
{
	eth_send(buffer);
	return ERROR_OK;
}

error_t ipv4_recv(sock_buf* buffer)
{
	ipv4* ip = (ipv4*)buffer->data;

	// setup buffer header addresses
	memcpy(buffer->src_addrs[1].addr, ip->src_ip, 4);
	memcpy(buffer->dst_addrs[1].addr, ip->dest_ip, 4);

	uint8 proto = ip->protocol;

	//if (proto == 17)
	//{
	//	printf("ip from: %u", buffer->src_addrs[1].addr[0]);
	//	for (int i = 1; i < 4; i++)
	//		printf(".%u", buffer->src_addrs[1].addr[i]);

	//	printfln("");
	//}
	

	sock_buf_push(buffer, ip->ihl * 4);
	//printfln("proto: %u", proto);

	net_protocol* n_proto = net_layer_get_proto(TRANSPORT_LAYER, proto);
	if (n_proto == 0)
	{
		//DEBUG("IP PROTO NULL");
		return ERROR_OCCUR;
	}

	n_proto->ops.recv(buffer);

	return ERROR_OK;
}

error_t init_ipv4(uint32 layer)
{
	net_operations ops;
	ops.recv = ipv4_recv;
	ops.send = ipv4_send;

	return net_layer_register_proto(layer, net_protocol_create(0x0800, ops));
}

void* ipv4_get_data_addr(ipv4* header)
{
	return ((uint8*)header) + header->ihl * 4;
}