#ifndef OPEN_FILE_TABLE_H_28102016
#define OPEN_FILE_TABLE_H_28102016

#include "utility.h"
#include "types.h"
#include "vector.h"
#include "vfs.h"
#include "spinlock.h"

enum LOCAL_FILE_FLAGS
{
	FILE_INVALID,
	FILE_READ,
	FILE_WRITE
};

enum OPEN_FILE_TBL_ERROR
{
	OPEN_FILE_NONE,
	OPEN_FILE_OUT_OF_BOUNDS,
	OPEN_FILE_NOT_EXIST
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

typedef vector<global_file_entry> global_file_table;

typedef global_file_entry gfe;
typedef local_file_entry lfe;

// per process file table
typedef struct local_file_table
{
	vector<lfe> entries;
	spinlock lock;
};

/* Local File Table Functions */

// initialize the per-process local file table
error_t init_local_file_table(local_file_table* lfe, uint32 initial_size);

// create a local file entry
lfe create_lfe(uint32 flags);

// insert a local file entry in the local table
uint32 lft_insert(local_file_table* lft, lfe entry, vfs_node* file_node);

// check whether the local file entry is invalid
bool lfe_is_invalid(lfe* lfe);

// remove the local file entry indicated by index
error_t lft_remove(local_file_table* lft, uint32 index);

// retireve the local file entry indicated by index
lfe* lft_get(local_file_table* lft, uint32 index);

void lft_print(local_file_table* lft);


/* Global File Table functions */

// initializes the global file table
error_t init_global_file_table(uint32 initial_size);

// initializes a global open file entry
gfe create_gfe(vfs_node* file);

// insert a global file entry to the global table and returns its inserted index
error_t gft_insert(gfe entry, uint32* index);

// safe insert (if duplicate exists no new entry is created) and return inseted index
uint32 gft_insert_s(gfe entry);

// checks whetther the global file entry is invalid
bool gfe_is_invalid(gfe* gfe);

// remove a global file entry from the structure
bool gft_remove(uint32 index);

// if the gfe exists, increases its open count variable
error_t gfe_increase_open_count(uint32 index);

// get a global file entry based on its index
gfe* gft_get(uint32 index);

// get the index of the global file entry associated with the given node or UINT_MAX if it doesn't exist.
uint32 gft_get_n(vfs_node* node);

// prints all the global file table
void gft_print();

// return the global open file table
global_file_table* gft_get();

#endif
