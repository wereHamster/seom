
#include <seom/seom.h>

#include <getopt.h>

static const int version[3] = { 0, 1, 0 };

static void help(void)
{
	printf(
		"seomFilter version %d.%d.%d\n"
		"\t-r:  frames per second\n"
		"\t-h:  print help text\n",
		version[0], version[1], version[2]
	);
}

static void writeFrame(int fd, seomFrame * frame, uint32_t width, uint32_t height)
{
	static const char header[] = "FRAME\n";
	write(fd, header, sizeof(header) - 1);

	uint8_t *data = (uint8_t *) & frame->data[0];

	for (uint32_t y = height - 1; y < height; --y) {
		write(fd, data + y * width, width);
	}

	data += width * height;

	for (int i = 0; i < 2; ++i) {
		for (uint32_t y = (height / 2) - 1; y < (height / 2); --y) {
			write(fd, data + y * (width / 2), (width / 2));
		}
		data += width * height / 4;
	}
}

static uint64_t diff(uint64_t t1, uint64_t t2)
{
	return t1 < t2 ? t2 - t1 : t1 - t2;
}

int main(int argc, char *argv[])
{
	int fps = 25;

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

	char spec[4096];
	snprintf(spec, 4096, "file://%s", argv[argc - 1]);

	uint32_t size[2];
	seomStream *stream = seomStreamCreate('i', spec, size);

	char header[4096];
	int n = snprintf(header, 4096, "YUV4MPEG2 W%d H%d F%d:1 Ip\n", stream->size[0], stream->size[1], fps);
	write(1, header, n);

	seomFrame *frames[2] = { seomStreamGet(stream), seomStreamGet(stream) };

	uint64_t timeStep = 1000000 / fps;
	uint64_t timeNext = frames[0]->pts;

	for (;;) {
		while (diff(frames[0]->pts, timeNext) > diff(frames[1]->pts, timeNext)) {
			seomFrameDestroy(frames[0]);
			frames[0] = frames[1];
			frames[1] = seomStreamGet(stream);
			if (frames[1] == NULL) {
				goto out;
			}
		}

		writeFrame(1, frames[0], stream->size[0], stream->size[1]);
		timeNext += timeStep;
	}

out:
	writeFrame(1, frames[0], stream->size[0], stream->size[1]);
	seomFrameDestroy(frames[0]);

	return 0;
}
