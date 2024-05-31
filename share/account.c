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

#if _WIN32 && __MINGW32__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#ifndef NDEBUG
#include <assert.h>
#endif

#if __cplusplus
extern "C"
{
#endif

#include "accessibility.h"
#include "account.h"
#include "config.h"
#include "networking.h"
#include "text.h"

#include "common.h"
#include "fs.h"

#if __cplusplus
}
#endif

/*---------------------------------------------------------------------------*/

int account_is_init = 0;
int account_changeable = 1;
int account_busy = 0;

/* Integer options. */

int ACCOUNT_DATA_WALLET_COINS;
int ACCOUNT_DATA_WALLET_GEMS;
int ACCOUNT_PRODUCT_LEVELS;
int ACCOUNT_PRODUCT_BALLS;
int ACCOUNT_PRODUCT_BONUS;
int ACCOUNT_PRODUCT_MEDIATION;
int ACCOUNT_SET_UNLOCKS;
int ACCOUNT_CONSUMEABLE_EARNINATOR;
int ACCOUNT_CONSUMEABLE_FLOATIFIER;
int ACCOUNT_CONSUMEABLE_SPEEDIFIER;
int ACCOUNT_CONSUMEABLE_EXTRALIVES;


/* String options. */

int ACCOUNT_PLAYER;
int ACCOUNT_BALL_FILE;
int ACCOUNT_BALL_FILE_LL;
int ACCOUNT_BALL_FILE_L;
int ACCOUNT_BALL_FILE_C;
int ACCOUNT_BALL_FILE_R;
int ACCOUNT_BALL_FILE_RR;

/*---------------------------------------------------------------------------*/

static struct
{
    int        *sym;
    const char *name;
    const int   def;
    int         cur;
}
account_d[] =
{
    { &ACCOUNT_DATA_WALLET_COINS,      "wallet_coins",           0 },
    { &ACCOUNT_DATA_WALLET_GEMS,       "wallet_gems",            15 },
    { &ACCOUNT_PRODUCT_LEVELS,         "product_levels",         0 },
    { &ACCOUNT_PRODUCT_BALLS,          "product_balls",          0 },
    { &ACCOUNT_PRODUCT_BONUS,          "product_bonus",          0 },
    { &ACCOUNT_PRODUCT_MEDIATION,      "product_mediation",      0 },
    { &ACCOUNT_SET_UNLOCKS,            "set_unlocks",            1 },
    { &ACCOUNT_CONSUMEABLE_EARNINATOR, "consumeable_earninator", 1 },
    { &ACCOUNT_CONSUMEABLE_FLOATIFIER, "consumeable_floatifier", 1 },
    { &ACCOUNT_CONSUMEABLE_SPEEDIFIER, "consumeable_speedifier", 1 },
    { &ACCOUNT_CONSUMEABLE_SPEEDIFIER, "consumeable_extralives", 0 }
};

