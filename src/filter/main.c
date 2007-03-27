
#include <getopt.h>
#include <seom/seom.h>

static const int version[3] = { 0, 1, 0 };

static void help(void)
{
	printf(
		"seom-filter version %d.%d.%d\n"
		"\t-r:  frames per second\n"
		"\t-h:  print help text\n",
		version[0], version[1], version[2]
	);

	exit(1);
}

static void writeFrame(int fd, seomFrame * frame, uint32_t width, uint32_t height)
{
	static const char header[] = "FRAME\n";
	write(fd, header, sizeof(header) - 1);

	uint8_t *data = (uint8_t *) & frame->data[0];

	for (uint32_t y = height - 1; y < height; --y)
		write(fd, data + y * width, width);

	data += width * height;

	for (int i = 0; i < 2; ++i) {
		for (uint32_t y = (height / 2) - 1; y < (height / 2); --y)
			write(fd, data + y * (width / 2), (width / 2));
		data += width * height / 4;
	}
}

static inline uint64_t diff(uint64_t t1, uint64_t t2)
{
	return t1 < t2 ? t2 - t1 : t1 - t2;
}

static uint32_t size[2];
static seomFrame *getNextFrame(int fd, unsigned char subStreamID)
{
	seomStreamPacket streamPacket;

	for (;;) {
		read(fd, &streamPacket, sizeof(streamPacket));
		if (streamPacket.subStreamID == subStreamID)
			break;
		lseek(fd, streamPacket.payloadLength, SEEK_CUR);
	}

	static unsigned long streamBufferLength;
	static void *stramBuffer;

	if (streamBufferLength < streamPacket.payloadLength) {
		streamBufferLength = streamPacket.payloadLength;
		stramBuffer = realloc(stramBuffer, streamBufferLength);
	}

	read(fd, stramBuffer, streamPacket.payloadLength);
	seomFrame *frame = seomFrameCreate('r', size);

	frame->pts = *(uint64_t *) stramBuffer;
	seomCodecDecode(frame->data, stramBuffer + sizeof(uint64_t), size[0] * size[1] * 3 / 2);

	return frame;
}

int main(int argc, char *argv[])
{
	int fps = 25;

	if (argc < 2)
		help();

	for (;;) {
		int c = getopt(argc, argv, "r:h");

		if (c == -1) {
			break;
		} else {
			switch (c) {
			case 'r':
				fps = atoi(optarg);
				break;
			case 'h':
				help();
				exit(0);
			default:
				break;
			}
		}
	}

	int fd = open(argv[argc - 1], O_RDONLY);
	if (fd < 0)
		help();

	unsigned char videoSubStreamID = 0;
	do {
		seomStreamPacket streamPacket;
		read(fd, &streamPacket, sizeof(streamPacket));
		if (streamPacket.subStreamID == 0) {
			seomStreamMap streamMap;
			read(fd, &streamMap, sizeof(streamMap));
			if (streamMap.contentTypeID == 'v') {
				videoSubStreamID = streamMap.subStreamID;
				read(fd, size, sizeof(size));
			} else {
				lseek(fd, streamPacket.payloadLength - sizeof(streamMap), SEEK_CUR);
			}
		} else {
			lseek(fd, streamPacket.payloadLength, SEEK_CUR);
		}
	} while (videoSubStreamID == 0);

	char header[4096];
	int n = snprintf(header, 4096, "YUV4MPEG2 W%d H%d F%d:1 Ip\n", size[0], size[1], fps);
	write(1, header, n);
	
	seomFrame *frames[2] = { getNextFrame(fd, videoSubStreamID), getNextFrame(fd, videoSubStreamID) };

	uint64_t timeStep = 1000000 / fps;
	uint64_t timeNext = frames[0]->pts;

	for (;;) {
		while (diff(frames[0]->pts, timeNext) > diff(frames[1]->pts, timeNext)) {
			seomFrameDestroy(frames[0]);
			frames[0] = frames[1];
			frames[1] = getNextFrame(fd, videoSubStreamID);
			if (frames[1] == NULL)
				goto out;
		}

		writeFrame(1, frames[0], size[0], size[1]);
		timeNext += timeStep;
	}

out:
	writeFrame(1, frames[0], size[0], size[1]);
	seomFrameDestroy(frames[0]);
	close(fd);

	return 0;
}
