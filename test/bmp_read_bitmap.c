/// Test for bmp_read() function.
///
/// This test reads a 24-bit bitmap file and checks that the first pixel is black.
///
/// @see bmp_read()
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "bmp.h"

int main(int argc, char *argv[]) {
    bmp_file_header file_header = {0};
    bmp_info_header info_header = {0};
    char *image = NULL;

    if (argc != 2) {
        return EXIT_FAILURE;
    }

    char const *bmp_file = argv[1];

    if (bmp_read(bmp_file, &file_header, &info_header, &image) != 0) {
        return EXIT_FAILURE;
    }

    bmp_pixel24 expected = {
        .b = 0,
        .g = 0,
        .r = 255,
    };

    if (memcmp(&expected, (bmp_pixel24 *)image, sizeof(bmp_pixel24)) != 0) {
        free(image);
        return EXIT_FAILURE;
    }

    free(image);
    return EXIT_SUCCESS;
}
