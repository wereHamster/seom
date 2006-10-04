
#ifndef __SEOM_CLIENT_H__
#define __SEOM_CLIENT_H__

#include <seom/seom.h>

typedef struct _seomClient seomClient;
typedef struct _seomClientFrame seomClientFrame;

seomClient *seomClientCreate(Display *dpy, GLXDrawable drawable, const char *ns);
void seomClientCapture(seomClient *client);
void seomClientDestroy(seomClient *client);

struct _seomClient {
	Display *dpy;
	GLXDrawable drawable;

	pthread_mutex_t mutex;
	pthread_t thread;
	
	seomBuffer *buffer;
	void (*copy)(uint8_t *out[3], uint32_t *in, uint64_t w, uint64_t h);
	
	uint64_t area[4];
	uint64_t size[2];
	
	double interval;
	
	struct {
		double captureInterval;
		double captureDelay;
		uint64_t lastCapture;
		double engineInterval;
	} stat;
	
	int socket;
};

struct _seomClientFrame {
	uint64_t pts;
	uint8_t data[0];
};

void seomResample(uint32_t *buf, uint64_t w, uint64_t h);
void seomConvert(uint8_t *out[3], uint32_t *in, uint64_t w, uint64_t h);


#endif /* __SEOM_CLIENT_H__ */
