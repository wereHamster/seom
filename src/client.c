
#include <seom/seom.h>

static uint32_t streamGet(void *priv, void *data, uint32_t size)
{
	seomClient *client = priv;
	uint32_t *psize = data;
	
	psize[0] = htonl(client->size[0]);
	psize[1] = htonl(client->size[1]);
	
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
	seomFrame *dst = seomFrameCreate('c', client->size[0], client->size[1]);
	
	for (;;) {
		uint64_t start = seomTime();
		seomFrame *src = seomBufferTail(client->buffer);
		if (src->type == 0) {
			seomBufferTailAdvance(client->buffer);
			break;
		}
		
		client->copy(dst, src, client->size[0], client->size[1]);		
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

seomClient *seomClientCreate(Display *dpy, GLXDrawable drawable, const char *ns)
{
	Window root;
	unsigned int width, height, uunused;
	int sunused;
 
	XGetGeometry(dpy, drawable, &root, &sunused, &sunused, &width, &height, &uunused, &uunused);

	seomClient *client = malloc(sizeof(seomClient));
	if (client == NULL) {
		printf("seomClientStart(): out of memory\n");
		return NULL;
	}
	
	client->dpy = dpy;
	client->drawable = drawable;
	
	uint32_t insets[4];
	seomConfigInsets(ns, insets);
	
	if (insets[1] + insets[3] > width) {
		printf("seomClientStart(): right+left insets > width\n");
		insets[1] = insets[3] = 0;
	} else if (insets[0] + insets[2] > height) {
		printf("seomClientStart(): top+bottom insets > height\n");
		insets[0] = insets[2] = 0;
	}
	
	client->area[0] = insets[3];
	client->area[1] = insets[2];
	client->area[2] = width - insets[1] - insets[3];
	client->area[3] = height - insets[0] - insets[2];
	
	char scale[64];
	seomConfigScale(ns, scale);
	
	if (strcmp(scale, "full") == 0) {
		client->area[2] &= ~(1);
		client->area[3] &= ~(1);
		client->size[0] = client->area[2];
		client->size[1] = client->area[3];
		client->copy = copyFrameFull;
	} else {
		client->area[2] &= ~(3);
		client->area[3] &= ~(3);
		client->size[0] = client->area[2] >> 1;
		client->size[1] = client->area[3] >> 1;
		client->copy = copyFrameHalf;		
	}
	
	char server[256];
	seomConfigServer(ns, server);
	unsigned int serverPort;
	char serverAddr[64];
	int success = sscanf(server, "%s %u", serverAddr, &serverPort);
	if (success == 0) {
		printf("seomClientStart(): malformed server option\n");
		free(client);
		return NULL;
	}
	
	int fdSocket = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(serverPort);
	addr.sin_addr.s_addr = inet_addr(serverAddr);

	if (connect(fdSocket, &addr, sizeof(addr)) == 0) {
		client->socket = fdSocket;
	} else {
		close(fdSocket);
		perror("seomClientStart()");
		free(client);
		return NULL;
	}

	client->buffer = seomBufferCreate(sizeof(seomFrame) + client->area[2] * client->area[3] * 4, 16);	

	seomConfigInterval(ns, &client->interval);
	
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


void seomClientCapture(seomClient *client)
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
			uint32_t *area = client->area;
			
			seomFrame *frame = seomBufferHead(client->buffer);
			
			frame->type = 'r';
			frame->pts = timeCurrent;
			glReadPixels(area[0], area[1], area[2], area[3], GL_BGRA, GL_UNSIGNED_BYTE, &frame->data[0]);
			
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
