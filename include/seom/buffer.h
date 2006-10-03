
#ifndef __SEOM_BUFFER_H__
#define __SEOM_BUFFER_H__

typedef struct _seomBuffer seomBuffer;

struct _seomBuffer
{
	pthread_mutex_t mutex;
	pthread_cond_t cond;

	uint64_t size;
	uint64_t count;

	uint64_t head;
	uint64_t tail;

	uint64_t bufferCount;

	void *array[0];
};

seomBuffer *seomBufferCreate(uint64_t size, uint64_t count);
void seomBufferDestroy(seomBuffer *buffer);

void *seomBufferHead(seomBuffer *buffer);
void seomBufferHeadAdvance(seomBuffer *buffer);
void *seomBufferTail(seomBuffer *buffer);
void seomBufferTailAdvance(seomBuffer *buffer);

uint64_t seomBufferStatus(seomBuffer *buffer);


#endif /* __SEOM_BUFFER_H__ */
