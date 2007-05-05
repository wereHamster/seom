
#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE

#define llu(val) ( (unsigned long long int) val )

#include <seom/seom.h>

#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvlib.h>

static const int format = 0x30323449;

static int XVideoGetPort(Display * dpy)
{
	XvAdaptorInfo *pAdaptor;
	unsigned int i, numAdaptors;
	int selectedPort = -1;

	switch (XvQueryExtension(dpy, &i, &i, &i, &i, &i)) {
	case Success:
		break;
	case XvBadExtension:
		fprintf(stderr, "XvBadExtension");
		return -1;
	case XvBadAlloc:
		fprintf(stderr, "XvBadAlloc");
		return -1;
	default:
		fprintf(stderr, "XvQueryExtension failed");
		return -1;
	}

	switch (XvQueryAdaptors(dpy, DefaultRootWindow(dpy), &numAdaptors, &pAdaptor)) {
	case Success:
		break;
	case XvBadExtension:
		fprintf(stderr, "XvBadExtension for XvQueryAdaptors");
		return -1;
	case XvBadAlloc:
		fprintf(stderr, "XvBadAlloc for XvQueryAdaptors");
		return -1;
	default:
		fprintf(stderr, "XvQueryAdaptors failed");
		return -1;
	}

	for (unsigned int i_adaptor = 0; i_adaptor < numAdaptors; ++i_adaptor) {
		if (!(pAdaptor[i_adaptor].type & XvInputMask) || !(pAdaptor[i_adaptor].type & XvImageMask)) {
			continue;
		}

		int numFormats;
		XvImageFormatValues *pFormats = XvListImageFormats(dpy, pAdaptor[i_adaptor].base_id, &numFormats);

		for (int i_format = 0; i_format < numFormats && (selectedPort == -1); i_format++) {
			if (pFormats[i_format].id != 0x30323449)
				continue;

			int portLast = pAdaptor[i_adaptor].base_id + pAdaptor[i_adaptor].num_ports;
			for (int i_port = pAdaptor[i_adaptor].base_id; (i_port < portLast) && (selectedPort == -1); i_port++) {
				if (XvGrabPort(dpy, i_port, CurrentTime) == Success)
					selectedPort = i_port;
			}

			if (selectedPort == -1)
				continue;

			int numAttributes;
			XvAttribute *pAttributes = XvQueryPortAttributes(dpy, selectedPort, &numAttributes);
			for (int i_attr = 0; i_attr < numAttributes; i_attr++) {
				if (strcmp(pAttributes[i_attr].name, "XV_SYNC_TO_VBLANK") == 0) {
					const Atom sync = XInternAtom(dpy, "XV_SYNC_TO_VBLANK", False);
					XvSetPortAttribute(dpy, selectedPort, sync, 1);
					break;
				}
			}

			if (pAttributes != NULL)
				XFree(pAttributes);
		}

		if (pFormats != NULL)
			XFree(pFormats);
	}

	if (numAdaptors > 0)
		XvFreeAdaptorInfo(pAdaptor);

	return selectedPort;
}

static uint8_t *yuvImage;

static Display *dpy = NULL;
static Window win = 0;

static Bool WaitFor__MapNotify(Display * dpy, XEvent * e, char *arg)
{
	return dpy && (e->type == MapNotify) && (e->xmap.window == (Window) arg);
}

static void createWindow(uint32_t width, uint32_t height)
{
	dpy = XOpenDisplay(NULL);

	XVisualInfo vit;
	XSetWindowAttributes swa;
	XEvent event;
	int num;
	
	Window root;
	unsigned int rootWidth, rootHeight, uunused;
	int sunused;

	XGetGeometry(dpy, DefaultRootWindow(dpy), &root, &sunused, &sunused, &rootWidth, &rootHeight, &uunused, &uunused);

	if (width >= rootWidth)
		width = rootWidth - 128;
	
	if (height >= rootHeight)
		height = rootHeight - 128;
	
	vit.visualid = XVisualIDFromVisual(DefaultVisual(dpy, DefaultScreen(dpy)));
	XVisualInfo *vi = XGetVisualInfo(dpy, VisualIDMask, &vit, &num);
	Colormap cmap = XCreateColormap(dpy, DefaultRootWindow(dpy), vi->visual, AllocNone);

	swa.colormap = cmap;
	swa.border_pixel = 0;
	swa.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask | ExposureMask;
	win = XCreateWindow(dpy, DefaultRootWindow(dpy), 64, 64, width, height, 0, vi->depth, InputOutput, vi->visual, CWBorderPixel | CWColormap | CWEventMask, &swa);
	XMapWindow(dpy, win);
	XIfEvent(dpy, &event, WaitFor__MapNotify, (char *)win);
}

