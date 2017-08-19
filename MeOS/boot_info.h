#ifndef BOOT_INFO_H_090516
#define BOOT_INFO_H_090516

#include "types.h"

#pragma pack(push, 1)

struct multiboot_info {

	uint32	m_flags;
	uint32	m_memoryLo;
	uint32	m_memoryHi;
	uint32	m_bootDevice;
	uint32	m_cmdLine;
	uint32	m_modsCount;
	uint32	m_modsAddr;
	uint32	m_syms0;
	uint32	m_syms1;
	uint32	m_syms2;
	uint32	m_mmap_length;
	uint32	m_mmap_addr;
	uint32	m_drives_length;
	uint32	m_drives_addr;
	uint32	m_config_table;
	uint32	m_bootloader_name;
	uint32	m_apm_table;
	uint32	m_vbe_control_info;
	uint16	m_vbe_mode_info;
	uint16  m_vbe_interface_seg;
	uint16  m_vbe_interface_off;
	uint16	m_vbe_interface_len;
};

#pragma pack(pop, 1)

struct bios_memory_region
{
	uint32	startLo;
	uint32	startHi;
	uint32	sizeLo;
	uint32	sizeHi;
	uint32	type;
	uint32	acpi_3_0;
};

char* strMemoryTypes[] =
{
	{ "Available" },
	{ "Reserved" },
	{ "ACPI Reclaim" },
	{ "ACPI NVS" }
};

#endif