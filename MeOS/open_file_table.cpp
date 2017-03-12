#include "open_file_table.h"

// private data

global_file_table gft;

// public functions

void local_file_entry_init(local_file_entry* lfe, uint32 flags, uint32 gfd)
{
	lfe->flags = flags;
	lfe->gfd = gfd;
	lfe->pos = 0;
}

bool local_file_entry_is_invalid(local_file_entry* lfe)
{
	return (lfe->flags == FILE_INVALID);
}

// global file table functions

void init_global_file_table(uint32 initial_size)
{
	vector_init(&gft, initial_size);
}

gfe create_gfe(vfs_node* file)
{
	gfe entry;
	entry.file_node = file;
	entry.open_count = 0;

	return entry;
}

uint32 gft_insert(gfe entry)
{
	uint32 index = vector_find_first(&gft, gfe_is_invalid);

	if (index >= gft.count)					// there is no invalid entry (every entry is used)
	{
		vector_insert_back(&gft, entry);
		index = gft.count - 1;
		printfln("Expanding gft to: %u", gft.count);
	}
	else
		gft[index] = entry;

	return index;
}

uint32 gft_insert_s(gfe entry)
{
	uint32 entry_index = gft_get_n(entry.file_node);	// possible entry index in the global table

	if (entry_index == -1)								// entry doesn't exist
		entry_index = gft_insert(entry);

	gft[entry_index].open_count++;						// open one more time
	return entry_index;
}

bool gfe_is_invalid(gfe* gfe)
{
	return (gfe->file_node == 0);
}

bool gft_remove(uint32 index)
{
	if (index >= gft.count)		// out of range
		return false;

	gft[index].file_node = 0;
	gft[index].open_count = 0;

	return true;
}

gfe* gft_get(uint32 index)
{
	if (index >= gft.count)
		return 0;

	return &gft[index];
}

uint32 gft_get_n(vfs_node* node)
{
	for (uint32 i = 0; i < gft.count; i++)
		if (gft[i].file_node == node)
			return i;

	return (uint32)-1;
}

void gft_print()
{
	for (uint32 i = 0; i < gft.count; i++)
		printfln("node: %h,  open count: %u", gft[i].file_node, gft[i].open_count);
}

global_file_table* gft_get()
{
	return &gft;
}