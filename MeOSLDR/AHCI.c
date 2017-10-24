#include "AHCI.h"

// private data and helper function

HBA_MEM_t* abar;		// PCI header Base address register 5 relative to the ahci controller
uint32 AHCI_BASE = 0;

bool ahci_start_cmd(HBA_PORT_t* port)
{
	if ((port->cmd & HBA_PxCMD_CR) != 0 || (port->cmd & HBA_PxCMD_FR) != 0 || (port->cmd & HBA_PxCMD_FRE) != 0)
		PANIC("Port is still running");

	// Set FRE (bit4) and ST (bit0)
	port->cmd |= HBA_PxCMD_FRE;

	uint32 start = millis();
	while (millis() - start <= 500);

	start = millis();

	while ((port->tfd & 0x80) != 0 || (port->tfd & 0x8) != 0 || (port->ssts & 0xF) != 3)
	{
		if (millis() - start >= 1000)
			return false;
	}

	port->cmd |= HBA_PxCMD_ST;
	return true;
}

bool ahci_stop_cmd(HBA_PORT_t* port)
{
	// Clear ST (bit0)
	port->cmd &= ~HBA_PxCMD_ST;

	uint32 start = millis();
	while ((port->cmd & HBA_PxCMD_CR) == 1)
	{
		if (millis() - start >= 500)		// wait for at least 500 millis
			return false;
	}

	// Clear FRE (bit4)
	port->cmd &= ~HBA_PxCMD_FRE;

	start = millis();
	while ((port->cmd & HBA_PxCMD_FR) == 1)
	{
		if (millis() - start >= 500)		// wait another 500 millis at least
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

uint32 ahci_data_transfer(HBA_PORT_t* port, DWORD startl, DWORD starth, DWORD count, BYTE PTR buf, bool read)
{
	port->is = (DWORD)-1;	// clear interrupts

	int slot = ahci_find_empty_slot(port);

	if (slot == -1) { printf("SLOT_ERROR\n"); return SLOT_ERROR; }

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
	uint32 start = millis();
	while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)))
	{
		if (millis() - start >= 500)
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
		printf("TASK ERROR\n");
		return TASK_ERROR;
	}

	port->ci = 0;

	return AHCI_NO_ERROR;
}

//public functions

void init_ahci(HBA_MEM_t* _abar, uint32 base)
{
	if (_abar == 0)
		PANIC("null abar");

	//if (ahci_is_enabled() == false)
	//	PANIC("AHCI Silicon is disabled");

	ahci_enable_interrupts(false);

	AHCI_BASE = base;
	abar = _abar;
#define DEBUG_REQUIRED_PORTS 2
	printfln("ports: %u", ahci_get_no_ports());

	for (uint8 i = 1; i < DEBUG_REQUIRED_PORTS; i++)
	{
		if (ahci_is_port_implemented(i))
			ahci_port_rebase(i);
	}
}

void ahci_port_rebase(uint8 port_num)
{
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
}

uint32 ahci_read(uint8 port_num, DWORD startl, DWORD starth, DWORD count, VOID* _buf)
{
	HBA_PORT_t* port = &abar->ports[port_num];
	BYTE PTR buf = (BYTE PTR)_buf;
	return ahci_data_transfer(port, startl, starth, count, buf, true);
}

uint32 ahci_write(uint8 port_num, DWORD startl, DWORD starth, DWORD count, VOID* _buf)
{
	HBA_PORT_t* port = &abar->ports[port_num];
	BYTE PTR buf = (BYTE PTR)_buf;

	return ahci_data_transfer(port, startl, starth, count, buf, false);
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

void ahci_enable_interrupts(bool value)
{
	if (value)
		abar->ghc |= GHC_IE;
	else
		abar->ghc &= ~GHC_IE;
}

inline bool ahci_is_64bit()
{
	return abar->cap & CAP_S64A;
}

inline volatile bool ahci_is_port_implemented(uint8 port)
{
	return (abar->pi & (1 << port));
}