#ifndef	NET_PROTOCOL_H_03102017
#define NET_PROTOCOL_H_03102017

#include "types.h"
#include "net.h"
#include "sock_buf.h"
#include "vector.h"

struct net_operations
{
	error_t(*recv)(sock_buf* buffer);
	error_t(*send)(sock_buf* buffer);
};

struct net_stats
{
	uint32 pkts_sent;			// total packets sent
	uint32 pkts_recvd;			// total packets received
	uint32 pkts_dropped;		// total packet dropped
	uint32 pkts_rejected;		// total packets rejected
};

struct net_protocol;

struct net_layer
{
	vector<net_protocol> protocols;
	//net_layer* next_layer;				// layer that is closer to the user
	//net_layer* prev_layer;				// layer that is closer to the hardware
	uint32 layer_ind;						// layer index
};

struct net_protocol
{
	uint32 identifier;
	net_operations ops;
	net_stats stats;
	// protocol filters
	// protocol layer ptr
};

net_protocol net_protocol_create(uint32 id, net_operations operations);

error_t net_layer_init(uint32 layer_ind, uint32 proto_count);

error_t net_layer_register_proto(uint32 layer_ind, net_protocol proto);

net_protocol* net_layer_get_proto(uint32 layer_ind, uint32 proto_id);


#endif