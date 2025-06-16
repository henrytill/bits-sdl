CFLAGS = -g -std=gnu11 -Wall -Wextra -Wconversion -Wsign-conversion -Iinclude
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

OBJECTS =
OBJECTS += src/bmp.o
OBJECTS += src/generate_atlas_from_bdf.o
OBJECTS += src/generate_test_bmp.o
OBJECTS += src/get_displays.o
OBJECTS += src/library_versions.o
OBJECTS += src/main.o
OBJECTS += src/message_queue.o
OBJECTS += test/bmp_read_bitmap.o
OBJECTS += test/bmp_read_bitmap_v4.o
OBJECTS += test/message_queue_basic.o
OBJECTS += test/message_queue_copies.o

BINARIES =
BINARIES += $(BINOUT)/generate_atlas_from_bdf
BINARIES += $(BINOUT)/generate_test_bmp
BINARIES += $(BINOUT)/get_displays
BINARIES += $(BINOUT)/library_versions
BINARIES += $(BINOUT)/main
BINARIES += $(BINOUT)/bmp_read_bitmap
BINARIES += $(BINOUT)/bmp_read_bitmap_v4

TEST_BINARIES =
TEST_BINARIES += $(BINOUT)/bmp_read_bitmap
TEST_BINARIES += $(BINOUT)/bmp_read_bitmap_v4

-include config.mk

all: $(OBJECTS) $(BINARIES)

$(OBJECTS): $(HEADERS)

src/generate_atlas_from_bdf.o: CFLAGS += $(FREETYPE_CFLAGS)

src/get_displays.o: CFLAGS += $(SDL_CFLAGS)

src/library_versions.o: CFLAGS += $(FREETYPE_CFLAGS) $(LUA_CFLAGS) $(SDL_CFLAGS)

src/main.o: CFLAGS += $(LUA_CFLAGS) $(SDL_CFLAGS)

src/message_queue.o: CFLAGS += $(SDL_CFLAGS)

$(BINOUT):
	mkdir -p -- $(BINOUT)

$(BINOUT)/generate_atlas_from_bdf: LDLIBS += -lm $(FREETYPE_LDLIBS)
$(BINOUT)/generate_atlas_from_bdf: src/generate_atlas_from_bdf.o src/bmp.o | $(BINOUT)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BINOUT)/generate_test_bmp: LDLIBS += -lm
$(BINOUT)/generate_test_bmp: src/generate_test_bmp.o src/bmp.o | $(BINOUT)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BINOUT)/get_displays: LDLIBS += $(SDL_LDLIBS)
$(BINOUT)/get_displays: src/get_displays.o | $(BINOUT)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BINOUT)/library_versions: LDLIBS += $(FREETYPE_LDLIBS) $(LUA_LDLIBS) $(SDL_LDLIBS)
$(BINOUT)/library_versions: src/library_versions.o | $(BINOUT)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BINOUT)/main: LDLIBS += -lm $(LUA_LDLIBS) $(SDL_LDLIBS)
$(BINOUT)/main: src/main.o src/message_queue.o | $(BINOUT)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BINOUT)/bmp_read_bitmap: LDLIBS += -lm
$(BINOUT)/bmp_read_bitmap: test/bmp_read_bitmap.o src/bmp.o | $(BINOUT)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BINOUT)/bmp_read_bitmap_v4: LDLIBS += -lm
$(BINOUT)/bmp_read_bitmap_v4: test/bmp_read_bitmap_v4.o src/bmp.o | $(BINOUT)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

assets/10x20.bmp: $(BINOUT)/generate_atlas_from_bdf
	$< $@

assets/test.bmp: $(BINOUT)/generate_test_bmp
	$< $@

.PHONY: check
check: $(TEST_BINARIES) assets/test.bmp
	$(BINOUT)/bmp_read_bitmap_v4 assets/test.bmp
	$(BINOUT)/bmp_read_bitmap assets/sample_24bit.bmp

.PHONY: clean
clean:
	rm -f -- $(BINARIES) $(OBJECTS)
	rmdir $(BINOUT)
	rm -f assets/test.bmp
