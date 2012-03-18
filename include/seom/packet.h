
#ifndef __SEOM_PACKET__
#define __SEOM_PACKET__

#include <seom/base.h>

struct seomPacket {
	uint8_t type;
	uint64_t time;
	uint64_t size;
} __attribute__((packed));

struct seomPacket *seomPacketCreate(unsigned char type, unsigned long size);
void *seomPacketPayload(struct seomPacket *packet);
void seomPacketDestroy(struct seomPacket *packet);

#endif /* __SEOM_PACKET__ */
