CFLAGS = -g -std=gnu11 -Iinclude -Wall -Wextra -Wconversion -Wsign-conversion
BINOUT = _bin

LUA_CFLAGS = $(shell pkg-config --cflags lua5.1)
LUA_LDLIBS = $(shell pkg-config --libs lua5.1)

SDL_CFLAGS = $(shell pkg-config --cflags sdl2)
SDL_LDLIBS = $(shell pkg-config --libs sdl2)

FREETYPE_CFLAGS = $(shell pkg-config --cflags freetype2)
FREETYPE_LDLIBS = $(shell pkg-config --libs freetype2)

HEADERS =
HEADERS += include/bmp.h
HEADERS += include/macro.h
HEADERS += include/message_queue.h
HEADERS += include/prelude_sdl.h
HEADERS += include/prelude_stdlib.h
HEADERS += src/compat.h
HEADERS += src/shared.h

OBJECTS =
OBJECTS += src/bmp.o
OBJECTS += src/generate_atlas_from_bdf.o
OBJECTS += src/generate_test_bmp.o
OBJECTS += src/get_displays.o
OBJECTS += src/main.o
OBJECTS += src/message_queue_posix.o
OBJECTS += src/message_queue_sdl.o
OBJECTS += src/shared.o
OBJECTS += test/bmp_read_bitmap.o
OBJECTS += test/bmp_read_bitmap_v4.o
OBJECTS += test/message_queue_basic.o
OBJECTS += test/message_queue_copies.o

BINARIES =
BINARIES += $(BINOUT)/generate_atlas_from_bdf
BINARIES += $(BINOUT)/generate_test_bmp
BINARIES += $(BINOUT)/get_displays
BINARIES += $(BINOUT)/main
BINARIES += $(BINOUT)/bmp_read_bitmap
BINARIES += $(BINOUT)/bmp_read_bitmap_v4
BINARIES += $(BINOUT)/message_queue_basic
BINARIES += $(BINOUT)/message_queue_copies

-include config.mk

all: $(OBJECTS) $(BINARIES)

$(OBJECTS): $(HEADERS)

src/generate_atlas_from_bdf.o: CFLAGS += $(FREETYPE_CFLAGS)
src/generate_atlas_from_bdf.o: src/bmp.o

src/get_displays.o: CFLAGS += $(SDL_CFLAGS)

src/main.o: CFLAGS += $(LUA_CFLAGS) $(SDL_CFLAGS)

src/message_queue_sdl.o: CFLAGS += $(SDL_CFLAGS)

test/message_queue_basic.o: CFLAGS += $(SDL_CFLAGS)

test/message_queue_copies.o: CFLAGS += $(SDL_CFLAGS)

$(BINOUT)/generate_atlas_from_bdf: LDLIBS += -lm $(FREETYPE_LDLIBS)
$(BINOUT)/generate_atlas_from_bdf: src/generate_atlas_from_bdf.o src/bmp.o
	@mkdir -p -- $(BINOUT)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BINOUT)/generate_test_bmp: LDLIBS += -lm
$(BINOUT)/generate_test_bmp: src/generate_test_bmp.o src/bmp.o
	@mkdir -p -- $(BINOUT)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BINOUT)/get_displays: LDLIBS += $(SDL_LDLIBS)
$(BINOUT)/get_displays: src/get_displays.o
	@mkdir -p -- $(BINOUT)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BINOUT)/main: LDLIBS += -lm $(LUA_LDLIBS) $(SDL_LDLIBS)
$(BINOUT)/main: src/main.o src/message_queue_sdl.o
	@mkdir -p -- $(BINOUT)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BINOUT)/bmp_read_bitmap: LDLIBS += -lm
$(BINOUT)/bmp_read_bitmap: test/bmp_read_bitmap.o src/bmp.o
	@mkdir -p -- $(BINOUT)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BINOUT)/bmp_read_bitmap_v4: LDLIBS += -lm
$(BINOUT)/bmp_read_bitmap_v4: test/bmp_read_bitmap_v4.o src/bmp.o
	@mkdir -p -- $(BINOUT)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BINOUT)/message_queue_basic: LDLIBS += $(SDL_LDLIBS)
$(BINOUT)/message_queue_basic: test/message_queue_basic.o src/shared.o src/message_queue_sdl.o
	@mkdir -p -- $(BINOUT)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BINOUT)/message_queue_copies: LDLIBS += $(SDL_LDLIBS)
$(BINOUT)/message_queue_copies: test/message_queue_copies.o src/shared.o src/message_queue_sdl.o
	@mkdir -p -- $(BINOUT)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

.PHONY: check
check:
	@echo Hello

.PHONY: clean
clean:
	rm -f -- $(BINARIES) $(OBJECTS)
