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

#define log_errorf log_printf

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

#if NB_STEAM_API==1 || NB_EOS_SDK==1
//#define ENABLE_ACCOUNT_BINARY
#endif

#include "accessibility.h"
#include "account_transfer.h"
#include "binary.h"
#include "config.h"
#include "networking.h"
#include "text.h"

#include "common.h"
#include "fs.h"

#include "log.h"

#if _DEBUG && _MSC_VER
#ifndef _CRTDBG_MAP_ALLOC
#pragma message(__FILE__": Missing _CRT_MAP_ALLOC, recreate: _CRTDBG_MAP_ALLOC + crtdbg.h")
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

/*---------------------------------------------------------------------------*/

int account_transfer_is_init = 0;
int account_transfer_busy = 0;

/* Integer options. */

int ACCOUNT_TRANSFER_DATA_WALLET_COINS;
int ACCOUNT_TRANSFER_DATA_WALLET_GEMS;
int ACCOUNT_TRANSFER_PRODUCT_LEVELS;
int ACCOUNT_TRANSFER_PRODUCT_BALLS;
int ACCOUNT_TRANSFER_PRODUCT_BONUS;
int ACCOUNT_TRANSFER_PRODUCT_MEDIATION;
int ACCOUNT_TRANSFER_SET_UNLOCKS;
int ACCOUNT_TRANSFER_CONSUMEABLE_EARNINATOR;
int ACCOUNT_TRANSFER_CONSUMEABLE_FLOATIFIER;
int ACCOUNT_TRANSFER_CONSUMEABLE_SPEEDIFIER;
int ACCOUNT_TRANSFER_CONSUMEABLE_EXTRALIVES;


/* String options. */

int ACCOUNT_TRANSFER_PLAYER;
int ACCOUNT_TRANSFER_BALL_FILE;
int ACCOUNT_TRANSFER_BALL_FILE_LL;
int ACCOUNT_TRANSFER_BALL_FILE_L;
int ACCOUNT_TRANSFER_BALL_FILE_C;
int ACCOUNT_TRANSFER_BALL_FILE_R;
int ACCOUNT_TRANSFER_BALL_FILE_RR;

/*---------------------------------------------------------------------------*/

static struct
{
    int curr;
} steam_account_d[11];

static struct
{
    char *curr;
} steam_account_s[7];

static struct
{
    int        *sym;
    const char *name;
    const int   def;
    int         cur;
}
account_transfer_d[] =
{
#if 0
    { &ACCOUNT_TRANSFER_DATA_WALLET_COINS,      "wallet_coins",           16650 },
    { &ACCOUNT_TRANSFER_DATA_WALLET_GEMS,       "wallet_gems",            16650 },
#else
    { &ACCOUNT_TRANSFER_DATA_WALLET_COINS,      "wallet_coins",           0 },
    { &ACCOUNT_TRANSFER_DATA_WALLET_GEMS,       "wallet_gems",            15 },
#endif
    { &ACCOUNT_TRANSFER_PRODUCT_LEVELS,         "product_levels",         0 },
    { &ACCOUNT_TRANSFER_PRODUCT_BALLS,          "product_balls",          0 },
    { &ACCOUNT_TRANSFER_PRODUCT_BONUS,          "product_bonus",          0 },
    { &ACCOUNT_TRANSFER_PRODUCT_MEDIATION,      "product_mediation",      0 },
    { &ACCOUNT_TRANSFER_SET_UNLOCKS,            "set_unlocks",            1 },
    { &ACCOUNT_TRANSFER_CONSUMEABLE_EARNINATOR, "consumeable_earninator", 0 },
    { &ACCOUNT_TRANSFER_CONSUMEABLE_FLOATIFIER, "consumeable_floatifier", 0 },
    { &ACCOUNT_TRANSFER_CONSUMEABLE_SPEEDIFIER, "consumeable_speedifier", 0 },
    { &ACCOUNT_TRANSFER_CONSUMEABLE_EXTRALIVES, "consumeable_extralives", 0 }
};

static struct
{
    int        *sym;
    const char *name;
    const char *def;
    char       *cur;
} account_transfer_s[] =
{
    { &ACCOUNT_TRANSFER_PLAYER,       "player",       "" },
#if defined(CONFIG_INCLUDES_MULTIBALLS)
    { &ACCOUNT_TRANSFER_BALL_FILE,    "ball_file", "ball/" },
#else
    { &ACCOUNT_TRANSFER_BALL_FILE,    "ball_file", "ball/basic-ball/basic-ball" },
#endif
    { &ACCOUNT_TRANSFER_BALL_FILE_LL, "ball_file_ll", "ball/" },
    { &ACCOUNT_TRANSFER_BALL_FILE_L,  "ball_file_l",  "ball/" },
    { &ACCOUNT_TRANSFER_BALL_FILE_C,  "ball_file_c",  "ball/legacy-ball/legacy-ball" },
    { &ACCOUNT_TRANSFER_BALL_FILE_R,  "ball_file_r",  "ball/" },
    { &ACCOUNT_TRANSFER_BALL_FILE_RR, "ball_file_rr", "ball/" }
};

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

