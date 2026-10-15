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

#include <string.h>
#include <stdlib.h>

#include "dbg_config.h"

#include "font.h"
#include "common.h"
#include "fs.h"

#include "log.h"

/*---------------------------------------------------------------------------*/

static int _ft_is_init = 0;

int font_load(struct font *ft, const char *path, int sizes[FONT_SIZE_MAX])
{
    if (!_ft_is_init) font_init();

    if (_ft_is_init)
    {
        if (ft && path && *path)
        {
            memset(ft, 0, sizeof (*ft));

            if ((ft->data = fs_load(path, &ft->datalen)))
            {
                int i;

                SAFECPY(ft->path, path);

#if NB_HAVE_PB_BOTH==1 && NB_PB_SDL3==1
                if ((ft->rwops = SDL_IOFromConstMem(ft->data, ft->datalen)))
#else
                if ((ft->rwops = SDL_RWFromConstMem(ft->data, ft->datalen)))
#endif
                {
                    int opened = 0;

                    for (i = 0; i < ARRAYSIZE(ft->ttf); i++)
                    {
#if NB_HAVE_PB_BOTH==1 && NB_PB_SDL3==1
                        SDL_IOseek(ft->rwops, 0, SEEK_SET);
                        if ((ft->ttf[i] = TTF_OpenFontIO(ft->rwops, 0, sizes[i])))
#else
                        SDL_RWseek(ft->rwops, 0, SEEK_SET);
                        if ((ft->ttf[i] = TTF_OpenFontRW(ft->rwops, 0, sizes[i])))
#endif
                            opened++;
                    }

                    if (opened > 0)
                        return 1;
                }
            }
        }
    }
    else
    {
        log_errorf("Failure to load font! TTF must be initialized!: %s\n",
                   GAMEDBG_GETSTRERROR_CHOICES_SDL);
#if _DEBUG
        SDL_TriggerBreakpoint();
#endif
    }

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
#if NB_HAVE_PB_BOTH==1 && NB_PB_SDL3==1
            SDL_CloseIO(ft->rwops);
#else
            SDL_RWclose(ft->rwops);
#endif

        if (ft->data)
        {
            free(ft->data);
            ft->data = NULL;
        }

        memset(ft, 0, sizeof (*ft));
    }
}

int font_init(void)
{
    if (_ft_is_init) font_quit();
    _ft_is_init = (TTF_Init() == 0);

    return _ft_is_init;
}

void font_quit(void)
{
    _ft_is_init = 0;
    TTF_Quit();
}

/*---------------------------------------------------------------------------*/
