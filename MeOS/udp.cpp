#include "udp.h"

uint16 udp_checksum(udp_header* header)
{
	return 0;
}

udp_header* udp_create(virtual_addr header, uint16 src_port, uint16 dest_port, uint16 data_len)
{
	udp_header* udp = (udp_header*)header;
	udp->csum = 0;

	udp->src_port = htons(src_port);
	udp->dest_port = htons(dest_port);
	udp->len = htons(sizeof(udp_header) + data_len);

	return udp;
}