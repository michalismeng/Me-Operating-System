#include "vmmngr_pde.h"

// INTERFACE FUNCTIONS

inline void pd_entry_add_attrib(pd_entry* e, uint32 attrib) 
{
	*e |= attrib;
}

inline void pd_entry_del_attrib(pd_entry* e, uint32 attrib)
{
	*e &= ~attrib;
}

inline bool pd_entry_test_attrib(pd_entry* e, uint32 attrib)
{
	return ((*e & attrib) == attrib);
}

inline void pd_entry_set_frame(pd_entry* e, physical_addr addr)
{
	*e = (*e & ~I86_PDE_FRAME) | addr;
}

inline bool pd_entry_is_present(pd_entry e)
{
	return e & I86_PDE_PRESENT;
}

inline bool pd_entry_is_writable(pd_entry e)
{
	return e & I86_PDE_WRITABLE;
}

inline bool pd_entry_is_user(pd_entry e) 
{
	return e & I86_PDE_USER;
}

inline bool pd_entry_is_4mb(pd_entry e) 
{
	return e & I86_PDE_4MB;
}

inline physical_addr pd_entry_get_frame(pd_entry e)
{
	return e & I86_PDE_FRAME;
}