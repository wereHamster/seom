
#ifndef __SEOM_SERVER_H__
#define __SEOM_SERVER_H__

#include <seom/seom.h>

#define seomServerPort 42803

typedef struct seomServerThread {
	pthread_t handle;
	int socket;
	int output;
} seomServerThread;

typedef struct seomServer {
	int socket;
	char *path;
	
	pthread_mutex_t mutex;
	seomServerThread thread[16];
} seomServer;

seomServer *seomServerCreate(char *path);
void seomServerDispatch(seomServer *server);
void seomServerDestroy(seomServer *server);

#endif /* __SEOM_SERVER_H__ */
