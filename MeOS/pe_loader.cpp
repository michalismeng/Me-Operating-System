#include "pe_loader.h"
#include "file.h"
#include "print_utility.h"
#include "thread_sched.h"

// private functions

void pe_get_section_protection_and_flags(uint32 characteristics, uint32* protection, uint32* flags)
{
	if ((characteristics & 0x20) == 0x20)		// code section
	{
		*protection = PROT_READ | PROT_EXEC;
		*flags = MMAP_EXEC | MMAP_SHARED;
	}
	else if ((characteristics & 0x40) == 0x40)	// initialized data section
	{
		*protection = PROT_READ | PROT_WRITE;
		*flags = MMAP_PRIVATE;
	}
	else if ((characteristics & 0x80) == 0x80)	// uninitialized data section
	{
		*protection = PROT_READ | PROT_WRITE;
		*flags = MMAP_ANONYMOUS | MMAP_PRIVATE;
	}

	*flags |= MMAP_USER;
}

void pe_get_export_function(IMAGE_EXPORT_DIRECTORY* export_directory, uint32 image_base, uint32 index, char** name, virtual_addr* address)
{
	char** names = (char**)((uint32)export_directory->AddressOfNames + image_base);
	uint16* ordinals = (uint16*)((uint32)export_directory->AddressOfNameOrdinal + image_base);
	virtual_addr* addrs = (virtual_addr*)((uint32)export_directory->AddressOfFunctions + image_base);

	*name = names[index] + image_base;
	*address = addrs[ordinals[index]] + image_base;
}

// public functions

IMAGE_DATA_DIRECTORY* pe_get_data_directory(IMAGE_DOS_HEADER* header)
{
	return pe_get_nt_headers(header)->OptionalHeader.DataDirectory;
}

IMAGE_SECTION_HEADER* pe_get_section_header(IMAGE_DOS_HEADER* header)
{
	IMAGE_NT_HEADERS* nt_header = pe_get_nt_headers(header);
	return (IMAGE_SECTION_HEADER*)((char*)&nt_header->OptionalHeader + nt_header->FileHeader.SizeOfOptionalHeader);;
}

IMAGE_DOS_HEADER* pe_load_image(uint32 gfd, PCB* proc)
{
	// read the first page of data where the PE header is located (at least 512 bytes are required)
	if (read_file_global(gfd, 0, PAGE_CACHE_SIZE, -1, VFS_CAP_READ | VFS_CAP_CACHE) < 512)
	{
		printfln("read error: %e", get_raw_error());
		return 0;
	}

	void* buffer = (void*)page_cache_get_buffer(gfd, 0);
	if (buffer == 0)
		PANIC("Some strange error");

	if (!validate_PE_image(buffer))
	{
		DEBUG("Could not load PE image. Corrupt image or data.");
		return 0;
	}

	IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)buffer;
	IMAGE_NT_HEADERS* nt_header = pe_get_nt_headers(dos_header);
	uint32 image_base = nt_header->OptionalHeader.ImageBase;

	// map section data - assume they are page aligned
	IMAGE_SECTION_HEADER* section = pe_get_section_header(dos_header);
	for (uint32 i = 0; i < nt_header->FileHeader.NumberOfSections; i++)
	{
		for (int j = 0; j < 8; j++)
			serial_printf("%c", section[i].Name[j]);
		serial_printf("\n");

		serial_printf("mmaping fd: %u to %h, size of raw: %h, raw offset: %h, size of virtual: %h\n", gfd,
			section[i].VirtualAddress + image_base, section[i].SizeOfRawData, section[i].PointerToRawData, section[i].VirtualSize);

		uint32 protection = PROT_NONE;
		uint32 flags = MMAP_NO_ACCESS;

		pe_get_section_protection_and_flags(section[i].Characteristics, &protection, &flags);

		serial_printf("section characteristics: %h => (protection, flags): (%h, %h)\n", section[i].Characteristics, protection, flags);

		if (vfs_mmap_p(proc, section[i].VirtualAddress + image_base, gfd, section[i].PointerToRawData, 4096, protection, flags) == MAP_FAILED)
		{
			serial_printf("address: %h size %h", section[i].VirtualAddress + image_base, section[i].SizeOfRawData);
			PANIC("Could not map for process");
		}
	}

	return dos_header;
}

