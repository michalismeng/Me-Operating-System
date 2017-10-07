#ifndef ARP_H_17092017
#define ARP_H_17092017

#include "types.h"
#include "net.h"
#include "net_protocol.h"
#include "sock_buf.h"

/* hardware (link layer) type values */
#define HW_ETHER	0x1

/* protocol type values */
#define PROTO_IPv4	0x0800

/* Opcode values */
#define ARP_REQ		1
#define ARP_REP		2
#define RARP_REQ	3
#define RARP_REP	4

#pragma pack(push, 1)

// general address resolution protocol header
struct arp_header
{
	uint16 hw_type;			// hardware type
	uint16 prot_type;		// protocol type
	uint8 hw_len;			// hardware address length (Ethernet mac address = 6)
	uint8 prot_len;			// protocol address length (IPv4 = 4)
	uint16 opcode;			// arp operation ocde
	void* data[];			// arp data
};

// IPv4 arp data (6 byte mac addresses - 4 byte ip addresses)
struct arp_ipv4
{
	uint8 src_mac[6];		// source mac address
	uint8 src_ip[4];		// source protocol address
	uint8 dest_mac[6];		// destination mac address
	uint8 dest_ip[4];		// destication protocol address
};

#pragma pack(pop, 1)

// called when an arp packet is received by the link layer
error_t arp_recv(sock_buf* buffer);

// creates an arp packet at the address given (usually the link layer data field)
void arp_create(sock_buf* buffer, uint16 hw_type, uint16 prot_type, uint8 hw_len, uint8 prot_len, uint16 opcode, uint8* src_hw, uint8* src_prot,
	uint8* dest_hw, uint8* dest_prot);

// sends an arp header over the network
error_t arp_send(sock_buf* buffer);

error_t init_arp(uint32 layer);
bool protocol_addr_equal(uint8* proto1, uint8* proto2, uint8 len);

#endif