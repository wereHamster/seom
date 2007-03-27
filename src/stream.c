
#include <seom/seom.h>

seomStream *seomStreamCreate(char *spec)
{
	seomStream *stream = malloc(sizeof(seomStream));
	if (__builtin_expect(stream == NULL, 0))
		return NULL;

	stream->fileDescriptor = -1;
	
	if (strncmp(spec, "file://", 7) == 0) {
		stream->fileDescriptor = open(&spec[7], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	} else if (strncmp(spec, "path://", 7) == 0) {
		char buffer[4096];
		time_t tim = time(NULL);
		struct tm *tm = localtime(&tim);
		snprintf(buffer, 4096, "%s/%d-%02d-%02d--%02d:%02d:%02d.seom", &spec[7], tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
		stream->fileDescriptor = open(buffer, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	} else if (strncmp(spec, "unix://", 7) == 0) {
		fprintf(stderr, "unix sockets unsupported !\n");
	} else if (strncmp(spec, "ipv4://", 7) == 0) {
		struct sockaddr_in addr = {
			.sin_family = AF_INET,
			.sin_port = htons(42803),
			.sin_addr.s_addr = inet_addr(&spec[7])
		};
		
		stream->fileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
		connect(stream->fileDescriptor, &addr, sizeof(addr));
	} else {
		fprintf(stderr, "unknown spec: %s\n", spec);
		free(stream);
		return NULL;
	}
	
	if (stream->fileDescriptor < 0) {
		perror("seomStreamCreate()");
		free(stream);
		return NULL;
	}
	
	return stream;
}

unsigned char seomStreamInsert(seomStream *stream, unsigned char contentTypeID, unsigned long length, void *payload)
{
	for (unsigned long subStreamID = 1; subStreamID < 256; ++subStreamID) {
		if (stream->subStreams[subStreamID] == 0) {
			stream->subStreams[subStreamID] = 1;

			seomStreamPacket streamPacket = { 0, sizeof(seomStreamMap) + length };
			seomStreamMap streamMap = { subStreamID, contentTypeID };

			write(stream->fileDescriptor, &streamPacket, sizeof(streamPacket));
			write(stream->fileDescriptor, &streamMap, sizeof(streamMap));
			write(stream->fileDescriptor, payload, length);

			return subStreamID;
		}
	}

	return 0;
}

void seomStreamPut(seomStream *stream, unsigned char subStreamID, unsigned long length, void *payload)
{
	seomStreamPacket streamPacket = { subStreamID, length };

	write(stream->fileDescriptor, &streamPacket, sizeof(streamPacket));
	write(stream->fileDescriptor, payload, length);
}

void seomStreamRemove(seomStream *stream, unsigned char subStreamID)
{
	stream->subStreams[subStreamID] = 0;
}

void seomStreamDestroy(seomStream *stream)
{
	close(stream->fileDescriptor);
	free(stream);
}
