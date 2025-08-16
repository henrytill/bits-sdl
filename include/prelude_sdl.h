#ifndef SDL_BITS_INCLUDE_PRELUDE_SDL_H
#define SDL_BITS_INCLUDE_PRELUDE_SDL_H

#include <SDL.h>
#include <SDL_audio.h>

#define ASSERT_TYPES_COMPATIBLE(a, b) \
    _Static_assert(__builtin_types_compatible_p(a, b), #a " is not " #b);

#define COMPATIBLE_TYPES         \
    X(Uint8, uint8_t)            \
    X(Uint16, uint16_t)          \
    X(Uint32, uint32_t)          \
    X(Uint64, uint64_t)          \
    X(Sint8, int8_t)             \
    X(Sint16, int16_t)           \
    X(Sint32, int32_t)           \
    X(Sint64, int64_t)           \
    X(SDL_AudioFormat, uint16_t) \
    X(SDL_AudioDeviceID, uint32_t)

#define X(sdl_type, c_type) ASSERT_TYPES_COMPATIBLE(sdl_type, c_type);
COMPATIBLE_TYPES
#undef X

#undef COMPATIBLE_TYPES
#undef ASSERT_TYPES_COMPATIBLE

#define now SDL_GetPerformanceCounter

enum {
    APP = SDL_LOG_CATEGORY_CUSTOM,
    ERR,
};

/// Log a message and the contents of SDL_GetError().
///
/// @param msg The message to log
static inline void log_sdl_error(char const *msg) {
    char const *err = SDL_GetError();
    if (strlen(err) != 0) {
        SDL_LogError(ERR, "%s (%s)", msg, err);
    } else {
        SDL_LogError(ERR, "%s", msg);
    }
}

#endif // SDL_BITS_INCLUDE_PRELUDE_SDL_H
