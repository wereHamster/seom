
#include <seom/seom.h>

/* median(L, LT ,T) => L, L + T - LT, T */
static uint8_t median(uint8_t L, uint8_t LT, uint8_t T)
{
	return ( L + L+T-LT + T) / 3;
}

static void writePlane(seomClient *client, uint8_t *plane, int width, int height)
{
	static uint32_t out[1280*1024*8];
	
#define src(x,y) ( plane[(y)*width+(x)] )	
	for (int y = height - 1; y > 0; --y) {
		for (int x = width - 1; x > 0; --x) {
			plane[y * width + x] -= median(src(x-1,y), src(x-1,y-1), src(x,y-1));
		}
		plane[y * width] -= median(0, 0, src(0,y-1));
	}
	
	for (int x = width - 1; x > 0; --x) {
		plane[x] -= median(src(x-1,0), 0, 0);
	}
#undef src
	
	uint32_t *start = out;
	extern struct seomCodecTable seomCodecTable;
	uint32_t *end = seomCodecEncode(out, plane, plane + width * height, &seomCodecTable);
	uint64_t size = (end - start) * 4;
	write(client->socket, &size, sizeof(size));
	write(client->socket, out, size);
}

static inline uint64_t read_time(void)
{
    uint64_t a, d;
    __asm__ __volatile__( "rdtsc" : "=a" (a), "=d" (d) );
    return (d << 32) | a;
}

static void *seomClientThreadCallback(void *data)
{
	seomClient *client = data;
	
	uint8_t *yuvPlanes[3];
	yuvPlanes[0] = malloc(client->size[0] * client->size[1] * 3 / 2);
	yuvPlanes[1] = yuvPlanes[0] + client->size[0] * client->size[1];
	yuvPlanes[2] = yuvPlanes[1] + client->size[0] * client->size[1] / 4;

	for (;;) {
		uint64_t start = seomTime();
		seomClientFrame *frame = seomBufferTail(client->buffer);
		if (frame->pts == 0) {
			seomBufferTailAdvance(client->buffer);
			break;
		}
		
		client->copy(yuvPlanes, (uint32_t *)&frame->data[0], client->area[2], client->area[3]);
		
		uint64_t tStamp = frame->pts;
		
		seomBufferTailAdvance(client->buffer);
		
		write(client->socket, &tStamp, sizeof(tStamp));
		writePlane(client, yuvPlanes[0], client->size[0], client->size[1]);
		writePlane(client, yuvPlanes[1], client->size[0] / 2, client->size[1] / 2);
		writePlane(client, yuvPlanes[2], client->size[0] / 2, client->size[1] / 2);
		
		double tElapsed = (double) ( seomTime() - start );
		
		pthread_mutex_lock(&client->mutex);
		double eInterval = client->stat.engineInterval;
		double eDecay = 1.0 / 60.0;
		client->stat.engineInterval = eInterval * ( 1.0 - eDecay ) + tElapsed * eDecay;
		pthread_mutex_unlock(&client->mutex);
	}
	
	free(yuvPlanes[0]);
}


static void copyFrameFull(uint8_t *out[3], uint32_t *in, uint32_t w, uint32_t h)
{
	seomConvert(out, in, w, h);
}

static void copyFrameHalf(uint8_t *out[3], uint32_t *in, uint32_t w, uint32_t h)
{
	seomResample(in, w, h);
	seomConvert(out, in, w / 2, h / 2);
}

seomClient *seomClientCreate(Display *dpy, GLXDrawable drawable, const char *ns)
{
	Window root;
	int unused;
	int width, height;

	XGetGeometry(dpy, drawable, &root, &unused, &unused, &width, &height, &unused, &unused);

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

	client->buffer = seomBufferCreate(sizeof(seomClientFrame) + client->area[2] * client->area[3] * 4, 16);	

	seomConfigInterval(ns, &client->interval);
	
	client->stat.captureInterval = client->interval;
	client->stat.engineInterval = client->interval;
	client->stat.captureDelay = 0.0;

	client->stat.lastCapture = seomTime();
	
	uint32_t size[2] = { htonl(client->size[0]), htonl(client->size[1]) };
	
	write(client->socket, size, sizeof(size));
	
	pthread_mutex_init(&client->mutex, NULL);
	pthread_create(&client->thread, NULL, seomClientThreadCallback, client);
	
	return client;
}

void seomClientDestroy(seomClient *client)
{
	seomClientFrame *frame = seomBufferHead(client->buffer);
	frame->pts = 0;
	seomBufferHeadAdvance(client->buffer);

	do { } while (seomBufferStatus(client->buffer) < 16);
	
	seomBufferDestroy(client->buffer);
	
	close(client->socket);

	pthread_join(client->thread, NULL);
	pthread_mutex_destroy(&client->mutex);

	free(client);
}


void seomClientCapture(seomClient *client)
{
	Display *dpy = client->dpy;
	GLXDrawable drawale = client->drawable;

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
			
			seomClientFrame *frame = seomBufferHead(client->buffer);
			
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
