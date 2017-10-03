#include "udp.h"
#include "ip.h"

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

void udp_send(sock_buf* buffer)
{
	//sock_buf_pop(buffer, sizeof(udp_header));
	ipv4_send(buffer);
}

void udp_recv(sock_buf* buffer)
{
}
