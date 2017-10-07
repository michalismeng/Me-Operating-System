#ifndef SOCK_BUF_H_03102017
#define SOCK_BUF_H_03102017

#include "net.h"
#include "list.h"

struct sock_buf
{
	net_addr src_addrs[NET_STACK_LAYERS];	 // source addresses for each layer of the stack
	net_addr dst_addrs[NET_STACK_LAYERS];	 // destination addresses for each layer of the stack

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
uint32 sock_buf_get_header_len(sock_buf* buf);

error_t sock_buf_release(sock_buf* buf);
void sock_buf_reset(sock_buf* buf);

#endif