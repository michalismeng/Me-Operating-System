#include "vfs.h"

vfs_node* root;

vfs_node* vfs_create_node(char* name, bool copy_name, uint32 attributes, uint32 file_length, uint32 deep_metadata_length)
{
	vfs_node* n = (vfs_node*)malloc(sizeof(vfs_node) + deep_metadata_length);

	list_init(&n->children);

	n->shallow_md.name_length = strlen(name);

	if (copy_name)
	{
		n->shallow_md.name = (char*)malloc(n->shallow_md.name_length + 1);		// deep copy name
		strcpy(n->shallow_md.name, name);
	}
	else
		n->shallow_md.name = name;				// shallow copy name

	n->shallow_md.attributes = attributes;
	n->shallow_md.file_length = file_length;

	return n;
}

vfs_node* vfs_find_child(vfs_node* node, char* name)
{
	list_node<vfs_node*>* temp = node->children.head;

	while (temp != 0)
	{
		if (strcmp(name, temp->data->shallow_md.name) == 0)
			return temp->data;

		temp = temp->next;
	}

	return 0;
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

void* vfs_create_device(char* name, uint32 deep_metadata_length)
{
	auto node = vfs_create_node(name, true, VFS_DEVICE | VFS_READ, 0, deep_metadata_length);

	if (node == 0)
		return 0;

	list_insert_back(&vfs_get_dev()->children, node);
	return node->deep_md;
}

void init_vfs()
{
	// create root - /dev
	root = vfs_create_node("root", false, 0, 0, 0);
	list_insert_back(&root->children, vfs_create_node("dev", false, 0, 0, 0));
}

void print_vfs(vfs_node* node, int level)
{
	for (int i = 0; i < level; i++)
		printf("-");
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
	printf("%s\t%u bytes, %u", node->shallow_md.name, node->shallow_md.file_length, node->shallow_md.attributes);
}

void vfs_print_all()
{
	print_vfs(root, 0);
}