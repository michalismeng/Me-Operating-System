#ifndef VFS_H_29092016
#define VFS_H_29092016

#include "utility.h"
#include "types.h"
#include "memory.h"
#include "list.h"

enum VFS_ATTRIBUTES
{
	VFS_UNDEFINED,			// 00000
	VFS_FILE,				// 00001
	VFS_DIRECTORY,			// 00010
	VFS_LINK,				// 00011
	VFS_DEVICE,				// 00100
	VFS_READ = 8,			// 01000
	VFS_WRITE = 16,			// 10000
};

enum VFS_ERROR
{
	VFS_OK,
	VFS_PATH_NOT_FOUND,
};

// vfs node structures

struct shallow_metadata
{
	char* name;				// file name
	uint32 name_length;		// name length
	uint32 attributes;		// file attributes
	uint32 file_length;		// file length (bytes)
	// ?? date_created - modified
};

typedef char* deep_metadata;

struct vfs_node
{
	shallow_metadata shallow_md;		// shallow metadata: fixed part of vfs node
	list<vfs_node*> children;			// children list
	char deep_md[];						// deep metadata:    file-system/attribute specific part of vfs node
};

// vfs node list definitions

// create a virtual file system node. Parameter name should be null terminated.
vfs_node* vfs_create_node(char* name, bool copy_name, uint32 attributes, uint32 file_length, uint32 deep_metadata_length);

// create a virtual file system device node and return it's deep metadata ptr
void* vfs_create_device(char* name, uint32 deep_metadata_length);

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

void print_vfs(vfs_node* node, int level);
void vfs_print_node(vfs_node* node);
void vfs_print_all();

#endif