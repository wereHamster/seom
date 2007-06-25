
#include <seom/seom.h>

static void copyFrame(seomFrame *dst, seomFrame *src, uint32_t size[2], uint32_t scale)
{
	uint32_t tmp[2] = { size[0], size[1] };

	while (scale--) {
		seomFrameResample(src, tmp);
		tmp[0] >>= 1; tmp[1] >>= 1;
	}

	seomFrameConvert(dst, src, tmp);
}

static void *clientThread(void *data)
{
	seomClient *client = data;

	uint32_t size[2] = { client->size[0] >> client->scale, client->size[1] >> client->scale };
	seomFrame *dst = seomFrameCreate('c', size);

	unsigned long codecBufferLength = sizeof(uint64_t) + size[0] * size[1] * 3;
	void *codecBuffer = malloc(codecBufferLength); /* FIXME: check return value */

	seomFrame *src;
	for (;;) {
		src = (seomFrame *) seomQueuePop(client->queue);
		if (__builtin_expect(src->pts == 0, 0))
			break;

		uint64_t start = seomTime();
		copyFrame(dst, src, client->size, client->scale);
		*(uint64_t *) codecBuffer = src->pts;
		void *end = seomCodecEncode(codecBuffer + sizeof(uint64_t), dst->data, size[0] * size[1] * 3 / 2);
		seomStreamPut(client->stream, client->videoSubStreamID, end - codecBuffer, codecBuffer);
		double tElapsed = (double) ( seomTime() - start );

		seomQueueFree(client->queue, (seomStreamPacket *) src);
		
		const double eDecay = 1.0 / 60.0;
		pthread_mutex_lock(&client->mutex);
		client->stat.engineInterval = client->stat.engineInterval * ( 1.0 - eDecay ) + tElapsed * eDecay;
		pthread_mutex_unlock(&client->mutex);
		
	}

	seomQueueFree(client->queue, (seomStreamPacket *) src);
	seomFrameDestroy(dst);

	return NULL;
}

seomClient *seomClientCreate(seomClientConfig *config)
{
	seomClient *client = malloc(sizeof(seomClient));
	if (__builtin_expect(client == NULL, 0))
		return NULL;
	
	client->scale = config->scale;
	
	client->size[0] = config->size[0] & ~((1 << (client->scale + 1)) - 1);
	client->size[1] = config->size[1] & ~((1 << (client->scale + 1)) - 1);
	
	uint32_t size[2] = { client->size[0] >> config->scale, client->size[1] >> config->scale };
	if (size[0] == 0 || size[1] == 0)
		return NULL;
	
	client->stream = seomStreamCreate(config->output);
	if (__builtin_expect(client->stream == NULL, 0)) {
		free(client);
		return NULL;
	}
	
	client->videoSubStreamID = seomStreamInsert(client->stream, 'v', sizeof(size), size);
	
	client->queue = seomQueueCreate();	

	client->interval = 1000000.0 / (1.1 * config->fps);	
	client->stat.captureInterval = client->interval;
	client->stat.engineInterval = client->interval;
	client->stat.captureDelay = 0.0;

	client->stat.lastCapture = seomTime();
	
	pthread_mutex_init(&client->mutex, NULL);
	pthread_create(&client->thread, NULL, clientThread, client);
	
	return client;
}

void seomClientDestroy(seomClient *client)
{
	seomFrame *frame = (seomFrame *) seomQueueAlloc(client->queue, sizeof(seomFrame));
	frame->pts = 0;
	seomQueuePush(client->queue, (seomStreamPacket *) frame);

	do { } while (seomQueueLength(client->queue) > 0);
	
	seomQueueDestroy(client->queue);
	seomStreamDestroy(client->stream);

	pthread_join(client->thread, NULL);
	pthread_mutex_destroy(&client->mutex);

	free(client);
}

void seomClientCapture(seomClient *client, unsigned long xoff, unsigned long yoff)
{
	const unsigned long queueLength = seomQueueLength(client->queue);
	//fprintf(stderr, "%lu\n", queueLength);

	pthread_mutex_lock(&client->mutex);
	const double eInterval = client->stat.engineInterval;
	pthread_mutex_unlock(&client->mutex);

	double cInterval = client->stat.captureInterval;
	double iCorrection = ( eInterval + (-2.0 + queueLength) * 100.0 ) - cInterval;

	client->stat.captureInterval = cInterval * 0.9 + ( cInterval + iCorrection ) * 0.1;
	if (client->stat.captureInterval < client->interval)
		client->stat.captureInterval = client->interval;

	const uint64_t timeCurrent = seomTime();
	const double tElapsed = (double) (timeCurrent - client->stat.lastCapture);
	client->stat.lastCapture = timeCurrent;

	const double tDelay = client->stat.captureDelay - tElapsed;
	const double delayMargin = client->stat.captureInterval / 10.0;
	if (tDelay < delayMargin) {
		if (queueLength < 8) {
			seomFrame *frame = (seomFrame *) seomQueueAlloc(client->queue, sizeof(seomFrame) + client->size[0] * client->size[1] * 4);
			
			frame->pts = timeCurrent;
			glReadPixels(xoff, yoff, client->size[0], client->size[1], GL_BGRA, GL_UNSIGNED_BYTE, &frame->data[0]);

			seomQueuePush(client->queue, (seomStreamPacket *) frame);

			if (tDelay < 0) { // frame too late
				if (client->stat.captureInterval + tDelay < 0.0) { // lag? drop frame(s) and return to normal frame interval
					client->stat.captureDelay = client->stat.captureInterval;
				} else { // frame too late, adjust capture interval
					client->stat.captureDelay = client->stat.captureInterval + tDelay;
				}
			} else { // frame too early, adjust capture interval
				client->stat.captureDelay = client->stat.captureInterval + tDelay;
			}
		} else { // encoder too slow, try next frame
			if (tDelay < 0) { // we are already too late
				client->stat.captureDelay = 0;
			} else { // we get another chance to capture in time
				client->stat.captureDelay = tDelay;
			}
		}
	} else { // normal update
		client->stat.captureDelay = tDelay;
	}
}
