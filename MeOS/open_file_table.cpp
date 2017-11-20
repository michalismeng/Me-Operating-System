#include "open_file_table.h"
#include "print_utility.h"
#include "critlock.h"
#include "atomic.h"
#include "thread_sched.h"

// private data

global_file_table gft;
spinlock gft_lock;

// public functions

// local file table functions

error_t init_local_file_table(local_file_table* lft, uint32 initial_size)
{
	if (vector_init(&lft->entries, initial_size) != ERROR_OK)
		return ERROR_OCCUR;

	spinlock_init(&lft->lock);

	return ERROR_OK;
}

lfe create_lfe(uint32 gfd, uint32 flags)
{
	lfe entry;
	entry.gfd = gfd;
	entry.flags = flags;

	return entry;
}

uint32 lft_insert(local_file_table* lft, lfe entry)
{
	if (entry.gfd >= gft.count)
	{
		set_last_error(EBADF, OPEN_FILE_BAD_GLOBAL_DESCRIPTOR, EO_OPEN_FILE_TBL);
		return INVALID_FD;
	}

	spinlock_acquire(&lft->lock);
	uint32 loc_index;

	// if there is space past the last entry or there are no unused entries => push backa new one
	if ((lft->entries.count < lft->entries.r_size) || (loc_index = vector_find_first(&lft->entries, lfe_is_invalid)) >= lft->entries.r_size)
	{
		if (vector_insert_back(&lft->entries, entry) != ERROR_OK)
		{
			spinlock_release(&lft->lock);
			return INVALID_FD;
		}

		loc_index = lft->entries.count - 1;
	}

	// else consume the existing entry found above
	lft->entries[loc_index] = entry;

	spinlock_release(&lft->lock);
	return loc_index;
}

bool lfe_is_invalid(lfe* lfe)
{
	return (lfe->gfd == INVALID_FD);
}

error_t lft_remove(local_file_table* lft, uint32 index)
{
	if (index >= lft->entries.count)		// out of range
	{
		set_last_error(EBADF, OPEN_FILE_OUT_OF_BOUNDS, EO_OPEN_FILE_TBL);
		return ERROR_OCCUR;
	}

	lft->entries[index].gfd = INVALID_FD;
	lft->entries[index].flags = VFS_CAP_NONE;

	return ERROR_OK;
}

lfe* lft_get(local_file_table* lft, uint32 index)
{
	if (index >= lft->entries.count)
	{
		set_last_error(EBADF, OPEN_FILE_OUT_OF_BOUNDS, EO_OPEN_FILE_TBL);
		return 0;
	}

	return &lft->entries[index];
}

void lft_print(local_file_table* lft)
{
	for (uint32 i = 0; i < lft->entries.count; i++)
		serial_printf("fd: %u at %u\n", i, lft->entries[i].gfd);
}

// global file table functions

error_t init_global_file_table(uint32 initial_size)
{
	if (vector_init(&gft, initial_size) != ERROR_OK)
		return ERROR_OCCUR;

	spinlock_init(&gft_lock);

	return ERROR_OK;
}

gfe create_gfe(vfs_node* file)
{
	gfe entry;
	entry.file_node = file;
	entry.open_count = 0;
	spinlock_init(&entry.lock);
	list_init(&entry.pages);

	return entry;
}

error_t gft_insert(gfe entry, uint32* index)
{
	// assume global table lock is acquired

	uint32 _index;

	//	    try to place the entry past the last used index
	// then try to find an unused entry in the vector
	// then allocate more vector places
	if ((gft.count < gft.r_size) || ((_index = vector_find_first(&gft, gfe_is_invalid)) >= gft.r_size))
	{
		if (vector_insert_back(&gft, entry) != ERROR_OK)
			return ERROR_OCCUR;

		_index = gft.count - 1;
	}

	gft[_index] = entry;		// index is fixed above (look at the if statement)

	*index = _index;			// pass the index back to the caller
	return ERROR_OK;
}

uint32 gft_insert_s(gfe entry)
{
	critlock_acquire();

	uint32 entry_index = gft_get_n(entry.file_node);		// possible entry index in the global table

	if (entry_index == -1)									// entry doesn't exist
	{
		if (gft_insert(entry, &entry_index) != ERROR_OK)	// so create it
		{
			critlock_release();
			return INVALID_FD;
		}
	}

	gft[entry_index].open_count++;							// open one more time

	critlock_release();

	return entry_index;
}

bool gfe_is_invalid(gfe* gfe)
{
	return (gfe->file_node == 0);
}

bool gft_remove(uint32 index)
{
	if (index >= gft.count)		// out of range
	{
		set_last_error(EBADF, OPEN_FILE_OUT_OF_BOUNDS, EO_OPEN_FILE_TBL);
		return ERROR_OCCUR;
	}

	gft[index].file_node = 0;
	gft[index].open_count = 0;

	// TODO: Clear the page cache list

	return ERROR_OK;
}

error_t gfe_increase_open_count(uint32 index)
{
	gfe* entry = gft_get(index);

	if (entry == 0 || gfe_is_invalid(entry) == true)		// this entry does not exist
	{
		set_last_error(EBADF, OPEN_FILE_NOT_EXIST, EO_OPEN_FILE_TBL);
		return ERROR_OCCUR;
	}

	entry->open_count++;
	return ERROR_OK;
}

error_t gfe_decrement_open_count(uint32 index)
{
	critlock_acquire();

	gfe* entry = gft_get(index);

	if (entry == 0 || gfe_is_invalid(entry) == true)		// this entry does not exist
	{
		critlock_release();
		set_last_error(EBADF, OPEN_FILE_NOT_EXIST, EO_OPEN_FILE_TBL);
		return ERROR_OCCUR;
	}

	entry->open_count--;

	critlock_release();

	return ERROR_OK;
}

void gfe_decrement_open_count_raw(uint32 index)
{
	uint32* cnt_ptr = &gft[index].open_count;
	uint32 open_cnt = *cnt_ptr;

	while (CAS<uint32>(cnt_ptr, open_cnt, (uint32)(open_cnt - 1)) == false)
		open_cnt = *cnt_ptr;
}

gfe* gft_get(uint32 index)
{
	if (index >= gft.count)
	{
		set_last_error(EBADF, OPEN_FILE_OUT_OF_BOUNDS, EO_OPEN_FILE_TBL);
		return 0;
	}

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

uint32 gft_get_by_fd(uint32 fd)
{
	if (fd >= process_get_current()->lft.entries.count)
		return INVALID_FD;

	return process_get_current()->lft.entries[fd].gfd;
}

uint32 gft_get_by_name(char* name)
{
	for (uint32 i = 0; i < gft.count; i++)
		if (strcmp_insensitive(name, gft[i].file_node->name) == 0)
			return i;

	return INVALID_FD;
}

void gft_print()
{
	for (uint32 i = 0; i < gft.count; i++)
		serial_printf("node: %h %s, open count: %u\n", gft[i].file_node, gft[i].file_node->name, gft[i].open_count);
}

global_file_table* gft_get_table()
{
	return &gft;
}