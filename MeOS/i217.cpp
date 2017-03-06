#include "i217.h"

void e1000_write_command(e1000* dev, uint16 addr, uint32 value)
{
	if (dev->bar_type == 0)
	{
		*(uint32*)(dev->mem_base + addr) = value;
	}
	else
	{
		outportl(dev->io_base, addr);
		outportl(dev->io_base + 4, value);
	}
}

uint32 e1000_read_command(e1000* dev, uint16 addr)
{
	if (dev->bar_type == 0)
	{
		return *(uint32*)(dev->mem_base + addr);
	}
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