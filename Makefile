
PREFIX   = /usr
DESTDIR  = 
LIBDIR   = lib

RM       = rm
CC       = gcc
ASM      = yasm
LIBTOOL  = libtool
INSTALL  = install

ARCH     = C

CFLAGS  += -Iinclude -std=c99 -O3 -W -Wall
LDFLAGS += -Wl,--as-needed

SRC = src/buffer.c              \
      src/client.c              \
      src/codec.c               \
      src/frame.c               \
      src/server.c              \
      src/stream.c              \
      src/arch/$(ARCH)/frame.c  \

OBJS = $(SRC:%.c=%.lo)

APPS = filter player server
playerLIBS = -lX11 -lXv

.PHONY: all clean install
all: libseom.la $(APPS)

%.lo: %.asm
	$(LIBTOOL) --tag=CC --mode=compile $(ASM) -f elf -m $(ARCH) -o $@ -prefer-non-pic $<

%.lo: %.c
	$(LIBTOOL) --tag=CC --mode=compile $(CC) $(CFLAGS) -c -o $@ $<

libseom.la: $(OBJS)
	$(LIBTOOL) --tag=CC --mode=link $(CC) $(LDFLAGS) -rpath $(PREFIX)/$(LIBDIR) -o $@ $(OBJS) -ldl -lpthread

example: example.c
	$(CC) $(CFLAGS) $(LDFLAGS) -L.libs -o $@ $< -lseom -lX11 -lGL

$(APPS): libseom.la
	$(CC) $(CFLAGS) $(LDFLAGS) -L.libs -o $@ src/$@/main.c -lseom $($@LIBS)

seom.pc: seom.pc.in
	./seom.pc.in $(PREFIX) $(LIBDIR) `svn info | grep Revision | sed 's#Revision: ##'`

install: seom.pc libseom.la $(APPS)
	install -m 0755 -d $(DESTDIR)/$(PREFIX)/$(LIBDIR)/pkgconfig
	install -m 0644 seom.pc $(DESTDIR)/$(PREFIX)/$(LIBDIR)/pkgconfig

	install -m 0755 -d $(DESTDIR)/$(PREFIX)/include/seom
	install -m 0644 include/seom/* $(DESTDIR)/$(PREFIX)/include/seom

	install -m 0755 -d $(DESTDIR)/$(PREFIX)/$(LIBDIR)
	$(LIBTOOL) --mode=install $(INSTALL) libseom.la $(DESTDIR)/$(PREFIX)/$(LIBDIR)/libseom.la

	install -m 0755 -d $(DESTDIR)/$(PREFIX)/bin
	install -m 0755 filter $(DESTDIR)/$(PREFIX)/bin/seom-filter
	install -m 0755 player $(DESTDIR)/$(PREFIX)/bin/seom-player
	install -m 0755 server $(DESTDIR)/$(PREFIX)/bin/seom-server

	install -m 0755 src/scripts/backup $(DESTDIR)/$(PREFIX)/bin/seom-backup

clean:
	$(LIBTOOL) --mode=clean $(RM) -f $(OBJS) libseom.la
	$(RM) -f $(APPS) seom.pc

