#include "test_dl_list.h"

bool test_dl_list_insert()
{
	dl_list<int> test;
	dl_list_init(&test);

	auto node1 = new dl_list_node<int>();
	node1->data = 15;

	auto node2 = new dl_list_node<int>();
	node2->data = 20;

	auto node3 = new dl_list_node<int>();
	node3->data = 25;

	dl_list_insert_back_node(&test, node1);
	dl_list_insert_back_node(&test, node2);
	dl_list_insert_back_node(&test, node3);

	for (auto temp = test.head; temp != 0; temp = temp->next)
		serial_printf("node: %u\n", temp->data);

	RET_SUCCESS;
}

bool test_dl_list_remove()
{
	dl_list<int> test;
	dl_list_init(&test);

	auto node1 = new dl_list_node<int>();
	node1->data = 15;

	auto node2 = new dl_list_node<int>();
	node2->data = 20;

	auto node3 = new dl_list_node<int>();
	node3->data = 25;

	auto node4 = new dl_list_node<int>();
	node4->data = 30;

	dl_list_insert_back_node(&test, node1);
	dl_list_insert_back_node(&test, node2);
	dl_list_insert_back_node(&test, node3);
	dl_list_insert_back_node(&test, node4);

	for (auto temp = test.head; temp != 0; temp = temp->next)
		serial_printf("node: %u\n", temp->data);

	serial_printf("removing node 2\n");

	dl_list_remove_node(&test, node2);

	for (auto temp = test.head; temp != 0; temp = temp->next)
		serial_printf("node: %u\n", temp->data);
	serial_printf("head: %u, tail: %u\n", test.head->data, test.tail->data);

	serial_printf("removing node 4\n");
	dl_list_remove_node(&test, node4);

	for (auto temp = test.head; temp != 0; temp = temp->next)
		serial_printf("node: %u\n", temp->data);
	serial_printf("head: %u, tail: %u\n", test.head->data, test.tail->data);

	serial_printf("removing node 1\n");

	dl_list_remove_node(&test, node1);
	for (auto temp = test.head; temp != 0; temp = temp->next)
		serial_printf("node: %u\n", temp->data);
	serial_printf("head: %u, tail: %u\n", test.head->data, test.tail->data);

	RET_SUCCESS;
}
