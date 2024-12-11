#include <assert.h>
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "macro.h"
#include "message_queue.h"
#include "prelude_sdl.h"
#include "prelude_stdlib.h"

enum {
  AUDIO_NUM_CHANNELS = 2,
  CENTERED = SDL_WINDOWPOS_CENTERED,
};

enum events {
  EVENT_0 = SDL_USEREVENT,
  EVENT_MAX,
};

struct args {
  char *config_file;
};

#define WINDOW_TYPE_VARIANTS                                               \
  X(WINDOWED, 0, SDL_WINDOW_SHOWN, "Windowed")                             \
  X(FULLSCREEN, 1, SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN, "Fullscreen") \
  X(BORDERLESS, 2, SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP, "Borderless Fullscreen")

enum {
#define X(variant, i, flags, str) variant = (i),
  WINDOW_TYPE_VARIANTS
#undef X
};

static const uint32_t WINDOW_TYPE_FLAGS[] = {
#define X(variant, i, flags, str) [variant] = (flags),
  WINDOW_TYPE_VARIANTS
#undef X
};

static const char *const WINDOW_TYPE_STR[] = {
#define X(variant, i, flags, str) [variant] = (str),
  WINDOW_TYPE_VARIANTS
#undef X
};

struct config {
  int window_type;
  int x;
  int y;
  int width;
  int height;
  int frame_rate;
  char *asset_dir;
};

struct audio_state {
  const int sample_rate;      // Samples per second
  const uint16_t buffer_size; // Samples per buffer
  const double frequency;     // Frequency of the sine wave
  const double max_volume;    // Maximum volume
  double volume;              // Current volume, 0.0 to max_volume
  uint64_t elapsed;           // Number of buffer fills
};

struct state {
  SDL_AudioDeviceID audio_device;
  struct audio_state audio;
  int loop_stat;
  int tone_stat;
};

struct window {
  SDL_Window *window;
  SDL_Renderer *renderer;
};

static const double SECOND = 1000.0;

static const uint32_t QUEUE_CAP = 4U;

static uint64_t perf_freq = 0;

static struct args as = {.config_file = "config.lua"};

static struct config cfg = {
  .window_type = WINDOWED,
  .x = CENTERED,
  .y = CENTERED,
  .width = 1280,
  .height = 720,
  .frame_rate = 60,
  .asset_dir = "./assets",
};

static struct state st = {
  .audio_device = 0,
  .audio = {
    .sample_rate = 48000,
    .buffer_size = 2048,
    .frequency = 440.0,
    .max_volume = 0.25,
    .volume = 0.0,
    .elapsed = 0,
  },
  .loop_stat = 1,
  .tone_stat = 0,
};

/// Parses command line arguments and populates args with the results.
///
/// @param argc The number of arguments
/// @param argv The arguments
/// @param as The args struct to populate
static int parse_args(int argc, char *argv[], struct args *as) {
  char *arg = NULL;
  for (int i = 0; i < argc;) {
    arg = argv[i++];
    if (strcmp(arg, "-c") == 0 || strcmp(arg, "--config") == 0) {
      if (i + 1 >= argc) {
        return -1;
      }
      as->config_file = argv[i++];
    }
  }
  return 0;
}

/// Joins two paths together.  Caller is responsible for freeing the returned object.
///
/// @param a The first path
/// @param b The second path
/// @return A new path, or NULL on failure
static char *joinpath2(const char *a, const char *b) {
  const char separator = '/';
  size_t len = (size_t)snprintf(NULL, 0, "%s%c%s", a, separator, b);
  char *ret = ecalloc(++len, sizeof(*ret)); // incr for terminator
  (void)snprintf(ret, len, "%s%c%s", a, separator, b);
  return ret;
}

