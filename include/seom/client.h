
#ifndef __SEOM_CLIENT_H__
#define __SEOM_CLIENT_H__

#include <seom/seom.h>

typedef struct _seomClient seomClient;
typedef struct _seomClientBuffer seomClientBuffer;

seomClient *seomClientCreate(Display *dpy, GLXDrawable drawable, const char *ns);
void seomClientCapture(seomClient *client);
void seomClientDestroy(seomClient *client);

struct _seomClient {
	Display *dpy;
	GLXDrawable drawable;

	pthread_mutex_t mutex;
	pthread_t thread;

	struct {
		struct {
			seomBuffer *videoBuffer;
		} video;
	} dataBuffers;

	struct {
		struct {
			struct {
				uint64_t width;
				uint64_t height;
			} drawableSize;
			double captureInterval;
			long downScale;
			uint64_t offset[2];
		} video;
	} staticInfo;

	struct {
		struct {
			double captureInterval;
			double captureDelay;
			uint64_t lastCapture;
			double engineInterval;
		} video;
	} captureStatistics;

	struct {
		struct {
			int outputFile;
		} video;
	} outputStreams;
	
	void (*scale)(uint32_t *in, uint32_t *out, uint64_t w, uint64_t h);
};

struct _seomClientBuffer {
	uint64_t timeStamp;
	char bufferData[0];
};

void seomResample(uint32_t *out, uint32_t *in, uint64_t w, uint64_t h);
void seomConvert(uint8_t *out[3], uint32_t *in, uint64_t w, uint64_t h);


#endif /* __SEOM_CLIENT_H__ */
