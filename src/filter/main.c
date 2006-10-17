
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


static uint32_t streamGet(void *priv, void *data, uint32_t size)
{
	return read(0, data, size);
}

static uint32_t streamPut(void *priv, void *data, uint32_t size)
{
	return size;
}

static seomStreamOps streamOps = {
	.get = streamGet,
	.pos = NULL,
	.put = streamPut
};

int main(int argc, char *argv[])
{
	int fps = 25;

	for (;;) {
		int c = getopt(argc, argv, "r:h");
		
		if (c == -1) {
			break;
		} else {
			switch(c) {
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
	
	/* FIXME */
	close(0);
	open(argv[1], O_RDONLY);
	
	seomStream *stream = seomStreamCreate(&streamOps, NULL);
	
	char header[4096];
	int n = snprintf(header, 4096, "YUV4MPEG2 W%d H%d F%d Ip\n", stream->size[0], stream->size[1], fps);
	write(1, header, strlen(header));
	
	for (;;) {
		seomFrame *frame = seomStreamGet(stream);
		if (frame == NULL) {
			break;
		}
		
		n = snprintf(header, 4096, "FRAME\n");
		write(1, header, strlen(header));
		
		uint8_t *data = (uint8_t *) &frame->data[0];
		
		for (uint32_t y = stream->size[1] - 1; y < stream->size[1]; --y) { 
			write(1, data + y * stream->size[0], stream->size[0]);
		}
		
		data += stream->size[0] * stream->size[1];
		
		for (int i = 0; i < 2; ++i) {
			for (uint32_t y = (stream->size[1] / 2) - 1; y < (stream->size[1] / 2); --y) { 
				write(1, data + y * (stream->size[0] / 2), (stream->size[0] / 2));
			}
			data += stream->size[0] * stream->size[1] / 4;
		}
		
		seomFrameDestroy(frame);
	}
	
	return 0;
}
