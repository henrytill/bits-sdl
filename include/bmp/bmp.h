#pragma once

#include <stddef.h>
#include <stdint.h>

#include "macro.h"

extern const uint16_t bmp_FILETYPE;
extern const uint32_t bmp_BI_BITFIELDS;
extern const uint32_t bmp_LCS_WINDOWS_COLOR_SPACE;

enum bmp_HeaderSize {
  BITMAPCOREHEADER = 12,
  OS22XBITMAPHEADER = 64,
  BITMAPINFOHEADER = 40,
  BITMAPV2INFOHEADER = 52,
  BITMAPV3INFOHEADER = 56,
  BITMAPV4HEADER = 108,
  BITMAPV5HEADER = 124
};

typedef struct bmp_FileHeader {
  uint16_t fileType;
  uint32_t fileSize;
  uint16_t reserved1;
  uint16_t reserved2;
  uint32_t offset;
} __attribute__((packed)) bmp_FileHeader;

typedef struct bmp_InfoHeader {
  uint32_t size;         // DIB Header size (bytes)
  int32_t width;         // Image width (pixels)
  int32_t height;        // Image height (pixels)
  uint16_t planes;       // Number of planes
  uint16_t bitsPerPixel; // Bits per pixel
  uint32_t compression;  // Compression mode
  uint32_t imageSize;    // Image size (bytes)
  int32_t hRes;          // Horizontal resolution (pixels per meter)
  int32_t vRes;          // Vertical resolution (pixels per meter)
  uint32_t colors;       // Used colors
  uint32_t impColors;    // Important colors
} __attribute__((packed)) bmp_InfoHeader;

typedef struct bmp_Colorspace {
  int32_t rx;
  int32_t ry;
  int32_t rz;
  int32_t gx;
  int32_t gy;
  int32_t gz;
  int32_t bx;
  int32_t by;
  int32_t bz;
} __attribute__((packed)) bmp_Colorspace;

typedef struct bmp_V4Header {
  uint32_t size;         // DIB Header Size (bytes)
  int32_t width;         // Image width (pixels)
  int32_t height;        // Image height (pixels)
  uint16_t planes;       // Number of planes
  uint16_t bitsPerPixel; // Bits per pixel
  uint32_t compression;  // Compression mode
  uint32_t imageSize;    // Image size (bytes)
  int32_t hRes;          // Horizontal resolution (pixels per meter)
  int32_t vRes;          // Vertical resolution (pixels per meter)
  uint32_t colors;       // Used colors
  uint32_t impColors;    // Important colors
  uint32_t rMask;
  uint32_t gMask;
  uint32_t bMask;
  uint32_t aMask;
  uint32_t colorspaceType;
  bmp_Colorspace colorspace;
  uint32_t rGamma;
  uint32_t gGamma;
  uint32_t bGamma;
} __attribute__((packed)) bmp_V4Header;

typedef struct bmp_Pixel24 {
  uint8_t b;
  uint8_t g;
  uint8_t r;
} __attribute__((packed)) bmp_Pixel24;

typedef struct bmp_Pixel32 {
  uint8_t b;
  uint8_t g;
  uint8_t r;
  uint8_t a;
} __attribute__((packed)) bmp_Pixel32;

///
/// Calculate the number of bytes per row.
///
/// @param bitsPerPixel Bits per pixel.
/// @param width Image width.
/// @return Number of bytes per row.
///
size_t bmp_RowSize(uint16_t bitsPerPixel, int32_t width);

///
/// Read a BMP file.
///
/// @param file Path to the BMP file.
/// @param fileHeader The file header structure to be filled.
/// @param infoHeader The info header structure to be filled.
/// @param image The image data to be filled.
/// @return 0 on success, -1 on error.
///
int bmp_Read(const char* file, bmp_FileHeader* fileHeader, bmp_InfoHeader* infoHeader, char** image);

///
/// Reads a BMP file with a V4 header.
///
/// @param file Path to the BMP file.
/// @param fileHeader The file header structure to be filled.
/// @param v4Header The V4 header structure to be filled.
/// @param image The image data to be filled.
/// @return 0 on success, -1 on error.
///
int bmp_V4Read(const char* file, bmp_FileHeader* fileHeader, bmp_V4Header* v4Header, char** image);

///
/// Write a BMP file with a V4 header.
///
/// @param buffer The image data.
/// @param width Image width in pixels.
/// @param height Image height in pixels.
/// @param file Path to the BMP file
///
int bmp_V4Write(const bmp_Pixel32* buffer, size_t width, size_t height, const char* file);