int account_transfer_init(void)
{
    account_transfer_busy = 1;

    /*
     * Store index of each option in its associated config symbol and
     * initialise current values with defaults.
     */

    for (int i = 0; i < ARRAYSIZE(account_transfer_d); i++)
    {
        *account_transfer_d[i].sym = i;
        account_transfer_set_d(i, account_transfer_d[i].def);
    }

    for (int i = 0; i < ARRAYSIZE(account_transfer_s); i++)
    {
        *account_transfer_s[i].sym = i;
        account_transfer_set_s(i, account_transfer_s[i].def);
    }
#if ENABLE_ACCOUNT_BINARY
    for (int i = 0; i < ARRAYSIZE(account_transfer_d); i++)
    {
        steam_account_d[i].curr = account_transfer_d[i].cur;
    }

    for (int i = 0; i < ARRAYSIZE(account_transfer_s); i++)
    {
        steam_account_s[i].curr = account_transfer_s[i].cur;
    }
#endif

    account_transfer_busy = 0;
    account_transfer_is_init = 1;

    return 1;
}

void account_transfer_quit(void)
{
    if (!account_transfer_is_init) return;

    int i;

#if ENABLE_ACCOUNT_BINARY
    for (i = 0; i < ARRAYSIZE(steam_account_s); i++)
    {
        if (steam_account_s[i].curr)
        {
            free(steam_account_s[i].curr);
            steam_account_s[i].curr = NULL;
        }
    }
#endif

    for (i = 0; i < ARRAYSIZE(account_transfer_s); i++)
    {
        if (account_transfer_s[i].cur)
        {
            free(account_transfer_s[i].cur);
            account_transfer_s[i].cur = NULL;
        }
    }
    account_transfer_is_init = 0;
}

