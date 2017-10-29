#ifndef OPEN_FILE_TABLE_H_28102016
#define OPEN_FILE_TABLE_H_28102016

#include "utility.h"
#include "types.h"
#include "vector.h"
#include "vfs.h"
#include "spinlock.h"
#include "page_cache.h"

enum OPEN_FILE_TBL_ERROR
{
	OPEN_FILE_NONE,
	OPEN_FILE_OUT_OF_BOUNDS,
	OPEN_FILE_NOT_EXIST,
	OPEN_FILE_BAD_GLOBAL_DESCRIPTOR
};

// per process file table entry
struct local_file_entry
{
	uint32 flags;		// flags and permissions
	uint32 gfd;			// global file descriptor index
};

// system-wide file table entry
struct global_file_entry
{
	vfs_node* file_node;					// actual file description node
	uint32 open_count;						// shows how many times the file has been opened
	spinlock lock;							// per entry lock
	list<_page_cache_file_info> pages;		// file cached data (this may not be used if the file doesn't support caching)
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
lfe create_lfe(uint32 gfd, uint32 flags);

// insert a local file entry in the local table (the entry should be valid => contains a valid gfe index and valid flags)
uint32 lft_insert(local_file_table* lft, lfe entry);

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
//error_t gft_insert(gfe entry, uint32* index);

// safe insert (if duplicate exists no new entry is created) and return inserted index
uint32 gft_insert_s(gfe entry);

// checks whetther the global file entry is invalid
bool gfe_is_invalid(gfe* gfe);

// remove a global file entry from the structure
bool gft_remove(uint32 index);

// if the gfe exists, increases its open count variable
error_t gfe_increase_open_count(uint32 index);

// if the gfe exists, decreases its open count variable (for closing a file)
error_t gfe_decrement_open_count(uint32 index);

// decrements the open count pointed by index, without checking nor locking
void gfe_decrement_open_count_raw(uint32 index);

// get a global file entry based on its index
gfe* gft_get(uint32 index);

// get the index of the global file entry associated with the given node or UINT_MAX if it doesn't exist.
uint32 gft_get_n(vfs_node* node);

// get the index of the global file entry pointed by the given local file descriptor
uint32 gft_get_by_fd(uint32 fd);

// prints all the global file table
void gft_print();

// return the global open file table
global_file_table* gft_get_table();



#endif
