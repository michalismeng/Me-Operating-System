#ifndef PCI_H
#define PCI_H

#include "AHCI.h"		// we do not need everything to be extern C

#ifdef __cplusplus
extern "C" {
#endif

	// this file is totally incomplete and serves as a quick way to find the ahci driver

#include "types.h"
#include "SerialDebugger.h"

#define PCI_DATA_REGISTER 0xCF8
#define PCI_READ_REGISTER 0xCFC

	//void PCIEnumerate();

	uint32 PCIReadRegister(uint8 bus, uint8 slot, uint8 func, uint8 offset);
	uint32 check_type(HBA_PORT_t* port);
	int check(uint8 bus, uint8 device, uint8 function);
	int CheckAHCIType(HBA_MEM_t* abar);

	HBA_MEM_t* PCIFindAHCI();

#ifdef __cplusplus
}
#endif

#endif
