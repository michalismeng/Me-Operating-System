#include "vmmngr_pte.h"

// INTERFACE FUNCTIONS

inline void pt_entry_add_attrib(pt_entry* entry, uint32 attrib)
{
	*entry |= attrib;
}

inline void pt_entry_del_attrib(pt_entry* entry, uint32 attrib)
{
	*entry &= ~attrib;
}

inline void pt_entry_set_frame(pt_entry* entry, physical_addr addr)
{
	*entry = (*entry & ~I86_PTE_FRAME) | addr;		// addr is 4KB aligned => erase frame bits, set them with addr
}

inline bool pt_entry_is_present(pt_entry entry)
{
	return entry & I86_PTE_PRESENT;
}

inline bool pt_entry_is_writable(pt_entry entry)
{
	return entry & I86_PTE_WRITABLE;
}

inline physical_addr pt_entry_get_frame(pt_entry entry)
{
	return entry & I86_PTE_FRAME;
}
