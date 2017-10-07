#include "net_protocol.h"

net_layer net_layers[NET_STACK_LAYERS];

net_protocol net_protocol_create(uint32 id, net_operations operations)
{
	net_protocol proto;
	proto.identifier = id;
	proto.ops = operations;
	proto.stats.pkts_dropped = proto.stats.pkts_recvd = proto.stats.pkts_rejected = proto.stats.pkts_sent = 0;

	return proto;
}

error_t net_layer_init(uint32 layer_ind, uint32 proto_count)
{
	net_layers[layer_ind].layer_ind = layer_ind;
	return vector_init(&net_layers[layer_ind].protocols, proto_count);
}

error_t net_layer_register_proto(uint32 layer_ind, net_protocol proto)
{
	return vector_insert_back(&net_layers[layer_ind].protocols, proto);
}

// TODO: Add errors
net_protocol* net_layer_get_proto(uint32 layer_ind, uint32 proto_id)
{
	if (layer_ind >= NET_STACK_LAYERS)
		return 0;

	for (uint32 i = 0; i < net_layers[layer_ind].protocols.count; i++)
		if (net_layers[layer_ind].protocols[i].identifier == proto_id)
			return &net_layers[layer_ind].protocols[i];

	return 0;
}
