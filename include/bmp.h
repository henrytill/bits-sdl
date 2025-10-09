#ifndef SDL_BITS_INCLUDE_BMP_H
#define SDL_BITS_INCLUDE_BMP_H

#include <stddef.h>
#include <stdint.h>

typedef enum bmp_header_size {
  BITMAPCOREHEADER = 12,
  OS22XBITMAPHEADER = 64,
  BITMAPINFOHEADER = 40,
  BITMAPV2INFOHEADER = 52,
  BITMAPV3INFOHEADER = 56,
  BITMAPV4HEADER = 108,
  BITMAPV5HEADER = 124,
} bmp_header_size;

typedef struct bmp_file_header {
  uint16_t file_type;
  uint32_t file_size;
  uint16_t reserved1;
  uint16_t reserved2;
  uint32_t offset;
} __attribute__((packed)) bmp_file_header;

typedef struct bmp_info_header {
  uint32_t size;           // DIB Header size (bytes)
  int32_t width;           // Image width (pixels)
  int32_t height;          // Image height (pixels)
  uint16_t planes;         // Number of planes
  uint16_t bits_per_pixel; // Bits per pixel
  uint32_t compression;    // Compression mode
  uint32_t image_size;     // Image size (bytes)
  int32_t h_res;           // Horizontal resolution (pixels per meter)
  int32_t v_res;           // Vertical resolution (pixels per meter)
  uint32_t colors;         // Used colors
  uint32_t imp_colors;     // Important colors
} __attribute__((packed)) bmp_info_header;

typedef struct bmp_colorspace {
  int32_t rx;
  int32_t ry;
  int32_t rz;
  int32_t gx;
  int32_t gy;
  int32_t gz;
  int32_t bx;
  int32_t by;
  int32_t bz;
} __attribute__((packed)) bmp_colorspace;

typedef struct bmp_v4_header {
  uint32_t size;           // DIB Header Size (bytes)
  int32_t width;           // Image width (pixels)
  int32_t height;          // Image height (pixels)
  uint16_t planes;         // Number of planes
  uint16_t bits_per_pixel; // Bits per pixel
  uint32_t compression;    // Compression mode
  uint32_t image_size;     // Image size (bytes)
  int32_t h_res;           // Horizontal resolution (pixels per meter)
  int32_t v_res;           // Vertical resolution (pixels per meter)
  uint32_t colors;         // Used colors
  uint32_t imp_colors;     // Important colors
  uint32_t r_mask;
  uint32_t g_mask;
  uint32_t b_mask;
  uint32_t a_mask;
  uint32_t colorspace_type;
  bmp_colorspace colorspace;
  uint32_t r_gamma;
  uint32_t g_gamma;
  uint32_t b_gamma;
} __attribute__((packed)) bmp_v4_header;

typedef struct bmp_pixel24 {
  uint8_t b;
  uint8_t g;
  uint8_t r;
} __attribute__((packed)) bmp_pixel24;

typedef struct bmp_pixel32 {
  uint8_t b;
  uint8_t g;
  uint8_t r;
  uint8_t a;
} __attribute__((packed)) bmp_pixel32;

/// Calculates the number of bytes per row.
///
/// @param bits_per_pixel Bits per pixel.
/// @param width Image width.
/// @return Number of bytes per row.
size_t bmp_row_size(double bits_per_pixel, double width);

/// Reads a BMP file.
///
/// @param file Path to the BMP file.
/// @param file_header The file header structure to be filled.
/// @param info_header The info header structure to be filled.
/// @param image The image data to be filled.
/// @return 0 on success, -1 on error.
int bmp_read(char const *file, bmp_file_header *file_header, bmp_info_header *info_header, char **image);

/// Reads a BMP file with a V4 header.
///
/// @param file Path to the BMP file.
/// @param file_header The file header structure to be filled.
/// @param v4_header The V4 header structure to be filled.
/// @param image The image data to be filled.
/// @return 0 on success, -1 on error.
int bmp_v4_read(char const *file, bmp_file_header *file_header, bmp_v4_header *v4_header, char **image);

/// Writes a BMP file with a V4 header.
///
/// @param buffer The image data.
/// @param width Image width in pixels.
/// @param height Image height in pixels.
/// @param file Path to the BMP file
int bmp_v4_write(bmp_pixel32 const *buffer, size_t width, size_t height, char const *file);

#endif // SDL_BITS_INCLUDE_BMP_H
