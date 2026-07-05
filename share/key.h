/*
 * Copyright (C) 2026 Microsoft / Neverball authors / Jānis Rūcis
 *
 * NEVERBALL is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */

#ifndef KEY_H
#define KEY_H 1

#if !defined(__GAMECUBE__) && !defined(__WII__)
#if NB_HAVE_PB_BOTH==1 && NB_PB_SDL3==1
#include <SDL3/SDL_keycode.h>
#elif _WIN32 && __MINGW32__
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
