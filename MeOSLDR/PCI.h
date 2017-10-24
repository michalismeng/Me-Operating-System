#ifndef PCI_H
#define PCI_H

#include "AHCI.h"
#include "types.h"

#define PCI_DATA_REGISTER 0xCF8
#define PCI_READ_REGISTER 0xCFC

uint32 PCIReadRegister(uint8 bus, uint8 slot, uint8 func, uint8 offset);
uint32 check_type(HBA_PORT_t* port);
int check(uint8 bus, uint8 device, uint8 function);
int CheckAHCIType(HBA_MEM_t* abar);

HBA_MEM_t* PCIFindAHCI();

#endif
