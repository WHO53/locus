NAME = locus
LIBRARY = lib${NAME}.so
SRC = .
PREFIX ?= /usr
INCDIR = $(PREFIX)/include/$(NAME)
LIBDIR = $(PREFIX)/lib
PCDIR = $(LIBDIR)/pkgconfig

PKGS = wayland-client wayland-protocols egl glesv2 wayland-egl stb librsvg-2.0

LOCUS_SOURCES += $(wildcard $(SRC)/*.c) $(wildcard $(SRC)/**/*.c) 
LOCUS_HEADERS += $(wildcard $(SRC)/*.h) $(wildcard $(SRC)/**/*.h) 

CFLAGS += -std=gnu99 -Wall -g -DWITH_WAYLAND_SHM -fPIC
CFLAGS += $(shell pkg-config --cflags $(PKGS)) -I/usr/include/nanosvg/
LDFLAGS += $(shell pkg-config --libs $(PKGS)) -lm -lutil -lrt -L/lib/aarch64-linux-gnu/libnanovg.a -lnanovg -L/lib/aarch64-linux-gnu/libnanosvg.a -lnanosvg -L/lib/aarch64-linux-gnu/libnanosvgrast.a -lnanosvgrast

WAYLAND_HEADERS = $(wildcard proto/*.xml)

HDRS = $(WAYLAND_HEADERS:.xml=-client-protocol.h)
WAYLAND_SRC = $(HDRS:.h=.c)
SOURCES = $(LOCUS_SOURCES) $(WAYLAND_SRC)

OBJECTS = $(SOURCES:.c=.o)

all: ${LIBRARY}

proto/%-client-protocol.c: proto/%.xml
	wayland-scanner private-code < $? > $@

proto/%-client-protocol.h: proto/%.xml
	wayland-scanner client-header < $? > $@

$(OBJECTS): $(HDRS) $(LOCUS_HEADERS)

$(LIBRARY): $(OBJECTS)
	$(CC) -shared -o $@ $(OBJECTS) $(LDFLAGS)

install: $(LIBRARY) locus.pc
	install -d $(LIBDIR)
	install -m 0755 $(LIBRARY) $(LIBDIR)

	install -d $(INCDIR)
	install -m 0644 $(LOCUS_HEADERS) $(INCDIR)

	install -d $(INCDIR)/proto
	install -m 0644 $(HDRS) $(INCDIR)/proto

	install -d $(PCDIR)
	install -m 0644 locus.pc $(PCDIR)

uninstall:
	rm -f $(LIBDIR)/$(LIBRARY)
	rm -rf $(INCDIR)
	rm -rf $(PCDIR)/locus.pc

clean:
	rm -f $(OBJECTS) $(HDRS) $(WAYLAND_SRC) $(LIBRARY)

format:
	clang-format -i $(SOURCES) $(LOCUS_HEADERS)
