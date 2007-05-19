
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
	unsigned char videoSubStreamID;
} seomClient;

typedef struct seomClientConfig {
	uint32_t size[2];
	uint32_t scale;
	double fps;
	char *output;
} seomClientConfig;

seomClient *seomClientCreate(seomClientConfig *config);
void seomClientCapture(seomClient *client, unsigned long xoff, unsigned long yoff);
void seomClientDestroy(seomClient *client);

#endif /* __SEOM_CLIENT_H__ */
