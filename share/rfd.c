/*
 * Copyright (C) 2024 Microsoft / Neverball authors
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

#if ENABLE_RFD==1 && _MSC_VER
#pragma message(__FILE__ ": Neverball - Recipes for Disaster")
#endif

#if _WIN32 && __MINGW32__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#if NB_HAVE_PB_BOTH==1
#include "rfd.h"
#endif
#include "common.h"
#include "fs.h"

#if _DEBUG && _MSC_VER
#ifndef _CRTDBG_MAP_ALLOC
#pragma message(__FILE__": Missing _CRT_MAP_ALLOC, recreate: _CRTDBG_MAP_ALLOC + crtdbg.h")
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

/*---------------------------------------------------------------------------*/

/*
 * Neverball - Recipes for Disaster
 * - Ersohn Styne
 */

int rfd_is_init = 0;

/* Integer options. */

int RFD_CAMPAIGN_MILES_PERCENTAGE;
int RFD_SET_MILES_PERCENTAGE;
int RFD_CHALLENGE_MILES_PERCENTAGE;
int RFD_CHALLENGE_BALLS;
int RFD_CHALLENGE_EARNINATOR;
int RFD_CHALLENGE_FLOATIFIER;
int RFD_CHALLENGE_SPEEDIFIER;

/* String options. */

/* TODO: Add RFD string options here. */

/*---------------------------------------------------------------------------*/

static struct
{
    int        *sym;
    const char *name;
    const int   def;
    int         cur;
} option_d[] = {
    { &RFD_CAMPAIGN_MILES_PERCENTAGE,  "campaign_miles_percentage",  100 },
    { &RFD_SET_MILES_PERCENTAGE,       "set_miles_percentage",       0   },
    { &RFD_CHALLENGE_MILES_PERCENTAGE, "challenge_miles_percentage", 100 },
    { &RFD_CHALLENGE_BALLS,            "challenge_balls",            0   },
    { &RFD_CHALLENGE_EARNINATOR,       "challenge_earninator",       0   },
    { &RFD_CHALLENGE_FLOATIFIER,       "challenge_floatifier",       0   },
    { &RFD_CHALLENGE_SPEEDIFIER,       "challenge_speedifier",       0   },
};

/*---------------------------------------------------------------------------*/

void rfd_init(void)
{
    int i;

    /*
     * Store index of each option in its associated config symbol and
     * initialise current values with defaults.
     */

    for (i = 0; i < ARRAYSIZE(option_d); i++)
    {
        *option_d[i].sym = i;
        rfd_set_d(i, option_d[i].def);
    }

    rfd_is_init = 1;
}

void rfd_quit(void)
{
    if (!rfd_is_init) return;

    rfd_is_init = 0;
}

/*
 * Scan an option string and store pointers to the start of key and
 * value at the passed-in locations.  No memory is allocated to store
 * the strings; instead, the option string is modified in-place as
 * needed.  Return 1 on success, 0 on error.
 */
static int scan_key_and_value(char **dst_key, char **dst_val, char *line)
{
    if (line)
    {
        int ks, ke, vs;

        ks = -1;
        ke = -1;
        vs = -1;
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sscanf_s(line,
#else
        sscanf(line,
#endif
               " %n%*s%n %n", &ks, &ke, &vs);

        if (ks < 0 || ke < 0 || vs < 0)
            return 0;

        if (vs - ke < 1)
            return 0;

        line[ke] = 0;

        *dst_key = line + ks;
        *dst_val = line + vs;

        return 1;
    }

    return 0;
}

void rfd_load(void)
{
#if ENABLE_RFD==1
    fs_file fh;
#ifndef NDEBUG
    SDL_assert(SDL_WasInit(SDL_INIT_VIDEO));
#endif

    if (!rfd_is_init)
    {
        log_errorf("Failure to load RFD file! RFD configs must be initialized!\n");
#if _DEBUG
        SDL_TriggerBreakpoint();
#endif

        exit(1);
        return;
    }

    const char *filename = "rfdconf.txt";

    if ((fh = fs_open_read(filename)))
    {
        char *line, *key, *val;

        while (read_line(&line, fh))
        {
            if (scan_key_and_value(&key, &val, line))
            {
                int i;

                /* Look up an integer option by that name. */

                for (i = 0; i < ARRAYSIZE(option_d); i++)
                {
                    if (strcmp(key, option_d[i].name) == 0)
                    {
                        rfd_set_d(val, i);

                        break;
                    }
                }
            }
            free(line);
            line = NULL;
        }
        fs_close(fh);
    }
#endif
}

/*---------------------------------------------------------------------------*/

void rfd_set_d(int i, int d)
{
    option_d[i].cur = d;
}

int  rfd_get_d(int i)
{
    return option_d[i].cur;
}

/*---------------------------------------------------------------------------*/
