#include "PCI.h"

HBA_MEM_t* ab = 0;						// AHCI abar

int CheckAHCIType(HBA_MEM_t* _abar)
{
	uint32 pi = _abar->pi;

	uint32 i = 0;
	while (i < 32)
	{
		if (pi & 1)
		{
			uint32 dt = check_type(&_abar->ports[i]);

			if (dt == 4)
			{
				printf("SATA at port: %u with %h\n", i, (ab->cap >> 20) & 0x0F); ab = _abar; return 1;
			}
			else if (dt == 3)
				printf("PM at port: %u\n", i);
			else if (dt == 1)
				printf("ATAPI at port: %u\n", i);
			else if (dt == 2)
				printf("SEMB at port: %u\n", i);
			else
				printf("No drive found at port: %u\n", i);
		}

		pi >>= 1;
		i++;
	}

	return 0;
}

int check(uint8 bus, uint8 device, uint8 function)
{
	if ((PCIReadRegister(bus, device, function, 0) & 0xFFFF) == 0xFFFF)	// vendor test. 0xFFFF is a floating value so discard
		return 0;

	uint32 reg = PCIReadRegister(bus, device, function, 8);

	uint8 cls = reg >> 24;
	uint8 subcls = (reg >> 16) & 0xFF;
	uint8 progif = (reg >> 8) & 0xFF;

	if (cls == 0x1 && subcls == 0x6 && progif == 0x1)	// SATA AHCI
	{
		uint32 base = PCIReadRegister(bus, device, function, 0x24);
		printf("%u %u %h %h %h %u %h\n", bus, device, cls, subcls, progif, function, base);

		HBA_MEM_t* pbase = (HBA_MEM_t*)base;

		if (CheckAHCIType(pbase) == 1)
			return 1;
		else
			return 0;
	}
	else
		return 0;
}

HBA_MEM_t* PCIFindAHCI()
{
	uint16 bus = 0;
	uint8 device = 0;

	for (; bus < 256; bus++)
	{
		for (; device < 32; device++)
		{
			check(bus, device, 0);

			uint8 headerType = (uint8)(PCIReadRegister(bus, device, 0, 0xC) >> 16) & 0x0FF;
			uint8 function = 0;

			if ((headerType & 0x80) != 0)	// multi-function device
			{
				for (function = 1; function < 8; function++)	// check other functions
					check(bus, device, function);
			}
		}
	}

	return ab;
}

uint32 PCIReadRegister(uint8 bus, uint8 slot, uint8 func, uint8 offset)
{
	uint32 address;

	uint32 lbus = (uint32)bus;
	uint32 lslot = (uint32)slot;
	uint32 lfunc = (uint32)func;

	address = (uint32)((lbus << 16) | (lslot << 11) |
		(lfunc << 8) | (offset & 0xfc) | ((uint32)0x80000000));

	outportl(PCI_DATA_REGISTER, address);

	uint32 temp = inportl(PCI_READ_REGISTER);
	return temp;
}

uint32 check_type(HBA_PORT_t* port)
{
	DWORD ssts = port->ssts;

	BYTE ipm = (ssts >> 8) & 0x0F;
	BYTE det = ssts & 0x0F;

	if (det != 3)
		return 0;
	if (ipm != 1)
		return 0;

	switch (port->sig)
	{
	case SATA_SIG_ATAPI:
		return 1;
	case SATA_SIG_SEMB:
		return 2;
	case SATA_SIG_PM:
		return 3;
	default:
		return 4;
	}
}