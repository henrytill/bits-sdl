#include <stdlib.h>

#include "bmp.h"

enum {
    WIDTH = 4,
    HEIGHT = 2,
};

static const bmp_pixel32 BUF[] = {
    {0xFF, 0x00, 0x00, 0x7F},
    {0x00, 0xFF, 0x00, 0x7F},
    {0x00, 0x00, 0xFF, 0x7F},
    {0xFF, 0xFF, 0xFF, 0x7F},
    {0xFF, 0x00, 0x00, 0xFF},
    {0x00, 0xFF, 0x00, 0xFF},
    {0x00, 0x00, 0xFF, 0xFF},
    {0xFF, 0xFF, 0xFF, 0xFF},
};

int main(int argc, char *argv[])
{
    extern const bmp_pixel32 BUF[];

    if (argc != 2) {
        return EXIT_FAILURE;
    }

    const char *bmp_file = argv[1];

    return (bmp_v4_write(BUF, WIDTH, HEIGHT, bmp_file) == 0)
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
