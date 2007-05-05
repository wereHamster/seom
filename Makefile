
MAJOR    = 0
LIBRARY  = libseom.so

DESTDIR  = 
LIBDIR   = lib

CC       = gcc
ASM      = yasm

CFLAGS   = -Iinclude -std=c99
LDFLAGS  = -Wl,--as-needed

include config.make

OBJS = src/buffer.o src/client.o src/codec.o src/frame.o src/opengl.o \
       src/server.o src/stream.o src/arch/$(ARCH)/frame.o

TOOLS = filter player server
playerLIBS = -lX11 -lXv

.PHONY: all clean install
all: $(LIBRARY) $(TOOLS)

%.o: %.asm
	$(ASM) -m $(ARCH) -f elf -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

$(LIBRARY): $(OBJS)
	$(CC) -shared $(LDFLAGS) -Wl,-soname,$@.$(MAJOR) -o $@ $(OBJS) -ldl -lpthread

$(TOOLS): $(LIBRARY)
	$(CC) $(CFLAGS) $(LDFLAGS) -L. -o $@ src/tools/$@/main.c -lseom $($@LIBS)

seom.pc: seom.pc.in
	./seom.pc.in $(PREFIX) $(LIBDIR)

inst = install -m 755 -d $(DESTDIR)$(3); install -m $(1) $(2) $(DESTDIR)$(3)$(if $(4),/$(4));
install: $(LIBRARY) $(TOOLS) seom.pc
	$(call inst,644,seom.pc,$(PREFIX)/$(LIBDIR)/pkgconfig)
	$(call inst,755,$(LIBRARY),$(PREFIX)/$(LIBDIR),$(LIBRARY).$(MAJOR))
	ln -sf $(LIBRARY).$(MAJOR) $(DESTDIR)$(PREFIX)/$(LIBDIR)/$(LIBRARY)

	$(call inst,644,art/seom.svg,$(PREFIX)/share/seom,seom.svg)
	$(call inst,644,include/seom/*,$(PREFIX)/include/seom)
	$(call inst,755,src/scripts/backup,$(PREFIX)/bin,seom-backup)
	$(foreach tool,$(TOOLS),$(call inst,755,$(tool),$(PREFIX)/bin,seom-$(tool)))

clean:
	rm -f $(OBJS) $(LIBRARY) $(TOOLS) seom.pc

mrproper: clean
	rm -f config.make