/// Loads and parses a config file and populate config with the results.
///
/// @param file The config file to load
/// @param cfg The config struct to populate
/// @return 0 on success, -1 on failure
static int load_config(const char *file, struct config *cfg) {
  int ret = -1;
  lua_State *state = luaL_newstate();
  if (state == NULL) {
    SDL_LogError(ERR, "%s: luaL_newstate failed", __func__);
    return -1;
  }
  luaL_openlibs(state);
  if (luaL_loadfile(state, file) || lua_pcall(state, 0, 0, 0) != 0) {
    SDL_LogError(ERR, "%s: failed to load %s, %s", __func__,
                 file, lua_tostring(state, -1));
    goto out_close_state;
  }
  lua_getglobal(state, "width");
  lua_getglobal(state, "height");
  lua_getglobal(state, "framerate");
  if (!lua_isnumber(state, -3)) {
    SDL_LogError(ERR, "%s: width is not a number", __func__);
    goto out_close_state;
  }
  if (!lua_isnumber(state, -2)) {
    SDL_LogError(ERR, "%s: height is not a number", __func__);
    goto out_close_state;
  }
  if (!lua_isnumber(state, -1)) {
    SDL_LogError(ERR, "%s: framerate is not a number", __func__);
    goto out_close_state;
  }
  cfg->width = (int)lua_tonumber(state, -3);
  cfg->height = (int)lua_tonumber(state, -2);
  cfg->frame_rate = (int)lua_tonumber(state, -1);
  ret = 0;
out_close_state:
  lua_close(state);
  return ret;
}

/// Calculates a sine wave and write it to the stream.
///
/// @param userdata The userdata passed to SDL_OpenAudioDevice
/// @param stream The stream to write to
/// @param len The length of the stream
static void calc_sine(void *userdata, uint8_t *stream, int len) {
  struct audio_state *as = userdata;
  float *fstream = (float *)stream;

  _Static_assert(sizeof(*fstream) == 4, "sizeof(*fstream) != 4");
  _Static_assert(AUDIO_NUM_CHANNELS != 0, "AUDIO_NUM_CHANNELS == 0");
  assert((len / ((int)sizeof(*fstream) * AUDIO_NUM_CHANNELS)) == as->buffer_size);
  (void)len;

  const double sample_rate = (double)as->sample_rate;
  const uint64_t buffer_size = (uint64_t)as->buffer_size;
  const uint64_t offset = as->elapsed * buffer_size;

  for (uint64_t i = 0; i < buffer_size; ++i) {
    const double time = (double)(offset + i) / sample_rate;
    const double x = 2.0 * M_PI * time * as->frequency;
    const double y = as->volume * sin(x);
    fstream[(AUDIO_NUM_CHANNELS * i) + 0] = (float)y;
    fstream[(AUDIO_NUM_CHANNELS * i) + 1] = (float)y;
  }
  as->elapsed += 1;
}

/// Calculates the time in milliseconds for a frame.
///
/// @param frame_rate The frame rate
/// @return The time in milliseconds for a frame
static double calc_frame_time(const int frame_rate) {
  assert((double)frame_rate > 0);
  return SECOND / (double)frame_rate;
}

/// Calculates the time in milliseconds between two timestamps.
///
/// @param begin An initial timestamp in ticks
/// @param end A final timestamp in ticks
/// @return The time in milliseconds between the two timestamps
static double calc_delta(const uint64_t begin, const uint64_t end) {
  assert(begin <= end);
  assert((double)perf_freq > 0);
  const double delta_ticks = (double)(end - begin);
  return (delta_ticks * SECOND) / (double)perf_freq;
}

/// Waits until the current frame end before returning.
///
/// @param frame_time The desired time in milliseconds for a frame
/// @param begin The timestamp in ticks when the frame started
static void delay_frame(const double frame_time, const uint64_t begin) {
  assert(frame_time > 0);
  if (calc_delta(begin, now()) >= frame_time) {
    return;
  }
  const uint32_t time = (uint32_t)(frame_time - calc_delta(begin, now()) - 1.0);
  if (time > 0) {
    SDL_Delay(time);
  }
  while (calc_delta(begin, now()) < frame_time) {}
}

/// Initializes a window and renderer.
///
/// @param cfg The configuration.
/// @param title The window title.
/// @param win The window to initialize.
/// @return 0 on success, -1 on failure.
static int window_init(struct config *cfg, const char *title, struct window *win) {
  SDL_LogInfo(APP, "Window type: %s", WINDOW_TYPE_STR[cfg->window_type]);
  win->window = SDL_CreateWindow(title,
                                 cfg->x, cfg->y,
                                 cfg->width, cfg->height,
                                 WINDOW_TYPE_FLAGS[cfg->window_type]);
  if (win->window == NULL) {
    log_sdl_error("SDL_CreateWindow failed");
    return -1;
  }
  win->renderer = SDL_CreateRenderer(win->window, -1, SDL_RENDERER_ACCELERATED);
  if (win->renderer == NULL) {
    SDL_DestroyWindow(win->window);
    log_sdl_error("SDL_CreateRenderer failed");
    return -1;
  }
  const int rc = SDL_SetRenderDrawColor(win->renderer, 0x00, 0x00, 0x00, 0xFF);
  if (rc != 0) {
    SDL_DestroyWindow(win->window);
    SDL_DestroyRenderer(win->renderer);
    log_sdl_error("SDL_SetRenderDrawColor failed");
    return -1;
  }
  return 0;
}

