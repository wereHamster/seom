
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
	
	uint32_t size[2];
	read(0, size, sizeof(size));
	
	size[0] = ntohl(size[0]);
	size[1] = ntohl(size[1]);
	
	uint8_t *yuvPlanes[3];
	struct { uint64_t x; uint64_t y; } yuvPlanesSizes[3];
	
	yuvPlanesSizes[0].x = size[0];
	yuvPlanesSizes[0].y = size[1];
	
	yuvPlanesSizes[1].x = size[0] / 2;
	yuvPlanesSizes[1].y = size[1] / 2;
	
	yuvPlanesSizes[2].x = size[0] / 2;
	yuvPlanesSizes[2].y = size[1] / 2;
	
	yuvPlanes[0] = malloc(size[0] * size[1] * 3 / 2);
	yuvPlanes[1] = yuvPlanes[0] + yuvPlanesSizes[0].x * yuvPlanesSizes[0].y;
	yuvPlanes[2] = yuvPlanes[1] + yuvPlanesSizes[1].x * yuvPlanesSizes[1].y;
	
	fprintf(stderr, "size: %d:%d\n", size[0], size[1]);
	
	char header[4096];
	int n = snprintf(header, 4096, "YUV4MPEG2 W%d H%d F%d Ip\n", size[0], size[1], fps);
	write(1, header, n);
	
	for (;;) {
		uint64_t pts;
		if (read(0, &pts, sizeof(pts)) == 0)
			break;
		
		n = snprintf(header, 4096, "FRAME\n");
		write(1, header, n);
		
		for (int i = 0; i < 3; ++i) {
			uint64_t length;
			read(0, &length, sizeof(length));
			uint32_t *tmp = malloc(length);
			read(0, tmp, length);
			seomCodecDecode(yuvPlanes[i], tmp, yuvPlanes[i] + yuvPlanesSizes[i].x * yuvPlanesSizes[i].y, &seomCodecTable);
			free(tmp);
			
			uint64_t width = yuvPlanesSizes[i].x;
			uint64_t height = yuvPlanesSizes[i].y;
			
#define src(x,y) ( yuvPlanes[i][(y)*width+(x)] )
			for (unsigned int x = 1; x < width; ++x) {
				yuvPlanes[i][x] += median(src(x-1,0), 0, 0);
			}
			
			for (unsigned int y = 1; y < height; ++y) {
				yuvPlanes[i][y * width] += median(0, 0, src(0,y-1));
				for (unsigned int x = 1; x < width; ++x) {
					yuvPlanes[i][y * width + x] =+ median(src(x-1,y), src(x-1,y-1), src(x,y-1));
				}
			}
			
			for (unsigned int y = height - 1; y < height; --y) {   
				write(1, yuvPlanes[i] + y * width, width);
			}
#undef src
		}
	}
	
	return 0;
}
