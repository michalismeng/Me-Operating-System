#include "vfs.h"
#include "print_utility.h"

// private functions and data

vfs_node* root;

size_t vfs_default_read(uint32 fd, vfs_node* node, uint32 start, size_t count, virtual_addr address);
size_t vfs_default_write(uint32 fd, vfs_node* node, uint32 start, size_t count, virtual_addr address);
error_t vfs_default_sync(uint32 fd, vfs_node* node, uint32 start_page, uint32 end_page);
error_t vfs_default_open(vfs_node* node);
error_t vfs_default_lookup(vfs_node* parent, char* path, vfs_node** result);

static fs_operations default_fs_operations =
{
	vfs_default_read,
	vfs_default_write,
	vfs_default_open,
	NULL,
	vfs_default_sync,
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

size_t vfs_default_read(uint32 fd, vfs_node* node, uint32 start, size_t count, virtual_addr address)
{
	if (!node->tag)
	{
		set_last_error(EINVAL, VFS_INVALID_NODE_STRUCTURE, EO_VFS);
		return 0;
	}

	// return the node's tag (that is the mount point) read function
	return node->tag->fs_ops->fs_read(fd, node, start, count, address);
}

size_t vfs_default_write(uint32 fd, vfs_node* node, uint32 start, size_t count, virtual_addr address)
{
	if (!node->tag)
	{
		set_last_error(EINVAL, VFS_INVALID_NODE_STRUCTURE, EO_VFS);
		return 0;
	}

	return node->tag->fs_ops->fs_write(fd, node, start, count, address);
}

error_t vfs_default_sync(uint32 fd, vfs_node* node, uint32 start_page, uint32 end_page)
{
	if (!node->tag)
	{
		set_last_error(EINVAL, VFS_INVALID_NODE_STRUCTURE, EO_VFS);
		return ERROR_OCCUR;
	}

	return node->tag->fs_ops->fs_sync(fd, node, start_page, end_page);
}

error_t vfs_default_open(vfs_node* node)
{
	if (!node->tag)
	{
		set_last_error(EINVAL, VFS_INVALID_NODE_STRUCTURE, EO_VFS);
		return ERROR_OCCUR;
	}

	// TODO: update open_count + register in open file
	return node->tag->fs_ops->fs_open(node);
}

error_t vfs_default_lookup(vfs_node* parent, char* path, vfs_node** result)
{
	*result = vfs_find_relative_node(parent, path);

	if (*result == 0)
	{
		set_last_error(ENOENT, VFS_PATH_NOT_FOUND, EO_VFS);
		return ERROR_OCCUR;
	}

	return ERROR_OK;
}

// public functions

vfs_node* vfs_create_node(char* name, bool copy_name, uint32 attributes, uint32 file_length, uint32 deep_metadata_length, vfs_node* tag, 
							vfs_node* parent, fs_operations* file_fncs)
{
	vfs_node* n = (vfs_node*)malloc(sizeof(vfs_node) + deep_metadata_length);

	if (n == 0)			// allocation failed
		return 0;

	list_init(&n->children);

	n->name_length = strlen(name);
	n->tag = tag;
	n->parent = parent;

	if (file_fncs != NULL)
		n->fs_ops = file_fncs;
	else
		n->fs_ops = &default_fs_operations;

	if (file_fncs->fs_close == 0)
		file_fncs->fs_close = default_fs_operations.fs_close;
	if (file_fncs->fs_ioctl == 0)
		file_fncs->fs_ioctl = default_fs_operations.fs_ioctl;
	if (file_fncs->fs_lookup == 0)
		file_fncs->fs_lookup = default_fs_operations.fs_lookup;
	if (file_fncs->fs_open == 0)
		file_fncs->fs_open = default_fs_operations.fs_open;
	if (file_fncs->fs_read == 0)
		file_fncs->fs_read = default_fs_operations.fs_read;
	if (file_fncs->fs_write == 0)
		file_fncs->fs_write = default_fs_operations.fs_write;
	if (file_fncs->fs_sync == 0)
		file_fncs->fs_sync = default_fs_operations.fs_sync;


	if (copy_name)
	{
		n->name = (char*)malloc(n->name_length + 1);		// deep copy name
		if (n->name == 0)									// allocation failed
		{
			free(n);
			return 0;
		}

		strcpy(n->name, name);
	}
	else
		n->name = name;										// shallow copy name

	n->attributes = attributes;
	n->file_length = file_length;
	//n->is_open = false;									// by default file is closed and no operations can be done upon it

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

vfs_node* vfs_get_mount_point(vfs_node* node)
{
	vfs_node* mnt_point = node;

	while (mnt_point != NULL && CHK_BIT(mnt_point->attributes, VFS_MOUNT_PT) == false)
		mnt_point = mnt_point->parent;

	return mnt_point;
}

vfs_node* vfs_get_parent(vfs_node* node)
{
	if (node == 0)
		return 0;
	return node->parent;
}

/**********************************************/

vfs_node* vfs_find_relative_node(vfs_node* start, char* path)
{
	//expected path: something/folder/another_folder/file.txt  (or just something till a folder level). No leading slash
	if (path == 0)
	{
		set_last_error(EINVAL, VFS_BAD_ARGUMENTS, EO_VFS);
		return 0;
	}

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

		if (next == 0)							// path could not be constructed. Child not found
		{
			set_last_error(ENOENT, VFS_PATH_NOT_FOUND, EO_VFS);
			return 0;
		}
	}

	// we should never reach here
	return 0;
}

vfs_node* vfs_find_node(char* path)
{
	//expected path: something/folder/another_folder/file.txt  (or just something till a folder level). No leading slash
	return vfs_find_relative_node(vfs_get_root(), path);
}

vfs_node* vfs_create_device(char* name, uint32 deep_metadata_length, vfs_node* tag, fs_operations* dev_fncs)
{
	vfs_node* node = vfs_create_node(name, true, VFS_DEVICE | VFS_READ, 0, deep_metadata_length, tag, vfs_get_dev(), dev_fncs);

	if (node == 0)
		return 0;

	list_insert_back(&vfs_get_dev()->children, node);
	return node;
}

void vfs_add_child(vfs_node* parent, vfs_node* child)
{
	list_insert_back(&parent->children, child);
	child->parent = parent;
}

void init_vfs()
{
	// create root - /dev
	root = vfs_create_node("root", false, VFS_DIRECTORY, 0, 0, NULL, NULL, NULL);
	vfs_add_child(root, vfs_create_node("dev", false, 0, 0, 0, NULL, root, NULL));
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

// returns a string representation of the given attributes. Max string length is 6 (F, DIR, L, DEV, MNT, P, R, W, H)
void vfs_attributes_to_string(uint32 attrs, char str[11])
{
	int pos = 0;

	switch (attrs & 7)
	{
	case VFS_FILE:		strcpy(str + pos, "F ");	pos += 2; break;
	case VFS_DIRECTORY: strcpy(str + pos, "DIR ");	pos += 4; break;
	case VFS_LINK:		strcpy(str + pos, "L ");	pos += 2; break;
	case VFS_DEVICE:	strcpy(str + pos, "DEV ");	pos += 4; break;
	case VFS_MOUNT_PT:	strcpy(str + pos, "MNT ");	pos += 4; break;
	case VFS_PIPE:		strcpy(str + pos, "P ");	pos += 2; break;
	default:			strcpy(str + pos, "U ");	pos += 2; break;
	}

	if ((attrs & VFS_READ) == VFS_READ)
	{
		strcpy(str + pos, "R ");
		pos += 2;
	}

	if ((attrs & VFS_WRITE) == VFS_WRITE)
	{
		strcpy(str + pos, "W ");
		pos += 2;
	}

	if ((attrs & VFS_HIDDEN) == VFS_HIDDEN)
	{
		strcpy(str + pos, "H ");
		pos += 2;
	}
}

void vfs_print_node(vfs_node* node)
{
	if (node == 0)
	{
		printf("null node");
		return;
	}

	char attrs_str[11] = { 0 };
	vfs_attributes_to_string(node->attributes, attrs_str);

	printf("%s\t%u bytes, attr: %h %s \t%s", node->name, node->file_length, node->attributes, attrs_str, node->parent->name);
}

void vfs_print_all()
{
	auto temp = root->children.head;
	while (temp)
	{
		print_vfs(temp->data, 0);
		temp = temp->next;
	}
}

size_t vfs_read_file(uint32 fd, vfs_node* node, uint32 start, size_t count, virtual_addr address)
{
	if (!node)
		return VFS_INVALID_NODE;

	// TODO: Check read permissions
	return node->fs_ops->fs_read(fd, node, start, count, address);
}

size_t vfs_write_file(uint32 fd, vfs_node* node, uint32 start, size_t count, virtual_addr address)
{
	if (!node)
		return VFS_INVALID_NODE;

	// TODO: Check read permissions
	uint32 written = node->fs_ops->fs_write(fd, node, start, count, address);
	/*if (start + written > node->file_length)
	{
		serial_printf("new length: %u", start + written);
		node->file_length = start + written;
		node->fs_ops->fs_ioctl(node, 0);
	}*/

	return written;
}

error_t vfs_sync(uint32 fd, vfs_node* node, uint32 page_start, uint32 page_end)
{
	if (!node)
		return VFS_INVALID_NODE;

	// TODO: Check read permissions
	return node->fs_ops->fs_sync(fd, node, page_start, page_end);
}

error_t vfs_open_file(vfs_node* node)
{
	if (!node)
		return VFS_INVALID_NODE;

	return node->fs_ops->fs_open(node);
}

error_t vfs_lookup(vfs_node* parent, char* path, vfs_node** result)
{
	if (!parent)
	{
		set_last_error(EINVAL, VFS_INVALID_NODE, EO_VFS);
		return ERROR_OCCUR;
	}

	if (parent->fs_ops->fs_lookup == 0)
		return vfs_default_lookup(parent, path, result);

	return parent->fs_ops->fs_lookup(parent, path, result);
}

error_t vfs_root_lookup(char* path, vfs_node** result)
{
	return vfs_lookup(vfs_get_root(), path, result);
}