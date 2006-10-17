
#ifndef __SEOM_STREAM_H__
#define __SEOM_STREAM_H__

#include <seom/seom.h>

typedef struct seomStreamOps {
	uint32_t (*put)(void *priv, void *data, uint32_t size);
	uint64_t (*pos)(void *priv, uint64_t pos);
	uint32_t (*get)(void *priv, void *data, uint32_t size);
} seomStreamOps;

typedef struct seomStream {
	seomStreamOps *ops;
	void *priv;
	
	uint32_t size[2];
} seomStream;

seomStream *seomStreamCreate(seomStreamOps *ops, void *priv);
seomFrame *seomStreamGet(seomStream *stream);
void seomStreamPut(seomStream *stream, seomFrame *frame);
void seomStreamDestroy(seomStream *stream);


#endif /* __SEOM_STREAM_H__ */
