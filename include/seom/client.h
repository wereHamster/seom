
#ifndef __SEOM_CLIENT_H__
#define __SEOM_CLIENT_H__

#include <seom/seom.h>

typedef struct seomClient {
	pthread_mutex_t mutex;
	pthread_t thread;
	
	struct {
		uint32_t size[2];
	} src;
	
	struct {
		uint32_t size[2];
	} dst;
	
	seomBuffer *buffer;
	void (*copy)(seomFrame *dst, seomFrame *src, uint32_t w, uint32_t h);
	
	double interval;
	
	struct {
		double captureInterval;
		double captureDelay;
		uint64_t lastCapture;
		double engineInterval;
	} stat;
	
	seomStream *stream;
} seomClient;

seomClient *seomClientCreate(char *spec, uint32_t width, uint32_t height, double fps);
void seomClientCapture(seomClient *client, uint32_t xoffset, uint32_t yoffset);
void seomClientDestroy(seomClient *client);

#endif /* __SEOM_CLIENT_H__ */
