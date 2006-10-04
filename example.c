
#include <X11/Xlib.h>

#include <seom/seom.h>

static Bool waitMapNotify(Display *dpy, XEvent *e, char *arg)
{
	return dpy && (e->type == MapNotify) && (e->xmap.window == (Window) arg);
}

static Bool waitConfigureNotify(Display *dpy, XEvent *e, char *arg)
{
	return dpy && (e->type == ConfigureNotify) && (e->xconfigure.window == (Window) arg);
}

static Window glWindow(Display *dpy, int width, int height)
{
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
	Window win = XCreateWindow(dpy, RootWindow(dpy, vi->screen), 0, 0, width, height, 0, vi->depth, InputOutput, vi->visual, CWBorderPixel | CWColormap | CWEventMask, &swa);
	XSelectInput(dpy, win, StructureNotifyMask | KeyPressMask | KeyReleaseMask);
	XMapWindow(dpy, win);
	XIfEvent(dpy, &event, waitMapNotify, (char *)win);

	XMoveWindow(dpy, win, 100, 100);
	XIfEvent(dpy, &event, waitConfigureNotify, (char *)win);

	glXMakeCurrent(dpy, win, cx);
	return win;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Usage: %s [config namespace]\n", argv[0]);
		return 0; 
	}
	
	Display *dpy = XOpenDisplay(NULL);
	Window win = glWindow(dpy, 400, 400); 
	
	seomClient *client = seomClientCreate(dpy, win, argv[1]);
	if (client == NULL) {
		printf("couldn't create seomClient\n");
		return 0; 
	}
	
	const int max = 10000;
	const float step = (float) 1.0 / max;
	
	float color[3] = { 0.0, 1.0, 0.5 };
	for (int i = 0; i < max; ++i) {
		color[0] += step;
		color[1] -= step;
		
		glClearColor(color[0], color[1], color[2], 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		
		seomClientCapture(client);
		glXSwapBuffers(dpy, win);
	}
	
	seomClientDestroy(client);
}
