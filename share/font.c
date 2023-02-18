/*
 * Copyright (C) 2023 Microsoft / Neverball authors
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

#include <string.h>
#include <stdlib.h>

#if _WIN32 && __MINGW32__
#include <SDL3/SDL_ttf.h>
#include <SDL3/SDL_rwops.h>
#else
#include <SDL_ttf.h>
#include <SDL_rwops.h>
#endif

#include "font.h"
#include "common.h"
#include "fs.h"

/*---------------------------------------------------------------------------*/

static int _ft_is_init = 0;

int font_load(struct font *ft, const char *path, int sizes[3])
{
    if (!_ft_is_init)
        font_init();

    if (_ft_is_init)
    {
        if (ft && path && *path)
        {
            memset(ft, 0, sizeof(*ft));

            if ((ft->data = fs_load(path, &ft->datalen)))
            {
                int i;

                SAFECPY(ft->path, path);

                ft->rwops = SDL_RWFromConstMem(ft->data, ft->datalen);

                for (i = 0; i < ARRAYSIZE(ft->ttf); i++)
                {
                    SDL_RWseek(ft->rwops, 0, SEEK_SET);
                    ft->ttf[i] = TTF_OpenFontRW(ft->rwops, 0, sizes[i]);
                }
                return 1;
            }
        }
    }
    else
        log_errorf("Failure to load font! TTF must be initialized!: %s\n", SDL_GetError() ? SDL_GetError() : "Unknown error");

    return 0;
}

void font_free(struct font *ft)
{
    if (ft)
    {
        int i;

        for (i = 0; i < ARRAYSIZE(ft->ttf); i++)
            if (ft->ttf[i])
                TTF_CloseFont(ft->ttf[i]);

        if (ft->rwops)
            SDL_RWclose(ft->rwops);

        if (ft->data)
            free(ft->data);

        memset(ft, 0, sizeof (*ft));
    }
}

int font_init(void)
{
    if (_ft_is_init)
        font_quit();

    _ft_is_init = (TTF_Init() == 0);

    return _ft_is_init;
}

void font_quit(void)
{
    _ft_is_init = 0;
    TTF_Quit();
}

/*---------------------------------------------------------------------------*/
