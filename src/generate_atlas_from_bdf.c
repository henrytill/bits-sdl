#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "bmp.h"

#define eprintf(...) (void)fprintf(stderr, __VA_ARGS__)

#define STATIC_ASSERT(e) _Static_assert((e), #e)

STATIC_ASSERT(CHAR_BIT == 8);

#define SELECT_BIT(c, pos) (((pos) >= CHAR_BIT) ? 0 : ((c) & (1 << (CHAR_BIT + ~(pos)))))

#define SELECT_BIT_TESTS       \
  X(0b10000000, 0, 0b10000000) \
  X(0b01000000, 1, 0b01000000) \
  X(0b00100000, 2, 0b00100000) \
  X(0b00010000, 3, 0b00010000) \
  X(0b00001000, 4, 0b00001000) \
  X(0b00000100, 5, 0b00000100) \
  X(0b00000010, 6, 0b00000010) \
  X(0b00000001, 7, 0b00000001) \
  X(0b11111111, 0, 0b10000000) \
  X(0b11111111, 7, 0b00000001) \
  X(0b00000000, 0, 0b00000000) \
  X(0b00000000, 7, 0b00000000) \
  X(0b00000000, 8, 0b00000000)

#define X(in, pos, out) STATIC_ASSERT(SELECT_BIT((in), (pos)) == (out));
SELECT_BIT_TESTS
#undef X

#define GET_BIT(c, pos) (((pos) >= CHAR_BIT) ? 0 : (((c) >> (CHAR_BIT + ~(pos)) & 1)))

#define GET_BIT_TESTS          \
  X(0b10000000, 0, 0b00000001) \
  X(0b01000000, 1, 0b00000001) \
  X(0b00100000, 2, 0b00000001) \
  X(0b00010000, 3, 0b00000001) \
  X(0b00001000, 4, 0b00000001) \
  X(0b00000100, 5, 0b00000001) \
  X(0b00000010, 6, 0b00000001) \
  X(0b00000001, 7, 0b00000001) \
  X(0b11111111, 0, 0b00000001) \
  X(0b11111111, 7, 0b00000001) \
  X(0b00000000, 0, 0b00000000) \
  X(0b00000000, 7, 0b00000000) \
  X(0b00000000, 8, 0b00000000)

#define X(in, pos, out) STATIC_ASSERT(GET_BIT((in), (pos)) == (out));
GET_BIT_TESTS
#undef X

enum {
  WIDTH = 10,
  HEIGHT = 20,
  LOW = '!',
  HIGH = '~',
  CODES_SIZE = (HIGH - LOW) + 1,
};

static const char *const FONT_FILE = "./assets/ucs-fonts/10x20.bdf";
static const char *const BMP_FILE = "./assets/10x20.bmp";

static const bmp_pixel32 WHITE = {0xFF, 0xFF, 0xFF, 0x00};
static const bmp_pixel32 BLACK = {0x00, 0x00, 0x00, 0xFF};

// https://freetype.org/freetype2/docs/reference/ft2-basic_types.html#ft_bitmap
static void render_bitmap_char(FT_GlyphSlot slot, char *target, const size_t code_size, const size_t offset) {
  const unsigned char *buffer = slot->bitmap.buffer;
  const size_t rows = (size_t)slot->bitmap.rows;
  const size_t width = (size_t)slot->bitmap.width;
  const size_t pitch = (size_t)abs(slot->bitmap.pitch);
  const size_t stride = width * code_size;

  assert(width == WIDTH);

  for (size_t y = 0, p = 0; y < rows; ++y, p += pitch) {
    for (size_t i = 0; i < pitch; ++i) {
      for (size_t j = 0, x; j < CHAR_BIT; ++j) {
        x = j + (i * CHAR_BIT);
        if (x >= width) {
          continue;
        }
        target[(y * stride) + (x + (offset * width))] = GET_BIT(buffer[p + i], j);
      }
    }
  }
}

