#include "vfs.h"

vfs_node* root;

vfs_node* vfs_create_node(char* name, bool copy_name, uint32 attributes, uint32 length, uint32 deep_metadata_length)
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
	n->shallow_md.file_length = length;

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

void init_vfs()
{
	// create root - /dev
	root = vfs_create_node("root", false, 0, 0, 0);
	auto dev = vfs_create_node("dev", false, 0, 0, 0);

	list_insert_back(&root->children, dev);

	list_insert_back(&dev->children, vfs_create_node("sda", false, 0, 0, 0));
	list_insert_back(&dev->children, vfs_create_node("sdb", false, 0, 0, 0));
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
	printf("%s\t%u bytes, %u", node->shallow_md.name, node->shallow_md.file_length, node->shallow_md.attributes);
}

void vfs_test()
{
	init_vfs();

	auto temp = vfs_create_node("folder", false, 0, 0, 0);
	auto temp2 = vfs_create_node("michalis.txt", false, 0, 1, 0);

	list_insert_back(&root->children, temp);
	list_insert_back(&vfs_find_child(root, "folder")->children, temp2);

	print_vfs(root, 0);

	printfln("finish");
}