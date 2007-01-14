
#include <seom/seom.h>

seomStream *seomStreamCreate(char type, char *spec, uint32_t size[2])
{
	seomStream *stream = malloc(sizeof(seomStream));

	if (__builtin_expect(stream == NULL, 0))
		return NULL;

	stream->fd = -1;
	
	if (strncmp(spec, "file://", 7) == 0) {
		if (type == 'o') {
			stream->fd = open(&spec[7], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
		} else if (type == 'i') {
			stream->fd = open(&spec[7], O_RDONLY);
		}
	} else if (strncmp(spec, "path://", 7) == 0) {
		if (type == 'i') {
			fprintf(stderr, "path:// input not supported !\n");
			free(stream);
			return NULL;
		}
		char buffer[4096];
		time_t tim = time(NULL);
		struct tm *tm = localtime(&tim);
		snprintf(buffer, 4096, "%s/%d-%02d-%02d--%02d:%02d:%02d.seom", &spec[7], tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
		fprintf(stderr, "path:// output: %s\n", buffer);
		stream->fd = open(buffer, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	} else if (strncmp(spec, "unix://", 7) == 0) {
		fprintf(stderr, "unix sockets unsupported !\n");
	} else if (strncmp(spec, "ipv4://", 7) == 0) {
		if (type == 'i') {
			fprintf(stderr, "ipv4:// input not supported !\n");
			free(stream);
			return NULL;
		}
		struct sockaddr_in addr = {
			.sin_family = AF_INET,
			.sin_port = htons(42803),
			.sin_addr.s_addr = inet_addr(&spec[7])
		};
		
		stream->fd = socket(AF_INET, SOCK_STREAM, 0);
		connect(stream->fd, &addr, sizeof(addr));
	} else if (strncmp(spec, "ipv6://", 7) == 0) {
		fprintf(stderr, "IPv6 unsupported !\n");
	} else {
		fprintf(stderr, "unknown spec: %s\n", spec);
		free(stream);
		return NULL;
	}
	
	if (stream->fd < 0) {
		perror("seomStreamCreate()");
		free(stream);
		return NULL;
	}
	
	stream->size[0] = htonl(size[0]);
	stream->size[1] = htonl(size[1]);
	
	if (type == 'o') {
		write(stream->fd, stream->size, sizeof(stream->size));
	} else if (type == 'i') {
		read(stream->fd, stream->size, sizeof(stream->size));
	} else {
		seomStreamDestroy(stream);
		return NULL;
	}
	
	size[0] = stream->size[0] = ntohl(stream->size[0]);
	size[1] = stream->size[1] = ntohl(stream->size[1]);
	
	stream->buffer = malloc(stream->size[0] * stream->size[1] * 4 + 36000);
	
	return stream;
}

void seomStreamPut(seomStream *stream, seomFrame *frame)
{
	uint64_t pts = frame->pts;	
	
	uint32_t size = stream->size[0] * stream->size[1] * 3 / 2;
	uint8_t *end = seomCodecEncode(stream->buffer, frame->data, size);
	uint32_t len = end - stream->buffer;
	
	write(stream->fd, &pts, sizeof(pts));
	write(stream->fd, &len, sizeof(len));
	write(stream->fd, stream->buffer, len);
}

uint64_t seomStreamPos(seomStream *stream, uint64_t pos)
{
	return lseek(stream->fd, SEEK_CUR, pos);
}

seomFrame *seomStreamGet(seomStream *stream)
{
	uint64_t pts;
	if (read(stream->fd, &pts, sizeof(pts)) == 0) {
		return NULL;
	}
	
	seomFrame *frame = seomFrameCreate('c', stream->size);
	frame->pts = pts;
	
	uint32_t len;
	read(stream->fd, &len, sizeof(len));
	read(stream->fd, stream->buffer, len);
	
	uint32_t size = stream->size[0] * stream->size[1] * 3 / 2;
	uint8_t *end = seomCodecDecode(frame->data, stream->buffer, size);
	
	if (end - stream->buffer != len) {
		return frame;
	}
	
	return frame;
}

void seomStreamDestroy(seomStream *stream)
{
	close(stream->fd);
	free(stream->buffer);
	free(stream);
}
