#include "udp.h"
#include "ip.h"
#include "print_utility.h"

uint16 udp_checksum(udp_header* header)
{
	return 0;
}

udp_header* udp_create(sock_buf* buffer, uint16 src_port, uint16 dest_port, uint16 data_len)
{
	udp_header* udp = (udp_header*)buffer->data;
	udp->csum = 0;

	udp->src_port = htons(src_port);
	udp->dest_port = htons(dest_port);
	udp->len = htons(sizeof(udp_header) + data_len);

	sock_buf_push(buffer, sizeof(udp_header));

	return udp;
}

error_t udp_send(sock_buf* buffer)
{
	//sock_buf_pop(buffer, sizeof(udp_header));
	ipv4_send(buffer);

	return ERROR_OK;
}

uint32 udp_recved = 0;


error_t udp_recv(sock_buf* buffer)
{
	udp_header* udp = (udp_header*)buffer->data;

	// save the source and destination ports
	memcpy(buffer->src_addrs[2].addr, (uint8*)udp->src_port, 2);
	memcpy(buffer->dst_addrs[2].addr, (uint8*)udp->dest_port, 2);

	if (ntohs(udp->dest_port) == 12345)
		udp_recved++;

	//printfln("udp from: %u", ntohs(udp->src_port));

	sock_buf_push(buffer, sizeof(udp_header));

	// pass the sock_buf to the user bound socket
	/*printfln("message:");
	for (uint32 i = 0; i < (uint8*)buffer->tail - (uint8*)buffer->data; i++)
		printf("%c", ((uint8*)buffer->data)[i]);*/

	return ERROR_OK;
}

error_t init_udp(uint32 layer)
{
	net_operations ops;
	ops.recv = udp_recv;
	ops.send = udp_send;

	return net_layer_register_proto(layer, net_protocol_create(17, ops));
}
