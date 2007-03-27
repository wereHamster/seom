
#ifndef __SEOM_STREAM_H__
#define __SEOM_STREAM_H__

#include <seom/seom.h>

typedef struct {
	uint8_t subStreamID;
	uint64_t payloadLength;
} seomStreamPacket;

typedef struct {
	uint8_t subStreamID;
	uint8_t contentTypeID;
} seomStreamMap;

typedef struct seomStream {
	int fileDescriptor;
	unsigned char subStreams[256];
} seomStream;

seomStream *seomStreamCreate(char *spec);
unsigned char seomStreamInsert(seomStream *stream, unsigned char contentTypeID, unsigned long length, void *payload);
void seomStreamPut(seomStream *stream, unsigned char subStreamID, unsigned long length, void *payload);
void seomStreamRemove(seomStream *stream, unsigned char subStreamID);
void seomStreamDestroy(seomStream *stream);


#endif /* __SEOM_STREAM_H__ */