static int render_bitmap_chars(FT_Face face, const char codes[CODES_SIZE], char *image) {
  int rc = FT_Set_Pixel_Sizes(face, WIDTH, HEIGHT);
  if (rc != 0) {
    eprintf("FT_Set_Pixel_Sizes failed.  Error code: %d", rc);
    return -1;
  }

  FT_GlyphSlot slot = NULL;
  for (size_t i = 0; i < CODES_SIZE; ++i) {
    rc = FT_Load_Char(face, (FT_ULong)codes[i], FT_LOAD_NO_SCALE | FT_LOAD_MONOCHROME);
    if (rc != 0) {
      eprintf("FT_Load_Char failed.  Error code: %d", rc);
      return -1;
    }
    slot = face->glyph;
    rc = FT_Render_Glyph(slot, FT_RENDER_MODE_MONO);
    if (rc != 0) {
      eprintf("FT_Render_Glyph failed.  Error code: %d", rc);
      return -1;
    }
    assert(slot->format == FT_GLYPH_FORMAT_BITMAP);
    assert(slot->bitmap.pixel_mode == FT_PIXEL_MODE_MONO);
    render_bitmap_char(slot, image, CODES_SIZE, i);
  }

  return 0;
}

static int render_chars(const char codes[CODES_SIZE], char *image) {
  int ret = -1;

  FT_Library lib = NULL;
  int rc = FT_Init_FreeType(&lib);
  if (rc != 0) {
    eprintf("FT_Init_FreeType failed.  Error code: %d", rc);
    return -1;
  }

  FT_Face face = NULL;
  rc = FT_New_Face(lib, FONT_FILE, 0, &face);
  if (rc != 0) {
    eprintf("FT_New_Face failed.  Error code: %d", rc);
    goto out_done_lib;
  }

  rc = render_bitmap_chars(face, codes, image);
  if (rc != 0) {
    goto out_done_face;
  }

  ret = 0;
out_done_face:
  FT_Done_Face(face);
out_done_lib:
  FT_Done_FreeType(lib);
  return ret;
}

#ifdef DRAW_IMAGE
static void draw_image(const char *image, const size_t width, const size_t height) {
  for (size_t y = 0; y < height; ++y) {
    printf("%2zd|", y);
    for (size_t x = 0; x < width; ++x) {
      putchar(image[(y * width) + x] ? '*' : ' ');
    }
    printf("|\n");
  }
}
#else
static inline void draw_image(__attribute__((unused)) const char *image,
                              __attribute__((unused)) const size_t width,
                              __attribute__((unused)) const size_t height) {
}
#endif

int main(void) {
  extern const char *const FONT_FILE;
  extern const char *const BMP_FILE;
  extern const bmp_pixel32 WHITE;
  extern const bmp_pixel32 BLACK;

  int ret = EXIT_FAILURE;

  char codes[CODES_SIZE] = {0};
  for (int i = 0; i < CODES_SIZE; ++i) {
    codes[i] = (char)(i + LOW);
  }

  const size_t width = (size_t)WIDTH * CODES_SIZE;
  const size_t height = HEIGHT;
  char *image = calloc(width * height, sizeof(*image));
  if (image == NULL) {
    eprintf("alloc_image failed.");
    return EXIT_FAILURE;
  }

  int rc = render_chars(codes, image);
  if (rc != 0) {
    goto out_free_image;
  }

  draw_image(image, width, height);

  bmp_pixel32 *buffer = calloc(width * height, sizeof(*buffer));
  if (buffer == NULL) {
    goto out_free_image;
  }

  for (size_t y = height, i = 0; y-- > 0;) {
    for (size_t x = 0; x < width; ++x, ++i) {
      buffer[i] = image[(y * width) + x] ? BLACK : WHITE;
    }
  }

  rc = bmp_v4_write(buffer, width, height, BMP_FILE);
  if (rc != 0) {
    eprintf("bmp_v4_write failed.  Error code: %d", rc);
    goto out_free_buffer;
  }

  ret = EXIT_SUCCESS;
out_free_buffer:
  free(buffer);
out_free_image:
  free(image);
  return ret;
}
