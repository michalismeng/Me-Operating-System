#ifndef PE_LOADER_14112017
#define PE_LOADER_14112017

#include "types.h"
#include "mmngr_virtual.h"
#include "process.h"
#include "PE_Definitions.h"

// loads an open image associated with fd to the pdir address space
IMAGE_DOS_HEADER* pe_load_image(uint32 gfd, PCB* pdir);

bool validate_PE_image(void* image);

virtual_addr pe_get_export_function_addr(IMAGE_EXPORT_DIRECTORY* export_directory, uint32 image_base, char* function_name);
void pe_set_import_function_addr(virtual_addr bind);


void pe_print_export_functions(IMAGE_EXPORT_DIRECTORY* export_directory, uint32 image_base);

#endif