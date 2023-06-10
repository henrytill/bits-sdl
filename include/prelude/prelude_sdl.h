#pragma once

#include <SDL.h>
#include <SDL_audio.h>

#define now SDL_GetPerformanceCounter

enum {
	APP = SDL_LOG_CATEGORY_CUSTOM,
	ERR,
};

static_assert(__builtin_types_compatible_p(Uint16, uint16_t), "SDL-defined Uint16 is not uint16_t");
static_assert(__builtin_types_compatible_p(Uint32, uint32_t), "SDL-defined Uint32 is not uint32_t");
static_assert(__builtin_types_compatible_p(Uint64, uint64_t), "SDL-defined Uint64 is not uint64_t");
static_assert(__builtin_types_compatible_p(Sint16, int16_t), "SDL-defined Sint16 is not int16_t");
static_assert(__builtin_types_compatible_p(Sint32, int32_t), "SDL-defined Sint32 is not int32_t");
static_assert(__builtin_types_compatible_p(Sint64, int64_t), "SDL-defined Sint64 is not int64_t");
static_assert(__builtin_types_compatible_p(SDL_AudioFormat, uint16_t), "SDL-defined SDL_AudioFormat is not uint16_t");
static_assert(__builtin_types_compatible_p(SDL_AudioDeviceID, uint32_t), "SDL-defined SDL_AudioDeviceID is not uint32_t");

///
/// Log a message and the contents of SDL_GetError().
///
/// @param msg The message to log
///
static inline void sdl_error(const char *msg)
{
	const char *err = SDL_GetError();
	if (strlen(err) != 0) {
		SDL_LogError(ERR, "%s (%s)", msg, err);
	} else {
		SDL_LogError(ERR, "%s", msg);
	}
}
