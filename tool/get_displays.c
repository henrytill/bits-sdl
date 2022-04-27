#include <SDL.h>

enum {
    SUCCESS = 0,
    FAILURE = 1,
};

int main(int argc, char *argv[]) {
    SDL_DisplayMode display_mode;
    int error = FAILURE;

    (void)argc;
    (void)argv;

    SDL_Init(SDL_INIT_VIDEO);

    int num_displays = SDL_GetNumVideoDisplays();
    if (num_displays < 0) {
        const char *error_msg = SDL_GetError();
        SDL_Log("Failed to get number of video displays: %s", error_msg);
        error = FAILURE;
        goto out;
    }

    for (int i = 0; i < num_displays; ++i) {
        error = SDL_GetCurrentDisplayMode(i, &display_mode);
        if (error != SUCCESS) {
            const char *error_msg = SDL_GetError();
            SDL_Log("Failed to get display mode for display #%d: %s", i, error_msg);
            goto out;
        } else {
            SDL_Log("Display #%d: display mode is %dx%d @ %d hz.",
                    i,
                    display_mode.w,
                    display_mode.h,
                    display_mode.refresh_rate);
        }
    }

out:
    SDL_Quit();
    return error;
}