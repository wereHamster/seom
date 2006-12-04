
#ifndef __SEOM_STREAM_H__
#define __SEOM_STREAM_H__

#include <seom/seom.h>

typedef struct seomStream {
	int fd;
	
	uint32_t size[2];
	
	uint8_t *buffer;
} seomStream;

seomStream *seomStreamCreate(char type, char *spec, uint32_t size[2]);
seomFrame *seomStreamGet(seomStream *stream);
void seomStreamPut(seomStream *stream, seomFrame *frame);
void seomStreamDestroy(seomStream *stream);


#endif /* __SEOM_STREAM_H__ */
