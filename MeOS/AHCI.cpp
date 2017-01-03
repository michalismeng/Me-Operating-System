#include "AHCI.h"

// private data and helper function

HBA_MEM_t* abar;		// PCI header Base address register 5 relative to the ahci controller
uint32 AHCI_BASE = 0;
uint32 port_ok = 0;		// bit significant variable that states if port is ok for use

char buffer[4096];			// TODO: Make one page buffer more lovely

void* ahci_main(uint32 ahci_command, ...)
{
	va_list l;
	va_start(l, ahci_command);

	switch (ahci_command)
	{
	case 0:
	{
		ahci_storage_info* info = va_arg(l, ahci_storage_info*);
		DWORD startl = va_arg(l, DWORD);
		DWORD starth = va_arg(l, DWORD);
		DWORD count = va_arg(l, DWORD);
		DWORD addr = va_arg(l, DWORD);

		if (addr == 0)
			return ahci_read(info, startl, starth, count);
		else
		{
			ahci_read(info->volume_port, startl, starth, count, (VOID*)addr);
			return 0;
		}
	}
	}
}

bool ahci_start_cmd(HBA_PORT_t* port)
{
	if ((port->cmd & HBA_PxCMD_CR) != 0 || (port->cmd & HBA_PxCMD_FR) != 0 || (port->cmd & HBA_PxCMD_FRE) != 0)
		PANIC("Port is still running");

	// Set FRE (bit4) and ST (bit0)
	port->cmd |= HBA_PxCMD_FRE;

	Timer t;
	while (t.GetElapsedMillis() <= 500);

	t.Restart();

	while ((port->tfd & 0x80) != 0 || (port->tfd & 0x8) != 0 || (port->ssts & 0xF) != 3)
	{
		if (t.GetElapsedMillis() >= 1000)
			return false;
	}

	port->cmd |= HBA_PxCMD_ST;
	return true;
}

bool ahci_stop_cmd(HBA_PORT_t* port)
{
	// Clear ST (bit0)
	port->cmd &= ~HBA_PxCMD_ST;

	Timer t;
	while ((port->cmd & HBA_PxCMD_CR) == 1)
	{
		if (t.GetElapsedMillis() >= 500)		// wait for at least 500 millis
			return false;
	}

	// Clear FRE (bit4)
	port->cmd &= ~HBA_PxCMD_FRE;

	t.Restart();
	while ((port->cmd & HBA_PxCMD_FR) == 1)
	{
		if (t.GetElapsedMillis() >= 500)		// wait another 500 millis at least
			return false;
	}

	// TODO: test if either CR or FR are zero and if not attempt reset
	return true;
}

int32 ahci_find_empty_slot(HBA_PORT_t* port)
{
	DWORD slots = (port->sact | port->ci);

	for (int i = 0; i < ahci_get_no_command_slots(); i++)
	{
		if ((slots & 1) == 0)
			return i;

		slots >>= 1;
	}

	return -1;
}

