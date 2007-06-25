
#ifndef __SEOM_QUEUE_H__
#define __SEOM_QUEUE_H__

#include <seom/seom.h>

typedef struct seomQueueElement {
	struct seomQueueElement *next;
	uint64_t size;
} seomQueueElement;

typedef struct seomQueue
{
	pthread_mutex_t mutex;
	pthread_cond_t cond;

	seomQueueElement *stack, *head, *tail;
	unsigned long length;
} seomQueue;

seomQueue *seomQueueCreate(void);
void seomQueueDestroy(seomQueue *queue);

seomStreamPacket *seomQueueAlloc(seomQueue *queue, uint64_t payloadLength);
void seomQueuePush(seomQueue *queue, seomStreamPacket *packet);
seomStreamPacket *seomQueuePop(seomQueue *queue);
void seomQueueFree(seomQueue *queue, seomStreamPacket *packet);
unsigned long seomQueueLength(seomQueue *queue);

#endif /* __SEOM_QUEUE_H__ */
