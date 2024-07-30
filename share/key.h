#ifndef KEY_H
#define KEY_H 1

#if !defined(__GAMECUBE__) && !defined(__WII__)
#if _WIN32 && __MINGW32__
#include <SDL2/SDL_keycode.h>
#elif _WIN32 && _MSC_VER
#include <SDL_keycode.h>
#elif _WIN32
#error Security compilation error: No target include file in path for Windows specified!
#else
#include <SDL_keycode.h>
#endif
#else
#include <SDL_keysym.h>
#endif

/* Names for some hard-coded keys. */

#define KEY_EXIT       SDLK_ESCAPE

#define KEY_TOGGLESHOWHUD SDLK_F1

#define KEY_LOOKAROUND SDLK_F5
#define KEY_WIREFRAME  SDLK_F6
#define KEY_RESOURCES  SDLK_F7
#define KEY_LEVELSHOTS SDLK_F8

#define KEY_FPS        SDLK_F9
#define KEY_POSE       SDLK_F10
#define KEY_FULLSCREEN SDLK_F11
#define KEY_SCREENSHOT SDLK_F12

#define KEY_PUTT_UPGRADE SDLK_PAGEUP
#define KEY_PUTT_DNGRADE SDLK_PAGEDOWN

#endif
