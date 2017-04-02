#ifndef VFS_H_29092016
#define VFS_H_29092016

#include "utility.h"
#include "types.h"
#include "memory.h"
#include "list.h"
#include "MassStorageDefinitions.h"
#include "Debugger.h"

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
	VFS_BAD_ARGUMENTS
};

// vfs node structures

struct vfs_node;
typedef int vfs_result;
typedef char* deep_metadata;

struct fs_operations
{
	uint32(*fs_read)(int fd, vfs_node* file, uint32 start, uint32 count, virtual_addr address);
	uint32(*fs_write)(int fd, vfs_node* file, uint32 start, uint32 count, virtual_addr address);
	vfs_result(*fs_open)(vfs_node* node);
	vfs_result(*fs_close)();
	vfs_result(*fs_sync)(int fd, vfs_node* file, uint32 page_start, uint32 page_end);			// page end is end or past end?
	vfs_result(*fs_lookup)(vfs_node* parent, char* path, vfs_node** result);
	vfs_result(*fs_ioctl)(vfs_node* node, uint32 command, ...);
};

struct vfs_node
{
	// shallow metadata: fixed part of vfs node

	char* name;						// file name
	uint32 name_length;				// name length
	uint32 attributes;				// file attributes
	uint32 file_length;				// file length (bytes)
	vfs_node* tag;					// tag node associated with this node
	uint32 data;					// custom data associated with this node

	fs_operations* fs_ops;			// file basic operations
	list<vfs_node*> children;		// children list

	// deep metadata:    file-system/attribute specific part of vfs node

	char deep_md[];
};

// vfs node list definitions

// create a virtual file system node. Parameter name should be null terminated.
vfs_node* vfs_create_node(char* name, bool copy_name, uint32 attributes, uint32 file_length, uint32 deep_metadata_length, vfs_node* tag, fs_operations* file_fncs);

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

// search for a node starting at 'start' node and going down. Path should not contain leading slash
vfs_node* vfs_find_relative_node(vfs_node* start, char* path);

// search for a node starting at root and goind down. Path should not contain leading slash
vfs_node* vfs_find_node(char* path);

// initialize the virtual file system
void init_vfs();

// include read and write to and from page-cache functions
uint32 vfs_read_file(int fd, vfs_node* node, uint32 start, uint32 count, virtual_addr address);

// opens a file for operations. (Loads its drive layout)
vfs_result vfs_open_file(vfs_node* node);

// writes to an opened file
uint32 vfs_write_file(int fd, vfs_node* node, uint32 start, uint32 count, virtual_addr address);

// syncs the in memory changes to the underlying drive
vfs_result vfs_sync(int fd, vfs_node* file, uint32 page_start, uint32 page_end);

// looks down the path from parent until found or returns failure
vfs_result vfs_lookup(vfs_node* parent, char* path, vfs_node** result);

// begins a vfs_lookup operation at the root node
vfs_result vfs_root_lookup(char* path, vfs_node** result);

// vfs debug print functions
void print_vfs(vfs_node* node, int level);
void vfs_print_node(vfs_node* node);
void vfs_print_all();

#endif