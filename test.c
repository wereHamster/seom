
#include <X11/Xlib.h>

#include <seom/seom.h>

static Bool WaitFor__MapNotify(Display * d, XEvent * e, char *arg)
{
	return (e->type == MapNotify) && (e->xmap.window == (Window) arg);
}

static Bool WaitFor__ConfigureNotify(Display * d, XEvent * e, char *arg)
{
	return (e->type == ConfigureNotify) && (e->xconfigure.window == (Window) arg);
}

static Window glCaptureCreateWindow(Display *dpy, int width, int height)
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
	printf("VisualID: 0x%x\n", vi->visualid);

	cx = glXCreateNewContext(dpy, fbc[0], GLX_RGBA_TYPE, 0, GL_FALSE);
	cmap = XCreateColormap(dpy, RootWindow(dpy, vi->screen), vi->visual, AllocNone);

	swa.colormap = cmap;
	swa.border_pixel = 0;
	swa.event_mask = 0;
	Window win = XCreateWindow(dpy, RootWindow(dpy, vi->screen), 0, 0, width, height, 0, vi->depth, InputOutput, vi->visual, CWBorderPixel | CWColormap | CWEventMask, &swa);
	XSelectInput(dpy, win, StructureNotifyMask | KeyPressMask | KeyReleaseMask);
	XMapWindow(dpy, win);
	XIfEvent(dpy, &event, WaitFor__MapNotify, (char *)win);

	XMoveWindow(dpy, win, 64, 64);
	XIfEvent(dpy, &event, WaitFor__ConfigureNotify, (char *)win);

	glXMakeCurrent(dpy, win, cx);
	return win;
}

int main(int argc, char *argv[])
{
	Display *dpy = XOpenDisplay(NULL);
	Window win = glCaptureCreateWindow(dpy, 1280, 1024); 
	
	seomClient *client = seomClientCreate(dpy, win, "beryl");
	
	for (int i = 1; i < 10000; ++i) { seomClientCapture(client); }
	
	seomClientDestroy(client);
}
