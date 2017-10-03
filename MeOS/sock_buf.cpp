#include "sock_buf.h"

error_t sock_buf_init(sock_buf* buf, uint32 len)
{
	buf->head = malloc(len);

	if (buf->head == 0)
		return ERROR_OCCUR;

	buf->data = buf->head;
	buf->tail = (uint8*)buf->head + len;

	for (int i = 0; i < STACK_LAYERS; i++)
		buf->layer_addresses[i] = { 0 };

	return ERROR_OK;
}

error_t sock_buf_init_recv(sock_buf* buf, uint32 len, void* data)
{
	if (sock_buf_init(buf, len) != ERROR_OK)
		return ERROR_OCCUR;

	memcpy(buf->data, data, len);
	return ERROR_OK;
}

void sock_buf_put(sock_buf* buf, void* data, uint32 len)
{
	memcpy(buf->data, data, len);
	buf->data = (uint8*)buf->data + len;
}

void sock_buf_pull(sock_buf* buf, void* data, uint32 len)
{
	buf->data = (uint8*) buf->data - len;
	memcpy(data, buf->data, len);
}

void sock_buf_push(sock_buf* buf, uint32 len)
{
	buf->data = (uint8*)buf->data + len;
}

void sock_buf_pop(sock_buf* buf, uint32 len)
{
	buf->data = (uint8*)buf->data - len;
}

uint32 sock_buf_get_len(sock_buf* buf)
{
	return (uint32)buf->tail - (uint32)buf->head;
}

uint32 sock_buf_get_data_len(sock_buf* buf)
{
	return (uint32)buf->data - (uint32)buf->head;
}

// TODO: Take into account multiple references to this buffer
error_t sock_buf_release(sock_buf* buf)
{
	if (free(buf->head) != ERROR_OK)
		return ERROR_OCCUR;

	return ERROR_OK;
}