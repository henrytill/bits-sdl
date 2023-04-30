#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>
#include <SDL_audio.h>

#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

#define now SDL_GetPerformanceCounter

enum {
  APP = SDL_LOG_CATEGORY_CUSTOM,
  ERR,
};

enum {
  CENTERED = SDL_WINDOWPOS_CENTERED,
};

struct Args {
  char *cfgfile;
};

struct Config {
  enum WType {
    WINDOWED = 0,
    FULLSCREEN = 1,
    BORDERLESS = 2,
  } wtype;
  int x;
  int y;
  int width;
  int height;
  int framerate;
  char *assetdir;
};

struct AudioState {
  const int samplerate;
  const uint16_t buffsize;
  const double volume;
  const double frequency;
  uint64_t offset;
};

struct State {
  SDL_AudioDeviceID audiodev;
  struct AudioState audio;
  int loopstat;
  int pauseaudio;
};

struct Window {
  SDL_Window *window;
  SDL_Renderer *renderer;
};

static const double second = 1000.0;

static const uint32_t wtypeflags[] = {
  [WINDOWED] = SDL_WINDOW_SHOWN,
  [FULLSCREEN] = SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN,
  [BORDERLESS] = SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP,
};

static const char *const wtypestr[] = {
  [WINDOWED] = "Windowed",
  [FULLSCREEN] = "Fullscreen",
  [BORDERLESS] = "Borderless Fullscreen",
};

static uint64_t pfreq = 0;

static struct Args args = {.cfgfile = "config.lua"};

static struct Config config = {
  .wtype = WINDOWED,
  .x = CENTERED,
  .y = CENTERED,
  .width = 1280,
  .height = 720,
  .framerate = 60,
  .assetdir = "./assets",
};

static struct State state = {
  .audiodev = 0,
  .audio = {
    .samplerate = 48000,
    .buffsize = 2048,
    .volume = 0.25,
    .frequency = 440.0,
    .offset = 0,
  },
  .loopstat = 1,
  .pauseaudio = 1,
};

/**
 * Log a msg and the contents of SDL_GetError().
 *
 * @param msg The message to log
 */
static void logsdlerr(char *msg) {
  const char *err = SDL_GetError();
  if (strlen(err) != 0)
    SDL_LogError(ERR, "%s (%s)", msg, err);
  else
    SDL_LogError(ERR, "%s", msg);
}

/**
 * Parse command line arguments and populate an Args struct with the results.
 *
 * @param argc The number of arguments
 * @param argv The arguments
 * @param args The Args struct to populate
 */
static void parseargs(int argc, char *argv[], struct Args *args) {
  for (int i = 0; i < argc;) {
    char *arg = argv[i++];
    if (strcmp(arg, "-c") == 0)
      args->cfgfile = argv[i++];
  }
}

/**
 * Join two paths together.  User is responsible for freeing the returned path.
 *
 * @param a The first path
 * @param b The second path
 * @return A new path, or NULL on failure
 */
static char *joinpath(const char *a, const char *b) {
  size_t n = (size_t)snprintf(NULL, 0, "%s/%s", a, b);
  char *ret = calloc(++n, sizeof(char)); /* incr for terminator */
  if (ret != NULL) snprintf(ret, n, "%s/%s", a, b);
  return ret;
}

/**
 * Load and parse a config file and populate a Config struct with the results.
 *
 * @param f The config file to load
 * @param cfg The Config struct to populate
 * @return 0 on success, -1 on failure
 */
static int loadcfg(const char *f, struct Config *cfg) {
  int ret = -1;

  lua_State *state = luaL_newstate();
  if (state == NULL) {
    SDL_LogError(ERR, "%s: luaL_newstate failed", __func__);
    return ret;
  }
  luaL_openlibs(state);
  if (luaL_loadfile(state, f) || lua_pcall(state, 0, 0, 0) != LUA_OK) {
    SDL_LogError(ERR, "%s: failed to load %s, %s", __func__,
                 f, lua_tostring(state, -1));
    goto out;
  }
  lua_getglobal(state, "width");
  lua_getglobal(state, "height");
  lua_getglobal(state, "framerate");
  if (!lua_isnumber(state, -3)) {
    SDL_LogError(ERR, "%s: width is not a number", __func__);
    goto out;
  }
  if (!lua_isnumber(state, -2)) {
    SDL_LogError(ERR, "%s: height is not a number", __func__);
    goto out;
  }
  if (!lua_isnumber(state, -1)) {
    SDL_LogError(ERR, "%s: framerate is not a number", __func__);
    goto out;
  }
  cfg->width = (int)lua_tonumber(state, -3);
  cfg->height = (int)lua_tonumber(state, -2);
  cfg->framerate = (int)lua_tonumber(state, -1);
  ret = 0;
out:
  lua_close(state);
  return ret;
}

