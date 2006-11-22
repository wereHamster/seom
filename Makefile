
PREFIX   = /usr
DESTDIR  = $(PREFIX)
LIBDIR   = lib

CC       = gcc
LIBTOOL  = libtool
INSTALL  = install

CFLAGS  += -Iinclude -std=c99 -O3 -W -Wall
LDFLAGS += -ldl -lpthread

SRC = src/buffer.c         \
      src/client.c         \
      src/codec.c          \
      src/frame.c          \
      src/server.c         \
      src/stream.c         \
      src/quick.c          \
      src/asm/frame.c      \

OBJS = $(SRC:%.c=%.lo)

APPS = filter player server
filterLIBS =
playerLIBS = -lX11 -lXv
serverLIBS =

.PHONY: all clean install
all: libseom.la $(APPS)

%.lo: %.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) -c -o $@ $<

libseom.la: $(OBJS)
	$(LIBTOOL) --mode=link $(CC) $(LDFLAGS) -rpath $(PREFIX)/$(LIBDIR) -o $@ $(OBJS)

example: example.c
	$(CC) $(CFLAGS) $(LDFLAGS) -L.libs -lseom -lX11 -lGL -o example $<

$(APPS): libseom.la
	$(CC) $(CFLAGS) $(LDFLAGS) -L.libs -lseom $($@LIBS) -o $@ src/$@/main.c

install: libseom.la $(APPS)
	install -m 0755 -d $(DESTDIR)/include/seom $(DESTDIR)/$(LIBDIR) $(DESTDIR)/bin
	install -m 0644 include/seom/* $(DESTDIR)/include/seom
	$(LIBTOOL) --mode=install $(INSTALL) libseom.la $(DESTDIR)/$(LIBDIR)/libseom.la
	install -m 0755 filter $(DESTDIR)/bin/seom-filter
	install -m 0755 player $(DESTDIR)/bin/seom-player
	install -m 0755 server $(DESTDIR)/bin/seom-server

clean:
	rm -rf $(OBJS) .libs src/.libs src/asm/.libs libseom.* $(APPS) example
	find src -name *.o -type f -delete

