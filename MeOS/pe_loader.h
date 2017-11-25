#ifndef PE_LOADER_14112017
#define PE_LOADER_14112017

#include "types.h"
#include "mmngr_virtual.h"
#include "process.h"
#include "PE_Definitions.h"

// loads an open image associated with fd to the pdir address space
IMAGE_DOS_HEADER* pe_load_image(uint32 gfd, PCB* pdir);

// loads all addresses of the depenency_image needed by the image executable
error_t pe_parse_import_functions(IMAGE_DOS_HEADER* image, IMAGE_DOS_HEADER* dependency_image);

// given the image dos header returns the nt headers
inline IMAGE_NT_HEADERS* pe_get_nt_headers(IMAGE_DOS_HEADER* header) { return (IMAGE_NT_HEADERS*)(header->e_lfanew + (uint32)header); }

// given the image dos header returns the data directory array
IMAGE_DATA_DIRECTORY* pe_get_data_directory(IMAGE_DOS_HEADER* header);

// given the dos header returns the section header
IMAGE_SECTION_HEADER* pe_get_section_header(IMAGE_DOS_HEADER* header);

// given the image dos header returns the image base address
inline virtual_addr pe_get_image_base(IMAGE_DOS_HEADER* header) { return pe_get_nt_headers(header)->OptionalHeader.ImageBase; }

bool validate_PE_image(void* image);

virtual_addr pe_get_export_function_addr(IMAGE_EXPORT_DIRECTORY* export_directory, uint32 image_base, char* function_name);
void pe_set_import_function_addr(virtual_addr bind);


void pe_print_export_functions(IMAGE_EXPORT_DIRECTORY* export_directory, uint32 image_base);

#endif