/// De-initializes a window and renderer.
///
/// @param win The window to destroy.
static void window_finish(struct window *win) {
  if (win == NULL) {
    return;
  }
  if (win->renderer != NULL) {
    SDL_DestroyRenderer(win->renderer);
  }
  if (win->window != NULL) {
    SDL_DestroyWindow(win->window);
  }
}

/// Creates a window and renderer.
///
/// @param cfg The configuration.
/// @param title The window title.
/// @return The window on success, NULL on failure.
static struct window *window_create(struct config *cfg, const char *title) {
  struct window *win = emalloc(sizeof(*win));
  const int rc = window_init(cfg, title, win);
  if (rc != 0) {
    free(win);
    return NULL;
  }
  return win;
}

/// Destroys a window and renderer.
///
/// @param win The window to destroy.
static void window_destroy(struct window *win) {
  if (win == NULL) {
    return;
  }
  window_finish(win);
  free(win);
}

/// Gets the window's rectangle.
///
/// @param win The window.
/// @param rect The rectangle to initialize.
/// @return 0 on success, -1 on failure.
static int get_rect(struct window *win, SDL_Rect *rect) {
  if (win == NULL || win->renderer == NULL) {
    return -1;
  }
  const int rc = SDL_GetRendererOutputSize(win->renderer, &rect->w, &rect->h);
  if (rc != 0) {
    log_sdl_error("SDL_GetRendererOutputSize failed");
    return -1;
  }
  return 0;
}

/// Creates a texture from a bitmap file.
///
/// @param win The window.
/// @param path The path to the bitmap file.
/// @return The texture on success, NULL on failure.
static SDL_Texture *create_texture(struct window *win, const char *path) {
  SDL_Surface *surface = SDL_LoadBMP(path);
  if (surface == NULL) {
    log_sdl_error("SDL_LoadBMP failed");
    return NULL;
  }
  SDL_Texture *texture = SDL_CreateTextureFromSurface(win->renderer, surface);
  SDL_FreeSurface(surface);
  if (texture == NULL) {
    log_sdl_error("SDL_CreateTextureFromSurface failed");
    return NULL;
  }
  return texture;
}

/// Handles events.
///
/// @param data The data passed to the thread.
/// @return 0 on success, -1 on failure.
static int handle(void *data) {
  struct message_queue *queue = data;
  (void)queue;

  SDL_Event event = {
    .user = {
      .type = EVENT_0,
      .code = 0,
      .data1 = NULL,
      .data2 = NULL,
    },
  };
  const int rc = SDL_PushEvent(&event);
  if (rc == 0) {
    SDL_LogDebug(APP, "SDL_PushEvent filtered");
  } else if (rc < 0) {
    log_sdl_error("SDL_PushEvent failed");
    return -1;
  }
  return 0;
}

/// Handles keydown events.
///
/// @param key The keydown event.
/// @param st The state.
static void handle_keydown(SDL_KeyboardEvent *key, struct state *st) {
  switch (key->keysym.sym) {
  case SDLK_ESCAPE:
    st->loop_stat = 0;
    break;
  case SDLK_F1:
    st->tone_stat = (st->tone_stat == 1) ? 0 : 1;
    SDL_LockAudioDevice(st->audio_device);
    st->audio.volume = st->tone_stat * st->audio.max_volume;
    st->audio.elapsed = 0;
    SDL_UnlockAudioDevice(st->audio_device);
    break;
  default:
    break;
  }
}

/// Handles user events.
///
/// @param event The user event.
/// @param st The state.
static void handle_user(SDL_UserEvent *event, __attribute__((unused)) struct state *st) {
  SDL_LogDebug(APP, "EVENT_0: %d", event->timestamp);
}

