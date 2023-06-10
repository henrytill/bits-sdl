#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "bmp.h"

enum {
	DWORD_BITS = 32,
	DWORD_BYTES = 4,
};

static const uint16_t FILE_TYPE = 0x4D42;
static const uint32_t BI_BITFIELDS = 0x0003;
static const uint32_t LCS_WINDOWS_COLOR_SPACE = 0x57696E20;

size_t bmp_row_size(uint16_t bits_per_pixel, int32_t width)
{
	const double pixel_bits = (double)bits_per_pixel * width;
	return (size_t)(ceil(pixel_bits / DWORD_BITS)) * DWORD_BYTES;
}

int bmp_read(const char *file, bmp_file_header *file_header, bmp_info_header *info_header, char **image)
{
	int ret = -1;

	FILE *file_handle = fopen(file, "r");
	if (file_handle == NULL) {
		return -1;
	}

	size_t reads = fread(file_header, sizeof(*file_header), 1, file_handle);
	if (reads != 1) {
		goto out_fclose_file_handle;
	}

	fpos_t pos;
	int rc = fgetpos(file_handle, &pos);
	if (rc != 0) {
		goto out_fclose_file_handle;
	}

	uint32_t size = 0;
	reads = fread(&size, sizeof(size), 1, file_handle);
	if (reads != 1) {
		goto out_fclose_file_handle;
	}
	if (size != BITMAPINFOHEADER) {
		goto out_fclose_file_handle;
	}

	rc = fsetpos(file_handle, &pos);
	if (rc != 0) {
		goto out_fclose_file_handle;
	}

	reads = fread(info_header, sizeof(*info_header), 1, file_handle);
	if (reads != 1) {
		goto out_fclose_file_handle;
	}

	const uint32_t image_size = info_header->image_size;
	*image = calloc(image_size, sizeof(**image));
	if (*image == NULL) {
		goto out_fclose_file_handle;
	}

	reads = fread(*image, image_size * sizeof(**image), 1, file_handle);
	if (reads != 1) {
		free(*image);
		*image = NULL;
		goto out_fclose_file_handle;
	}

	ret = 0;
out_fclose_file_handle:
	fclose(file_handle);
	return ret;
}

int bmp_v4_read(const char *file, bmp_file_header *file_header, bmp_v4_header *v4_header, char **image)
{
	int ret = -1;

	FILE *file_handle = fopen(file, "r");
	if (file_handle == NULL) {
		return -1;
	}

	size_t reads = fread(file_header, sizeof(*file_header), 1, file_handle);
	if (reads != 1) {
		goto out_fclose_file_handle;
	}

	fpos_t pos;
	int rc = fgetpos(file_handle, &pos);
	if (rc != 0) {
		goto out_fclose_file_handle;
	}

	uint32_t size = 0;
	reads = fread(&size, sizeof(size), 1, file_handle);
	if (reads != 1) {
		goto out_fclose_file_handle;
	}
	if (size != BITMAPV4HEADER) {
		goto out_fclose_file_handle;
	}

	rc = fsetpos(file_handle, &pos);
	if (rc != 0) {
		goto out_fclose_file_handle;
	}

	reads = fread(v4_header, sizeof(*v4_header), 1, file_handle);
	if (reads != 1) {
		goto out_fclose_file_handle;
	}

	const uint32_t image_size = v4_header->image_size;
	*image = calloc(image_size, sizeof(**image));
	if (*image == NULL) {
		goto out_fclose_file_handle;
	}

	reads = fread(*image, image_size * sizeof(**image), 1, file_handle);
	if (reads != 1) {
		free(*image);
		*image = NULL;
		goto out_fclose_file_handle;
	}

	ret = 0;
out_fclose_file_handle:
	fclose(file_handle);
	return ret;
}

int bmp_v4_write(const bmp_pixel32 *buffer, size_t width, size_t height, const char *file)
{
	if (buffer == NULL || file == NULL) {
		return -1;
	}
	if (width > INT32_MAX || height > INT32_MAX) {
		return -1;
	}

	const size_t image_size = (width * height) * sizeof(bmp_pixel32);
	if (image_size > UINT32_MAX) {
		return -1;
	}

	const size_t offset = sizeof(bmp_file_header) + sizeof(bmp_v4_header);

	const size_t file_size = offset + image_size;
	if (file_size > UINT32_MAX) {
		return -1;
	}

	bmp_file_header file_header = {
		.file_type = FILE_TYPE,
		.file_size = (uint32_t)file_size,
		.reserved1 = 0,
		.reserved2 = 0,
		.offset = (uint32_t)offset,
	};

	bmp_v4_header v4_header = {
		.size = BITMAPV4HEADER,
		.width = (int32_t)width,
		.height = (int32_t)height,
		.planes = 1,
		.bits_per_pixel = 32,
		.compression = BI_BITFIELDS,
		.image_size = (uint32_t)image_size,
		.h_res = 0,
		.v_res = 0,
		.colors = 0,
		.imp_colors = 0,
		.r_mask = 0x00FF0000,
		.g_mask = 0x0000FF00,
		.b_mask = 0x000000FF,
		.a_mask = 0xFF000000,
		.colorspace_type = LCS_WINDOWS_COLOR_SPACE,
		.colorspace = {0, 0, 0, 0, 0, 0, 0, 0, 0},
		.r_gamma = 0,
		.g_gamma = 0,
		.b_gamma = 0,
	};

	int ret = -1;

	FILE *file_handle = fopen(file, "wb");
	if (file_handle == NULL) {
		return -1;
	}

	size_t writes = fwrite(&file_header, sizeof(bmp_file_header), 1, file_handle);
	if (writes != 1) {
		goto out_fclose_file_handle;
	}

	writes = fwrite(&v4_header, sizeof(bmp_v4_header), 1, file_handle);
	if (writes != 1) {
		goto out_fclose_file_handle;
	}

	writes = fwrite(buffer, image_size, 1, file_handle);
	if (writes != 1) {
		goto out_fclose_file_handle;
	}

	ret = 0;
out_fclose_file_handle:
	fclose(file_handle);
	return ret;
}
