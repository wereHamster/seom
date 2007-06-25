
#include <seom/seom.h>

seomQueue *seomQueueCreate(void)
{
	seomQueue *queue = malloc(sizeof(seomQueue));
	if (__builtin_expect(queue == NULL, 0))
		return NULL;

	pthread_mutex_init(&queue->mutex, NULL);
	pthread_cond_init(&queue->cond, NULL);

	queue->stack = queue->head = queue->tail = NULL;
	queue->length = 0;

	return queue;
}

void seomQueueDestroy(seomQueue *queue)
{		
	while (queue->stack)
		free((void *) seomQueueAlloc(queue, 1) - sizeof(seomQueueElement));

	while (queue->length > 0) 
		free((void *) seomQueuePop(queue) - sizeof(seomQueueElement));

	pthread_mutex_destroy(&queue->mutex);
	pthread_cond_destroy(&queue->cond);

	free(queue);
}

seomStreamPacket *seomQueueAlloc(seomQueue *queue, uint64_t payloadLength)
{
	pthread_mutex_lock(&queue->mutex);

	seomQueueElement *ret;
	if (queue->stack) {
		ret = queue->stack;
		queue->stack = queue->stack->next;
		if (ret->size < payloadLength) {
			void *tmp = realloc(ret, sizeof(seomQueueElement) + sizeof(seomStreamPacket) + payloadLength);
			if (tmp) {
				ret = tmp;
				ret->size = payloadLength;
			} else {
				free(tmp);
				ret = NULL;
			}
		}
	} else {
		ret = malloc(sizeof(seomQueueElement) + sizeof(seomStreamPacket) + payloadLength);
		ret->size = payloadLength;
	}

	pthread_mutex_unlock(&queue->mutex);

	return ret ? (seomStreamPacket *) ((void *) ret + sizeof(seomQueueElement)) : NULL;
}

void seomQueuePush(seomQueue *queue, seomStreamPacket *packet)
{
	pthread_mutex_lock(&queue->mutex);

	if (queue->length == 0) {
		queue->head = queue->tail = (seomQueueElement *) ((void *) packet - sizeof(seomQueueElement));
	} else {
		queue->tail->next = (seomQueueElement *) ((void *) packet - sizeof(seomQueueElement));
		queue->tail = queue->tail->next;
		queue->tail->next = NULL;
	}

	++queue->length;

	pthread_mutex_unlock(&queue->mutex);
	pthread_cond_broadcast(&queue->cond);
}

seomStreamPacket *seomQueuePop(seomQueue *queue)
{
	pthread_mutex_lock(&queue->mutex);
	while (queue->length == 0)
		pthread_cond_wait(&queue->cond, &queue->mutex);

	seomStreamPacket *ret = (seomStreamPacket *) ((void *) queue->head + sizeof(seomQueueElement));
	queue->head = queue->head->next;

	--queue->length;

	pthread_mutex_unlock(&queue->mutex);

	return ret;
}

void seomQueueFree(seomQueue *queue, seomStreamPacket *packet)
{
	pthread_mutex_lock(&queue->mutex);

	seomQueueElement *tmp = queue->stack;
	queue->stack = (seomQueueElement *) ((void *) packet - sizeof(seomQueueElement));
	queue->stack->next = tmp;

	pthread_mutex_unlock(&queue->mutex);
}

unsigned long seomQueueLength(seomQueue *queue)
{
	pthread_mutex_lock(&queue->mutex);
	unsigned long ret = queue->length;
	pthread_mutex_unlock(&queue->mutex);

	return ret;
}

