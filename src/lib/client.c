
#include <seom/seom.h>

/* median(L, LT ,T) => L, L + T - LT, T */
static uint8_t median(uint8_t L, uint8_t LT, uint8_t T)
{
	return ( L + L+T-LT + T) / 3;
}

static void writePlane(seomClient *engine, uint8_t *plane, int width, int height)
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
	write(engine->outputStreams.video.outputFile, &size, sizeof(size));
	write(engine->outputStreams.video.outputFile, out, size);
}

static inline uint64_t read_time(void)
{
    uint64_t a, d;
    __asm__ __volatile__( "rdtsc" : "=a" (a), "=d" (d) );
    return (d << 32) | a;
}

static void *seomClientThreadCallback(void *data)
{
	seomClient *engine = data;
	
	printf("seomClient thread started\n");

	int srcWidth = engine->staticInfo.video.drawableSize.width;
	int srcHeight = engine->staticInfo.video.drawableSize.height;
	
	int destWidth = engine->staticInfo.video.downScale ? srcWidth / 2 : srcWidth;
	int destHeight = engine->staticInfo.video.downScale ? srcHeight / 2 : srcHeight;
	
	printf("seomClient: size: %d:%d => %d:%d\n", srcWidth, srcHeight, destWidth, destHeight);
	
	uint32_t *scaledFrame = malloc(destWidth * destHeight * 4);
	
	uint8_t *yuvPlanes[3];
	yuvPlanes[0] = malloc(destWidth * destHeight * 3 / 2);
	yuvPlanes[1] = yuvPlanes[0] + destWidth * destHeight;
	yuvPlanes[2] = yuvPlanes[1] + destWidth * destHeight / 4;
	
	uint64_t timeStart;
	uint64_t timeStop;
	uint64_t timeElapsed;

	while (1) {
		seomClientBuffer *videoBuffer = seomBufferTail(engine->dataBuffers.video.videoBuffer);
		if (videoBuffer->timeStamp == 0) {
			seomBufferTailAdvance(engine->dataBuffers.video.videoBuffer);
			break;
		}
				
		timeStart = seomTime();
		
		uint32_t *buf = (uint32_t *)&videoBuffer->bufferData[0];
		
		engine->scale(buf, buf, srcWidth, srcHeight);
		seomConvert(yuvPlanes, buf, destWidth, destHeight);
		
		uint64_t tStamp = videoBuffer->timeStamp;
		
		seomBufferTailAdvance(engine->dataBuffers.video.videoBuffer);
		
		write(engine->outputStreams.video.outputFile, &tStamp, sizeof(tStamp));
		writePlane(engine, yuvPlanes[0], destWidth, destHeight);
		writePlane(engine, yuvPlanes[1], destWidth / 2, destHeight / 2);
		writePlane(engine, yuvPlanes[2], destWidth / 2, destHeight / 2);
		
		timeStop = seomTime();
		timeElapsed = timeStop - timeStart;
		
		double tElapsed = (double) timeElapsed;
		
		pthread_mutex_lock(&engine->mutex);
		double eInterval = engine->captureStatistics.video.engineInterval;
		double eDecay = 1.0 / 60.0;
		engine->captureStatistics.video.engineInterval = eInterval * ( 1.0 - eDecay ) + tElapsed * eDecay;
		pthread_mutex_unlock(&engine->mutex);
	}
	
	free(yuvPlanes[0]);
}


static void scale_full(uint32_t *out, uint32_t *in, uint64_t w, uint64_t h)
{
	memcpy(out, in, w * h * 4);
}

static void scale_half(uint32_t *in, uint32_t *out, uint64_t w, uint64_t h)
{
	seomResample(out, in, w, h);
}