int  account_transfer_exists(void)
{
    char paths[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(paths, MAXSTR,
#else
    sprintf(paths,
#endif
            "Accounts_transfer/account-%s.nbaccount", config_get_s(CONFIG_PLAYER));

    fs_file input = fs_open_read(paths);

    account_transfer_busy = 1;
    if (input)
    {
        fs_close(input);
        account_transfer_busy = 0;
        return 1;
    }
    account_transfer_busy = 0;
    return 0;
}

void account_transfer_load(const char *paths)
{
    fs_file fh;

#ifndef NDEBUG
    SDL_assert(SDL_WasInit(SDL_INIT_VIDEO));
#endif

    if (!account_transfer_is_init)
    {
        log_errorf("Failure to load account transfer file! Account transfer must be initialized!");
        return;
    }

    if (fh = fs_open_read(paths))
    {
        account_transfer_busy = 1;
#if ENABLE_ACCOUNT_BINARY
        for (int i = 0; i < ARRAYSIZE(account_transfer_d); i++)
        {
            steam_account_d[i].curr = get_index(fh);
            account_transfer_d[i].cur = steam_account_d[i].curr;
        }

        for (int i = 0; i < ARRAYSIZE(account_transfer_s); i++)
        {
            get_string(fh, steam_account_s[i].curr, MAXSTR);
            account_transfer_s[i].cur = strdup(steam_account_s[i].curr);
        }
#else
        char *line, *key, *val;

        while (read_line(&line, fh))
        {
            if (scan_key_and_value(&key, &val, line))
            {
                int i;

                /* Look up an integer option by that name. */

                for (i = 0; i < ARRAYSIZE(account_transfer_d); i++)
                {
                    if (strcmp(key, account_transfer_d[i].name) == 0)
                    {
                        account_transfer_set_d(i, atoi(val));

                        /* Stop looking. */

                        break;
                    }
                }

                /* Look up a string option by that name.*/

                for (i = 0; i < ARRAYSIZE(account_transfer_s); i++)
                {
                    if (strcmp(key, account_transfer_s[i].name) == 0)
                    {
                        account_transfer_set_s(i, val);
                        break;
                    }
                }
            }
            free(line);
            line = NULL;
        }
#endif
        fs_close(fh);

        account_transfer_busy = 0;
    }
}

void account_transfer_load_externals(const char *paths)
{
    fs_file fh; fh = (fs_file) calloc(1, sizeof (fh));

#ifndef NDEBUG
    SDL_assert(SDL_WasInit(SDL_INIT_VIDEO));
#endif

    if (!account_transfer_is_init)
    {
        log_errorf("Failure to load account transfer file! Account transfer must be initialized!");
        return;
    }

    if (fh = fs_open_read(paths))
    {
        account_transfer_busy = 1;
#if ENABLE_ACCOUNT_BINARY
        for (int i = 0; i < ARRAYSIZE(account_transfer_d); i++)
        {
            steam_account_d[i].curr = get_index(fh);
            account_transfer_d[i].cur = steam_account_d[i].curr;
        }

        for (int i = 0; i < ARRAYSIZE(account_transfer_s); i++)
        {
            get_string(fh, steam_account_s[i].curr, MAXSTR);
            account_transfer_s[i].cur = strdup(steam_account_s[i].curr);
        }
#else
        char *line, * key, * val;

        while (read_line(&line, fh))
        {
            if (scan_key_and_value(&key, &val, line))
            {
                int i;

                /* Look up an integer option by that name. */

                for (i = 0; i < ARRAYSIZE(account_transfer_d); i++)
                {
                    if (strcmp(key, account_transfer_d[i].name) == 0)
                    {
                        account_transfer_set_d(i, atoi(val));

                        /* Stop looking. */

                        break;
                    }
                }

                /* Look up a string option by that name.*/

                for (i = 0; i < ARRAYSIZE(account_transfer_s); i++)
                {
                    if (strcmp(key, account_transfer_s[i].name) == 0)
                    {
                        account_transfer_set_s(i, val);
                        break;
                    }
                }
            }
            free(line);
            line = NULL;
        }
#endif
        fs_close(fh);

        account_transfer_busy = 0;
    }
}

void account_transfer_save(const char *playername)
{
    fs_file fh;

#ifndef NDEBUG
    SDL_assert(SDL_WasInit(SDL_INIT_VIDEO));
#endif

    char paths[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(paths, MAXSTR,
#else
    sprintf(paths,
#endif
            "Accounts_transfer/%s", playername);

    if (!account_transfer_is_init)
    {
        log_errorf("Failure to save account transfer file! Account transfer must be initialized!");
        return;
    }

    if (fh = fs_open_write(paths))
    {
        account_transfer_busy = 1;
#if ENABLE_ACCOUNT_BINARY
        for (int i = 0; i < ARRAYSIZE(account_transfer_d); i++)
        {
            steam_account_d[i].curr = account_transfer_d[i].cur;
            put_index(fh, steam_account_d[i].curr);
        }

        for (int i = 0; i < ARRAYSIZE(account_transfer_s); i++)
        {
            get_string(fh, steam_account_s[i].curr, MAXSTR);
            account_transfer_s[i].cur = strdup(steam_account_s[i].curr);
        }
#else
        int i;

        /* Write out integer options. */

        for (i = 0; i < ARRAYSIZE(account_transfer_d); i++)
        {
            const char *s = NULL;

            if (s)
                fs_printf(fh, "%-25s %s\n", account_transfer_d[i].name, s);
            else
                fs_printf(fh, "%-25s %d\n", account_transfer_d[i].name, account_transfer_d[i].cur);
        }

        /* Write out string options. */

        for (i = 0; i < ARRAYSIZE(account_transfer_s); i++)
            fs_printf(fh, "%-25s %s\n", account_transfer_s[i].name, account_transfer_s[i].cur);
#endif
        fs_close(fh);
    }
    else
        log_errorf("Failure to save account transfer file!: %s / %s\n", paths, fs_error());

    account_transfer_busy = 0;
}

/*---------------------------------------------------------------------------*/

void account_transfer_set_d(int i, int d)
{
    account_transfer_busy = 1;
    account_transfer_d[i].cur = d;
    account_transfer_busy = 0;
}

void account_transfer_tgl_d(int i)
{
    account_transfer_busy = 1;
    account_transfer_d[i].cur = (account_transfer_d[i].cur ? 0 : 1);
    account_transfer_busy = 0;
}

int account_transfer_tst_d(int i, int d)
{
    return (account_transfer_d[i].cur == d) ? 1 : 0;
}

int account_transfer_get_d(int i)
{
    return account_transfer_d[i].cur;
}

/*---------------------------------------------------------------------------*/

void account_transfer_set_s(int i, const char *src)
{
    account_transfer_busy = 1;

    if (account_transfer_s[i].cur)
    {
        free(account_transfer_s[i].cur);
        account_transfer_s[i].cur = NULL;
    }

    account_transfer_s[i].cur = strdup(src);

    account_transfer_busy = 0;
}

const char *account_transfer_get_s(int i)
{
    return account_transfer_s[i].cur;
}

/*---------------------------------------------------------------------------*/
