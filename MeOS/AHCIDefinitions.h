#ifndef AHCI_DEFINITIONS_H
#define AHCI_DEFINITIONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include "utility.h"
#include "timer.h"

#define	SATA_SIG_ATA	0x00000101	// SATA drive
#define	SATA_SIG_ATAPI	0xEB140101	// SATAPI drive
#define	SATA_SIG_SEMB	0xC33C0101	// Enclosure management bridge
#define	SATA_SIG_PM		0x96690101	// Port multiplier

#define HBA_PxCMD_ST	1			// (start) bit 0 in the command register
#define HBA_PxCMD_CR	1 << 15		// (command list running) bit 15 in the command register
#define HBA_PxCMD_FRE	1 << 4		// (FIS Receive Enable) bit 4 in the command register
#define HBA_PxCMD_FR 	1 << 14		// (FIS Receive Running) bit 14 in the command register

#define HBA_PxIS_TFES	1 << 30
#define ATA_DEV_BUSY	0x80
#define ATA_DEV_DRQ		0x08

#define CMD_SLOTS 		5

	enum FIS_TYPE
	{
		FIS_TYPE_REG_H2D = 0x27,	// Register FIS - Host to Device
		FIS_TYPE_REG_D2H = 0x34, // Register FIS - Device to Host
		FIS_TYPE_DMA_ACT = 0x39, // DMA activate FIS - Device to Host
		FIS_TYPE_DMA_SETUP = 0x41, // DMA Setup FIS - Bidirectional
		FIS_TYPE_DATA = 0x46, // Data FIS - Bidirectional
		FIS_TYPE_BIST = 0x58, // BIST activate FIS - Bidirectional
		FIS_TYPE_PIO_SETUP = 0x5F, // PIO setup FIS - Device to Host
		FIS_TYPE_DEV_BITS = 0xA1 // Set device bits FIS - Device to Host
	};

	// ------------------FIS Definitions------------------------------