/**
 * Calculate a sine wave and write it to the stream.
 *
 * @param userdata The userdata passed to SDL_OpenAudioDevice
 * @param stream The stream to write to
 * @param len The length of the stream
 */
static void calcsine(void *userdata, uint8_t *stream, int len) {
  struct AudioState *as = (struct AudioState *)userdata;
  float *fstream = (float *)stream;

  assert((len / (4 * 2)) == as->buffsize);
  const double samplerate = (double)as->samplerate;
  const uint64_t buffsize = (uint64_t)as->buffsize;

  for (uint64_t i = 0; i < buffsize; ++i) {
    const double time = (double)((as->offset * buffsize) + i) / samplerate;
    const double x = 2.0 * M_PI * time * as->frequency;
    fstream[2 * i + 0] = (float)(as->volume * sin(x));
    fstream[2 * i + 1] = (float)(as->volume * sin(x));
  }
  as->offset += 1;
}

/**
 * Calculate the time in milliseconds for a frame.
 *
 * @param fps The framerate
 * @return The time in milliseconds for a frame
 */
static double calcframetime(int fps) {
  extern const double second;
  assert(fps > 0);
  return second / (double)fps;
}

/**
 * Calculate the time in milliseconds between two timestamps.
 *
 * @param begin An initial timestamp in ticks
 * @param end A final timestamp in ticks
 * @return The time in milliseconds between the two timestamps
 */
static double calcdelta(uint64_t begin, uint64_t end) {
  extern const double second;
  extern uint64_t pfreq;
  assert(pfreq > 0);
  assert(begin <= end);
  const double delta_ticks = (double)(end - begin);
  return (delta_ticks * second) / (double)pfreq;
}

/**
 * Wait until the current frame end before returning.
 *
 * @param frametime The desired time in milliseconds for a frame
 * @param begin The timestamp in ticks when the frame started
 */
static void delay(double frametime, uint64_t begin) {
  if (calcdelta(begin, now()) >= frametime) return;
  const uint32_t delay = (uint32_t)(frametime - calcdelta(begin, now()) - 1.0);
  if (delay > 0) SDL_Delay(delay);
  while (calcdelta(begin, now()) < frametime) continue;
}

/**
 * Create a window and renderer.
 *
 * @param cfg The configuration.
 * @param title The window title.
 * @param win The window to initialize.
 * @return 0 on success, -1 on failure.
 */
static int initwin(struct Config *cfg, const char *title, struct Window *win) {
  extern const char *const wtypestr[];
  extern const uint32_t wtypeflags[];
  SDL_LogInfo(APP, "Window type: %s", wtypestr[cfg->wtype]);
  win->window = SDL_CreateWindow(title,
                                 cfg->x, cfg->y,
                                 cfg->width, cfg->height,
                                 wtypeflags[cfg->wtype]);
  if (win->window == NULL) {
    logsdlerr("SDL_CreateWindow failed");
    return -1;
  }
  win->renderer = SDL_CreateRenderer(win->window, -1, SDL_RENDERER_ACCELERATED);
  if (win->renderer == NULL) {
    logsdlerr("SDL_CreateRenderer failed");
    return -1;
  }
  SDL_SetRenderDrawColor(win->renderer, 0x00, 0x00, 0x00, 0xFF);
  return 0;
}

/**
 * Destroy a window and renderer.
 *
 * @param win The window to destroy.
 */
static void finishwin(struct Window *win) {
  if (win == NULL) return;
  if (win->renderer != NULL) SDL_DestroyRenderer(win->renderer);
  if (win->window != NULL) SDL_DestroyWindow(win->window);
}

/**
 * Get the window's rectangle.
 *
 * @param win The window.
 * @param rect The rectangle to initialize.
 * @return 0 on success, -1 on failure.
 */
static int getrect(struct Window *win, SDL_Rect *rect) {
  if (win == NULL || win->renderer == NULL)
    return -1;
  const int rc = SDL_GetRendererOutputSize(win->renderer, &rect->w, &rect->h);
  if (rc != 0) {
    logsdlerr("SDL_GetRendererOutputSize failed");
    return -1;
  }
  return 0;
}

