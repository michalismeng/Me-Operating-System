#include "i217.h"
#include "print_utility.h"
#include "SerialDebugger.h"
#include "isr.h"
#include "ethernet.h"
#include "mmngr_virtual.h"
#include "process.h"
#include "queue_lf.h"
#include "thread_sched.h"

TCB* net_daemon = 0;
queue_lf<uint32> recv_queue;

void e1000_write_command(e1000* dev, uint16 addr, uint32 value)
{
	if (dev->bar_type == 0)
		*(uint32*)(dev->mem_base + addr) = value;
	else
	{
		outportl(dev->io_base, addr);
		outportl(dev->io_base + 4, value);
	}
}

uint32 e1000_read_command(e1000* dev, uint16 addr)
{
	if (dev->bar_type == 0)
		return *(uint32*)(dev->mem_base + addr);
	else
	{
		outportl(dev->io_base, addr);
		return inportl(dev->io_base + 4);
	}
}

bool e1000_detect_eeprom(e1000* dev)
{
	uint32 val = 0;
	e1000_write_command(dev, REG_EEPROM, 0x1);

	dev->eeprom_exists = false;

	for (int i = 0; i < 1000 & !dev->eeprom_exists; i++)
	{
		val = e1000_read_command(dev, REG_EEPROM);

		if (val & 0x10)
			dev->eeprom_exists = true;
	}

	return dev->eeprom_exists;
}

uint32 e1000_eeprom_read(e1000* dev, uint8 addr)
{
	uint16 data = 0;
	uint32 temp = 0;

	if (dev->eeprom_exists)
	{
		e1000_write_command(dev, REG_EEPROM, 1 | (((uint32)addr) << 8));
		while (!((temp = e1000_read_command(dev, REG_EEPROM)) & (1 << 4)));
	}
	else
	{
		e1000_write_command(dev, REG_EEPROM, 1 | (((uint32)addr) << 2));
		while (!((temp = e1000_read_command(dev, REG_EEPROM)) & (1 << 1)));
	}

	data = (uint16)((temp >> 16) & 0xFFFF);
	return data;
}

bool e1000_read_mac_address(e1000* dev)
{
	if (dev->eeprom_exists)
	{
		uint32 temp;
		temp = e1000_eeprom_read(dev, 0);
		dev->mac[0] = temp & 0xff;
		dev->mac[1] = temp >> 8;
		temp = e1000_eeprom_read(dev, 1);
		dev->mac[2] = temp & 0xff;
		dev->mac[3] = temp >> 8;
		temp = e1000_eeprom_read(dev, 2);
		dev->mac[4] = temp & 0xff;
		dev->mac[5] = temp >> 8;
	}
	else
	{
		uint8* mem_base_mac_8 = (uint8 *)(dev->mem_base + 0x5400);
		uint32* mem_base_mac_32 = (uint32 *)(dev->mem_base + 0x5400);

		if (mem_base_mac_32[0] != 0)
			for (int i = 0; i < 6; i++)
				dev->mac[i] = mem_base_mac_8[i];
		else return false;
	}

	return true;
}

void rx_init(e1000* dev, physical_addr base_rx)
{
	e1000_rx_desc* descs = (e1000_rx_desc*)base_rx;

	for (uint32 i = 0; i < E1000_NUM_RX_DESC; i++)
	{
		dev->rx_descs[i] = (e1000_rx_desc*)(descs + i);
		dev->rx_descs[i]->addr = 0x360000 + i * (2048 + 16);
		dev->rx_descs[i]->status = 0;
	}

	e1000_write_command(dev, REG_RXDESCLO, base_rx);
	e1000_write_command(dev, REG_RXDESCHI, 0);

	e1000_write_command(dev, REG_RXDESCLEN, E1000_NUM_RX_DESC * 16);

	e1000_write_command(dev, REG_RXDESCHEAD, 0);
	e1000_write_command(dev, REG_RXDESCTAIL, E1000_NUM_RX_DESC - 1);
	dev->rx_cur = 0;
	e1000_write_command(dev, REG_RCTRL, RCTL_EN | RCTL_SBP | RCTL_UPE | RCTL_MPE | RCTL_LBM_NONE | RTCL_RDMTS_HALF | RCTL_BAM | RCTL_SECRC | RCTL_BSIZE_2048);
}

