 
#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE

#define llu(val) ( (unsigned long long int) val )

#include <seom/seom.h>

extern void yv12_to_rgba_c (
	uint8_t * x_ptr,
	int x_stride,
	uint8_t * y_src,
	uint8_t * v_src,
	uint8_t * u_src,
	int y_stride,
	int uv_stride,
	int width,
	int height,
	int vflip
);

static uint8_t *yuvPlanes[3];
static struct { uint64_t x; uint64_t y; } yuvPlanesSizes[3];
static uint8_t *rgbFrame;

static Display *dpy = NULL;
static Window win = 0;

static Bool WaitFor__MapNotify(Display *dpy, XEvent *e, char *arg)
{
	return dpy && (e->type == MapNotify) && (e->xmap.window == (Window) arg);
}

static Bool WaitFor__ConfigureNotify(Display *dpy, XEvent *e, char *arg)
{
	return dpy && (e->type == ConfigureNotify) && (e->xconfigure.window == (Window) arg);
}

static void glCaptureCreateWindow(int width, int height)
{
	dpy = XOpenDisplay(NULL);

	GLXFBConfig *fbc;
	XVisualInfo *vi;
	Colormap cmap;
	XSetWindowAttributes swa;
	GLXContext cx;
	XEvent event;
	int nElements;

	int attr[] = { GLX_DOUBLEBUFFER, True, None };

	fbc = glXChooseFBConfig(dpy, DefaultScreen(dpy), attr, &nElements);
	vi = glXGetVisualFromFBConfig(dpy, fbc[0]);

	cx = glXCreateNewContext(dpy, fbc[0], GLX_RGBA_TYPE, 0, GL_FALSE);
	cmap = XCreateColormap(dpy, RootWindow(dpy, vi->screen), vi->visual, AllocNone);

	swa.colormap = cmap;
	swa.border_pixel = 0;
	swa.event_mask = 0;
	win = XCreateWindow(dpy, RootWindow(dpy, vi->screen), 0, 0, width, height, 0, vi->depth, InputOutput, vi->visual, CWBorderPixel | CWColormap | CWEventMask, &swa);
	XSelectInput(dpy, win, StructureNotifyMask | KeyPressMask | KeyReleaseMask);
	XMapWindow(dpy, win);
	XIfEvent(dpy, &event, WaitFor__MapNotify, (char *)win);

	XMoveWindow(dpy, win, 64, 64);
	XIfEvent(dpy, &event, WaitFor__ConfigureNotify, (char *)win);

	glXMakeCurrent(dpy, win, cx);
}

extern void colorspace_init(void);

/* median(L, LT ,T) => L, L + T - LT, T */
static uint8_t median(uint8_t L, uint8_t LT, uint8_t T)
{
	return ( L + L+T-LT + T) / 3;
}

static struct timeval barTimer;

static void DrawBarBorder(float xf)
{
	static const float t = 0.02;
	static const float x = 0.95;
	static const float w = 0.03;
	static const float h = 0.05;
	
	glBegin(GL_TRIANGLE_FAN);
		glVertex2f(xf * ( x - t ), h);
		glVertex2f(xf * ( x - t - w ), h);
		glVertex2f(xf * ( x - t - w ), h + t);
		glVertex2f(xf * ( x ), h + t);
		glVertex2f(xf * ( x ), -h - t);
		glVertex2f(xf * ( x - t ), -h);
	glEnd();
	
	glBegin(GL_TRIANGLE_FAN);
		glVertex2f(xf * ( x - t ), -h );
		glVertex2f(xf * ( x ), -h - t);
		glVertex2f(xf * ( x - t - w ), -h - t);
		glVertex2f(xf * ( x - t - w ), -h);
	glEnd();
}

static int barSticky;