/// Handles SDL events.
///
/// @param st The state.
static void handle_events(struct state *st) {
  SDL_Event event = {0};
  while (SDL_PollEvent(&event) != 0) {
    switch (event.type) {
    case SDL_QUIT:
      st->loop_stat = 0;
      break;
    case SDL_KEYDOWN:
      handle_keydown(&event.key, st);
      break;
    case EVENT_0:
      handle_user(&event.user, st);
      break;
    default:
      break;
    }
  }
}

static void update(__attribute__((unused)) double delta) {}

/// Renders the texture to the window.
///
/// @param renderer The renderer
/// @param texture The texture
/// @param win_rect The window rectangle
/// @return 0 on success, -1 on failure.
static int render(SDL_Renderer *renderer, SDL_Texture *texture, SDL_Rect *win_rect) {
  int rc = SDL_RenderClear(renderer);
  if (rc != 0) {
    log_sdl_error("SDL_RenderClear failed");
    return -1;
  }
  rc = SDL_RenderCopy(renderer, texture, NULL, win_rect);
  if (rc != 0) {
    log_sdl_error("SDL_RenderCopy failed");
    return -1;
  }
  SDL_RenderPresent(renderer);
  return 0;
}

int init(void) {
  int rc = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  if (rc != 0) {
    log_sdl_error("init failed");
    return -1;
  }

  AT_EXIT(SDL_Quit);

  perf_freq = SDL_GetPerformanceFrequency();

  const uint32_t event_start = SDL_RegisterEvents(EVENT_MAX - EVENT_0);
  if (event_start == (uint32_t)-1) {
    log_sdl_error("SDL_RegisterEvents failed");
    return -1;
  }
  assert(event_start == EVENT_0);

  SDL_AudioSpec want = {
    .freq = st.audio.sample_rate,
    .format = AUDIO_F32,
    .channels = 2,
    .samples = st.audio.buffer_size,
    .callback = calc_sine,
    .userdata = (void *)&st.audio,
  };
  SDL_AudioSpec have = {0};
  st.audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
  if (st.audio_device < 2) {
    log_sdl_error("SDL_OpenAudio failed");
    return -1;
  }

  SDL_PauseAudioDevice(st.audio_device, 0);

  return 0;
}

int main(int argc, char *argv[]) {
  int ret = EXIT_FAILURE;

  SDL_LogSetAllPriority(SDL_LOG_PRIORITY_DEBUG);
  (void)parse_args(argc, argv, &as);
  (void)load_config(as.config_file, &cfg);

  int rc = init();
  if (rc != 0) {
    return EXIT_FAILURE;
  }

  const char *const win_title = "Hello, world!";
  struct window *win = window_create(&cfg, win_title);
  if (win == NULL) {
    goto out_close_audio_device;
  }

  SDL_Rect win_rect = {0};
  rc = get_rect(win, &win_rect);
  if (rc != 0) {
    goto out_destroy_window;
  }

  const char *const test_bmp = "test.bmp";
  char *bmp_file = joinpath2(cfg.asset_dir, test_bmp);
  if (bmp_file == NULL) {
    goto out_destroy_window;
  }

  SDL_Texture *texture = create_texture(win, bmp_file);
  free(bmp_file);
  if (texture == NULL) {
    goto out_destroy_window;
  }

  struct message_queue *queue = message_queue_create(QUEUE_CAP);
  if (queue == NULL) {
    goto out_destroy_texture;
  }

  SDL_Thread *handler = SDL_CreateThread(handle, "handler", queue);
  if (handler == NULL) {
    goto out_message_queue_destroy;
  }

  const double frame_time = calc_frame_time(cfg.frame_rate);

  double delta = frame_time;
  uint64_t begin = now();
  uint64_t end = 0;

  while (st.loop_stat == 1) {
    handle_events(&st);

    update(delta);

    rc = render(win->renderer, texture, &win_rect);
    if (rc != 0) {
      goto out_wait_thread;
    }

    delay_frame(frame_time, begin);
    end = now();
    delta = calc_delta(begin, end);
    begin = end;
  }

  SDL_PauseAudioDevice(st.audio_device, 1);

  ret = EXIT_SUCCESS;
out_wait_thread:
  SDL_WaitThread(handler, NULL);
out_message_queue_destroy:
  message_queue_destroy(queue);
out_destroy_texture:
  SDL_DestroyTexture(texture);
out_destroy_window:
  window_destroy(win);
out_close_audio_device:
  SDL_CloseAudioDevice(st.audio_device);
  return ret;
}