seomClient *seomClientCreate(Display *dpy, GLXDrawable drawable, const char *ns)
{
	Window rootWindow;
	int unused;
	int width, height;
	struct sockaddr_in addr;

	XGetGeometry(dpy, drawable, &rootWindow, &unused, &unused, &width, &height, &unused, &unused);

	struct timeval currentTime;
	gettimeofday(&currentTime, 0);

	seomClient *engine = malloc(sizeof(seomClient));
	if (engine == NULL) {
		printf("malloc\n");
		return NULL;
	}
	
	printf("seomClientStart(): %p - 0x%08x\n", dpy, drawable);
	
	uint64_t insets[4];
	seomConfigInsets(ns, insets);
	
	if (insets[1] + insets[3] > width) {
		printf("seomClientStart(): right+left insets > width\n");
		insets[1] = insets[3] = 0;
	} else if (insets[0] + insets[2] > width) {
		printf("seomClientStart(): top+bottom insets > height\n");
		insets[0] = insets[2] = 0;
	}
	
	printf("seomClientStart(): %p, insets: %llu:%llu:%llu:%llu\n", engine, insets[0], insets[1], insets[2], insets[3]);
	
	width = width - insets[1] - insets[3];
	height = height - insets[0] - insets[2];
	
	char scale[64];
	seomConfigScale(ns, scale);
	engine->staticInfo.video.downScale = strcmp(scale, "full");
	printf("scale: %s\n", scale);
	
	if (engine->staticInfo.video.downScale) {
		width &= ~(3);
		height &= ~(3);
		engine->scale = scale_half;
	} else {
		width &= ~(1);
		height &= ~(1);
		engine->scale = scale_full;
	}
	
	engine->staticInfo.video.offset[0] = insets[3];
	engine->staticInfo.video.offset[1] = insets[2];
	
	char server[256];
	seomConfigServer(ns, server);
	unsigned int serverPort;
	char serverAddr[64];
	int success = sscanf(server, "%s %u", serverAddr, &serverPort);
	if (success == 0) {
		printf("server\n");
		return NULL;
	}
	
	printf("server address is: %s:%d\n", serverAddr, serverPort);
	
	int fdSocket = socket(AF_INET, SOCK_STREAM, 0);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(serverPort);
	addr.sin_addr.s_addr = inet_addr(serverAddr);

	if (connect(fdSocket, &addr, sizeof(addr)) == 0) {
		engine->outputStreams.video.outputFile = fdSocket;
		printf("connection to server established\n");
	} else {
		close(fdSocket);
		perror("failed to connect to the server");
		return NULL;
	}
	
	engine->dpy = dpy;
	engine->drawable = drawable;

	engine->dataBuffers.video.videoBuffer = seomBufferCreate(sizeof(seomClientBuffer) + width * height * 4, 16);	
	
	engine->staticInfo.video.drawableSize.width = width;
	engine->staticInfo.video.drawableSize.height = height;

	seomConfigInterval(ns, &engine->staticInfo.video.captureInterval);
	
	printf("interval: %f\n", engine->staticInfo.video.captureInterval);
	
	engine->captureStatistics.video.captureInterval = engine->staticInfo.video.captureInterval;
	engine->captureStatistics.video.engineInterval = engine->staticInfo.video.captureInterval;
	engine->captureStatistics.video.captureDelay = 0.0;

	engine->captureStatistics.video.lastCapture = seomTime();
	
	pthread_create(&engine->thread, NULL, seomClientThreadCallback, engine);

	uint64_t streamWidth = (uint64_t) engine->staticInfo.video.downScale ? width / 2 : width;
	uint64_t streamHeight = (uint64_t) engine->staticInfo.video.downScale ? height / 2 : height;
	write(engine->outputStreams.video.outputFile, &streamWidth, sizeof(streamWidth));
	write(engine->outputStreams.video.outputFile, &streamHeight, sizeof(streamHeight));

	printf("seomClientStart(): %p, capturing %llu:%llu\n", engine, streamWidth, streamHeight);
	
	pthread_mutex_init(&engine->mutex, NULL);
	
	return engine;
}

void seomClientDestroy(seomClient *client)
{
	printf("seomClientDestroy(): %p\n", client);
	
	seomClientBuffer *videoBuffer = seomBufferHead(client->dataBuffers.video.videoBuffer);
	videoBuffer->timeStamp = 0;
	seomBufferHeadAdvance(client->dataBuffers.video.videoBuffer);

	do { } while (seomBufferStatus(client->dataBuffers.video.videoBuffer) < 16);
	
	seomBufferDestroy(client->dataBuffers.video.videoBuffer);
	
	close(client->outputStreams.video.outputFile);

	pthread_join(client->thread, NULL);
	pthread_mutex_destroy(&client->mutex);

	free(client);
}


