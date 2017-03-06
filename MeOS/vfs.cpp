#include "vfs.h"

// private functions and data

vfs_node* root;

vfs_result vfs_default_read(vfs_node* node, uint32 page, virtual_addr address);
vfs_result vfs_default_write(vfs_node* node, uint32 page, virtual_addr address);
vfs_result vfs_default_open(vfs_node* node);
vfs_result vfs_default_lookup(vfs_node* parent, char* path, vfs_node** result);

static fs_operations default_fs_operations =
{
	vfs_default_read,
	vfs_default_write,
	vfs_default_open,
	NULL,
	vfs_default_lookup,
	NULL
};

vfs_node* vfs_find_child(vfs_node* node, char* name)
{
	list_node<vfs_node*>* temp = node->children.head;

	while (temp != 0)
	{
		if (strcmp(name, temp->data->name) == 0)
			return temp->data;

		temp = temp->next;
	}

	return 0;
}

// fs default operations functions

vfs_result vfs_default_read(vfs_node* node, uint32 page, virtual_addr address)
{
	if (!node->tag)
		return VFS_INVALID_NODE_STRUCTURE;

	// return the node's tag (that is the mount point) read function
	return node->tag->fs_ops->fs_read(node, page, address);
}

vfs_result vfs_default_write(vfs_node* node, uint32 page, virtual_addr address)
{
	if (!node->tag)
		return VFS_INVALID_NODE_STRUCTURE;

	return node->tag->fs_ops->fs_write(node, page, address);
}

vfs_result vfs_default_open(vfs_node* node)
{
	if (!node->tag)
		return VFS_INVALID_NODE_STRUCTURE;

	// TODO: update open_count + register in open file
	return node->tag->fs_ops->fs_open(node);
}

vfs_result vfs_default_lookup(vfs_node* parent, char* path, vfs_node** result)
{
	*result = vfs_find_relative_node(parent, path);

	if (*result == 0)
		return VFS_PATH_NOT_FOUND;

	return VFS_OK;
}

// public functions

vfs_node* vfs_create_node(char* name, bool copy_name, uint32 attributes, uint32 file_length, uint32 deep_metadata_length, vfs_node* tag, fs_operations* file_fncs)
{
	vfs_node* n = (vfs_node*)malloc(sizeof(vfs_node) + deep_metadata_length);

	list_init(&n->children);

	n->name_length = strlen(name);
	n->tag = tag;

	if (file_fncs != 0)
		n->fs_ops = file_fncs;
	else
		n->fs_ops = &default_fs_operations;

	if (copy_name)
	{
		n->name = (char*)malloc(n->name_length + 1);		// deep copy name
		strcpy(n->name, name);
	}
	else
		n->name = name;										// shallow copy name

	n->attributes = attributes;
	n->file_length = file_length;
	n->is_open = false;										// by default file is closed and no operations can be done upon it

	return n;
}

vfs_node* vfs_get_dev()
{
	return vfs_find_child(root, "dev");
}

vfs_node* vfs_get_root()
{
	return root;
}

vfs_node* vfs_find_relative_node(vfs_node* start, char* path)
{
	//expected path: something/folder/another_folder/file.txt  (or just something till a folder level). No leading slash
	if (path == 0)
		return 0;

	vfs_node* next = start;
	char* slash;

	while (true)
	{
		slash = strchr(path, '/');

		if (slash == 0)		// remaining path contains no slashes
		{
			// just find the last-on-path node and return
			next = vfs_find_child(next, path);
			return next;
		}
		else
		{
			*slash = 0;							// null terminate the path substring to make it a temp string

			next = vfs_find_child(next, path);

			*slash = '/';						// restore the substring
			path = slash + 1;					// continue to the next substring
		}

		if (next == 0)	// path could not be constructed. Child not found
			return 0;
	}

	return 0;
}

vfs_node* vfs_find_node(char* path)
{
	//expected path: something/folder/another_folder/file.txt  (or just something till a folder level). No leading slash
	return vfs_find_relative_node(vfs_get_root(), path);
}

vfs_node* vfs_create_device(char* name, uint32 deep_metadata_length, vfs_node* tag, fs_operations* dev_fncs)
{
	vfs_node* node = vfs_create_node(name, true, VFS_DEVICE | VFS_READ, 0, deep_metadata_length, tag, dev_fncs);

	if (node == 0)
		return 0;

	list_insert_back(&vfs_get_dev()->children, node);
	return node;
}

void vfs_add_child(vfs_node* parent, vfs_node* child)
{
	list_insert_back(&parent->children, child);
}

void init_vfs()
{
	// create root - /dev
	root = vfs_create_node("root", false, 0, 0, 0, NULL, NULL);
	vfs_add_child(root, vfs_create_node("dev", false, 0, 0, 0, NULL, NULL));
}

void print_vfs(vfs_node* node, int level)
{
	char prefix = '-';

	if ((node->attributes & VFS_HIDDEN) == VFS_HIDDEN)
		prefix = '*';
	else if ((node->attributes & VFS_MOUNT_PT) == VFS_MOUNT_PT)
		prefix = '+';
	else if ((node->attributes & VFS_LINK) == VFS_LINK)
		prefix = '~';

	for (int i = 0; i < level; i++)
		printf("%c", prefix);
	vfs_print_node(node);
	printfln("");

	list_node<vfs_node*>* temp = node->children.head;

	while (temp != 0)
	{
		print_vfs(temp->data, level + 1);
		temp = temp->next;
	}
}

void vfs_print_node(vfs_node* node)
{
	if (node == 0)
	{
		printf("null node");
		return;
	}
	printf("%s\t%u bytes, attribute: %h", node->name, node->file_length, node->attributes);
}

void vfs_print_all()
{
	print_vfs(root, 0);
}

vfs_result vfs_read_file(vfs_node* node, uint32 page, virtual_addr address)
{
	if (!node)
		return VFS_INVALID_NODE;

	if (node->is_open == false)
		return VFS_FILE_NOT_OPEN;

	// TODO: Check read permissions
	return node->fs_ops->fs_read(node, page, address);
}

vfs_result vfs_write_file(vfs_node* node, uint32 page, virtual_addr address)
{
	if (!node)
		return VFS_INVALID_NODE;

	if (node->is_open == false)
		return VFS_FILE_NOT_OPEN;

	// TODO: Check read permissions
	return node->fs_ops->fs_write(node, page, address);
}

vfs_result vfs_open_file(vfs_node* node)
{
	if (!node)
		return VFS_INVALID_NODE;

	if (node->is_open == true)
		return VFS_OK;

	node->is_open = true;
	return node->fs_ops->fs_open(node);
}

vfs_result vfs_lookup(vfs_node* parent, char* path, vfs_node** result)
{
	if (!parent)
		return VFS_INVALID_NODE;

	return parent->fs_ops->fs_lookup(parent, path, result);
}

vfs_result vfs_root_lookup(char* path, vfs_node** result)
{
	return vfs_lookup(vfs_get_root(), path, result);
}