#pragma pack(push, 1)

	typedef struct FIS_REG_H2D_struct
	{
		// DWORD 0
		BYTE fis_type;

		BYTE pm_port : 4;	// port multiplier
		BYTE rsv0 : 3; 		// reserved
		BYTE c : 1;			// Command : 1 | Control : 0

		BYTE command;		// Command register
		BYTE featurel;		// feature register 7 : 0 (low)

		// DWORD 1
		BYTE lba0;			// lba low register 7 : 0
		BYTE lba1;			// lba mid register 15 : 8
		BYTE lba2;			// lba high register 23 : 16
		BYTE device;		// device register

		// DOWRD 2
		BYTE lba3;			// lba register 31 : 24
		BYTE lba4;			// lba register 39 : 32
		BYTE lba5;			// lba register 47 : 40
		BYTE featureh;		// feature register 15 : 8 (high)

		// DWORD 3
		BYTE countl;		// count register 7 : 0
		BYTE counth;		// count register 15 : 8
		BYTE icc;			// isosynchronous command completion
		BYTE control;		// control register

		// DWORD 4
		BYTE rsv1[4];		// reserved
	} FIS_REG_H2D;

	typedef struct FIS_REG_D2H_struct
	{
		// DWORD 0
		BYTE	fis_type;    	// FIS_TYPE_REG_D2H

		BYTE	pm_port : 4;    // Port multiplier
		BYTE	rsv0 : 2;      	// Reserved
		BYTE	i : 1;         	// Interrupt bit
		BYTE	rsv1 : 1;      	// Reserved

		BYTE	status;      	// Status register
		BYTE	error;       	// Error register

		// DWORD 1
		BYTE	lba0;        	// LBA low register, 7:0
		BYTE	lba1;        	// LBA mid register, 15:8
		BYTE	lba2;        	// LBA high register, 23:16
		BYTE	device;     	// Device register

		// DWORD 2
		BYTE	lba3;        	// LBA register, 31:24
		BYTE	lba4;        	// LBA register, 39:32
		BYTE	lba5;        	// LBA register, 47:40
		BYTE	rsv2;        	// Reserved

		// DWORD 3
		BYTE	countl;      	// Count register, 7:0
		BYTE	counth;      	// Count register, 15:8
		BYTE	rsv3[2];     	// Reserved

		// DWORD 4
		BYTE	rsv4[4];     	// Reserved
	} FIS_REG_D2H;

	typedef struct FIS_DATA_struct
	{
		// DWORD 0
		BYTE	fis_type;		// FIS_TYPE_DATA

		BYTE	pm_port : 4;	// Port multiplier
		BYTE	rsv0 : 4;		// Reserved

		BYTE	rsv1[2];		// Reserved

		// DWORD 1 ~ N
		DWORD	data[1];		// Payload
	} FIS_DATA;

	typedef struct tagFIS_DMA_SETUP
	{
		// DWORD 0
		BYTE	fis_type;	// FIS_TYPE_DMA_SETUP

		BYTE	pmport : 4;	// Port multiplier
		BYTE	rsv0 : 1;		// Reserved
		BYTE	d : 1;		// Data transfer direction, 1 - device to host
		BYTE	i : 1;		// Interrupt bit
		BYTE	a : 1;            // Auto-activate. Specifies if DMA Activate FIS is needed

		BYTE    rsved[2];       // Reserved

	//DWORD 1&2

		QWORD   DMAbufferID;    // DMA Buffer Identifier. Used to Identify DMA buffer in host memory. SATA Spec says host specific and not in Spec. Trying AHCI spec might work.

			//DWORD 3
		DWORD   rsvd;           //More reserved

			//DWORD 4
		DWORD   DMAbufOffset;   //Byte offset into buffer. First 2 bits must be 0

			//DWORD 5
		DWORD   TransferCount;  //Number of bytes to transfer. Bit 0 must be 0

			//DWORD 6
		DWORD   resvd;          //Reserved
	} FIS_DMA_SETUP;

	typedef struct tagFIS_PIO_SETUP
	{
		// DWORD 0
		BYTE	fis_type;	// FIS_TYPE_PIO_SETUP

		BYTE	pmport : 4;	// Port multiplier
		BYTE	rsv0 : 1;		// Reserved
		BYTE	d : 1;		// Data transfer direction, 1 - device to host
		BYTE	i : 1;		// Interrupt bit
		BYTE	rsv1 : 1;

		BYTE	status;		// Status register
		BYTE	error;		// Error register

		// DWORD 1
		BYTE	lba0;		// LBA low register, 7:0
		BYTE	lba1;		// LBA mid register, 15:8
		BYTE	lba2;		// LBA high register, 23:16
		BYTE	device;		// Device register

		// DWORD 2
		BYTE	lba3;		// LBA register, 31:24
		BYTE	lba4;		// LBA register, 39:32
		BYTE	lba5;		// LBA register, 47:40
		BYTE	rsv2;		// Reserved

		// DWORD 3
		BYTE	countl;		// Count register, 7:0
		BYTE	counth;		// Count register, 15:8
		BYTE	rsv3;		// Reserved
		BYTE	e_status;	// New value of status register

		// DWORD 4
		WORD	tc;		// Transfer count
		BYTE	rsv4[2];	// Reserved
	} FIS_PIO_SETUP;

	// ------------------HBA Definitions------------------------------

	typedef volatile struct HBA_PORT_struct
	{
		DWORD clb;			// Command List Base address, 1K - byte aligned
		DWORD clbu;			// Command List Base address upper 32 bits
		DWORD fb;			// FIS Base address, 256 - byte aligned
		DWORD fbu;			// FIS Base address upper 32 bits
		DWORD is;			// interrupt state
		DWORD ie;			// interrupt enable
		DWORD cmd;			// command and status
		DWORD rsv0;			// reserved
		DWORD tfd;			// Task File Data
		DWORD sig;			// signature
		DWORD ssts;			// SATA status (SCR0:SStatus)
		DWORD sctl;			// SATA control (SCR2:SControl)
		DWORD serr;			// SATA error (SCR1:SError)
		DWORD sact;			// SATA active (SCR3:SActive)
		DWORD ci;			// command issue
		DWORD sntf;			// SATA notification (SCR4:SNotification)
		DWORD fbs;			// FIS-based switch control
		DWORD rsv1[11];		// Reserved
		DWORD vendor[4];	// vendor specific
	} HBA_PORT_t;

	typedef volatile struct HBA_MEM_struct
	{
		// 0x0 - 0x2B Generic Host Control
		DWORD cap;			// Host capability
		DWORD ghc;			// Global host control
		DWORD is;			// Interrupt status
		DWORD pi;			// ports implemented
		DWORD vs;			// version
		DWORD ccc_ctl;		// complete another time
		DWORD ccc_pts;		// complete another time
		DWORD em_loc;		// complete another time
		DWORD em_ctl;		// complete another time
		DWORD cap2;			// Host extended capability
		DWORD bohc;			// BIOS/OS handoff control and status

		// 0x2C - 0x9F reserved
		BYTE rsv[0xA0 - 0x2C];		// 9F - 2C + 1 (subtraction quirk)

		// 0xA0 - 0xFF, Vendor specific registers
		BYTE vendor[0x100 - 0xA0];

		// 0x100 - 0x10FF, Port control registers
		HBA_PORT_t ports[1];		// 1 ~ 32
	} HBA_MEM_t;

	typedef volatile struct HBA_FIS_struct
	{
		// 0x00
		FIS_DMA_SETUP dsfis;	// DMA setup FIS
		BYTE pad0[4];

		// 0x20
		FIS_PIO_SETUP psfis;	// PIO setup FIS
		BYTE pad1[12];

		// 0x40
		FIS_REG_D2H rfis;		// Register - Device to Host
		BYTE pad2[4];

		// 0x58
		WORD sdbfis;			// unkown- not given in osdev wiki example

		// 0x60
		BYTE ufis[64];

		// 0xA0
		BYTE rsv[0x100 - 0xA0];
	} HBA_FIS_t;

	typedef struct HBA_CMD_HEADER
	{
		// DWORD 0
		BYTE cfl : 5;	// Command FIS length in DWORDS : 2 ~ 16
		BYTE a : 1;		// ATAPI
		BYTE w : 1;		// 1 : H2D, 0 : D2H
		BYTE P : 1;		// prefetchable

		BYTE r : 1;		// reset
		BYTE b : 1;		// bist
		BYTE c : 1;		// clear busy upon R_OK
		BYTE rsv0 : 1;	// reserved
		BYTE pmp : 4;	// port multiplier port

		WORD prdtl;		// Physical region descriptor table length in entries

		// DWORD 1
		volatile DWORD prdbc;	//Physical region descriptor table byte count transferred

		// DWORD 2, 3
		DWORD ctba;		// Command table descriptor address base
		DWORD ctbau;	// Command table descriptor address base upper 32 bits (64-bit address)

		// DWORD 4 - 7
		DWORD resv1[4];	// reserved
	} HBA_CMD_HEADER_t;

	typedef struct HBA_PRDT_ENTRY
	{
		DWORD dba;	// data base address
		DWORD dbau;	// data base address upper 32 bits
		DWORD rsv0;

		DWORD dbc : 22;	// data byte count, Max 4M
		DWORD rsv1 : 9;
		DWORD i : 1;	// interrupt
	} HBA_PRDT_ENTRY_t;

	typedef struct HBA_CMD_TBL
	{
		// 0x00
		BYTE cfis[64];	// Command FIS

		// 0x40
		BYTE acmd[16];	// ATAPI command, 12 or 16 bytes

		// 0x50
		BYTE rsv[48];	// reserved

		// 0x80
		HBA_PRDT_ENTRY_t prdt_entry[1];	// Physical region descriptor table entries, 0 ~ 65535
	} HBA_CMD_TBL_t;

