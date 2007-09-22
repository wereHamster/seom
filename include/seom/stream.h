
#ifndef __SEOM_STREAM_H__
#define __SEOM_STREAM_H__

#include <seom/base.h>
#include <seom/codec.h>
#include <seom/packet.h>

struct seomStream {
	int fileDescriptor;
	struct {
		unsigned long size;
		void *data;
	} buffer;
};

typedef struct seomStream seomStream;

struct seomStream *seomStreamCreate(int fileDescriptor);
void seomStreamPut(struct seomStream *stream, struct seomPacket *packet);
struct seomPacket *seomStreamGet(struct seomStream *stream);
void seomStreamDestroy(struct seomStream *stream);

#endif /* __SEOM_STREAM_H__ */
