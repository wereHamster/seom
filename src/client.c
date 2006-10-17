
#include <seom/seom.h>

static uint32_t streamGet(void *priv, void *data, uint32_t size)
{
	seomClient *client = priv;
	uint32_t *psize = data;
	
	psize[0] = htonl(client->dst.size[0]);
	psize[1] = htonl(client->dst.size[1]);
	
	return size;
}

static uint32_t streamPut(void *priv, void *data, uint32_t size)
{
	seomClient *client = priv;
	write(client->socket, data, size);
	return size;
}

static seomStreamOps streamOps = {
	.get = streamGet,
	.pos = NULL,
	.put = streamPut
};


static void *seomClientThreadCallback(void *data)
{
	seomClient *client = data;
	seomFrame *dst = seomFrameCreate('c', client->dst.size[0], client->dst.size[1]);
	
	for (;;) {
		uint64_t start = seomTime();
		seomFrame *src = seomBufferTail(client->buffer);
		if (src->type == 0) {
			seomBufferTailAdvance(client->buffer);
			break;
		}
		
		client->copy(dst, src, client->src.size[0], client->src.size[1]);		
		seomStreamPut(client->stream, dst);
		
		seomBufferTailAdvance(client->buffer);
		
		double tElapsed = (double) ( seomTime() - start );
		
		pthread_mutex_lock(&client->mutex);
		double eInterval = client->stat.engineInterval;
		double eDecay = 1.0 / 60.0;
		client->stat.engineInterval = eInterval * ( 1.0 - eDecay ) + tElapsed * eDecay;
		pthread_mutex_unlock(&client->mutex);
	}
	
	seomFrameDestroy(dst);
	
	return NULL;
}


static void copyFrameFull(seomFrame *dst, seomFrame *src, uint32_t w, uint32_t h)
{
	seomFrameConvert(dst, src, w, h);
}

static void copyFrameHalf(seomFrame *dst, seomFrame *src, uint32_t w, uint32_t h)
{
	seomFrameResample(src, w, h);
	seomFrameConvert(dst, src, w / 2, h / 2);
}

seomClient *seomClientCreate(seomConfig *config, uint32_t width, uint32_t height)
{
	seomClient *client = malloc(sizeof(seomClient));
	if (client == NULL) {
		printf("seomClientStart(): out of memory\n");
		return NULL;
	}
	
	if (config->insets[1] + config->insets[3] > width) {
		printf("seomClientStart(): right+left insets > width\n");
		free(client);
		return NULL;
	} else if (config->insets[0] + config->insets[2] > height) {
		printf("seomClientStart(): top+bottom insets > height\n");
		free(client);
		return NULL;
	}
	
	client->src.size[0] = width - config->insets[1] - config->insets[3];
	client->src.size[1] = height - config->insets[0] - config->insets[2];
	
	if (config->scale) {
		client->copy = copyFrameHalf;
		
		client->src.size[0] &= (~3);
		client->dst.size[0] = client->src.size[0] >> 1;
		
		client->src.size[1] &= (~3);
		client->dst.size[1] = client->src.size[1] >> 1;
	} else {
		client->copy = copyFrameFull;
		
		client->src.size[0] &= (~1);
		client->dst.size[0] = client->src.size[0];
		
		client->src.size[1] &= (~1);
		client->dst.size[1] = client->src.size[1];
	}
	
	client->socket = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(client->socket, &config->addr, sizeof(config->addr))) {
		close(client->socket);
		perror("seomClientStart()");
		free(client);
		return NULL;
	}
	
	client->buffer = seomBufferCreate(sizeof(seomFrame) + client->src.size[0] * client->src.size[1] * 4, 16);	

	client->interval = config->interval;	
	client->stat.captureInterval = client->interval;
	client->stat.engineInterval = client->interval;
	client->stat.captureDelay = 0.0;

	client->stat.lastCapture = seomTime();
	
	pthread_mutex_init(&client->mutex, NULL);
	pthread_create(&client->thread, NULL, seomClientThreadCallback, client);
	
	client->stream = seomStreamCreate(&streamOps, client);
	
	return client;
}

void seomClientDestroy(seomClient *client)
{
	seomFrame *frame = seomBufferHead(client->buffer);
	frame->type = 0;
	seomBufferHeadAdvance(client->buffer);

	do { } while (seomBufferStatus(client->buffer) < 16);
	
	seomBufferDestroy(client->buffer);
	seomStreamDestroy(client->stream);
	
	close(client->socket);

	pthread_join(client->thread, NULL);
	pthread_mutex_destroy(&client->mutex);

	free(client);
}

static void capture(uint32_t x, uint32_t y, uint32_t w, uint32_t h, void *p)
{
	static void (*glReadPixels)(GLint x, GLint y, GLsizei w, GLsizei h, GLenum f, GLenum t, GLvoid *p);
	
	if (glReadPixels == NULL) {
		glReadPixels = dlsym(RTLD_DEFAULT, "glReadPixels");
	}
	
	if (glReadPixels) {
		(*glReadPixels)(x, y, w, h, GL_BGRA, GL_UNSIGNED_BYTE, p);
	}
}

void seomClientCapture(seomClient *client, uint32_t xoffset, uint32_t yoffset)
{
	uint64_t bufferStatus = seomBufferStatus(client->buffer);
	
	pthread_mutex_lock(&client->mutex);
	double eInterval = client->stat.engineInterval;
	pthread_mutex_unlock(&client->mutex);
	
	double cInterval = client->stat.captureInterval;
	int64_t bStatus = 8 - bufferStatus;
	double iCorrection = ( eInterval + bStatus * 100 ) - cInterval;
	client->stat.captureInterval = cInterval * 0.9 + ( cInterval + iCorrection ) * 0.1;
	if (client->stat.captureInterval < client->interval) {
		client->stat.captureInterval = client->interval;
	}

	uint64_t timeCurrent = seomTime();
	uint64_t timeElapsed = timeCurrent - client->stat.lastCapture;
	client->stat.lastCapture = timeCurrent;
	
	double tElapsed = (double) timeElapsed;
	double tDelay = client->stat.captureDelay - tElapsed;

	double delayMargin = client->stat.captureInterval / 10.0;
	if (tDelay < delayMargin) {
		if (bufferStatus) {			
			seomFrame *frame = seomBufferHead(client->buffer);
			
			frame->type = 'r';
			frame->pts = timeCurrent;
			capture(xoffset, yoffset, client->src.size[0], client->src.size[1], &frame->data[0]);
			
			seomBufferHeadAdvance(client->buffer);

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