#pragma pack(pop, 1)

	// Generic Host Control Registers offsets
	enum GHC_REGISTERS
	{
		GHC_CAP = 0x0,				// capabilities
		GHC_GHC = 0x4,				// global host control
		GHC_IS = 0x8,				// interrupt status (bit significant)
		GHC_PI = 0xC,				// ports implemented (bit significant)
		GHC_VS = 0x10,				// version

		//////////////////    AHCI REV 1.0    //////////////////////////

		GHC_CCC_CTL = 0x14,
		GHC_CCC_PORTS = 0x18,
		GHC_EM_LOC = 0x1C,
		GHC_EM_CTL = 0x20,
		GHC_CAP2 = 0x24,
		GHC_BOHC = 0x28

		//////////////////    AHCI REV 3.1    //////////////////////////
	};

	// Generic Host Control Registers Masks

	//CAP -> HBA capabilities
	enum GHC_CAP_MASKS
	{
		CAP_S64A = 1 << 31,		// support for 64-bit structures
		CAP_SNCQ = 1 << 30,		// support for native command queueing
		CAP_SSNTF = 1 << 29,	// reserved for 1.0
		CAP_SMPS = 1 << 28,		// supports mechanical presence for hot plug
		CAP_SSS = 1 << 27,		// supports staggered spin-up
		CAP_SALP = 1 << 26,		// ?
		CAP_SAL = 1 << 25,		// supports activity LED
		CAP_SCLO = 1 << 24,		// ?
		CAP_ISS = 0xf << 20,	// interface speed
		CAP_ISS_SHIFT = 20,
		CAP_SAM = 1 << 18,		// when set to 1 sata does not support legacy SFF-8038i interface
		CAP_SPM = 1 << 17,		// support for port multiplier
		CAP_FBSS = 1 << 16,		// reserved for 1.0
		CAP_PMD = 1 << 15,		// ?
		CAP_SSC = 1 << 14,		// ?
		CAP_PSC = 1 << 13,		// ?
		CAP_NCS = 0xF << 8,		// 0 based value indicating the maximum number of command slots
		CAP_NCS_SHIFT = 8,
		CAP_CCCS = 1 << 7,		// reserved for 1.0
		CAP_EMS = 1 << 6,		// reserved for 1.0
		CAP_SXS = 1 << 5,		// reserved for 1.0
		CAP_NP = 0xF << 0		// 0 based maximum number of ports of the HBA (all may not be implemented)
	};

	// GCH -> Global HBA Control
	enum GHC_GHC_MASKS
	{
		GCH_AE = 1 << 31,		// AHCI enable
		GHC_MRSM = 1 << 2,		// reserved for 1.0
		GHC_IE = 1 << 1,		// global interrupt enable
		GCH_HR = 1 << 0			// HBA Reset
	};

	// VS -> AHCI Version
	enum GHC_VER_MASKS
	{
		VS_MJR = 0xFF << 16,	// Version Major number
		VS_MJR_SHIFT = 16,
		VS_MNR = 0xFF << 0		// Version Minor number
	};

	// CCC_CTL -> Command Completion Coalescing Control

#define	CCC_CTL_TV 	0xFF << 16		// Timeout value to fire interrupt
#define	CCC_CTL_CC	0xF << 8
#define CCC_CTL_INT 0x5 << 3
#define CCC_CTL_EN  1	<< 0

/* TODO: Fill some other masks that remain (not very useful though) */

	// Port x interrupt status. Can be used for interrupt enable
	enum PORT_INTR_STS_MASKS
	{
		INTR_STS_DSS = 1 << 2	// DMA Setup FIS received
	};

#ifdef __cplusplus
}
#endif

#endif