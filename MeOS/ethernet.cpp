#include "ethernet.h"
#include "print_utility.h"
#include "SerialDebugger.h"
#include "i217.h"

extern e1000* nic_dev;

void ether_send(eth_header header)
{
	//e1000_sendPacket();
}

void eth_print_mac(uint8* mac)
{
	for (uint8 i = 0; i < 6; i++)
		printf("%h ", mac[i]);
}

void eth_serial_print_mac(uint8* mac)
{
	for (uint8 i = 0; i < 6; i++)
		serial_printf("%h ", mac[i]);
}

void eth_print(eth_header* header)
{
	printfln("----------------------------------");
	printf("destination mac: ");
	eth_print_mac(header->dest_mac);
	printfln("");

	printf("source mac: ");
	eth_print_mac(header->src_mac);
	printfln("");

	printfln("eth type: %x", header->eth_type);
	printfln("----------------------------------");
}

void eth_serial_print(eth_header* header)
{
	serial_printf("----------------------------------\n");
	serial_printf("destination mac: ");
	eth_serial_print_mac(header->dest_mac);
	serial_printf("\n");

	serial_printf("source mac: ");
	eth_serial_print_mac(header->src_mac);
	serial_printf("\n");

	serial_printf("eth type: %x\n", header->eth_type);
	serial_printf("----------------------------------\n");
}

bool eth_cmp_mac(uint8* mac1, uint8* mac2)
{
	for (uint8 i = 0; i < 6; i++)
		if (mac1[i] != mac2[i])
			return false;

	return true;
}

eth_header* eth_create(virtual_addr header, uint8* dest_mac, uint8* src_mac, uint16 eth_type)
{
	eth_header* eth = (eth_header*)header;

	memcpy(eth->dest_mac, dest_mac, 6);
	memcpy(eth->src_mac, src_mac, 6);

	eth->eth_type = htons(eth_type);

	return eth;
}

void eth_send(eth_header* eth, uint32 data_size)
{
	uint32 packet_size = sizeof(eth_header) + data_size;
	e1000_sendPacket(nic_dev, eth, packet_size + max(0, 60 - packet_size));
}
