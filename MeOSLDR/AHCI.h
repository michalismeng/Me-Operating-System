#ifndef AHCI_H
#define AHCI_H

#include "AHCIDefinitions.h"

void init_ahci(HBA_MEM_t* abar, uint32 base);
void ahci_port_rebase(uint8 port_no);

uint32 ahci_read(uint8 port, DWORD startl, DWORD starth, DWORD count, VOID* buf);
uint32 ahci_write(uint8 port, DWORD startl, DWORD starth, DWORD count, VOID* buf);

inline uint8 ahci_get_no_command_slots();
inline uint8 ahci_get_no_ports();

inline bool ahci_is_enabled();
void ahci_enable_interrupts(bool value);
inline bool ahci_is_64bit();

volatile inline bool ahci_is_port_implemented(uint8 port);

#endif
