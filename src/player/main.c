
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
	XvAdaptorInfo *p_adaptor;
	unsigned int i;
	unsigned int i_adaptor, i_num_adaptors;
	int i_selected_port;

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

	switch (XvQueryAdaptors(dpy, DefaultRootWindow(dpy), &i_num_adaptors, &p_adaptor)) {
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

	printf("%d adaptors\n", i_num_adaptors);

	i_selected_port = -1;

	for (i_adaptor = 0; i_adaptor < i_num_adaptors; ++i_adaptor) {
		XvImageFormatValues *p_formats;
		int i_format, i_num_formats;
		int i_port;

		/* If the adaptor doesn't have the required properties, skip it */
		if (!(p_adaptor[i_adaptor].type & XvInputMask) || !(p_adaptor[i_adaptor].type & XvImageMask)) {
			continue;
		}

		/* Check that adaptor supports our requested format... */
		p_formats = XvListImageFormats(dpy, p_adaptor[i_adaptor].base_id, &i_num_formats);

		for (i_format = 0; i_format < i_num_formats && (i_selected_port == -1); i_format++) {
			XvAttribute *p_attr;
			int i_attr, i_num_attributes;

			if (p_formats[i_format].id != 0x30323449)
				continue;
			/* Look for the first available port supporting this format */
			for (i_port = p_adaptor[i_adaptor].base_id; (i_port < (int)(p_adaptor[i_adaptor].base_id + p_adaptor[i_adaptor].num_ports))
				 && (i_selected_port == -1); i_port++) {

				if (XvGrabPort(dpy, i_port, CurrentTime)
					== Success) {
					i_selected_port = i_port;
				}
			}

			/* If no free port was found, forget it */
			if (i_selected_port == -1) {
				continue;
			}

			/* If we found a port, print information about it */
			fprintf(stderr, "adaptor %i, port %i, format 0x%x (%4.4s) %s\n", i_adaptor, i_selected_port, p_formats[i_format].id, (char *)&p_formats[i_format].id, (p_formats[i_format].format == XvPacked) ? "packed" : "planar");

			/* Make sure XV_AUTOPAINT_COLORKEY is set */
			p_attr = XvQueryPortAttributes(dpy, i_selected_port, &i_num_attributes);

			for (i_attr = 0; i_attr < i_num_attributes; i_attr++) {
				if (!strcmp(p_attr[i_attr].name, "XV_AUTOPAINT_COLORKEY")) {
					const Atom autopaint = XInternAtom(dpy,
													   "XV_AUTOPAINT_COLORKEY", False);
					XvSetPortAttribute(dpy, i_selected_port, autopaint, 1);
					break;
				}
			}

			if (p_attr != NULL) {
				XFree(p_attr);
			}
		}

		if (p_formats != NULL) {
			XFree(p_formats);
		}

	}

	if (i_num_adaptors > 0) {
		XvFreeAdaptorInfo(p_adaptor);
	}

	return i_selected_port;
}

static uint8_t *yuvImage;

static Display *dpy = NULL;
static Window win = 0;

static Bool WaitFor__MapNotify(Display * dpy, XEvent * e, char *arg)
{
	return dpy && (e->type == MapNotify) && (e->xmap.window == (Window) arg);
}

static Bool WaitFor__ConfigureNotify(Display * dpy, XEvent * e, char *arg)
{
	return dpy && (e->type == ConfigureNotify) && (e->xconfigure.window == (Window) arg);
}

