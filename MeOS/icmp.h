#ifndef ICMP_H_05102017
#define ICMP_H_05102017

#include "net.h"
#include "sock_buf.h"
#include "net_protocol.h"

struct icmp_header
{
	uint8 type;
	uint8 code;
	uint16 csum;
};

error_t icmp_send(sock_buf* buffer);
error_t icmp_recv(sock_buf* buffer);

error_t init_icmp(uint32 layer);

#endif