AHCIResult ahci_data_transfer(HBA_PORT_t* port, DWORD startl, DWORD starth, DWORD count, BYTE PTR buf, bool read)
{
	port->is = (DWORD)-1;	// clear interrupts

	int slot = ahci_find_empty_slot(port);

	if (slot == -1) { printf("SLOT_ERROR\n"); return SPIN_ERROR; }

	HBA_CMD_HEADER_t* cmd = (HBA_CMD_HEADER_t*)port->clb;
	cmd += slot;
	cmd->cfl = sizeof(FIS_REG_H2D) / sizeof(DWORD);

	if (read)
		cmd->w = 0;
	else
		cmd->w = 1;

	cmd->prdtl = (WORD)((count - 1) >> 4) + 1;

	HBA_CMD_TBL_t* cmdtbl = (HBA_CMD_TBL_t*)cmd->ctba;
	memset(cmdtbl, 0, sizeof(HBA_CMD_TBL_t) + (cmd->prdtl - 1) * sizeof(HBA_PRDT_ENTRY_t));

	uint32 i;
	// 8K bytes (16 sectors) per PRDT
	for (i = 0; i < cmd->prdtl - 1; i++)
	{
		cmdtbl->prdt_entry[i].dba = (DWORD)buf;
		cmdtbl->prdt_entry[i].dbc = 8 * 1024;	// 8K bytes
		cmdtbl->prdt_entry[i].i = 1;
		buf += 4 * 1024;	// 4K words
		count -= 16;	// 16 sectors
	}

	// Last entry
	cmdtbl->prdt_entry[i].dba = (DWORD)buf;
	cmdtbl->prdt_entry[i].dbc = count << 9;	// 512 bytes per sector
	cmdtbl->prdt_entry[i].i = 1;

	// Setup command
	FIS_REG_H2D *cmdfis = (FIS_REG_H2D*)(&cmdtbl->cfis);

	cmdfis->fis_type = FIS_TYPE_REG_H2D;
	cmdfis->c = 1;	// Command

	if (read)
		cmdfis->command = 0x25;//0xc8;
	else
		cmdfis->command = 0x35;

	cmdfis->lba0 = (BYTE)startl;
	cmdfis->lba1 = (BYTE)(startl >> 8);
	cmdfis->lba2 = (BYTE)(startl >> 16);
	cmdfis->device = 1 << 6;	// LBA mode

	cmdfis->lba3 = (BYTE)(startl >> 24);
	cmdfis->lba4 = (BYTE)starth;
	cmdfis->lba5 = (BYTE)(starth >> 8);

	cmdfis->countl = (BYTE)count;
	cmdfis->counth = (BYTE)(count >> 8);

	// The below loop waits until the port is no longer busy before issuing a new command
	Timer t;
	while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)))
	{
		if (t.GetElapsedMillis() >= 500)
		{
			printf("SPIN ERROR\n");
			return SPIN_ERROR;
		}
	}

	port->ci = 1 << slot;	// Issue command

	while (1)		// Wait for completion
	{
		// In some longer duration reads, it may be helpful to spin on the DPS bit
		// in the PxIS port field as well (1 << 5)
		if ((port->ci & (1 << slot)) == 0)
			break;
		if (port->is & HBA_PxIS_TFES)	// Task file error
		{
			printf("TASK ERROR\n");
			return TASK_ERROR;
		}
	}

	// Check again
	if (port->is & HBA_PxIS_TFES)
	{
		printf("ERROR\n");
		return TASK_ERROR;
	}

	port->ci = 0;

	return AHCI_NO_ERROR;
}

void ahci_callback(registers_t* regs)
{
	printfln("sata callback");
	for (uint8 i = 0; i < ahci_get_no_ports(); i++)
	{
		if (ahci_is_interrupt_pending(i))
		{
			HBA_PORT_t* port = &abar->ports[i];
			printfln("\nPort %u status: %h and ci %h", i, port->is, port->ci);

			port->ie &= ~1;
			ahci_clear_interrupt(i);

			//if (abar->is == 0)
			//	PANIC("is zero");

			//FIS_REG_D2H* fis = (FIS_REG_D2H*)port->fb;
			//printfln("FIS at %h %h", fis, fis->fis_type);
		}
	}

	//PANIC("ahci interrupt");
}

//public functions

void init_ahci(HBA_MEM_t* _abar, uint32 base)
{
	if (_abar == 0)
		PANIC("null abar");

	AHCI_BASE = base;
	abar = _abar;

	if (!vmmngr_alloc_page((virtual_addr)abar))
		PANIC("Virtual memory allocation for ahci failed");

	for (uint8 i = 0; i < ahci_get_no_ports(); i++)
	{
		if (ahci_is_port_implemented(i))
		{
			ahci_port_rebase(i);

			if (ahci_is_port_ok(i))
				ahci_setup_vfs_port(i);
		}
	}

	//register_interrupt_handler(43, ahci_callback);
	//ahci_enable_interrupts(false);

	printfln("ahci driver initialized with port_ok %h", port_ok);
}

void ahci_print_dmd(ahci_storage_info* info)
{
	printfln("size: %h, port: %u, serial: %s", info->storage_info.volume_size, info->volume_port, info->storage_info.serial_number);
}

void ahci_setup_vfs_port(uint8 port_num)
{
	char buf[512];

	// read identify packet
	if (ahci_send_identify(port_num, buf) != AHCIResult::AHCI_NO_ERROR)
		return;

	// create device node and get deep metadata
	char name[8] = { 0 };
	name[0] = 's'; name[1] = 'd';		// to be safe

	uitoalpha(port_num, name + 2);
	ahci_storage_info* dev_dmd = (ahci_storage_info*)vfs_create_device(name, sizeof(ahci_storage_info));

	// populate storage struct
	uint16* ptr = (uint16*)buf;

	dev_dmd->storage_info.volume_size = *(uint32*)(ptr + 60);
	memcpy(dev_dmd->storage_info.serial_number, ptr + 10, 20);
	dev_dmd->storage_info.sector_size = 512;
	dev_dmd->storage_info.entry_point = ahci_main;
	dev_dmd->volume_port = port_num;
}