void tx_init(e1000* dev, physical_addr base_tx)
{
	e1000_rx_desc* descs = (e1000_rx_desc*)base_tx;

	for (uint32 i = 0; i < E1000_NUM_TX_DESC; i++)
	{
		dev->tx_descs[i] = (e1000_tx_desc*)((uint8*)descs + i * sizeof(e1000_tx_desc));
		dev->tx_descs[i]->addr = 0;
		dev->tx_descs[i]->cmd = 0;
	}

	e1000_write_command(dev, REG_TXDESCLO, base_tx);
	e1000_write_command(dev, REG_TXDESCHI, 0);
	e1000_write_command(dev, REG_TXDESCLEN, E1000_NUM_TX_DESC * sizeof(e1000_tx_desc));

	e1000_write_command(dev, REG_TXDESCHEAD, 0);
	e1000_write_command(dev, REG_TXDESCTAIL, E1000_NUM_TX_DESC);
	dev->tx_cur = 0;
	e1000_write_command(dev, REG_TCTRL, TCTL_EN
		| TCTL_PSP
		| (15 << TCTL_CT_SHIFT)
		| (64 << TCTL_COLD_SHIFT)
		| TCTL_RTLC);

	e1000_write_command(dev, REG_TCTRL, 0b0110000000000111111000011111010);
	e1000_write_command(dev, REG_TIPG, 0x0060200A);
}

int e1000_send(e1000* dev, void* data, uint16 p_len)
{
	dev->tx_descs[dev->tx_cur]->addr = vmmngr_get_phys_addr((virtual_addr)data);
	dev->tx_descs[dev->tx_cur]->length = p_len;
	dev->tx_descs[dev->tx_cur]->cmd = CMD_EOP | CMD_IFCS | CMD_RS | CMD_RPS;
	dev->tx_descs[dev->tx_cur]->status = 0;
	uint8 old_cur = dev->tx_cur;
	dev->tx_cur = (dev->tx_cur + 1) % E1000_NUM_TX_DESC;
	e1000_write_command(dev, REG_TXDESCTAIL, dev->tx_cur);

	while (!(dev->tx_descs[old_cur]->status & 0xf));
	return 0;
}

extern e1000* nic_dev;
bool packet_in_process = false;

void e1000_recv_defered()
{
	while (true)
	{
		while (queue_lf_is_empty(&recv_queue) == false)
		{
			packet_in_process = true;

			uint32 pkt_ind = queue_lf_peek(&recv_queue);
			queue_lf_remove(&recv_queue);

			uint8 *pkt = (uint8 *)nic_dev->rx_descs[pkt_ind]->addr;
			uint16 pktlen = nic_dev->rx_descs[pkt_ind]->length;

			sock_buf buffer;

			if (sock_buf_init_recv(&buffer, pktlen, pkt) != ERROR_OK)
			{
				DEBUG("net deferred context: could not init skb.");
				serial_printf("error %e\n", get_last_error());
			}

			eth_recv(&buffer);
			sock_buf_release(&buffer);

			packet_in_process = false;
		}

		thread_block(thread_get_current());
	}
}

#pragma region receive text

#include "timer.h"
#include "arp.h"
extern uint8 my_ip[4];

void arp_receive_ipv4(arp_header* arp)
{
	arp_ipv4* arp4 = (arp_ipv4*)arp->data;

	bool merged = false;

	// check arp cache for <ipv4, src_ip>
	// if exists => merged = true + upddate the cache with src_mac
	//printfln("arp proc: %u", millis());

	// check dest_ip with my own
	if (protocol_addr_equal(arp4->dest_ip, my_ip, 4))
	{
		// if not merged add into cache <ipv4, src_ip, src_mac>
		if (ntohs(arp->opcode) == ARP_REQ)		// if this is a request then a reply is needed
		{
			// swap src and dest hardware/protocol addressed and send arp_reply

			// reuse the same sock buffer to send the reply
			//sock_buf_reset(buffer);

			eth_header* eth = (eth_header*)malloc(60);
			memcpy(eth->dest_mac, arp4->src_mac, 6);
			memcpy(eth->src_mac, nic_dev->mac, 6);

			eth->eth_type = htons(0x0806);

			arp_header* arp = (arp_header*)eth->eth_data;

			arp->hw_type = htons(HW_ETHER);
			arp->prot_type = htons(PROTO_IPv4);
			arp->hw_len = 6;
			arp->prot_len = 4;
			arp->opcode = htons(ARP_REP);
			printfln("here1");


			uint8* ptr = (uint8*)arp->data;
			memcpy(ptr, nic_dev->mac, 6);
			ptr += 6;

			memcpy(ptr, arp4->dest_ip, 4);
			ptr += 4;

			memcpy(ptr, arp4->src_mac, 6);
			ptr += 6;

			memcpy(ptr, arp4->dest_mac, 4);

			uint16 arp_size = sizeof(arp_header) + 2 * arp->hw_len + 2 * arp->prot_len;

			uint32 packet_size = 42;
			e1000_send(nic_dev, eth, packet_size /*+ max(0, 60 - packet_size)*/);
			printfln("here2");

		}
	}
}

