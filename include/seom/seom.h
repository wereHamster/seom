
#ifndef __SEOM_H__
#define __SEOM_H__

#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE

#define _GNU_SOURCE
#define _BSD_SOURCE

#include <assert.h>
#include <errno.h>
#include <limits.h>

#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>

#include <dlfcn.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <GL/gl.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <seom/frame.h>
#include <seom/codec.h>
#include <seom/stream.h>
#include <seom/queue.h>
#include <seom/client.h>

#define seomTime() ({ struct timeval tv; gettimeofday(&tv, 0); (uint64_t) tv.tv_sec * 1000000 + tv.tv_usec; })

#endif /* __SEOM_H__ */