void ahci_port_rebase(uint8 port_num)
{
	printfln("rebase port: %u", port_num);
	HBA_PORT_t* port = &abar->ports[port_num];

	if (ahci_stop_cmd(port) == false)
	{
		printfln("port %u is not ok due to stop", port_num);
		return;
	}

	// Command list entry size = 32 bytes
	// Command list max entries = 32
	// Command list max size per port = 32 * 32 bytes = 1 KB = 2^10
	port->clb = AHCI_BASE + (port_num << 10);

	if (ahci_is_64bit())
		port->clbu = 0;

	memset((VOID PTR)port->clb, 0, 1024);

	// FIS offset = end_of_command_list (32 KB)
	// FIS entry size = 256 bytes per port
	port->fb = AHCI_BASE + (32 << 10) + (port_num << 8);

	if (ahci_is_64bit())
		port->fbu = 0;

	memset((VOID PTR)port->fb, 0, 256);

	// Command table offset = end_of_FIS (32 KB + 8 KB = 40 KB)
	// Command table size = 256 * 32 entries = 8 KB = 2^13 per port
	HBA_CMD_HEADER_t* cmd = (HBA_CMD_HEADER_t*)(port->clb);
	for (int i = 0; i < 32; i++)
	{
		cmd[i].prdtl = 8;	// 8 prdt entries per table => 256 bytes per command table
		cmd[i].ctba = AHCI_BASE + (40 << 10) + (port_num << 13) + (i << 8);

		if (ahci_is_64bit())
			cmd[i].ctbau = 0;

		memset((VOID PTR)cmd[i].ctba, 0, 256);
	}

	port->serr = (DWORD)-1;		// clear the error status register
	//port->ie = (DWORD)-1;
	if (ahci_start_cmd(port) == false)
	{
		printfln("port %u is not ok due to start", port_num);
		return;
	}

	port_ok |= (1 << port_num);
}

AHCIResult ahci_read(uint8 port_num, DWORD startl, DWORD starth, DWORD count, VOID* _buf)
{
	HBA_PORT_t* port = &abar->ports[port_num];
	BYTE PTR buf = (BYTE PTR)_buf;

	if (ahci_is_port_ok(port_num) == false)
		return PORT_NOT_OK;

	return ahci_data_transfer(port, startl, starth, count, buf, true);
}

char* ahci_read(ahci_storage_info* info, DWORD startl, DWORD starth, DWORD count)
{
	uint8 cnt = min(count, 8);
	if (ahci_read(info->volume_port, startl, starth, cnt, (VOID*)vmmngr_get_phys_addr((virtual_addr)buffer))
		!= AHCIResult::AHCI_NO_ERROR)
		return 0;

	return buffer;
}

AHCIResult ahci_write(uint8 port_num, DWORD startl, DWORD starth, DWORD count, VOID* _buf)
{
	HBA_PORT_t* port = &abar->ports[port_num];
	BYTE PTR buf = (BYTE PTR)_buf;

	if (ahci_is_port_ok(port_num) == false)
		return PORT_NOT_OK;

	return ahci_data_transfer(port, startl, starth, count, buf, false);
}

AHCIResult ahci_send_identify(uint8 port_num, VOID* _buf)
{
	HBA_PORT_t* port = &abar->ports[port_num];
	BYTE PTR buf = (BYTE PTR)_buf;

	port->is = (DWORD)-1;	// clear interrupts
	int spin = 0;			// counter

	int slot = ahci_find_empty_slot(port);

	if (slot == -1) { printf("SLOT_ERROR\n"); return SLOT_ERROR; }

	HBA_CMD_HEADER_t* cmd = (HBA_CMD_HEADER_t*)port->clb;
	cmd += slot;
	cmd->cfl = sizeof(FIS_REG_H2D) / sizeof(DWORD);
	cmd->w = 0;
	cmd->prdtl = 1;

	HBA_CMD_TBL_t* cmdtbl = (HBA_CMD_TBL_t*)cmd->ctba;
	memset(cmdtbl, 0, sizeof(HBA_CMD_TBL_t) + (cmd->prdtl - 1) * sizeof(HBA_PRDT_ENTRY_t));

	// 8K bytes (16 sectors) per PRDT
	int i;
	for (i = 0; i < cmd->prdtl - 1; i++)
	{
		cmdtbl->prdt_entry[i].dba = (DWORD)buf;
		cmdtbl->prdt_entry[i].dbc = 8 * 1024;	// 8K bytes
		cmdtbl->prdt_entry[i].i = 1;
		buf += 4 * 1024;	// 4K words
	}

	cmdtbl->prdt_entry[i].dba = (DWORD)buf;
	cmdtbl->prdt_entry[i].dbc = 512;	// 512 bytes per sector
	cmdtbl->prdt_entry[i].i = 1;

	// Setup command
	FIS_REG_H2D *cmdfis = (FIS_REG_H2D*)(&cmdtbl->cfis);

	cmdfis->fis_type = FIS_TYPE_REG_H2D;
	cmdfis->c = 1;	// Command
	cmdfis->command = 0xEC;
	cmdfis->device = 0;	// LBA mode

	// The below loop waits until the port is no longer busy before issuing a new command
	while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000)
	{
		spin++;
	}
	if (spin == 1000000)
	{
		printf("SPIN ERROR\n");
		return SPIN_ERROR;
	}

	port->ci = 1 << slot;	// Issue command

	// Wait for completion
	while (1)
	{
		// In some longer duration reads, it may be helpful to spin on the DPS bit
		// in the PxIS port field as well (1 << 5)
		if ((port->ci & (1 << slot)) == 0)
			break;
		if (port->is & HBA_PxIS_TFES)	// Task file error
		{
			printf("TASK ERROR\n");
			return TASK_ERROR;
		}
	}

	// Check again
	if (port->is & HBA_PxIS_TFES)
	{
		printf("ERROR\n");
		return TASK_ERROR;
	}

	return AHCI_NO_ERROR;
}

