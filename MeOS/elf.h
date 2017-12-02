#ifndef ELF_H_02122017
#define ELF_H_02122017

#include "types.h"

typedef uint32 elf32_addr;
typedef uint16 elf32_half;
typedef uint32 elf32_off;
typedef int32 elf32_sword;
typedef uint32 elf32_word;

#define ELFMAG0		0x7f
#define ELFMAG1		'E'
#define ELFMAG2		'L'
#define ELFMAG3		'F'

/* Holds the indices into the elf identification field of useful stuff */
enum ELF32_ID_INDICES
{
	EI_MAG0,				// elf magic field 0
	EI_MAG1,				// elf magic field 1
	EI_MAG2,				// elf magic field 2
	EI_MAG3,				// elf magic field 3
	EI_CLASS,				// elf file class 
	EI_DATA,				// elf data encoding (little-big endian)
	EI_VERSION,				// elf file version
	EI_PAD,					// begin of PAD bytes
	EI_NIDENT	= 16		// size of e_ident array field
};

enum ELF32_FILE_CLASS
{
	ELFCLASSNONE,			// invalid class
	ELFCLASS32,				// 32-bit object
	ELFCLASS64				// 64-bit object
};

enum ELF32_DATA_ENC
{
	ELFDATANONE,
	ELFDATA2LSB,
	ELFDATA2MSB
};

enum ELF32_TYPE
{
	ET_NONE,					// no file type
	ET_REL,						// relocatable type
	ET_EXEC,					// executable type
	ET_DYN,						// shared object
	ET_CORE,					// core file
	ET_LOPROC = 0xff00,			// processor-specific
	ET_HIPROC = 0xffff			// processor-specific
};

enum ELF32_MACHINE
{
	EM_NONE,
	EM_M32,
	EM_SPARC,
	EM_386,				// intel 386
	EM_68K,
	EM_88K,
	EM_860,
	EM_MIPS
};

/* elf reserved section header indices */
enum ELF32_SECTION_IDX
{
	SHN_UNDEF,
	SHN_LORESERVE	= 0xff00,
	SHN_LOPROC		= 0xff00,
	SHN_HIPROC		= 0xff1f,
	SHN_ABS			= 0xfff1,
	SHN_COMMON		= 0xfff2,
	SHN_HIRESERVE	= 0xffff
};

enum ELF32_SECTION_TYPE
{
	SHT_NULL,						// marks an inactive section
	SHT_PROGBITS,					// section holds program bits
	SHT_SYMTAB,						// section holds symbol table
	SHT_STRTAB,						// section holds string table
	SHT_RELA,						// section holds relocation entries
	SHT_HASH,						// section holds symbol hash table
	SHT_DYNAMIC,					// section holds information for dynamic linking
	SHT_NOTE,						// section holds file marks
	SHT_NOBITS,						// section occupies no in-file space, but has the properties of SHT_PROGBITS
	SHT_REL,						// section holds relocation entries
	SHT_SHLIB,						// reserved
	SHT_DYNSYM,						// section holds symbol table
	SHT_LOPROC		= 0x70000000,
	SHT_HIPROC		= 0x7fffffff,
	SHT_LOUSER		= 0x80000000,
	SHT_HI_USER		= 0xffffffff
};

/* section flag bits. When set their properties apply to the section-defined memory region */
enum ELF32_SECTION_FLAGS
{
	SHF_WRITE		= 1,				// section contains writable data
	SHF_ALLOC		= 2,				// section occupies memory during execution
	SFH_EXECINSTR	= 4,				// section contains executable code
	SHF_MASKPROC	= 0xf0000000		// reserved
};

struct elf32_ehdr
{
	uint8		e_ident[EI_NIDENT];		// elf identification bytes
	elf32_half	e_type;					// iidentifies the elf file type
	elf32_half	e_machine;				// specifies the required architecture
	elf32_word	e_version;				// identifies the current version
	elf32_addr	e_entry;				// specifies the virtual address that the program begins
	elf32_off	e_phoff;				// specifies the program header table's file offset
	elf32_off	e_shoff;				// specifies the section header table's file offset
	elf32_word	e_flags;				// specifies processor specific flags
	elf32_half	e_ehsize;				// specifies the ELF header's size in bytes
	elf32_half	e_phentsize;			// specifies the program header table entry's size (all entries are of the same size)
	elf32_half	e_phnum;				// specifies the number of program header entries
	elf32_half	e_shentsize;			// specifies the section header table entry's size (all entries are of the same size)
	elf32_half	e_shnum;				// specifies the number of section header table entris
	elf32_half	e_shstrndx;				// specifies the index of the name string table
};

struct elf32_section_hdr
{
	elf32_word	sh_name;				// index to string table for the name of this section
	elf32_word	sh_type;				// specifies the section's contents and semantics
	elf32_word	sh_flags;				// describes section's attributes
	elf32_addr	sh_addr;				// address in memory to map section (if the section is not to be present in memory, this is zero)
	elf32_off	sh_offset;				// in-file offset of the section
	elf32_word	sh_size;				// section size in bytes (note SHT_NOBITS)
	elf32_word	sh_link;				 
	elf32_word	sh_info;				// extra information based to section type
	elf32_word	sh_addralign;			// section alignment constraint
	elf32_word	sh_entsize;				
};

#endif