// this is a test receive function for the driver to check the - not solved - problem of slow packet reception
void test_recv_function(uint32 recv_index)
{
	eth_header* eth = (eth_header*)nic_dev->rx_descs[recv_index]->addr;

	// check if this packet's destination is our pc
	if (eth_cmp_mac(eth->dest_mac, nic_dev->mac) == false && eth_cmp_mac(eth->dest_mac, mac_broadcast) == false)
		return;

	uint16 eth_type = ntohs(eth->eth_type);

	if (eth->eth_type == htons(0x0806))
	{
		arp_header* arp = (arp_header*)eth->eth_data;

		// check hardware supprt
		if (arp->hw_type != htons(HW_ETHER))
			return;

		uint16 prot_type = ntohs(arp->prot_type);

		// the arp receive functions require the arp header. so do not push sock buf
		// sock_buf_push(buffer, sizeof(arp_header));

		switch (prot_type)
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
}



#pragma endregion


void e1000_recv_packet(e1000* dev)
{
	uint32 recv_index = dev->rx_cur;
	// increment and write the tail to the device
	dev->rx_cur = (dev->rx_cur + 1) % E1000_NUM_RX_DESC;
	e1000_write_command(dev, REG_RXDESCTAIL, dev->rx_cur);

	

	// insert the reception index for deferred processing
	if (!queue_lf_insert(&recv_queue, recv_index))
		printfln("queue is full");

	if (net_daemon != 0)
		thread_notify(net_daemon);
	else
		DEBUG("net daemon is null");
}

void e1000_callback(registers_t* regs)
{
	extern e1000* nic_dev;

	uint32 icr = e1000_read_command(nic_dev, 0xC0);

	if (icr & 0x4)
		printfln("link status changed");

	if (icr & 0x80)
	{
		if (packet_in_process)
			serial_printf("---------packet in process------------\n");

		e1000_recv_packet(nic_dev);
	}
}


void e1000_enable_interrupts(e1000* dev)
{
	e1000_write_command(dev, REG_IMASK, 0x1F6DC);
	e1000_write_command(dev, REG_IMASK, 0xff & ~4);
	e1000_read_command(dev, 0xc0);

	register_interrupt_handler(32 + 10, e1000_callback);
}


e1000* e1000_start(uint8 bar_type, uint32 mem_base, physical_addr tx_base, physical_addr rx_base)
{
	e1000* dev = new e1000;
	dev->bar_type = bar_type;
	dev->mem_base = mem_base;

	serial_printf("--------eeprom detection\n");
	e1000_detect_eeprom(dev);
	serial_printf("--------read MAC\n");
	e1000_read_mac_address(dev);

	// setup link state
	e1000_write_command(dev, REG_CTRL, (e1000_read_command(dev, REG_CTRL) | CTRL_SLU));

	for (int i = 0; i < 0x80; i++)
		e1000_write_command(dev, 0x5200 + i * 4, 0);

	serial_printf("MAC address: %x %x %x %x %x %x\n", dev->mac[0], dev->mac[1], dev->mac[2], dev->mac[3], dev->mac[4], dev->mac[5]);

	queue_lf_init(&recv_queue, 32);
	net_daemon = thread_create(thread_get_current()->parent, (uint32)e1000_recv_defered, 3 GB + 10 MB + 496 KB, 4 KB, 1);
	serial_printf("net daemon id: %u\n\n", net_daemon->id);
	thread_insert(net_daemon);
	thread_block(net_daemon);

	e1000_enable_interrupts(dev);

	serial_printf("--------initialization of tx\n");
	rx_init(dev, 0x350000);
	tx_init(dev, tx_base);
	serial_printf("--------end initialization of tx\n");

	return dev;
}