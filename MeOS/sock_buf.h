#ifndef SOCK_BUF_H_03102017
#define SOCK_BUF_H_03102017

#include "net.h"
#include "list.h"

struct sock_buf
{
	net_addr layer_addresses[STACK_LAYERS];	 // addresses for each layer of the stack
	void* head;
	void* data;
	void* tail;
};

struct sock_buf_group
{
	uint32 id;					// group id
	list<sock_buf> buffers;		// included buffers
};

typedef sock_buf SKB;

error_t sock_buf_init(sock_buf* buf, uint32 len);
error_t sock_buf_init_recv(sock_buf* buf, uint32 len, void* data);

void sock_buf_put(sock_buf* buf, void* data, uint32 len);
void sock_buf_pull(sock_buf* buf, void* data, uint32 len);
void sock_buf_push(sock_buf* buf, uint32 len);
void sock_buf_pop(sock_buf* buf, uint32 len);

uint32 sock_buf_get_len(sock_buf* buf);
uint32 sock_buf_get_data_len(sock_buf* buf);

error_t sock_buf_release(sock_buf* buf);

#endif