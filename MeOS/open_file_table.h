#ifndef OPEN_FILE_TABLE_H_28102016
#define OPEN_FILE_TABLE_H_28102016

#include "utility.h"
#include "types.h"
#include "vector.h"
#include "vfs.h"

enum LOCAL_FILE_FLAGS
{
	FILE_INVALID,
	FILE_READ,
	FILE_WRITE
};

// per process file table entry
struct local_file_entry
{
	uint32 flags;		// flags and permissions
	uint32 gfd;			// global file descriptor index
	uint32 pos;			// in-file position
};

// system-wide file table entry
struct global_file_entry
{
	vfs_node* file_node;		// actual file description node
	vfs_node* file_mount;		// file mount point. Used for file access
	uint32 open_count;			// shows how many times the file has been opened
	// TODO: Include list of every page of the file cached
};

typedef vector<local_file_entry> local_file_table;
typedef vector<global_file_entry> global_file_table;

// initializes a per process open file entry
void local_file_entry_init(local_file_entry* lfe, uint32 flags, uint32 gfd);

// checks whether the open file entry is invalid
bool local_file_entry_is_invalid(local_file_entry* lfe);

// initializes a global open file entry
void global_file_entry_init(global_file_entry* gfe, vfs_node* file, vfs_node* mount);

// checks whetther the global file entry is invalid
bool global_file_entry_is_invalid(global_file_entry* gfe);

// return the global open file table
global_file_table* get_global_file_table();

#endif
