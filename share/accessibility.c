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

#if _WIN32 && __MINGW32__
#include <SDL2/SDL.h>
#elif _WIN32 && _MSC_VER
#include <SDL.h>
#elif _WIN32
#error Security compilation error: No target include file in path for Windows specified!
#else
#include <SDL.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#ifndef NDEBUG
#include <assert.h>
#elif defined(_MSC_VER) && defined(_AFXDLL)
#include <afx.h>
/**
 * HACK: assert() for Microsoft Windows Apps in Release builds
 * will be replaced to VERIFY() - Ersohn Styne
 */
#define assert VERIFY
#else
#define assert(_x) (_x)
#endif

#include "accessibility.h"
#include "account.h"
#include "config.h"
#include "common.h"
#include "fs.h"

#include "log.h"

/*---------------------------------------------------------------------------*/

int accessibility_busy = 0;

int ACCESSIBILITY_SLOWDOWN;

static struct
{
    int        *sym;
    const char *name;
    const int   def;
    int         cur;
}
accessibility_d[] =
{
    { &ACCESSIBILITY_SLOWDOWN, "slowdown", 100 },
};

static int dirty = 0;

/*---------------------------------------------------------------------------*/

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
        int valid_num, ks, ke, vs;

        ks = -1;
        ke = -1;
        vs = -1;

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        valid_num = sscanf_s(line,
#else
        valid_num = sscanf(line,
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

/*---------------------------------------------------------------------------*/

void accessibility_init(void)
{
    accessibility_busy = 1;

    /*
     * Store index of each option in its associated config symbol and
     * initialise current values with defaults.
     */

    for (int i = 0; i < ARRAYSIZE(accessibility_d); i++)
    {
        *accessibility_d[i].sym = i;
        accessibility_set_d(i, accessibility_d[i].def);
    }

    accessibility_busy = 0;
}

void accessibility_load(void)
{
    fs_file fh;

#ifndef NDEBUG
    assert(!account_busy && !config_busy &&
           "This account or config data is busy and cannot be edit there!");
    SDL_assert(SDL_WasInit(SDL_INIT_VIDEO));
#endif

    char *filename = strdup("accessibility");

    if ((fh = fs_open_read(filename)))
    {
        accessibility_busy = 1;
        char *line, * key, * val;

        while (read_line(&line, fh))
        {
            if (scan_key_and_value(&key, &val, line))
            {
                int i;

                /* Look up an integer option by that name. */

                for (i = 0; i < ARRAYSIZE(accessibility_d); i++)
                {
                    if (strcmp(key, accessibility_d[i].name) == 0)
                    {
                        accessibility_set_d(i, atoi(val));

                        /* Stop looking. */

                        break;
                    }
                }
            }

            free(line);
            line = NULL;
        }
        fs_close(fh);

        dirty = 0;
        accessibility_busy = 0;
    }
    else
        log_errorf("Failure to load accessibility file!: %s / %s\n",
                   filename, fs_error());

    free(filename);
    filename = NULL;
}

void accessibility_save(void)
{
    fs_file fh;

#ifndef NDEBUG
    assert(!account_busy && !config_busy &&
           "This account or config data is busy and cannot be edit there!");
    SDL_assert(SDL_WasInit(SDL_INIT_VIDEO));
#endif

    char *filename = strdup("accessibility");

    if ((fh = fs_open_write(filename)))
    {
        accessibility_busy = 1;
        int i;

        /* Write out integer options. */

        for (i = 0; i < ARRAYSIZE(accessibility_d); i++)
        {
            const char *s = NULL;

            if (s)
                fs_printf(fh, "%-25s %s\n", accessibility_d[i].name, s);
            else
                fs_printf(fh, "%-25s %d\n", accessibility_d[i].name, accessibility_d[i].cur);
        }

        fs_close(fh);
    }
    else if (dirty)
        log_errorf("Failure to save accessibility file!: %s / %s\n",
                   filename, fs_error());

    free(filename);
    filename = NULL;

    dirty = 0;
    accessibility_busy = 0;
}

/*---------------------------------------------------------------------------*/

void accessibility_set_d(int i, int d)
{
#ifndef NDEBUG
    assert(!account_busy && !config_busy &&
           "This account or config data is busy and cannot be edit there!");
#endif
    if (!account_busy && !config_busy)
    {
        account_busy = 1;
        accessibility_d[i].cur = d;
        dirty = 1;
        account_busy = 0;
    }
}

void accessibility_tgl_d(int i)
{
#ifndef NDEBUG
    assert(!account_busy && !config_busy &&
           "This account or config data is busy and cannot be edit there!");
#endif
    if (!account_busy && !config_busy)
    {
        account_busy = 1;
        accessibility_d[i].cur = (accessibility_d[i].cur ? 0 : 1);
        dirty = 1;
        account_busy = 0;
    }
}

int accessibility_tst_d(int i, int d)
{
    return (accessibility_d[i].cur == d) ? 1 : 0;
}

int accessibility_get_d(int i)
{
    return accessibility_d[i].cur;
}
