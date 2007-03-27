
#include <seom/seom.h>

static unsigned long num(seomBuffer *buffer)
{
	return ((buffer->head - buffer->tail) + buffer->count) % buffer->count;
}

seomBuffer *seomBufferCreate(unsigned long size, unsigned long count)
{
	seomBuffer *buffer = malloc(sizeof(seomBuffer) + count * (size + sizeof(void *)));
	if (__builtin_expect(buffer == NULL, 0))
		return NULL;

	pthread_mutex_init(&buffer->mutex, NULL);
	pthread_cond_init(&buffer->cond, NULL);

	buffer->count = count;

	buffer->head = 0;
	buffer->tail = 0;

	for (unsigned long index = 0; index < count; ++index)
		buffer->array[index] = (void *) buffer + sizeof(seomBuffer) + count * sizeof(void *) + index * size;

	return buffer;
}

void seomBufferDestroy(seomBuffer *buffer)
{
	pthread_mutex_destroy(&buffer->mutex);
	pthread_cond_destroy(&buffer->cond);

	free(buffer);
}

void *seomBufferHead(seomBuffer *buffer)
{
	pthread_mutex_lock(&buffer->mutex);
	while (num(buffer) == buffer->count)
		pthread_cond_wait(&buffer->cond, &buffer->mutex);

	void *ret = buffer->array[buffer->head];
	pthread_mutex_unlock(&buffer->mutex);

	return ret;
}


void seomBufferHeadAdvance(seomBuffer *buffer)
{
	pthread_mutex_lock(&buffer->mutex);

	buffer->head = (buffer->head + 1) % buffer->count;

	pthread_mutex_unlock(&buffer->mutex);
	pthread_cond_broadcast(&buffer->cond);
}

void *seomBufferTail(seomBuffer *buffer)
{
	pthread_mutex_lock(&buffer->mutex);
	while (num(buffer) == 0)
		pthread_cond_wait(&buffer->cond, &buffer->mutex);

	void *ret = buffer->array[buffer->tail];
	pthread_mutex_unlock(&buffer->mutex);

	return ret;
}

void seomBufferTailAdvance(seomBuffer *buffer)
{
	pthread_mutex_lock(&buffer->mutex);

	buffer->tail = (buffer->tail + 1) % buffer->count;

	pthread_mutex_unlock(&buffer->mutex);
	pthread_cond_broadcast(&buffer->cond);
}

unsigned long seomBufferStatus(seomBuffer *buffer)
{
	return buffer->count - num(buffer);
}

