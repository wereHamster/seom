
#include <seom/seom.h>

seomBuffer *seomBufferCreate(uint64_t size, uint64_t count)
{
	seomBuffer *buffer = malloc(sizeof(seomBuffer) + count * (size + sizeof(void *)));
	if (buffer == 0) {
		return 0;
	}

	pthread_mutex_init(&buffer->mutex, NULL);
	pthread_cond_init(&buffer->cond, NULL);

	buffer->size = size;
	buffer->count = count;

	buffer->head = 0;
	buffer->tail = 0;

	buffer->bufferCount = 0;

	for (uint64_t bufferIndex = 0; bufferIndex < count; ++bufferIndex) {
		buffer->array[bufferIndex] = (void *)buffer + sizeof(seomBuffer) + count * sizeof(void *) + bufferIndex * size;
	}

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
	void *retVal = 0;

	pthread_mutex_lock(&buffer->mutex);

	while (buffer->bufferCount == buffer->count) {
		pthread_cond_wait(&buffer->cond, &buffer->mutex);
	}

	retVal = buffer->array[buffer->head];

	pthread_mutex_unlock(&buffer->mutex);

	return retVal;
}


void seomBufferHeadAdvance(seomBuffer *buffer)
{
	pthread_mutex_lock(&buffer->mutex);

	buffer->head = (buffer->head + 1) % buffer->count;
	++buffer->bufferCount;

	pthread_mutex_unlock(&buffer->mutex);

	pthread_cond_broadcast(&buffer->cond);
}

void *seomBufferTail(seomBuffer *buffer)
{
	void *retVal = 0;

	pthread_mutex_lock(&buffer->mutex);

	while (buffer->bufferCount == 0) {
		pthread_cond_wait(&buffer->cond, &buffer->mutex);
	}

	retVal = buffer->array[buffer->tail];

	pthread_mutex_unlock(&buffer->mutex);

	return retVal;
}

void seomBufferTailAdvance(seomBuffer *buffer)
{
	pthread_mutex_lock(&buffer->mutex);

	while (buffer->bufferCount == 0) {
		pthread_cond_wait(&buffer->cond, &buffer->mutex);
	}
	
	buffer->tail = (buffer->tail + 1) % buffer->count;
	--buffer->bufferCount;

	pthread_mutex_unlock(&buffer->mutex);

	pthread_cond_broadcast(&buffer->cond);
}

uint64_t seomBufferStatus(seomBuffer *buffer)
{
	return buffer->count - buffer->bufferCount;
}

