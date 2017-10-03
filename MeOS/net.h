#ifndef NET_H_03102017
#define NET_H_03102017

#include "types.h"

#define STACK_LAYERS 4
#define MAX_NET_ADDRLEN 24

typedef struct { char value[MAX_NET_ADDRLEN]; } net_addr;

// returns the netowrk representation of the long host parameter
uint32 htonl(uint32 host);

// returns the network representation of the short host parameter
uint16 htons(uint16 host);

// returns the host representation of the long net parameter
uint32 ntohl(uint32 net);

// returns the host representation of the short net parameter
uint16 ntohs(uint16 net);

#endif