/*
 * Copyright (C) 2025 Microsoft / Neverball authors / Jānis Rūcis
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

#ifndef FONT_H
#define FONT_H

#if _WIN32 && __MINGW32__
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_rwops.h>
#elif _WIN32 && _MSC_VER
#include <SDL_ttf.h>
#include <SDL_rwops.h>
#elif _WIN32
#error Security compilation error: No target include file in path for Windows specified!
#else
#include <SDL_ttf.h>
#include <SDL_rwops.h>
#endif

#include "base_config.h"

enum
{
    FONT_SIZE_MIN = 0,

    FONT_SIZE_TNY = FONT_SIZE_MIN,
    FONT_SIZE_XS,
    FONT_SIZE_SML,
    FONT_SIZE_TCH,
    FONT_SIZE_MED,
    FONT_SIZE_LRG,

    FONT_SIZE_MAX
};

struct font
{
    char path[PATHMAX];

    TTF_Font  *ttf[FONT_SIZE_MAX];
    SDL_RWops *rwops;
    void      *data;
    int        datalen;
};

int  font_load(struct font *, const char *path, int sizes[FONT_SIZE_MAX]);
void font_free(struct font *);

int  font_init(void);
void font_quit(void);

#endif
