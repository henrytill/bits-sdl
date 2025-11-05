#ifndef SDL_BITS_INCLUDE_MACRO_H
#define SDL_BITS_INCLUDE_MACRO_H

#ifndef NDEBUG
#    define DEBUG
#endif

// General

#ifdef DEBUG
#    define debug_printf(fmt, ...) (void)printf(fmt, ##__VA_ARGS__)
#else
#    define debug_printf(fmt, ...)
#endif

#define STATIC_ASSERT(e) _Static_assert((e), #e)

// Cleanup

#define AT_EXIT(func)                                            \
    do                                                           \
    {                                                            \
        if (atexit(func) != 0)                                   \
        {                                                        \
            (void)fprintf(stderr, "atexit(%s) failed\n", #func); \
            exit(EXIT_FAILURE);                                  \
        }                                                        \
    } while (0)

#endif // SDL_BITS_INCLUDE_MACRO_H
