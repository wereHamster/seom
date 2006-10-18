
#ifndef __SEOM_H__
#define __SEOM_H__

#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE

#define _GNU_SOURCE
#define _BSD_SOURCE

#include <errno.h>
#include <assert.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <GL/glu.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#include <seom/config.h>
#include <seom/buffer.h>
#include <seom/frame.h>
#include <seom/codec.h>
#include <seom/stream.h>
#include <seom/client.h>

/*
typedef seomTime uint64_t
#define seomTimeCurrent() ({ struct timeval tv; gettimeofday(&tv, 0); (seomTime) tv.tv_sec * 1000000 + tv.tv_usec; })
*/

#define seomTime() ({ struct timeval tv; gettimeofday(&tv, 0); (uint64_t) tv.tv_sec * 1000000 + tv.tv_usec; })
	

#endif /* __SEOM_H__ */
