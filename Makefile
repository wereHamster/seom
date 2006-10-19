PREFIX=/usr
DESTDIR = $(PREFIX)/lib

CC      = gcc
LIBTOOL = libtool
INSTALL = install

CFLAGS  = -Iinclude -std=c99 -pipe -O3 -W -Wall
LDFLAGS = -ldl -lpthread

SRC = src/buffer.c         \
      src/client.c         \
      src/codec.c          \
      src/config.c         \
      src/frame.c          \
      src/server.c         \
      src/stream.c         \
      src/asm/codec.c      \
      src/asm/frame.c      \

OBJS = $(SRC:%.c=%.lo)

APPS = filter player server
filterLIBS = -lseom
playerLIBS = -lseom -lX11 -lXv
serverLIBS = -lseom

.PHONY: all clean install
all: libseom.la $(APPS)

%.lo: %.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) -c -o $@ $<

libseom.la: $(OBJS)
	$(LIBTOOL) --mode=link $(CC) $(LDFLAGS) -rpath $(DESTDIR) -o $@ $(OBJS)

example: example.c
	$(CC) $(CFLAGS) -L.libs $(LDFLAGS) -lseom -lX11 -lGL -o example $<

$(APPS): libseom.la
	$(CC) $(CFLAGS) -L.libs $(LDFLAGS) $($@LIBS) -o seom-$@ src/$@/main.c

install: libseom.la $(APPS)
	install -m 0644 include/seom/* $(PREFIX)/include/seom
	$(LIBTOOL) --mode=install $(INSTALL) libseom.la $(DESTDIR)/libseom.la
	install -m 0755 seom-filter $(PREFIX)/bin/seom-filter
	install -m 0755 seom-player $(PREFIX)/bin/seom-player
	install -m 0755 seom-server $(PREFIX)/bin/seom-server

clean:
	rm -rf $(OBJS) .libs libseom.* seom-* example

