#include <assert.h>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

#include "util.h"

/* Some helper macros */

#define CHECKED_INIT(buff, size) assert(util_buffer_init(buff, size) == 0)

#define CHECKED_DEINIT(buff) assert(util_buffer_deinit(buff) == 0)

#define CHECKED_PUSH(buff, item) assert(util_buffer_push(buff, item) == 0)

#define CHECKED_READ(buff, index, expected)                                    \
	do {                                                                   \
		char out;                                                      \
		assert(util_buffer_read(buff, index, &out) == 0);              \
		assert(out == (expected));                                     \
	} while (0)

#define CHECKED_SET(buff, index, item)                                         \
	assert(util_buffer_set(buff, index, item) == 0)

#define CHECK_CAP(buff, expected) assert(util_buffer_cap(buff) == expected)

#define CHECK_COUNT(buff, expected) assert(util_buffer_count(buff) == expected)

int
main(int argc, char *argv[])
{
	util_buffer_t *buffer = NULL;

	(void)argc;
	(void)argv;

#if defined(_MSC_VER) && defined(_DEBUG)
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif

	CHECKED_INIT(&buffer, 3);
	CHECKED_READ(buffer, 0, 0);
	CHECKED_READ(buffer, 1, 0);
	CHECKED_READ(buffer, 2, 0);
	CHECK_CAP(buffer, 3);
	CHECK_COUNT(buffer, 0);

	CHECKED_PUSH(buffer, 0);
	CHECKED_PUSH(buffer, 1);
	CHECKED_PUSH(buffer, 2);
	CHECKED_READ(buffer, 0, 0);
	CHECKED_READ(buffer, 1, 1);
	CHECKED_READ(buffer, 2, 2);
	CHECK_CAP(buffer, 3);
	CHECK_COUNT(buffer, 3);

	CHECKED_PUSH(buffer, 3);
	CHECKED_READ(buffer, 0, 0);
	CHECKED_READ(buffer, 1, 1);
	CHECKED_READ(buffer, 2, 2);
	CHECKED_READ(buffer, 3, 3);
	CHECK_CAP(buffer, 6);
	CHECK_COUNT(buffer, 4);

	CHECKED_SET(buffer, 3, 42);
	CHECKED_READ(buffer, 0, 0);
	CHECKED_READ(buffer, 1, 1);
	CHECKED_READ(buffer, 2, 2);
	CHECKED_READ(buffer, 3, 42);
	CHECK_CAP(buffer, 6);
	CHECK_COUNT(buffer, 4);

	CHECKED_DEINIT(&buffer);
	assert(buffer == NULL);
	CHECK_CAP(buffer, 0);
	CHECK_COUNT(buffer, 0);

	CHECKED_DEINIT(&buffer);
	CHECKED_INIT(&buffer, 3);
	CHECKED_SET(buffer, 0, 42);
	CHECKED_READ(buffer, 0, 42);
	CHECK_CAP(buffer, 3);
	CHECK_COUNT(buffer, 1);

	CHECKED_DEINIT(&buffer);
	CHECKED_INIT(&buffer, 3);
	CHECKED_SET(buffer, 2, 42);
	CHECKED_READ(buffer, 2, 42);
	CHECK_CAP(buffer, 3);
	CHECK_COUNT(buffer, 3);

	CHECKED_DEINIT(&buffer);
	CHECKED_INIT(&buffer, 3);
	CHECKED_SET(buffer, 3, 42);
	CHECKED_READ(buffer, 3, 42);
	CHECK_CAP(buffer, 6);
	CHECK_COUNT(buffer, 4);

	CHECKED_DEINIT(&buffer);
	CHECKED_INIT(&buffer, 3);
	CHECKED_SET(buffer, 5, 42);
	CHECKED_READ(buffer, 5, 42);
	CHECK_CAP(buffer, 6);
	CHECK_COUNT(buffer, 6);

	CHECKED_DEINIT(&buffer);
	CHECKED_INIT(&buffer, 3);
	CHECKED_SET(buffer, 6, 42);
	CHECKED_READ(buffer, 6, 42);
	CHECK_CAP(buffer, 12);
	CHECK_COUNT(buffer, 7);

	CHECKED_DEINIT(&buffer);
	CHECKED_INIT(&buffer, 3);
	CHECKED_SET(buffer, 11, 42);
	CHECKED_READ(buffer, 11, 42);
	CHECK_CAP(buffer, 12);
	CHECK_COUNT(buffer, 12);

	CHECKED_DEINIT(&buffer);
	CHECKED_INIT(&buffer, 3);
	CHECKED_SET(buffer, 12, 42);
	CHECKED_READ(buffer, 12, 42);
	CHECK_CAP(buffer, 24);
	CHECK_COUNT(buffer, 13);

	CHECKED_DEINIT(&buffer);
	CHECKED_INIT(&buffer, 3);
	CHECKED_SET(buffer, UINT32_MAX - 1, 42);
	CHECKED_READ(buffer, UINT32_MAX - 1, 42);
	CHECK_CAP(buffer, UINT32_MAX);
	CHECK_COUNT(buffer, UINT32_MAX);

	return 0;
}