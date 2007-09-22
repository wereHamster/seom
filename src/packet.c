
#include <seom/packet.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static uint64_t getTime()
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (uint64_t) tv.tv_sec * 1000000 + tv.tv_usec;
}

static unsigned long *cache[64];

unsigned long *get(unsigned long size)
{
	unsigned char index = size;

	for (index = 0; index < 64; ++index) {
		if (cache[index])
			break;
	}

	if (index == 64)
		return NULL;

	unsigned long *ret = cache[index];
	cache[index] = NULL;
	return ret;
}

unsigned char put(unsigned long *obj)
{
	for (unsigned char index = 0; index < 64; ++index) {
		if (cache[index] == NULL) {
			cache[index] = obj;
			return index;
		}
	}
	
	return 255;
}

struct seomPacket *seomPacketCreate(unsigned char type, unsigned long size)
{
	pthread_mutex_lock(&mutex);
	unsigned long *obj = get(size);
	pthread_mutex_unlock(&mutex);

	if (obj == NULL) {
		obj = malloc(sizeof(unsigned long) + sizeof(struct seomPacket) + size);
		if (obj == NULL)
			return NULL;
		*obj = size;
	} else if (*obj < size) {
		unsigned long *copy = realloc(obj, sizeof(unsigned long) + sizeof(struct seomPacket) + size);
		if (copy == NULL)
			put(obj);
		else
			*copy = size;
		obj = copy;
	}

	if (obj == NULL)
		return NULL;

	struct seomPacket *packet = (struct seomPacket *) (obj + 1);

	packet->type = type;
	packet->time = getTime();
	packet->size = size;

	return packet;
}

void *seomPacketPayload(struct seomPacket *packet)
{
	return (void *) (packet + 1);
}

void seomPacketDestroy(struct seomPacket *packet)
{
	unsigned long *obj = (unsigned long *) packet - 1;

	pthread_mutex_lock(&mutex);
	if (put(obj) == 255)
		free(obj);
	pthread_mutex_unlock(&mutex);
}
