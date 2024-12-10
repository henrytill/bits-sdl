#include <stdlib.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <SDL.h>
#include <lauxlib.h>
#include <lua.h>

enum {
  APP = SDL_LOG_CATEGORY_CUSTOM,
};

static void log_freetype_version(void) {
  FT_Library lib = NULL;
  int rc = FT_Init_FreeType(&lib);
  if (rc != 0) {
    SDL_LogError(APP, "Failed to initialize FreeType");
    return;
  }
  FT_Int major = 0;
  FT_Int minor = 0;
  FT_Int patch = 0;
  FT_Library_Version(lib, &major, &minor, &patch);
  SDL_LogInfo(APP, "Linking against FreeType %u.%u.%u",
              major, minor, patch);
  FT_Done_FreeType(lib);
}

static void log_lua_version(void) {
  lua_State *state = luaL_newstate();
  if (state == NULL) {
    SDL_LogError(APP, "Failed to initialize Lua");
    return;
  }
  SDL_LogInfo(APP, "Compiled against %s ...", LUA_RELEASE);
  lua_close(state);
}

static void log_sdl_version(void) {
  SDL_version compiled = {0};
  SDL_version linked = {0};
  SDL_VERSION(&compiled);
  SDL_GetVersion(&linked);
  SDL_LogInfo(APP, "Compiled against SDL %u.%u.%u ...",
              compiled.major, compiled.minor, compiled.patch);
  SDL_LogInfo(APP, "... and linking against SDL %u.%u.%u.",
              linked.major, linked.minor, linked.patch);
}

int main(__attribute__((unused)) int argc, __attribute__((unused)) char *argv[]) {
  SDL_LogSetAllPriority(SDL_LOG_PRIORITY_DEBUG);
  log_freetype_version();
  log_lua_version();
  log_sdl_version();
  return EXIT_SUCCESS;
}
