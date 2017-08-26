#ifndef VFS_H_29092016
#define VFS_H_29092016

#include "utility.h"
#include "types.h"
#include "memory.h"
#include "list.h"
#include "MassStorageDefinitions.h"
#include "Debugger.h"
#include "error.h"

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
	VFS_GENERAL_ERROR
};

// vfs node structures

struct vfs_node;
typedef char* deep_metadata;

struct fs_operations
{
	size_t(*fs_read)(int fd, vfs_node* file, uint32 start, size_t count, virtual_addr address);
	size_t(*fs_write)(int fd, vfs_node* file, uint32 start, size_t count, virtual_addr address);
	error_t(*fs_open)(vfs_node* node);
	error_t(*fs_close)();
	error_t(*fs_sync)(int fd, vfs_node* file, uint32 page_start, uint32 page_end);			// page end is end or past end?
	error_t(*fs_lookup)(vfs_node* parent, char* path, vfs_node** result);
	error_t(*fs_ioctl)(vfs_node* node, uint32 command, ...);
};

struct vfs_node
{
	// shallow metadata: fixed part of vfs node

	char* name;						// file name
	uint32 name_length;				// name length
	uint32 attributes;				// file attributes
	uint32 file_length;				// file length (bytes)
	vfs_node* tag;					// tag node associated with this node
	vfs_node* parent;				// the parent node
	uint32 flags;					// file flags

	fs_operations* fs_ops;			// file basic operations
	list<vfs_node*> children;		// children list

	// deep metadata:    file-system/attribute specific part of vfs node

	char deep_md[];
};

// vfs node list definitions

// create a virtual file system node. Parameter name should be null terminated.
vfs_node* vfs_create_node(char* name, bool copy_name, uint32 attributes, uint32 file_length, uint32 deep_metadata_length, vfs_node* tag, 
							vfs_node* parent, fs_operations* file_fncs);

// create a virtual file system device node and return it's deep metadata ptr
vfs_node* vfs_create_device(char* name, uint32 deep_metadata_length, vfs_node* tag, fs_operations* dev_fncs);

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

// opens a file for operations. (Loads its drive layout)
error_t vfs_open_file(vfs_node* node);

// include read and write to and from page-cache functions
size_t vfs_read_file(int fd, vfs_node* node, uint32 start, size_t count, virtual_addr address);

// writes to an opened file
size_t vfs_write_file(int fd, vfs_node* node, uint32 start, size_t count, virtual_addr address);

// syncs the in memory changes to the underlying drive
error_t vfs_sync(int fd, vfs_node* file, uint32 page_start, uint32 page_end);

// looks down the path from parent until found or returns failure
error_t vfs_lookup(vfs_node* parent, char* path, vfs_node** result);

// begins a vfs_lookup operation at the root node
error_t vfs_root_lookup(char* path, vfs_node** result);

// vfs debug print functions
void print_vfs(vfs_node* node, int level);
void vfs_print_node(vfs_node* node);
void vfs_print_all();

#endif