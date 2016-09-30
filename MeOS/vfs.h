#ifndef VFS_H_29092016
#define VFS_H_29092016

#include "utility.h"
#include "types.h"
#include "memory.h"
#include "list.h"

enum VFS_ATTRIBUTES {};

// vfs node structures

struct shallow_metadata
{
	char* name;				// file name
	uint32 name_length;		// name length
	uint32 attributes;		// file attributes
	uint32 file_length;		// file length (bytes)
	// ?? date_created - modified
};

struct vfs_node
{
	shallow_metadata shallow_md;		// shallow metadata: fixed part of vfs node
	list<vfs_node*> children;			// children list
	char deep_md[];						// deep metadata:    file-system/attribute specific part of vfs node
};

// vfs node list definitions

vfs_node* vfs_create_node(char* name, bool copy_name, uint32 attributes, uint32 length, uint32 deep_metadata_length);
vfs_node* vfs_find_child(vfs_node* node, char* name);

void init_vfs();

void print_vfs(vfs_node* node, int level);
void vfs_print_node(vfs_node* node);

void vfs_test();

#endif