inline bool ahci_is_64bit()
{
	return abar->cap & CAP_S64A;
}

inline bool ahci_has_LED()
{
	return abar->cap & CAP_SAL;
}

inline uint8 ahci_max_interface_speed()
{
	return (abar->cap & CAP_ISS) >> CAP_ISS_SHIFT;
}

inline uint8 ahci_get_no_command_slots()
{
	return ((abar->cap & CAP_NCS) >> CAP_NCS_SHIFT) + 1;
}

inline uint8 ahci_get_no_ports()
{
	return (abar->cap & CAP_NP) + 1;
}

inline bool ahci_is_enabled()
{
	return abar->ghc & GCH_AE;
}

inline bool ahci_interrupts_enabled()
{
	return abar->ghc & GHC_IE;
}

void ahci_enable_interrupts(bool value)
{
	if (value)
		abar->ghc |= GHC_IE;
	else
		abar->ghc &= ~GHC_IE;
}

volatile inline bool ahci_is_interrupt_pending(uint8 port)
{
	if (ahci_is_port_ok(port) == false || ((abar->is & (1 << port))) == 0)
		return false;

	return true;
}

void ahci_clear_interrupt(uint8 port)
{
	// IS is RWC (write 1 to clear)
	if (ahci_is_interrupt_pending(port))
		abar->is |= (1 << port);
}

inline uint16 ahci_get_major_vs()
{
	return (abar->vs & VS_MJR) >> VS_MJR_SHIFT;
}

inline uint16 ahci_get_minor_vs()
{
	return abar->vs & VS_MNR;
}

inline volatile bool ahci_is_port_implemented(uint8 port)
{
	return (abar->pi & (1 << port));
}

inline bool ahci_is_port_ok(uint8 port)
{
	return (port_ok & (1 << port));
}

void ahci_print_caps()
{
	ClearScreen();

	printf("AHCI controller version: %u.%u", ahci_get_major_vs(), ahci_get_minor_vs());
	if (ahci_is_64bit())
		printf(" 64 bit version");
	else
		printf(" 32 bit version");

	printf("\nPorts implemented: ");
	for (uint32 i = 0; i < 32; i++)
		if (ahci_is_port_implemented(i))
			printf("%u ", i);

	printf("\nPorts ready for communication: ");
	for (uint32 i = 0; i < 32; i++)
		if ((port_ok & (1 << i)) != 0)
			printf("%u ", i);

	printf("\n");

	printf("Number of ports: %u\n", ahci_get_no_ports());
	printf("Number of command slots: %u\n", ahci_get_no_command_slots());

	switch (ahci_max_interface_speed())
	{
	case 1: printf("Generation 1: 1.5 Gbps\n");	break;
	case 2: printf("Generation 2: 3 Gbps\n");	break;
	case 3: printf("Generation 3: 6 Gbps\n");	break;
	default: printf("Error in controller generation\n");
	}

	if (ahci_has_LED())
		printf("Drive supports LED to indicate activity\n");
	else
		printf("No LED to indicate activity found\n");

	if (ahci_interrupts_enabled())
		printf("Currently interrupts enabled\n");
	else
		printf("Currently interrupts disabled\n");
}