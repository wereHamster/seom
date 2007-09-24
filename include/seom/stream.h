
#ifndef __SEOM_STREAM_H__
#define __SEOM_STREAM_H__

#include <seom/base.h>
#include <seom/codec.h>
#include <seom/packet.h>

struct seomStreamOps {
	unsigned long (*put)(void *private, const struct iovec vec[], unsigned long num);
	unsigned long (*get)(void *private, const struct iovec vec[], unsigned long num);
};

struct seomStream {
	struct seomStreamOps *ops;
	void *private;

	void *context;

	struct {
		unsigned long size;
		void *data;
	} buffer;
};

typedef struct seomStream seomStream;

struct seomStream *seomStreamCreate(struct seomStreamOps *ops, void *private);
void seomStreamPut(struct seomStream *stream, struct seomPacket *packet);
struct seomPacket *seomStreamGet(struct seomStream *stream);
void seomStreamDestroy(struct seomStream *stream);

#endif /* __SEOM_STREAM_H__ */
