#ifndef PE_DEFINITIONS_H_29082016
#define PE_DEFINITIONS_H_29082016

// PE format definition structures

#include "types.h"

#define IMAGE_DOS_SIGNATURE				0x5A4D      /* MZ */
#define IMAGE_NT_SIGNATURE              0x00004550  /* PE00 */

#define IMAGE_FILE_MACHINE_I386         0x14c   /* Intel 386. */

#define IMAGE_FILE_EXECUTABLE_IMAGE          0x0002  /* File is executable  (i.e. no unresolved externel references). */
#define IMAGE_FILE_32BIT_MACHINE             0x0100  /* 32 bit word machine. */

typedef struct _IMAGE_DATA_DIRECTORY
{
	uint32 VirtualAddress;
	uint32 Size;
} IMAGE_DATA_DIRECTORY,
*PIMAGE_DATA_DIRECTORY;

typedef struct _IMAGE_DOS_HEADER
{
	uint16 e_magic;
	uint16 e_cblp;
	uint16 e_cp;
	uint16 e_crlc;
	uint16 e_cparhdr;
	uint16 e_minalloc;
	uint16 e_maxalloc;
	uint16 e_ss;
	uint16 e_sp;
	uint16 e_csum;
	uint16 e_ip;
	uint16 e_cs;
	uint16 e_lfarlc;
	uint16 e_ovno;
	uint16 e_res[4];
	uint16 e_oemid;
	uint16 e_oeminfo;
	uint16 e_res2[10];
	uint32 e_lfanew;
}IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

typedef struct _IMAGE_FILE_HEADER
{
	uint16 Machine;
	uint16 NumberOfSections;
	uint32 TimeDateStamp;
	uint32 PointerToSymbolTable;
	uint32 NumberOfSymbols;
	uint16 SizeOfOptionalHeader;
	uint16 Characteristics;
} IMAGE_FILE_HEADER,
*PIMAGE_FILE_HEADER;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES     16

typedef struct _IMAGE_OPTIONAL_HEADER
{
	uint16                Magic;
	uint8                 MajorLinkerVersion;
	uint8                 MinorLinkerVersion;
	uint32                SizeOfCode;
	uint32                SizeOfInitializedData;
	uint32                SizeOfUninitializedData;
	uint32                AddressOfEntryPoint;
	uint32                BaseOfCode;
	uint32                BaseOfData;
	uint32                ImageBase;
	uint32                SectionAlignment;
	uint32                FileAlignment;
	uint16                MajorOperatingSystemVersion;
	uint16                MinorOperatingSystemVersion;
	uint16                MajorImageVersion;
	uint16                MinorImageVersion;
	uint16                MajorSubsystemVersion;
	uint16                MinorSubsystemVersion;
	uint32                Win32VersionValue;
	uint32                SizeOfImage;
	uint32                SizeOfHeaders;
	uint32                CheckSum;
	uint16                Subsystem;
	uint16                DllCharacteristics;
	uint32                SizeOfStackReserve;
	uint32                SizeOfStackCommit;
	uint32                SizeOfHeapReserve;
	uint32                SizeOfHeapCommit;
	uint32                LoaderFlags;
	uint32                NumberOfRvaAndSizes;

	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER, *PIMAGE_OPTIONAL_HEADER;

typedef struct _IMAGE_NT_HEADERS {
	uint32					Signature;
	IMAGE_FILE_HEADER		FileHeader;
	IMAGE_OPTIONAL_HEADER	OptionalHeader;
} IMAGE_NT_HEADERS,
*PIMAGE_NT_HEADERS;

typedef struct _IMAGE_SECTION_HEADER {
	uint8 Name[8];
	uint32 VirtualSize;
	uint32 VirtualAddress;
	uint32 SizeOfRawData;
	uint32 PointerToRawData;
	uint32 PointerToRelocations;
	uint16 PointerToLineNumbers;
	uint16 NumberOfRelocations;
	uint32 NumberOfLineNumbers;
	uint32 Characteristics;
}IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

typedef struct _IMAGE_EXPORT_DIRECTORY {
	uint32 Characteristics;
	uint32 TimeDateStamp;
	uint16 MajorVersion;
	uint16 MinorVersion;
	uint32 Name;
	uint32 Base;
	uint32 NumberOfFunctions;
	uint32 NumberOfNames;
	uint32** AddressOfFunctions;
	uint32** AddressOfNames;
	uint16** AddressOfNameOrdinal;
}IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;

typedef struct _IMAGE_IMPORT_DESCRIPTOR {
	union {
		uint32 Characteristics;			// 0 for terminating null import descriptor
		uint32 OriginalFirstThunk;		// RVA to INT
	};
	uint32 TimeDateStamp;				// Time/Date of module, or other properties (see below)
	uint32 ForwarderChain;				// Forwarder chain ID
	uint32 Name;						// Module name
	uint32 FirstThunk;					// RVA to IAT (if bound this IAT has actual addresses)
} IMAGE_IMPORT_DESCRIPTOR;
typedef IMAGE_IMPORT_DESCRIPTOR *PIMAGE_IMPORT_DESCRIPTOR;

typedef struct _IMAGE_IMPORT_BY_NAME {
	uint16  Hint;									// Possible ordinal number to use
	uint8   Name[1];								// Name of function, null terminated
} IMAGE_IMPORT_BY_NAME, *PIMAGE_IMPORT_BY_NAME;

typedef struct _IMAGE_THUNK_DATA {
	union {
		uint32* Function;							// address of imported function
		uint32  Ordinal;							// ordinal value of function
		PIMAGE_IMPORT_BY_NAME AddressOfData;		// RVA of imported name
		DWORD ForwarderString;						// RVA to forwarder string
	};
} IMAGE_THUNK_DATA, *PIMAGE_THUNK_DATA;

#endif
