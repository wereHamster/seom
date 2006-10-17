
#include <seom/server.h>

static int output(char *path)
{
	char buf[4096];
	time_t tim = time(NULL);
	struct tm *tm = localtime(&tim);
	snprintf(buf, 4096, "%s/%d-%02d-%02d--%02d:%02d:%02d.seom", path, tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	return open(buf, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IRGRP | S_IROTH);
}

static void *loop(void *arg)
{
	seomServerThread *thread = arg;

	size_t bsize = 4096;
	char buffer[4096];
	
	for (;;) {
		int rb = read(thread->socket, buffer, bsize);
		if (rb == 0) {
			break;
		} else if (rb < 0) {
			if (errno == EINTR)
				continue;
			goto out;
		}

		char *p = buffer;
		do {
			int wb = write(thread->output, p, rb);
			if (wb == 0) {
				goto out;
			} else if (wb < 0) {
				if (errno == EINTR)
					continue;
				goto out;
			}
			p += wb;
			rb -= wb;
		} while (rb);
	}
	
out:
	close(thread->socket);
	close(thread->output);
	
	thread->handle = 0;
	
	return NULL;
}

seomServer *seomServerCreate(char *path)
{
	seomServer *server = malloc(sizeof(seomServer));
	
	server->socket = socket(AF_INET, SOCK_STREAM, 0);
	server->path = strdup(path);
	
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(seomServerPort);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	bind(server->socket, &addr, sizeof(addr));
	listen(server->socket, 1);
	
	pthread_mutex_init(&server->mutex, NULL);
	for (int i = 0; i < 16; ++i) {
		server->thread[i].handle = 0;
	}
	
	return server;
}

void seomServerDispatch(seomServer *server)
{
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
		
	int sockfd = accept(server->socket, &addr, &len);
	if (sockfd < 0) {
		return;
	}
	
	int outfd = output(server->path);
	if (outfd < 0) {
		close(sockfd);
		return;
	}
	
	pthread_mutex_lock(&server->mutex);
	
	for (int i = 0; i < 16; ++i) {
		if (server->thread[i].handle == 0) {
			server->thread[i].socket = sockfd;
			server->thread[i].output = outfd;
			pthread_create(&server->thread[i].handle, NULL, loop, (void *) &server->thread[i]);
			goto out;
		}
	}

out:
	pthread_mutex_unlock(&server->mutex);
}

void seomServerDestroy(seomServer *server)
{
	close(server->socket);
	free(server->path);
	free(server);
}
