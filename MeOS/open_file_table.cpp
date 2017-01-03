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

void global_file_entry_init(global_file_entry* gfe, vfs_node* file, vfs_node* mount)
{
	gfe->file_node = file;
	gfe->file_mount = mount;
	gfe->open_count = 0;
}

bool global_file_entry_is_invalid(global_file_entry* gfe)
{
	return (gfe->file_mount == 0 || gfe->file_node == 0);
}

global_file_table* get_global_file_table()
{
	return &gft;
}