void seomClientCapture(seomClient *client)
{
	uint64_t timeCurrent;
	uint64_t timeElapsed;

	Window rootWindow;
	int unused;
	int width;
	int height;
	
	//printf("drawable: %lx\n", drawable);

	Display *dpy = client->dpy;
	GLXDrawable drawale = client->drawable;
	seomClient *engine = client;

	width = engine->staticInfo.video.drawableSize.width;
	height = engine->staticInfo.video.drawableSize.height;

	uint64_t bufferStatus = seomBufferStatus(engine->dataBuffers.video.videoBuffer);
	
	pthread_mutex_lock(&engine->mutex);
	double eInterval = engine->captureStatistics.video.engineInterval;
	pthread_mutex_unlock(&engine->mutex);
	
	double cInterval = engine->captureStatistics.video.captureInterval;
	int64_t bStatus = 8 - bufferStatus;
	double iCorrection = ( eInterval + bStatus * 100 ) - cInterval;
	engine->captureStatistics.video.captureInterval = cInterval * 0.9 + ( cInterval + iCorrection ) * 0.1;
	if (engine->captureStatistics.video.captureInterval < engine->staticInfo.video.captureInterval) {
		engine->captureStatistics.video.captureInterval = engine->staticInfo.video.captureInterval;
	}

	timeCurrent = seomTime();
	timeElapsed = timeCurrent - engine->captureStatistics.video.lastCapture;
	engine->captureStatistics.video.lastCapture = timeCurrent;
	
	double tElapsed = (double) timeElapsed;
	double tDelay = engine->captureStatistics.video.captureDelay - tElapsed;
	
	//static char buf[1024];
	//int ret = sprintf(buf, "%llu %f %f %f %f\n", bufferStatus, eInterval, engine->captureStatistics.video.captureInterval, iCorrection, tElapsed);
	//write(engine->statFile, buf, ret);

	double delayMargin = engine->captureStatistics.video.captureInterval / 10.0;
	if (tDelay < delayMargin) {
		if (bufferStatus) {
			seomClientBuffer *videoBuffer = seomBufferHead(engine->dataBuffers.video.videoBuffer);

			videoBuffer->timeStamp = timeCurrent;
			uint64_t x = engine->staticInfo.video.offset[0];
			uint64_t y = engine->staticInfo.video.offset[1];
			glReadPixels(x, y, width, height, GL_BGRA, GL_UNSIGNED_BYTE, &videoBuffer->bufferData[0]);

			seomBufferHeadAdvance(engine->dataBuffers.video.videoBuffer);
		//	printf("timeElapsed.usec: %llu\n", timeElapsed.usec);

			if (tDelay < 0) { // frame too late
				if (engine->captureStatistics.video.captureInterval + tDelay < 0.0) { // lag? drop frame(s) and return to normal frame interval
					engine->captureStatistics.video.captureDelay = engine->captureStatistics.video.captureInterval;
				} else { // frame too late, adjust capture interval
					engine->captureStatistics.video.captureDelay = engine->captureStatistics.video.captureInterval + tDelay;
				}
			} else { // frame too early, adjust capture interval
				engine->captureStatistics.video.captureDelay = engine->captureStatistics.video.captureInterval + tDelay;
			}
		} else { // encoder too slow, try next frame
			if (tDelay < 0) { // we are already too late
				engine->captureStatistics.video.captureDelay = 0;
			} else { // we get another chance to capture in time
				engine->captureStatistics.video.captureDelay = tDelay;
			}
		}
	} else { // normal update
		engine->captureStatistics.video.captureDelay = tDelay;
	}

//	printf("bufferStatus: %llu, captureInterval: %llu, captureDelay: %llu\n\n", bufferStatus, engine->captureStatistics.video.captureInterval, engine->captureStatistics.video.captureDelay);
}