static void DrawBar(float val)
{
	glDisable(GL_TEXTURE_2D);
	
	struct timeval currentTime = { 0, 0 };
	gettimeofday(&currentTime, 0);
	
	struct timeval elapsed = { 0, 0 };
	timersub(&currentTime, &barTimer, &elapsed);
	
	float e = elapsed.tv_sec * 1000000 + elapsed.tv_usec;
	
	static const float fade = 5 * 1000000;
	
	float alpha = fade - e;
	if (barSticky) {
		alpha = 2000000;
	}
	if (alpha > 0.0) {
		glColor4f(1.0, 1.0, 1.0, alpha / fade);
		
		DrawBarBorder(-1.0);
		DrawBarBorder( 1.0);
	
		glBegin(GL_QUADS);
			float x = ( 0.92 + 0.92 ) * val;
			glVertex2f(-0.92, -0.04);
			glVertex2f(-0.92 + x, -0.04);
			glVertex2f(-0.92 + x, 0.04);
			glVertex2f(-0.92, 0.04);
		glEnd();
	
		glColor4f(1.0, 1.0, 1.0, 1.0);
	}
	
	glEnable(GL_TEXTURE_2D);
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Usage: %s [seom file]\n", argv[0]);
		return 0;
	}
	
	int inFile = open(argv[1], O_RDONLY);
	if (inFile == -1) {
		perror("can't open infile");
		exit(-1);
	}
	
	colorspace_init();

	struct stat statBuffer;
	fstat(inFile, &statBuffer);

	unsigned char *sourceData = (unsigned char *)mmap(0, statBuffer.st_size, PROT_READ, MAP_SHARED, inFile, 0);

	unsigned char *currentPosition = sourceData;

	uint32_t width = ntohl(*(uint32_t *) currentPosition);
	currentPosition += sizeof(uint32_t);
	uint32_t height = ntohl(*(uint32_t *) currentPosition);
	currentPosition += sizeof(uint32_t);

	uint64_t cFrameTotal = 0;
	unsigned char *mem = currentPosition;
	uint64_t time[2];
	time[0] = *(uint64_t *) mem;
	for (;;) {
		time[1] = *(uint64_t *) mem;
		mem += sizeof(uint64_t);
		for (int i = 0; i < 3; ++i) {
			uint64_t cSize = *(uint64_t *) mem;
			mem += sizeof(uint64_t) + cSize;
		}
		
		if (mem >= sourceData + statBuffer.st_size) {
			break;
		}
		
		++cFrameTotal;

		uint64_t c = (mem - sourceData) * 1000.0 / statBuffer.st_size;
		fprintf(stderr, "analysing video... %04.1f%% \r", (float)c / 10.0);
	}

	fprintf(stderr, "analysing video... done  \n");
	
	if (cFrameTotal == 0) {
		printf("empty video\n");
		exit(0);
	}
	
	uint64_t tt = (time[1] - time[0]) / 1000000;
	float fps = (float)cFrameTotal / tt;
	float mbs = (float)statBuffer.st_size / 1024 / 1024 / tt;
	printf("%llu frames, %llu seconds, %.1f fps, %.1f MiB/s\n", llu(cFrameTotal), llu(tt), fps, mbs);

	uint64_t cTimeTotal = *(uint64_t *) (sourceData + statBuffer.st_size - (width * height * 3 / 2 + sizeof(uint64_t))) - *(uint64_t *) currentPosition;
	printf("size: %u:%u, cFrameTotal: %llu, time: %llu\n", width, height, llu(cFrameTotal), llu(cTimeTotal / 1000000));

	uint64_t rawFrames = (statBuffer.st_size - 2 * sizeof(uint32_t)) / (width * height * 3 / 2 + sizeof(uint64_t));
	printf("ratio: %.3f\n", (float)rawFrames / cFrameTotal);

	
	yuvPlanesSizes[0].x = width;
	yuvPlanesSizes[0].y = height;
	
	yuvPlanesSizes[1].x = width / 2;
	yuvPlanesSizes[1].y = height / 2;
	
	yuvPlanesSizes[2].x = width / 2;
	yuvPlanesSizes[2].y = height / 2;
	
	rgbFrame = malloc(width * height * 4);
	yuvPlanes[0] = malloc(width * height * 3 / 2);
	yuvPlanes[1] = yuvPlanes[0] + yuvPlanesSizes[0].x * yuvPlanesSizes[0].y;
	yuvPlanes[2] = yuvPlanes[1] + yuvPlanesSizes[1].x * yuvPlanesSizes[1].y;

	struct timeval currentTime = { 0, 0 };
	gettimeofday(&currentTime, 0);

	glCaptureCreateWindow(width, height);

	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glEnable(GL_TEXTURE_2D);

	uint64_t firstFrame = *(uint64_t *) currentPosition;
	uint64_t pts = firstFrame;

	gettimeofday(&currentTime, 0);
	uint64_t tdiff = currentTime.tv_sec * 1000000 + currentTime.tv_usec - pts;

	uint64_t fIndex = 0;
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	int pause = 0;
	
	gettimeofday(&barTimer, 0);
	
	barTimer.tv_sec -= 10;
	
	for (;;) {
		pts = *(uint64_t *) currentPosition;
		currentPosition += sizeof(uint64_t);
		
		for (int i = 0; i < 3; ++i) {
			uint64_t cSize = *(uint64_t *) currentPosition;
			currentPosition += sizeof(uint64_t);
			seomCodecDecode(yuvPlanes[i], (uint32_t *)currentPosition, yuvPlanes[i] + yuvPlanesSizes[i].x * yuvPlanesSizes[i].y, &seomCodecTable);
			
			uint64_t width = yuvPlanesSizes[i].x;
			uint64_t height = yuvPlanesSizes[i].y;
			
#define src(x,y) ( yuvPlanes[i][(y)*width+(x)] )
			for (unsigned int x = 1; x < width; ++x) {
				yuvPlanes[i][x] = yuvPlanes[i][x] + median(src(x-1,0), 0, 0);
			}
			
			for (unsigned int y = 1; y < height; ++y) {
				yuvPlanes[i][y * width] = yuvPlanes[i][y * width] + median(0, 0, src(0,y-1));
				for (unsigned int x = 1; x < width; ++x) {
					yuvPlanes[i][y * width + x] = yuvPlanes[i][y * width + x] + median(src(x-1,y), src(x-1,y-1), src(x,y-1));
				}
			}
#undef src
			currentPosition += cSize;
		}

		yv12_to_rgba_c(rgbFrame, width * 4, yuvPlanes[0], yuvPlanes[1], yuvPlanes[2], width, width / 2, width, height, 0);
		glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, rgbFrame);

		glClear(GL_COLOR_BUFFER_BIT);
		
		glBegin(GL_QUADS);
		glTexCoord2d(0.0, 0.0);
		glVertex2d(-1.0, -1.0);
		glTexCoord2d(1.0, 0.0);
		glVertex2d(1.0, -1.0);
		glTexCoord2d(1.0, 1.0);
		glVertex2d(1.0, 1.0);
		glTexCoord2d(0.0, 1.0);
		glVertex2d(-1.0, 1.0);
		glEnd();
		
		DrawBar((float)fIndex / (cFrameTotal));
		
		gettimeofday(&currentTime, 0);
		uint64_t now = currentTime.tv_sec * 1000000 + currentTime.tv_usec;
		int64_t s = pts - now + tdiff;
		if (s > 0) {
			usleep((uint64_t) s);
		} else {
			tdiff = now - pts;
		}
		
		glXSwapBuffers(dpy, win);

		fprintf(stderr, "encoded frames: %llu/%llu \r", llu(fIndex), llu(cFrameTotal));

		int skipFrames = pause ? 0 : 1;
		while (XPending(dpy)) {
			XEvent e;
			XClientMessageEvent event;
			XNextEvent(dpy, &e);
			long key;

			switch (e.type) {
			case KeyPress:
				key = XLookupKeysym((XKeyEvent *) & e, 0);
				switch (key) {
				case XK_Right:
					skipFrames += 1;
					break;
				case XK_Left:
					skipFrames += -1;
					break;
				case XK_Up:
					skipFrames += 10;
					break;
				case XK_Down:
					skipFrames += -10;
					break;
				case XK_o:
					XResizeWindow(dpy, win, width, height);
					//XIfEvent(dpy, &e, WaitFor__ConfigureNotify, (char *)win);
					glViewport(0, 0, width, height);
					break;
				case XK_b:
					barSticky = !barSticky;
					break;
				case XK_f:
					memset(&event, 0, sizeof(XClientMessageEvent));
					event.type = ClientMessage;
					event.message_type = XInternAtom(dpy, "_NET_WM_STATE", False);
					event.display = dpy;
					event.window = win;
					event.format = 32;
					event.data.l[0] = 2;
					event.data.l[1] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);

					XSendEvent(dpy, DefaultRootWindow(dpy), False, SubstructureRedirectMask, (XEvent *) & event);
					break;
				case XK_Escape:
					exit(0);
					break;
				case XK_space:
					pause = !pause;
					break;
				}
				break;
			case ConfigureNotify:
				glViewport(0, 0, e.xconfigure.width, e.xconfigure.height);
				glClear(GL_COLOR_BUFFER_BIT);
				
				int w = e.xconfigure.width;
				int h = e.xconfigure.height;
				
				if ((float)width / height < (float)w / h) {
					w = h * width / height;
				} else {
					h = w * height / width;
				}
				
				int x = ( e.xconfigure.width - w ) / 2;
				int y = ( e.xconfigure.height - h ) / 2;
				
				glViewport(x, y, w, h);
				break;
			default:
				break;
			}
		}
		
		if (skipFrames < 0 && -skipFrames > (int)fIndex) {
			fIndex = 0;
		} else if (skipFrames > 0 && fIndex + skipFrames >= cFrameTotal) {
			fIndex = cFrameTotal;
			pause = 1;
		} else {
			fIndex += skipFrames;
		}
		
		if (1) {
			currentPosition = sourceData + 2 * sizeof(uint32_t);
			for (unsigned int i = 0; i < fIndex; ++i) {
				pts = *(uint64_t *) currentPosition;
				currentPosition += sizeof(uint64_t);
				for (int i = 0; i < 3; ++i) {
					uint64_t cSize = *(uint64_t *) currentPosition;
					currentPosition += sizeof(uint64_t) + cSize;
				}

				if (currentPosition >= sourceData + statBuffer.st_size) {
					break;
				}
			}
			pts = *(uint64_t *) currentPosition;
			
			if (skipFrames > 1 || skipFrames < 0) {
				gettimeofday(&barTimer, 0);
				
				gettimeofday(&currentTime, 0);
				tdiff = currentTime.tv_sec * 1000000 + currentTime.tv_usec - pts;
			}
		}
	}
	
	munmap(sourceData, statBuffer.st_size);
	close(inFile);
}
