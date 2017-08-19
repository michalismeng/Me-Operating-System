#include "open_file_table.h"
#include "spinlock.h"
#include "print_utility.h"

// private data

global_file_table gft;
spinlock gft_lock;

// public functions

// local file table functions

void init_local_file_table(local_file_table* lft, uint32 initial_size)
{
	vector_init(&lft->entries, initial_size);
	spinlock_init(&lft->lock);
}

lfe create_lfe(uint32 flags)
{
	lfe entry;
	entry.gfd = 0;
	entry.flags = flags;
	entry.pos = 0;

	return entry;
}

uint32 lft_insert(local_file_table* lft, lfe entry, vfs_node* file_node)
{
	uint32 loc_index = vector_find_first(&lft->entries, lfe_is_invalid);

	if (loc_index >= lft->entries.count)					// there is no invalid entry (every entry is used)
	{
		vector_insert_back(&lft->entries, entry);
		loc_index = lft->entries.count - 1;
	}
	else
		lft->entries[loc_index] = entry;					// replace the invalid (unused) entry with the new one

	// open the corresponding gfe
	uint32 gfd = gft_get_n(file_node);

	if (gfd == (uint32)-1)								// the gfe does not exist, so create it
		gfd = gft_insert_s(create_gfe(file_node));
	else
		if (gfe_increase_open_count(gfd) == false)
			printfln("could not increase count for: %u", gfd);

	lft->entries[loc_index].gfd = gfd;
	return loc_index;
}

bool lfe_is_invalid(lfe* lfe)
{
	return (lfe->flags == FILE_INVALID && lfe->gfd == 0);
}

bool lft_remove(local_file_table* lft, uint32 index)
{
	if (index >= lft->entries.count)		// out of range
		return false;

	lft->entries[index].pos = (uint32)(-1);
	lft->entries[index].flags = FILE_INVALID;
	lft->entries[index].gfd = 0;

	return true;
}

lfe* lft_get(local_file_table* lft, uint32 index)
{
	if (index >= lft->entries.count)
		return 0;

	return &lft->entries[index];
}

void lft_print(local_file_table* lft)
{
	for (uint32 i = 0; i < lft->entries.count; i++)
		printfln("fd: %u at %u", i, lft->entries[i].gfd);
}

// global file table functions

void init_global_file_table(uint32 initial_size)
{
	vector_init(&gft, initial_size);
	spinlock_init(&gft_lock);
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
	}
	else
		gft[index] = entry;

	// Why not increase open count ??

	return index;
}

uint32 gft_insert_s(gfe entry)
{
	spinlock_acquire(&gft_lock);

	uint32 entry_index = gft_get_n(entry.file_node);	// possible entry index in the global table

	if (entry_index == -1)								// entry doesn't exist
		entry_index = gft_insert(entry);

	gft[entry_index].open_count++;						// open one more time

	spinlock_release(&gft_lock);

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

bool gfe_increase_open_count(uint32 index)
{
	gfe* entry = gft_get(index);

	if (entry == 0 || gfe_is_invalid(entry) == true)		// this entry does not exist
		return false;

	entry->open_count++;
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
	uint32 count = gft.count;		// temporarily save the count to avoid concurrency problems (though they do not exist, but be safe)

	for (uint32 i = 0; i < count; i++)
		if (gft[i].file_node == node)
			return i;

	return (uint32)-1;
}

void gft_print()
{
	for (uint32 i = 0; i < gft.count; i++)
		printfln("node: %h %s, open count: %u", gft[i].file_node, gft[i].file_node->name, gft[i].open_count);
}

global_file_table* gft_get()
{
	return &gft;
}