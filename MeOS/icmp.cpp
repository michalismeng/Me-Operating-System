#include "icmp.h"
#include "ethernet.h"
#include "ip.h"

error_t icmp_send(sock_buf* buffer)
{
	return error_t();
}

error_t icmp_recv(sock_buf* buffer)
{
	icmp_header* icmp = (icmp_header*)buffer->data;

	if (icmp->type == 8)		// ICMP Request
	{

	}

	return ERROR_OK;
}

error_t init_icmp(uint32 layer)
{
	net_operations ops;
	ops.recv = icmp_recv;
	ops.send = icmp_send;

	return net_layer_register_proto(layer, net_protocol_create(1, ops));
}