static const char *timeFormat(uint64_t time)
{
	static char buf[64];
	time_t tim = (time_t) time / 1000000;
	struct tm *tm = localtime(&tim);
	snprintf(buf, 64, "%02d:%02d.%03d", tm->tm_min, tm->tm_sec, (int) (time / 1000) % 1000);
	return buf;
}

static int xOffset;
static int yOffset;

static int dWidth;
static int dHeight;

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Usage: %s [seom file]\n", argv[0]);
		return 0;
	}

	int inFile = open(argv[1], O_RDONLY);
	if (inFile == -1) {
		perror("can't open infile");
		exit(-1);
	}

	struct stat statBuffer;
	fstat(inFile, &statBuffer);

	unsigned char *sourceData = (unsigned char *)mmap(0, statBuffer.st_size, PROT_READ, MAP_SHARED, inFile, 0);

	unsigned char *currentPosition = sourceData;
	seomStreamPacket *streamPacket = (seomStreamPacket *) currentPosition;
	currentPosition += sizeof(seomStreamPacket);
	seomStreamMap *streamMap = (seomStreamMap *) currentPosition;
	currentPosition += sizeof(seomStreamMap);
	fprintf(stderr, "subStreamID: 0x%02x, contentTypeID: 0x%02x\n", streamMap->subStreamID, streamMap->contentTypeID);
	uint32_t width = *(uint32_t *) currentPosition;
	currentPosition += sizeof(uint32_t);
	uint32_t height = *(uint32_t *) currentPosition;
	currentPosition += sizeof(uint32_t);
	
	fprintf(stderr, "offset: %ld\n", currentPosition - sourceData);
	
	printf("Video: %u:%u pixels\n", width, height);

	uint64_t cFrameTotal = 0;
	unsigned char *mem = currentPosition;
	uint64_t time[2];
	time[0] = *(uint64_t *) mem;
	time[1] = (*(uint64_t *) mem) + 100000000;
	for (;;) {
		if (mem >= sourceData + statBuffer.st_size)
			break;

		streamPacket = (seomStreamPacket *) mem;
		mem += sizeof(seomStreamPacket) + streamPacket->payloadLength;

		++cFrameTotal;

		uint64_t c = (mem - sourceData) * 1000.0 / statBuffer.st_size;
		fprintf(stderr, "analysing video... %04.1f%% \r", (float)c / 10.0);
	}

	if (cFrameTotal == 0) {
		printf("Video seems to be empty. Sorry, can't replay it.\n");
		exit(0);
	}

	float tt = (float) (time[1] - time[0]) / 1000000.0;
	float fps = (float) cFrameTotal / tt;
	float mbs = (float) statBuffer.st_size / 1024 / 1024 / tt;
	uint64_t rawFrames = (statBuffer.st_size - 2 * sizeof(uint32_t)) / (width * height * 3 / 2 + sizeof(uint64_t));

	printf("Video: %u:%u pixels, %llu frames, %s seconds, %.1f fps, %.1f MiB/s\n", width, height, llu(cFrameTotal), timeFormat(time[1] - time[0]), fps, mbs);
	printf("Compression: %.3f\n", (float)rawFrames / cFrameTotal);

	yuvImage = malloc(width * height * 3 / 2);

	struct timeval currentTime = { 0, 0 };
	gettimeofday(&currentTime, 0);

	createWindow(width, height);
	
	if (XShmQueryExtension(dpy) == False) {
		fprintf(stderr, "XShm not available !\n");
		exit(0);
	}

	XGCValues xgcvalues;
	xgcvalues.graphics_exposures = False;
	GC gc = XCreateGC(dpy, win, GCGraphicsExposures, &xgcvalues);
	int xvport = XVideoGetPort(dpy);
	if (xvport < 0) {
		fprintf(stderr, "Couldn't create XVideo port !\n");
		exit(0);
	}
	XShmSegmentInfo shm;
	XvImage *img = XvShmCreateImage(dpy, xvport, format, NULL, width, height, &shm);

	shm.shmid = shmget(IPC_PRIVATE, img->data_size, IPC_CREAT | 0776);
	if (shm.shmid < 0) {
		fprintf(stderr, "cannot allocate shared image data (%s)", strerror(errno));
		return 0;
	}

	/* Attach shared memory segment to process (read/write) */
	shm.shmaddr = img->data = shmat(shm.shmid, 0, 0);
	if (shm.shmaddr == NULL) {
		fprintf(stderr, "cannot attach shared memory (%s)", strerror(errno));
		shmctl(shm.shmid, IPC_RMID, 0);
		return 0;
	}

	/* Read-only data. We won't be using XShmGetImage */
	shm.readOnly = True;

	XSynchronize(dpy, True);
	Status result = XShmAttach(dpy, &shm);
	if (result == False) {
		fprintf(stderr, "cannot attach shared memory to X server");
		shmctl(shm.shmid, IPC_RMID, 0);
		shmdt(shm.shmaddr);
		return 0;
	}
	XSynchronize(dpy, False);

	XSync(dpy, False);

	uint64_t firstFrame = *(uint64_t *) currentPosition;
	uint64_t pts = firstFrame;

	gettimeofday(&currentTime, 0);
	uint64_t tdiff = currentTime.tv_sec * 1000000 + currentTime.tv_usec - pts;

	uint64_t fIndex = 0;

	int pause = 0;

	for (;;) {
		streamPacket = (seomStreamPacket *) currentPosition;
		currentPosition += sizeof(seomStreamPacket) + sizeof(uint64_t);

		seomCodecDecode(yuvImage, currentPosition, width * height * 3 / 2);
		currentPosition += streamPacket->payloadLength;
		
		uint8_t *dst = (uint8_t *) img->data + img->offsets[0] + img->pitches[0] * (height - 1);
		uint8_t *src = yuvImage;
		for (uint32_t y = 0; y < height; ++y) {
			memcpy(dst, src, width);
			dst -= img->pitches[0];
			src += width;
		}

		for (int i = 1; i < 3; ++i) {
			dst = (uint8_t *) img->data + img->offsets[i] + img->pitches[i] * (height / 2 - 1);
			for (uint32_t y = 0; y < height / 2; ++y) {
				memcpy(dst, src, width / 2);
				dst -= img->pitches[i];
				src += width / 2;
			}

		}

		gettimeofday(&currentTime, 0);
		uint64_t now = currentTime.tv_sec * 1000000 + currentTime.tv_usec;
		int64_t s = pts - now + tdiff;
		/*if (s > 0) {
			usleep((uint64_t) s);
		} else {
			tdiff = now - pts;
		}*/
		usleep((uint64_t) 10000);
		XvShmPutImage(dpy, xvport, win, gc, img, 0, 0, width, height, xOffset, yOffset, dWidth, dHeight, False);
		XSync(dpy, False);

		fprintf(stderr, "   %s  %5.2f%%  %6llu/%llu \r", timeFormat(pts - firstFrame), (float) 100 * fIndex / (cFrameTotal - 1), llu(fIndex + 1), llu(cFrameTotal));

		int skipFrames = pause ? 0 : 1;
		for (;;) {
			XEvent e;
			XClientMessageEvent event;

			if (XPending(dpy) || (pause && skipFrames == 0)) {
				XNextEvent(dpy, &e);
			} else {
				break;
			}

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
				XFillRectangle(dpy, win, gc, 0, 0, e.xconfigure.width, e.xconfigure.height);

				dWidth = e.xconfigure.width;
				dHeight = e.xconfigure.height;

				if ((float)width / height < (float)dWidth / dHeight) {
					dWidth = dHeight * width / height;
				} else {
					dHeight = dWidth * height / width;
				}

				xOffset = (e.xconfigure.width - dWidth) / 2;
				yOffset = (e.xconfigure.height - dHeight) / 2;
			case Expose:
				goto display;
			default:
				break;
			}
		}

display:
		if (skipFrames < 0 && -skipFrames > (int)fIndex) {
			fIndex = 0;
		} else if (skipFrames > 0 && fIndex + skipFrames >= cFrameTotal) {
			fIndex = cFrameTotal - 1;
			pause = 1;
		} else {
			fIndex += skipFrames;
		}

		if (1) {
			currentPosition = sourceData + sizeof(seomStreamPacket) + sizeof(seomStreamMap) + sizeof(uint32_t) * 2;
			for (unsigned int i = 0; i < fIndex; ++i) {
				if (currentPosition >= sourceData + statBuffer.st_size) {
					break;
				}

				streamPacket = (seomStreamPacket *) currentPosition;
				currentPosition += sizeof(seomStreamPacket) + streamPacket->payloadLength;
			}

			if (skipFrames > 1 || skipFrames < 0) {
				gettimeofday(&currentTime, 0);
				tdiff = currentTime.tv_sec * 1000000 + currentTime.tv_usec - pts;
			}
		}
	}

	munmap(sourceData, statBuffer.st_size);
	close(inFile);
}
