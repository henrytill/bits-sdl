#include <SDL.h>

#include "macro.h"

static int log_current_mode(int i, SDL_DisplayMode *mode) {
  int const rc = SDL_GetCurrentDisplayMode(i, mode);
  if (rc != 0) {
    char const *err = SDL_GetError();
    SDL_Log("Failed to get mode for display #%d: %s", i, err);
    return -1;
  }
  SDL_Log("Display #%d: mode is %dx%d @ %d hz", i, mode->w, mode->h, mode->refresh_rate);
  return 0;
}

static int log_display_modes(void) {
  int const num_displays = SDL_GetNumVideoDisplays();
  if (num_displays < 0) {
    char const *err = SDL_GetError();
    SDL_Log("Failed to get number of video displays: %s", err);
    return -1;
  }
  int rc = -1;
  SDL_DisplayMode mode = {0};
  for (int i = 0; i < num_displays; ++i) {
    rc = log_current_mode(i, &mode);
    if (rc != 0)
      return -1;
  }
  return 0;
}

int main(__attribute__((unused)) int argc, __attribute__((unused)) char *argv[]) {
  int rc = SDL_Init(SDL_INIT_VIDEO);
  if (rc != 0)
    return EXIT_FAILURE;

  AT_EXIT(SDL_Quit);

  rc = log_display_modes();
  if (rc != 0)
    return EXIT_FAILURE;

  return 0;
}