static struct
{
    int        *sym;
    const char *name;
    const char *def;
    char       *cur;
} account_s[] =
{
    { &ACCOUNT_PLAYER,       "player",       "" },
#if defined(CONFIG_INCLUDES_MULTIBALLS)
    { &ACCOUNT_BALL_FILE,    "ball_file", "ball/" },
#else
    { &ACCOUNT_BALL_FILE,    "ball_file", "ball/basic-ball/basic-ball" },
#endif
    { &ACCOUNT_BALL_FILE_LL, "ball_file_ll", "ball/" },
    { &ACCOUNT_BALL_FILE_L,  "ball_file_l",  "ball/" },
    { &ACCOUNT_BALL_FILE_C,  "ball_file_c",  "ball/basic-ball/basic-ball" },
    { &ACCOUNT_BALL_FILE_R,  "ball_file_r",  "ball/" },
    { &ACCOUNT_BALL_FILE_RR, "ball_file_rr", "ball/" }
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

/*---------------------------------------------------------------------------*/

int account_init(void)
{
    account_busy = 1;

    /*
     * Store index of each option in its associated config symbol and
     * initialise current values with defaults.
     */

    for (int i = 0; i < ARRAYSIZE(account_d); i++)
    {
        *account_d[i].sym = i;
        account_set_d(i, account_d[i].def);
    }

    for (int i = 0; i < ARRAYSIZE(account_s); i++)
    {
        *account_s[i].sym = i;
        account_set_s(i, account_s[i].def);
    }

    account_busy = 0;
    account_is_init = 1;

    return 1;
}

void account_quit(void)
{
#ifndef NDEBUG
    assert(!account_busy);
#endif

    int i;

    for (i = 0; i < ARRAYSIZE(account_s); i++)
    {
        free(account_s[i].cur);
        account_s[i].cur = NULL;
    }

    account_is_init = 0;
}

int  account_exists(void)
{
    char paths[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(paths, MAXSTR,
#else
    sprintf(paths,
#endif
            "Accounts/account-%s.txt", config_get_s(CONFIG_PLAYER));

#ifndef NDEBUG
    assert(!networking_busy && !config_busy && !accessibility_busy &&
           "This networking, accessibility or configuration is busy and cannot be edit there!");
#endif

    return fs_exists(paths);
}

void account_load(void)
{
    fs_file fh;

#ifndef NDEBUG
    assert(!networking_busy && !config_busy && !accessibility_busy &&
           "This networking, accessibility or configuration is busy and cannot be edit there!");
    SDL_assert(SDL_WasInit(SDL_INIT_VIDEO));
#endif

    if (text_length(config_get_s(CONFIG_PLAYER)) < 1)
        return;

    for (int i = 0; i < text_length(config_get_s(CONFIG_PLAYER)); i++)
    {
        if (config_get_s(CONFIG_PLAYER)[i] == '\\' ||
            config_get_s(CONFIG_PLAYER)[i] == '/'  ||
            config_get_s(CONFIG_PLAYER)[i] == ':'  ||
            config_get_s(CONFIG_PLAYER)[i] == '*'  ||
            config_get_s(CONFIG_PLAYER)[i] == '?'  ||
            config_get_s(CONFIG_PLAYER)[i] == '"'  ||
            config_get_s(CONFIG_PLAYER)[i] == '<'  ||
            config_get_s(CONFIG_PLAYER)[i] == '>'  ||
            config_get_s(CONFIG_PLAYER)[i] == '|')
            return;
    }

    if (text_length(config_get_s(CONFIG_PLAYER)) < 3)
        return;

    char paths[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(paths, MAXSTR,
#else
    sprintf(paths,
#endif
            "Accounts/account-%s.txt", config_get_s(CONFIG_PLAYER));

    if (!account_is_init)
    {
        log_errorf("Failure to load account file! Account must be initialized!\n");
#if _DEBUG
        SDL_TriggerBreakpoint();
#endif

        exit(1);
        return;
    }

    if (dirty && (fh = fs_open_read(paths)))
    {
        account_busy = 1;
        char *line, * key, * val;

        while (read_line(&line, fh))
        {
            if (scan_key_and_value(&key, &val, line))
            {
                int i;

                /* Look up an integer option by that name. */

                for (i = 0; i < ARRAYSIZE(account_d); i++)
                {
                    if (strcmp(key, account_d[i].name) == 0)
                    {
                        account_set_d(i, atoi(val));

                        /* Stop looking. */

                        break;
                    }
                }

                /* Look up a string option by that name.*/

                for (i = 0; i < ARRAYSIZE(account_s); i++)
                {
                    if (strcmp(key, account_s[i].name) == 0)
                    {
                        account_set_s(i, val);
                        break;
                    }
                }
            }
            free(line);
            line = NULL;
        }
        fs_close(fh);

        dirty = 0;
        account_busy = 0;
    }
}

void account_save(void)
{
    fs_file fh;

#ifndef NDEBUG
    assert(!networking_busy && !config_busy && !accessibility_busy &&
           "This networking, accessibility or configuration is busy and cannot be edit there!");
    SDL_assert(SDL_WasInit(SDL_INIT_VIDEO));
#endif

    if (text_length(config_get_s(CONFIG_PLAYER)) < 1)
        return;

    for (int i = 0; i < text_length(config_get_s(CONFIG_PLAYER)); i++)
    {
        if (config_get_s(CONFIG_PLAYER)[i] == '\n' ||
            config_get_s(CONFIG_PLAYER)[i] == '/'  ||
            config_get_s(CONFIG_PLAYER)[i] == ':'  ||
            config_get_s(CONFIG_PLAYER)[i] == '*'  ||
            config_get_s(CONFIG_PLAYER)[i] == '?'  ||
            config_get_s(CONFIG_PLAYER)[i] == '"'  ||
            config_get_s(CONFIG_PLAYER)[i] == '<'  ||
            config_get_s(CONFIG_PLAYER)[i] == '>'  ||
            config_get_s(CONFIG_PLAYER)[i] == '|')
            return;
    }

    if (text_length(config_get_s(CONFIG_PLAYER)) < 3)
        return;

    char paths[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(paths, MAXSTR,
#else
    sprintf(paths,
#endif
            "Accounts/account-%s.txt", config_get_s(CONFIG_PLAYER));

    if (dirty && !account_is_init)
    {
        log_errorf("Failure to save account file! Account must be initialized!\n");
#if _DEBUG
        SDL_TriggerBreakpoint();
#endif

        if (fs_exists(paths))
            fs_remove(paths);

        exit(1);
        return;
    }

    if (dirty && (fh = fs_open_write(paths)))
    {
        account_busy = 1;
        int i;

        /* Write out integer options. */

        for (i = 0; i < ARRAYSIZE(account_d); i++)
        {
            const char *s = NULL;

            if (s)
                fs_printf(fh, "%-25s %s\n", account_d[i].name, s);
            else
                fs_printf(fh, "%-25s %d\n", account_d[i].name, account_d[i].cur);
        }

        /* Write out string options. */

        for (i = 0; i < ARRAYSIZE(account_s); i++)
            fs_printf(fh, "%-25s %s\n", account_s[i].name, account_s[i].cur);

        fs_close(fh);
        fs_persistent_sync();

        dirty = 0;
    }
    else if (dirty)
        log_errorf("Failure to save account file!: %s / %s\n",
                   paths, fs_error());

    account_busy = 0;
}

/*---------------------------------------------------------------------------*/

void account_set_d(int i, int d)
{
#ifndef NDEBUG
    assert(!networking_busy && !config_busy && !accessibility_busy &&
           "This networking, accessibility or configuration is busy and cannot be edit there!");
#endif
    if (!networking_busy && !config_busy && !accessibility_busy)
    {
        account_busy = 1;
        account_d[i].cur = d;
        dirty = 1;
        account_busy = 0;
    }
}

void account_tgl_d(int i)
{
#ifndef NDEBUG
    assert(!networking_busy && !config_busy && !accessibility_busy &&
           "This networking, accessibility or configuration is busy and cannot be edit there!");
#endif
    if (!networking_busy && !config_busy && !accessibility_busy)
    {
        account_busy = 1;
        account_d[i].cur = (account_d[i].cur ? 0 : 1);
        dirty = 1;
        account_busy = 0;
    }
}

int account_tst_d(int i, int d)
{
    return (account_d[i].cur == d) ? 1 : 0;
}

int account_get_d(int i)
{
    return account_d[i].cur;
}

/*---------------------------------------------------------------------------*/

void account_set_s(int i, const char *src)
{
#ifndef NDEBUG
    assert(!networking_busy && !config_busy && !accessibility_busy &&
           "This networking, accessibility or configuration is busy and cannot be edit there!");
#endif
    if (!networking_busy && !config_busy && !accessibility_busy)
    {
        account_busy = 1;

        if (account_s[i].cur)
        {
            free(account_s[i].cur);
            account_s[i].cur = NULL;
        }

        account_s[i].cur = dupe_string(src);

        dirty = 1;
        account_busy = 0;
    }
}

const char *account_get_s(int i)
{
    return account_s[i].cur;
}

/*---------------------------------------------------------------------------*/
