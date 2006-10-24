
#ifndef __SEOM_CLIENT_H__
#define __SEOM_CLIENT_H__

#include <seom/seom.h>

typedef struct seomClient {
	pthread_mutex_t mutex;
	pthread_t thread;
	
	uint32_t size[2];
	uint32_t scale;
	
	seomBuffer *buffer;
	
	double interval;
	
	struct {
		double captureInterval;
		double captureDelay;
		uint64_t lastCapture;
		double engineInterval;
	} stat;
	
	seomStream *stream;
} seomClient;

typedef struct seomClientConfig {
	char *spec;
	uint32_t size[2];
	double fps;
	uint8_t scale;
} seomClientConfig;

seomClient *seomClientCreate(char *spec, uint32_t width, uint32_t height, double fps);
void seomClientCapture(seomClient *client, uint32_t xoffset, uint32_t yoffset);
void seomClientDestroy(seomClient *client);

#endif /* __SEOM_CLIENT_H__ */
