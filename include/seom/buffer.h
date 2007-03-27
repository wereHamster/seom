
#ifndef __SEOM_BUFFER_H__
#define __SEOM_BUFFER_H__

typedef struct seomBuffer
{
	pthread_mutex_t mutex;
	pthread_cond_t cond;

	unsigned long count;
	unsigned long head, tail;

	void *array[0];
} seomBuffer;

seomBuffer *seomBufferCreate(unsigned long size, unsigned long count);
void seomBufferDestroy(seomBuffer *buffer);

void *seomBufferHead(seomBuffer *buffer);
void seomBufferHeadAdvance(seomBuffer *buffer);
void *seomBufferTail(seomBuffer *buffer);
void seomBufferTailAdvance(seomBuffer *buffer);

unsigned long seomBufferStatus(seomBuffer *buffer);

#endif /* __SEOM_BUFFER_H__ */
