
PREFIX   = /usr
DESTDIR  = $(PREFIX)
LIBDIR   = lib

RM       = rm
CC       = gcc
ASM      = yasm
LIBTOOL  = libtool
INSTALL  = install

ARCH     = C

CFLAGS  += -Iinclude -std=c99 -O3 -W -Wall
LDFLAGS += 

SRC = src/buffer.c              \
      src/client.c              \
      src/codec.c               \
      src/frame.c               \
      src/server.c              \
      src/stream.c              \
      src/arch/$(ARCH)/frame.c  \

OBJS = $(SRC:%.c=%.lo)

APPS = filter player server
filterLIBS =
playerLIBS = -lX11 -lXv
serverLIBS = -lpthread

.PHONY: all clean install
all: libseom.la $(APPS)

%.lo: %.asm
	$(LIBTOOL) --tag=CC --mode=compile $(ASM) -f elf -m $(ARCH) -o $@ -prefer-non-pic $<

%.lo: %.c
	$(LIBTOOL) --tag=CC --mode=compile $(CC) $(CFLAGS) -c -o $@ $<

libseom.la: $(OBJS)
	$(LIBTOOL) --tag=CC --mode=link $(CC) $(LDFLAGS) -o $@ $(OBJS) -ldl -lpthread

example: example.c
	$(CC) $(CFLAGS) $(LDFLAGS) -L.libs -o $@ $< -lseom -lX11 -lGL

$(APPS): libseom.la
	$(CC) $(CFLAGS) -L.libs -o $@ src/$@/main.c -lseom $($@LIBS)

install: libseom.la $(APPS)
	install -m 0755 -d $(DESTDIR)/include/seom $(DESTDIR)/$(LIBDIR) $(DESTDIR)/bin
	install -m 0644 include/seom/* $(DESTDIR)/include/seom
	$(LIBTOOL) --mode=install $(INSTALL) libseom.la $(DESTDIR)/$(LIBDIR)/libseom.la
	install -m 0755 filter $(DESTDIR)/bin/seom-filter
	install -m 0755 player $(DESTDIR)/bin/seom-player
	install -m 0755 server $(DESTDIR)/bin/seom-server

clean:
	$(LIBTOOL) --mode=clean $(RM) -f $(OBJS) libseom.la

