#ifndef AHCI_H
#define AHCI_H

#include "AHCIDefinitions.h"
#include "screen.h"
#include "timer.h"
#include "utility.h"
#include "mmngr_virtual.h"
#include "vfs.h"
#include "queue_lf.h"
#include "process.h"

enum AHCI_ERROR 
{ 
	AHCI_NONE = 0, 
	AHCI_NO_PORT_AVAIL,
	AHCI_CONTROLLER_DISABLED, 
	AHCI_PORT_NOT_IMPLEMENTED, 
	AHCI_CANT_STOP_PORT,
	AHCI_CANT_START_PORT,
	AHCI_PORT_ATAPI, 
	AHCI_SLOT_ERROR, 
	AHCI_SPIN_ERROR, 
	AHCI_TASK_ERROR, 
	AHCI_PORT_NOT_OK,
	AHCI_BAD_NODE_STRUCTURE
};

struct ahci_message
{
	TCB* issuer;
	HBA_PORT_t* port;
	uint32 low_lba;
	uint32 high_lba;
	uint32 count;
	physical_addr address;
	union
	{
		bool read;
		error_t result;
	};
};

struct ahci_storage_info
{
	mass_storage_info storage_info;		// general mass storage info
	uint8 volume_port;					// ahci port for this volume
	queue_lf<ahci_message*> messages;		// messages for this port (read/write requests)
};

error_t init_ahci(HBA_MEM_t* abar, uint32 base);

error_t ahci_port_rebase(uint8 port_no);
error_t ahci_send_identify(uint8 port, VOID* buf);

inline bool ahci_is_64bit();
inline bool ahci_has_LED();
inline uint8 ahci_max_interface_speed();
inline uint8 ahci_get_no_command_slots();
inline uint8 ahci_get_no_ports();

inline bool ahci_is_enabled();
inline bool ahci_interrupts_enabled();
void ahci_enable_interrupts(bool value);
volatile inline bool ahci_is_interrupt_pending(uint8 port);
void ahci_clear_interrupt(uint8 port);

inline uint16 ahci_get_major_vs();
inline uint16 ahci_get_minor_vs();

volatile inline bool ahci_is_port_implemented(uint8 port);
inline bool ahci_is_port_ok(uint8 port);

void ahci_print_caps();
error_t ahci_setup_vfs_port(uint8 port_num);
void ahci_print_dmd(ahci_storage_info* info);

#endif
