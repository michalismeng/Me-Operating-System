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
	uint32 open_count;			// shows how many times the file has been opened
	// TODO: Include list of every page of the file cached
};

typedef vector<local_file_entry> local_file_table;
typedef vector<global_file_entry> global_file_table;

typedef global_file_entry gfe;

// initializes a per process open file entry
void local_file_entry_init(local_file_entry* lfe, uint32 flags, uint32 gfd);

// checks whether the open file entry is invalid
bool local_file_entry_is_invalid(local_file_entry* lfe);

/* Global File Table functions */

// initializes the global file table
void init_global_file_table(uint32 initial_size);

// initializes a global open file entry
gfe create_gfe(vfs_node* file);

// insert a global file entry to the global table and returns its inserted index
uint32 gft_insert(gfe entry);

// safe insert (if duplicate exists no new entry is created) and return inseted index
uint32 gft_insert_s(gfe entry);

// checks whetther the global file entry is invalid
bool gfe_is_invalid(gfe* gfe);

// remove a global file entry from the structure
bool gft_remove(uint32 index);

// get a global file entry based on its index
gfe* gft_get(uint32 index);

// get the index of the global file entry associated with the given node or INT_MAX if it doesn't exist.
uint32 gft_get_n(vfs_node* node);

// prints all the global file table
void gft_print();

// return the global open file table
global_file_table* gft_get();

#endif
