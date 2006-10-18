
#include <seom/seom.h>

seomStream *seomStreamCreate(char type, char *spec, uint32_t size[2])
{
	seomStream *stream = malloc(sizeof(seomStream));
	stream->fd = -1;
	
	if (strncmp(spec, "file://", 7) == 0) {
		fprintf(stderr, "file:// output: %s\n", &spec[7]);
		if (type == 'o') {
			stream->fd = open(&spec[7], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
		} else if (type == 'i') {
			stream->fd = open(&spec[7], O_RDONLY);
		}
	} else if (strncmp(spec, "unix://", 7) == 0) {
		fprintf(stderr, "unix:// output: %s\n", &spec[7]);
		stream->fd = open(&spec[7], O_RDWR);
	} else if (strncmp(spec, "ipv4://", 7) == 0) {
		struct sockaddr_in addr = {
			.sin_family = AF_INET,
			.sin_port = htons(42803),
			.sin_addr.s_addr = inet_addr(&spec[7])
		};
		
		stream->fd = socket(AF_INET, SOCK_STREAM, 0);
		connect(stream->fd, &addr, sizeof(addr));
		
		fprintf(stderr, "ipv4:// output: %s\n", &spec[7]);
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
	
	stream->buffer = malloc(stream->size[0] * stream->size[1] * 4);
	
	return stream;
}

void seomStreamPut(seomStream *stream, seomFrame *frame)
{
	uint64_t pts = frame->pts;	
	
	uint32_t *end = seomCodecEncode(stream->buffer, frame->data, stream->size[0], stream->size[1]);
	uint32_t len = (end - stream->buffer) * sizeof(uint32_t);
	
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
	close(stream->fd);
	free(stream->buffer);
	free(stream);
}