static void createWindow(int width, int height)
{
	dpy = XOpenDisplay(NULL);

	XVisualInfo vit;
	XVisualInfo *vi;
	Colormap cmap;
	XSetWindowAttributes swa;
	XEvent event;
	int num;

	vit.visualid = XVisualIDFromVisual(DefaultVisual(dpy, DefaultScreen(dpy)));
	vi = XGetVisualInfo(dpy, VisualIDMask, &vit, &num);
	cmap = XCreateColormap(dpy, DefaultRootWindow(dpy), vi->visual, AllocNone);

	swa.colormap = cmap;
	swa.border_pixel = 0;
	swa.event_mask = 0;
	win = XCreateWindow(dpy, DefaultRootWindow(dpy), 0, 0, width, height, 0, vi->depth, InputOutput, vi->visual, CWBorderPixel | CWColormap | CWEventMask, &swa);
	XSelectInput(dpy, win, StructureNotifyMask | KeyPressMask | KeyReleaseMask);
	XMapWindow(dpy, win);
	XIfEvent(dpy, &event, WaitFor__MapNotify, (char *)win);

	XMoveWindow(dpy, win, 64, 64);
	XIfEvent(dpy, &event, WaitFor__ConfigureNotify, (char *)win);
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
		uint32_t cSize = *(uint32_t *) mem;
		mem += sizeof(uint32_t) + cSize;

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

	float tt = (float) (time[1] - time[0]) / 1000000.0;
	float fps = (float) cFrameTotal / tt;
	float mbs = (float) statBuffer.st_size / 1024 / 1024 / tt;
	printf("%llu frames, %.2f seconds, %.1f fps, %.1f MiB/s\n", llu(cFrameTotal), tt, fps, mbs);

	uint64_t cTimeTotal = *(uint64_t *) (sourceData + statBuffer.st_size - (width * height * 3 / 2 + sizeof(uint64_t))) - *(uint64_t *) currentPosition;
	printf("size: %u:%u, cFrameTotal: %llu, time: %llu\n", width, height, llu(cFrameTotal), llu(cTimeTotal / 1000000));

	uint64_t rawFrames = (statBuffer.st_size - 2 * sizeof(uint32_t)) / (width * height * 3 / 2 + sizeof(uint64_t));
	printf("ratio: %.3f\n", (float)rawFrames / cFrameTotal);

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
		pts = *(uint64_t *) currentPosition;
		currentPosition += sizeof(uint64_t);

		uint32_t cSize = *(uint32_t *) currentPosition;
		currentPosition += sizeof(uint32_t);

		seomCodecDecode(yuvImage, (uint32_t *) currentPosition, width, height);

		uint8_t *dst = (uint8_t *) img->data + width * (height - 1);
		uint8_t *src = yuvImage;

		for (uint32_t y = 0; y < height; ++y) {
			memcpy(dst, src, width);
			dst -= width;
			src += width;
		}

		dst += width * (height + 1);

		for (int i = 0; i < 2; ++i) {
			dst += width / 2 * (height / 2 - 1);
			for (uint32_t y = 0; y < height / 2; ++y) {
				memcpy(dst, src, width / 2);
				dst -= width / 2;
				src += width / 2;
			}
			dst += width / 2 * (height / 2 + 1);
		}

		currentPosition += cSize;

		gettimeofday(&currentTime, 0);
		uint64_t now = currentTime.tv_sec * 1000000 + currentTime.tv_usec;
		int64_t s = pts - now + tdiff;
		if (s > 0) {
			usleep((uint64_t) s);
		} else {
			tdiff = now - pts;
		}
		XvShmPutImage(dpy, xvport, win, gc, img, 0, 0, width, height, xOffset, yOffset, dWidth, dHeight, False);
		XSync(dpy, False);

		fprintf(stderr, "position: %llu/%llu \r", llu(fIndex), llu(cFrameTotal));

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
				uint32_t cSize = *(uint32_t *) currentPosition;
				currentPosition += sizeof(uint32_t) + cSize;

				if (currentPosition >= sourceData + statBuffer.st_size) {
					break;
				}
			}
			pts = *(uint64_t *) currentPosition;

			if (skipFrames > 1 || skipFrames < 0) {
				gettimeofday(&currentTime, 0);
				tdiff = currentTime.tv_sec * 1000000 + currentTime.tv_usec - pts;
			}
		}
	}

	munmap(sourceData, statBuffer.st_size);
	close(inFile);
}
