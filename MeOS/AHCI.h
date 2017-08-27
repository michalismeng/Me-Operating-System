#ifndef AHCI_H
#define AHCI_H

#include "AHCIDefinitions.h"
#include "screen.h"
#include "timer.h"
#include "utility.h"
#include "mmngr_virtual.h"
#include "vfs.h"

enum AHCIResult 
{ 
	AHCI_NO_ERROR = 0, 
	CONTROLLER_DISABLED, 
	PORT_NOT_IMPLEMENTED, 
	PORT_ATAPI, 
	SLOT_ERROR, 
	SPIN_ERROR, 
	TASK_ERROR, 
	AHCI_PORT_NOT_OK,
	AHCI_BAD_NODE_STRUCTURE
};

void init_ahci(HBA_MEM_t* abar, uint32 base);

void ahci_port_rebase(uint8 port_no);

void* ahci_main(uint32 ahci_command, ...);

AHCIResult ahci_read(uint8 port, DWORD startl, DWORD starth, DWORD count, VOID* buf);
char* ahci_read(ahci_storage_info* info, DWORD startl, DWORD starth, DWORD count);
AHCIResult ahci_write(uint8 port, DWORD startl, DWORD starth, DWORD count, VOID* buf);
AHCIResult ahci_send_identify(uint8 port, VOID* buf);

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
void ahci_setup_vfs_port(uint8 port_num);
void ahci_print_dmd(ahci_storage_info* info);

#endif
