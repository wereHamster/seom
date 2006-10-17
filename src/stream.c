
#include <seom/seom.h>

seomStream *seomStreamCreate(char type, char *spec, uint32_t size[2])
{
	seomStream *stream = malloc(sizeof(seomStream));
	
	if (strncmp(spec, "file://", 7) == 0) {
		stream->fd = open(&spec[7], O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IRGRP | S_IROTH);
		printf("file:// output: %s\n", &spec[7]);
	} else if (strncmp(spec, "unix://", 7) == 0) {
		stream->fd = open(&spec[7], O_RDWR);
		printf("unix:// output: %s\n", &spec[7]);
	} else if (strncmp(spec, "ipv4://", 7) == 0) {
		struct sockaddr_in addr = {
			.sin_family = AF_INET,
			.sin_port = htons(42803),
			.sin_addr.s_addr = inet_addr(&spec[7])
		};
		
		stream->fd = socket(AF_INET, SOCK_STREAM, 0);
		connect(stream->fd, &addr, sizeof(addr));
		
		printf("ipv4:// output: %s\n", &spec[7]);
	} else if (strncmp(spec, "ipv6://", 7) == 0) {
		printf("IPv6 unsupported !\n");
	} else {
		printf("unknown spec: %s\n", spec);
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
	
	return stream;
}

void seomStreamPut(seomStream *stream, seomFrame *frame)
{
	uint64_t pts = frame->pts;
	
	static uint32_t tmp[1280 * 1024 * 4 * 8];	
	
	uint32_t *end = seomCodecEncode(tmp, frame->data, stream->size[0], stream->size[1]);
	uint32_t len = (end - &tmp[0]) * sizeof(uint32_t);
	
	write(stream->fd, &pts, sizeof(pts));
	write(stream->fd, &len, sizeof(len));
	write(stream->fd, tmp, len);
}

uint64_t seomStreamPos(seomStream *stream, uint64_t pos)
{
	return lseek(stream->fd, SEEK_CUR, pos);
}

seomFrame *seomStreamGet(seomStream *stream)
{
	seomFrame *frame = seomFrameCreate('c', stream->size[0], stream->size[1]);
	
	uint64_t pts;
	if (read(stream->fd, &pts, sizeof(pts)) == 0) {
		seomFrameDestroy(frame);
		return NULL;
	}
	
	uint32_t len;
	read(stream->fd, &len, sizeof(len));
	
	static uint32_t tmp[1280 * 1024 * 4];
	read(stream->fd, tmp, len);
	
	uint32_t *end = seomCodecDecode(frame->data, tmp, stream->size[0], stream->size[1]);
	
	if ((end - &tmp[0]) * sizeof(uint32_t) != len) {
		return frame;
	}
	
	return frame;
}

void seomStreamDestroy(seomStream *stream)
{
	free(stream);
}
