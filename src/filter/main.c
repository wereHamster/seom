
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

/* median(L, LT ,T) => L, L + T - LT, T */
static uint8_t median(uint8_t L, uint8_t LT, uint8_t T)
{
	return ( L + L+T-LT + T) / 3;
}

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
	
	struct { uint32_t x; uint32_t y; } size[3];
	read(0, &size[0].x, sizeof(size[0].x));
	read(0, &size[0].y, sizeof(size[0].y));
	
	size[0].x = ntohl(size[0].x);
	size[0].y = ntohl(size[0].y);
	
	size[1].x = size[2].x = size[0].x / 2;
	size[1].y = size[2].y = size[0].y / 2;
	
	uint64_t len = size[0].x * size[0].y;
	uint32_t *tmp = malloc(len);
	uint8_t *data = malloc(size[0].x * size[0].y * 3 / 2);
	
	char header[4096];
	int n = snprintf(header, 4096, "YUV4MPEG2 W%d H%d F%d Ip\n", size[0].x, size[0].y, fps);
	write(1, header, strlen(header));
	
	for (;;) {
		uint64_t pts;
		if (read(0, &pts, sizeof(pts)) == 0)
			break;
		
		n = snprintf(header, 4096, "FRAME\n");
		write(1, header, strlen(header));
		
		for (int i = 0; i < 3; ++i) {
			uint64_t length;
			read(0, &length, sizeof(length));
			if (length > len) {
				len = length;
				tmp = realloc(tmp, len);
			}
			read(0, tmp, length);
			seomCodecDecode(data, tmp, data + size[i].x * size[i].y, &seomCodecTable);
			
#define src(xo,yo) ( data[(yo) * size[i].x + (xo)] )
			for (uint32_t x = 1; x < size[i].x; ++x) {
				data[x] += median(src(x-1,0), 0, 0);
			}
			
			for (uint32_t y = 1; y < size[i].y; ++y) {
				data[y * size[i].x] += median(0, 0, src(0,y-1));
				for (uint32_t x = 1; x < size[i].x; ++x) {
					data[y * size[i].x + x] += median(src(x-1,y), src(x-1,y-1), src(x,y-1));
				}
			}
			
			for (uint32_t y = size[i].y - 1; y < size[i].y; --y) { 
				write(1, data + y * size[i].x, size[i].x);
			}
#undef src
		}
	}
	
	return 0;
}