/**
 * Handle keydown events.
 *
 * @param key The keydown event.
 * @param state The state.
 */
static void keydown(SDL_KeyboardEvent *key, struct State *state) {
  switch (key->keysym.sym) {
  case SDLK_ESCAPE:
    state->loopstat = 0;
    break;
  case SDLK_F1:
    state->pauseaudio = (state->pauseaudio == 1) ? 0 : 1;
    SDL_LogInfo(APP, "PauseAudioDevice(%d)", state->pauseaudio);
    SDL_PauseAudioDevice(state->audiodev, state->pauseaudio);
    if (state->pauseaudio == 1) {
      state->audio.offset = 0;
    }
    break;
  }
}

static void update(double delta) {
  (void)delta;
}

int main(int argc, char *argv[]) {
  extern uint64_t pfreq;
  extern struct Args args;
  extern struct Config config;
  extern struct State state;

  int ret = EXIT_FAILURE;
  int rc;
  struct Window win = {NULL, NULL};
  SDL_AudioSpec want, have;
  SDL_Rect winrect = {0, 0, 0, 0};
  SDL_Surface *surface = NULL;
  SDL_Texture *texture = NULL;
  SDL_Event ev;
  const char *const wintitle = "Hello, world!";
  const char *const testbmp = "test.bmp";
  char *bmpfile;
  uint64_t begin, end;
  double delta;

  (void)argc;
  (void)argv;

  SDL_LogSetAllPriority(SDL_LOG_PRIORITY_DEBUG);
  parseargs(argc, argv, &args);
  loadcfg(args.cfgfile, &config);

  rc = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  if (rc != 0) {
    logsdlerr("SDL_Init failed");
    return EXIT_FAILURE;
  }

  pfreq = SDL_GetPerformanceFrequency();

  want.freq = state.audio.samplerate;
  want.format = AUDIO_F32;
  want.channels = 2;
  want.samples = state.audio.buffsize;
  want.callback = calcsine;
  want.userdata = (void *)&state.audio;
  state.audiodev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
  if (state.audiodev < 2) {
    logsdlerr("SDL_OpenAudio failed");
    goto out0;
  }

  rc = initwin(&config, wintitle, &win);
  if (rc != 0)
    goto out1;

  rc = getrect(&win, &winrect);
  if (rc != 0)
    goto out2;

  /* create texture from testbmp */
  {
    bmpfile = joinpath(config.assetdir, testbmp);
    if (bmpfile == NULL)
      goto out2;

    surface = SDL_LoadBMP(bmpfile);
    if (surface == NULL) {
      logsdlerr("SDL_LoadBMP failed");
      free(bmpfile);
      goto out2;
    }

    texture = SDL_CreateTextureFromSurface(win.renderer, surface);
    if (texture == NULL) {
      logsdlerr("SDL_CreateTextureFromSurface failed");
      SDL_FreeSurface(surface);
      free(bmpfile);
      goto out2;
    }
    SDL_FreeSurface(surface);
    free(bmpfile);
    surface = NULL;
    bmpfile = NULL;
  }

  const double frametime = calcframetime(config.framerate);

  delta = frametime;
  begin = now();
  while (state.loopstat == 1) {
    while (SDL_PollEvent(&ev) != 0) {
      switch (ev.type) {
      case SDL_QUIT:
        state.loopstat = 0;
        break;
      case SDL_KEYDOWN:
        keydown(&ev.key, &state);
        break;
      }
    }

    update(delta);

    rc = SDL_RenderClear(win.renderer);
    if (rc != 0) {
      logsdlerr("SDL_RenderClear failed");
      goto out3;
    }
    rc = SDL_RenderCopy(win.renderer, texture, NULL, &winrect);
    if (rc != 0) {
      logsdlerr("SDL_RenderCopy failed");
      goto out3;
    }
    SDL_RenderPresent(win.renderer);

    delay(frametime, begin);
    end = now();
    delta = calcdelta(begin, end);
    begin = end;
  }

  ret = EXIT_SUCCESS;
out3:
  SDL_DestroyTexture(texture);
out2:
  finishwin(&win);
out1:
  SDL_CloseAudioDevice(state.audiodev);
out0:
  SDL_Quit();
  return ret;
}
