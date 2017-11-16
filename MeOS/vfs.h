#ifndef VFS_H_29092016
#define VFS_H_29092016

#include "utility.h"
#include "types.h"
#include "memory.h"
#include "list.h"
#include "MassStorageDefinitions.h"
#include "Debugger.h"
#include "error.h"

#define DEVICE_DEFAULT_CAPS VFS_CAP_READ

// TODO: formalize error here!

enum VFS_ATTRIBUTES
{
	VFS_UNDEFINED,			// 00000
	VFS_FILE,				// 00001
	VFS_DIRECTORY,			// 00010
	VFS_LINK,				// 00011
	VFS_DEVICE,				// 00100
	VFS_MOUNT_PT,			// 00101
	VFS_PIPE,				// 00110
	VFS_READ = 8,			// 01000
	VFS_WRITE = 16,			// 10000
	VFS_HIDDEN = 32,
	//VFS_BLOCK_FILE = 64		// This is a block file (if not set then character file)
};

// file capabilities
enum VFS_CAPABILITIES
{
	VFS_CAP_NONE	= 1 << 0,
	VFS_CAP_READ	= 1 << 1,		// File can be read
	VFS_CAP_WRITE	= 1 << 2,		// File can be written
	VFS_CAP_CACHE	= 1 << 3,		// File can use the page cache

	VFS_CAP_MAX		= 0xFFFF,		// The maximum capabilities for vfs files is 16. The other 16 bits are for file-specific usage
};

enum VFS_ERROR
{
	VFS_OK,
	VFS_INVALID_NODE,
	VFS_INVALID_NODE_STRUCTURE,
	VFS_PATH_NOT_FOUND,
	VFS_PAGE_NOT_FOUND,
	VFS_READ_ERROR,
	VFS_FILE_NOT_OPEN,
	VFS_CACHE_FULL,
	VFS_BAD_ARGUMENTS,
	VFS_GENERAL_ERROR,
	VFS_CAPABILITIES_ERROR,
};

// vfs node structures

struct vfs_node;

// File system node operations
struct fs_operations
{
	// Attempts to read up to 'count' bytes from the fd and returns the number read.
	// A value of zero may imply no data to read. Use get_last_error to check for errors.
	size_t(*fs_read)(uint32 fd, vfs_node* file, uint32 start, size_t count, virtual_addr address);

	// Attempts to write up to 'count' bytes to the fd and returns the number written.
	// A value of zero may imply full buffer. Use get_last_error to check for errors.
	size_t(*fs_write)(uint32 fd, vfs_node* file, uint32 start, size_t count, virtual_addr address);

	// Opens the file pointed by node using the requested capabilities
	error_t(*fs_open)(vfs_node* node, uint32 capabilities);

	// TODO: Closes the file. Implement...
	error_t(*fs_close)();

	// Syncs any temporarily saved data to the underlying device. 
	error_t(*fs_sync)(uint32 fd, vfs_node* file, uint32 page_start, uint32 page_end);

	// Looks up for a node based on a current path.
	error_t(*fs_lookup)(vfs_node* parent, char* path, vfs_node** result);

	// Call functions specific to each node.
	error_t(*fs_ioctl)(vfs_node* node, uint32 command, ...);
};

struct vfs_node
{
	// shallow metadata: fixed part of vfs node

	char* name;						// file name
	uint32 name_length;				// name length
	uint32 attributes;				// file attributes
	uint32 capabilities;			// file capabilities
	uint32 file_length;				// file length (bytes)
	vfs_node* tag;					// tag node associated with this node
	vfs_node* parent;				// the parent node

	fs_operations* fs_ops;			// file basic operations
	list<vfs_node*> children;		// children list

	// deep metadata:    file-system/attribute specific part of vfs node

	char deep_md[];
};

// vfs node list definitions

// create a virtual file system node. Parameter name should be null terminated.
vfs_node* vfs_create_node(char* name, bool copy_name, uint32 attributes, uint32 capabilities, uint32 file_length, uint32 deep_metadata_length, vfs_node* tag, 
							vfs_node* parent, fs_operations* file_fncs);

// create a virtual file system device node and return it's deep metadata ptr
vfs_node* vfs_create_device(char* name, uint32 capabilities, uint32 deep_metadata_length, vfs_node* tag, fs_operations* dev_fncs);

// attaches a child node to its parent
void vfs_add_child(vfs_node* parent, vfs_node* child);

// find the child of a node based on its name
vfs_node* vfs_find_child(vfs_node* node, char* name);

// get the devices (/DEV) folder
vfs_node* vfs_get_dev();

// get the root (/) folder
vfs_node* vfs_get_root();

// returns the closest mount point that is above node
vfs_node* vfs_get_mount_point(vfs_node* node);

// returns the parent of the given node
vfs_node* vfs_get_parent(vfs_node* node);

// search for a node starting at 'start' node and going down. Path should not contain leading slash
vfs_node* vfs_find_relative_node(vfs_node* start, char* path);

// search for a node starting at root and goind down. Path should not contain leading slash
vfs_node* vfs_find_node(char* path);

// initialize the virtual file system
void init_vfs();

// opens a file for operations
inline error_t vfs_open_file(vfs_node* node, uint32 capabilities) { return node->fs_ops->fs_open(node, capabilities); }

// reads data from an opened file
inline size_t vfs_read_file(uint32 fd, vfs_node* node, uint32 start, size_t count, virtual_addr address) { return node->fs_ops->fs_read(fd, node, start, count, address); }

// writes data to an opened file
inline size_t vfs_write_file(uint32 fd, vfs_node* node, uint32 start, size_t count, virtual_addr address) { return node->fs_ops->fs_write(fd, node, start, count, address); }

// syncs the in memory changes to the underlying drive
inline error_t vfs_sync(uint32 fd, vfs_node* node, uint32 page_start, uint32 page_end) { return node->fs_ops->fs_sync(fd, node, page_start, page_end); }

// looks down the path from parent until found or returns failure
inline error_t vfs_lookup(vfs_node* parent, char* path, vfs_node** result) { return parent->fs_ops->fs_lookup(parent, path, result); }

// begins a vfs_lookup operation at the root node
error_t vfs_root_lookup(char* path, vfs_node** result);

// vfs debug print functions
void print_vfs(vfs_node* node, int level);
void vfs_print_node(vfs_node* node);
void vfs_print_all();

#endif