error_t pe_parse_import_functions(IMAGE_DOS_HEADER* image, IMAGE_DOS_HEADER* dependency_image)
{
	virtual_addr image_base = pe_get_image_base(image);
	IMAGE_DATA_DIRECTORY* data_dir = &pe_get_data_directory(image)[PE_IMPORT_DIRECTORY];
	IMAGE_IMPORT_DESCRIPTOR* import_descriptor = (IMAGE_IMPORT_DESCRIPTOR*)(data_dir->VirtualAddress + image_base);
	IMAGE_THUNK_DATA* thunk = (IMAGE_THUNK_DATA*)(image_base + import_descriptor->OriginalFirstThunk);

	virtual_addr dep_image_base = pe_get_image_base(dependency_image);
	IMAGE_DATA_DIRECTORY* dep_data_dir = &pe_get_data_directory(dependency_image)[PE_EXPORT_DIRECTORY];
	IMAGE_EXPORT_DIRECTORY* dep_export_dir = (IMAGE_EXPORT_DIRECTORY*)(dep_data_dir->VirtualAddress + dep_image_base);

	for (uint32 index = 0; thunk->Function != 0; thunk++, index++)
	{
		char* func_name = (char*)(image_base + thunk->AddressOfData->Name);
		uint32* addr = (uint32*)(image_base + import_descriptor->FirstThunk) + index;

		virtual_addr address = pe_get_export_function_addr(dep_export_dir, dep_image_base, func_name);
		serial_printf("found address for function: %s: %h\n", func_name, address);

		*addr = address;	// bind the function
	}

	return ERROR_OK;
}

bool validate_PE_image(void* image)
{
	IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)image;

	/* make sure program is valid */
	if (dos_header->e_lfanew == 0 || dos_header->e_magic != IMAGE_DOS_SIGNATURE)
		return false;

	IMAGE_NT_HEADERS* nt_headers = (IMAGE_NT_HEADERS*)(dos_header->e_lfanew + (uint32)image);

	/* make sure program header is valid */
	if (nt_headers->Signature != IMAGE_NT_SIGNATURE)
		return false;

	/* make sure executable is only for i386 cpu */
	if (nt_headers->FileHeader.Machine != IMAGE_FILE_MACHINE_I386)
		return false;

	/* make sure this is an executable image and that it is built for 32-bit arch */
	if (nt_headers->FileHeader.Characteristics & (IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_32BIT_MACHINE) == 0)
		return false;

	if ((nt_headers->FileHeader.Characteristics & 1) == 0)
		return false;

	/* make sure the image base is between 4MB and 2GB as this is the user land */
	if (nt_headers->OptionalHeader.ImageBase < 4 MB || nt_headers->OptionalHeader.ImageBase >= 2 GB)
		return false;

	return true;
}

virtual_addr pe_get_export_function_addr(IMAGE_EXPORT_DIRECTORY* export_directory, uint32 image_base, char* function_name)
{
	// points to exported function names
	char** names = (char**)((uint32)export_directory->AddressOfNames + image_base);

	// points to exported function ordinals
	uint16* ordinals = (uint16*)((uint32)export_directory->AddressOfNameOrdinal + image_base);

	// points to exported function addresses that are index based on the above ordinals
	virtual_addr* addrs = (virtual_addr*)((uint32)export_directory->AddressOfFunctions + image_base);

	for (int i = 0; i < export_directory->NumberOfFunctions; i++)
	{
		char* func_name = names[i] + image_base;

		if (strcmp(func_name, function_name) == 0)
		{
			uint16 ordinal = ordinals[i];
			virtual_addr address = addrs[ordinal] + image_base;
			return address;
		}
	}

	return 0;
}

void pe_print_export_functions(IMAGE_EXPORT_DIRECTORY* export_directory, uint32 image_base)
{
	for (uint32 i = 0; i < export_directory->NumberOfFunctions; i++)
	{
		char* func_name;
		virtual_addr address;

		pe_get_export_function(export_directory, image_base, i, &func_name, &address);

		serial_printf("function: %s at address: %h\n", func_name, address);
	}
}
