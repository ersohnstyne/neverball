/*
 * Copyright (C) 2021 Microsoft / Neverball Authors
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

#include <stddef.h>

size_t dstSize = 256;

#if _WIN32
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include "account_transfer.h"
#include "config.h"

#include "common.h"
#include "fs.h"

// log groups are lacked and could not be grouped!
#define log_errorf log_printf

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


/* String options. */

int ACCOUNT_TRANSFER_PLAYER;
int ACCOUNT_TRANSFER_BALL_FILE;

/*---------------------------------------------------------------------------*/

static struct
{
    int* sym;
    const char* name;
    const int   def;
    int         cur;
}
account_transfer_d[] =
{
    { &ACCOUNT_TRANSFER_DATA_WALLET_COINS,      "wallet_coins",           0 },
    { &ACCOUNT_TRANSFER_DATA_WALLET_GEMS,       "wallet_gems",            0 },
    { &ACCOUNT_TRANSFER_PRODUCT_LEVELS,         "product_levels",         0 },
    { &ACCOUNT_TRANSFER_PRODUCT_BALLS,          "product_balls",          0 },
    { &ACCOUNT_TRANSFER_PRODUCT_BONUS,          "product_bonus",          0 },
    { &ACCOUNT_TRANSFER_PRODUCT_MEDIATION,      "product_mediation",      0 },
    { &ACCOUNT_TRANSFER_SET_UNLOCKS,            "set_unlocks",            0 },
    { &ACCOUNT_TRANSFER_CONSUMEABLE_EARNINATOR, "consumeable_earninator", 0 },
    { &ACCOUNT_TRANSFER_CONSUMEABLE_FLOATIFIER, "consumeable_floatifier", 0 },
    { &ACCOUNT_TRANSFER_CONSUMEABLE_SPEEDIFIER, "consumeable_speedifier", 0 }
};

static struct
{
    int* sym;
    const char* name;
    const char* def;
    char* cur;
} account_transfer_s[] =
{
    { &ACCOUNT_TRANSFER_PLAYER, "player", "" },
    { &ACCOUNT_TRANSFER_BALL_FILE, "ball_file", "ball/legacy-ball/legacy-ball" }
};

/*---------------------------------------------------------------------------*/

/*
 * Scan an option string and store pointers to the start of key and
 * value at the passed-in locations.  No memory is allocated to store
 * the strings; instead, the option string is modified in-place as
 * needed.  Return 1 on success, 0 on error.
 */
static int scan_key_and_value(char** dst_key, char** dst_val, char* line)
{
    if (line)
    {
        int ks, ke, vs;

        ks = -1;
        ke = -1;
        vs = -1;
#if defined(_WIN32) && !defined(__EMSCRIPTEN__) && !defined(DISABLE_SAFECPY)
        sscanf_s(line, " %n%*s%n %n", &ks, &ke, &vs);
#else
        sscanf(line, " %n%*s%n %n", &ks, &ke, &vs);
#endif

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

    account_transfer_busy = 0;
    account_transfer_is_init = 1;

    return 1;
}

void account_transfer_quit(void)
{
    int i;

    for (i = 0; i < ARRAYSIZE(account_transfer_s); i++)
    {
        free(account_transfer_s[i].cur);
        account_transfer_s[i].cur = NULL;
    }
    account_transfer_is_init = 0;
}

int  account_transfer_exists(void)
{
#if defined(_WIN32) && !defined(__EMSCRIPTEN__) && !defined(DISABLE_SAFECPY)
    char paths[MAXSTR]; sprintf_s(paths, dstSize, "Accounts_transfer/account-%s.dat", config_get_s(CONFIG_PLAYER));
#else
    char paths[MAXSTR]; sprintf(paths, "Accounts_transfer/account-%s.dat", config_get_s(CONFIG_PLAYER));
#endif

#if defined(FS_VERSION_1)
    fs_file input = fs_open(paths, "r");
#else
    fs_file input = fs_open_read(paths);
#endif

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

void account_transfer_load(const char* paths)
{
    fs_file fh;

    SDL_assert(SDL_WasInit(SDL_INIT_VIDEO));
    
    if (!account_transfer_is_init)
    {
        log_errorf("Failure to load account transfer file! Account transfer must be initialized!");
        return;
    }

#if defined(FS_VERSION_1)
    if (fh = fs_open(paths, "r"))
#else
    if (fh = fs_open_read(paths))
#endif
    {
        account_transfer_busy = 1;
        char* line, * key, * val;

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
        }
        fs_close(fh);

        account_transfer_busy = 0;
    }
}

void account_transfer_load_externals(const char* paths)
{
    fs_file fh; fh = (fs_file)calloc(1, sizeof(fh));

    SDL_assert(SDL_WasInit(SDL_INIT_VIDEO));

    if (!account_transfer_is_init)
    {
        log_errorf("Failure to load account transfer file! Account transfer must be initialized!");
        return;
    }

#if defined(FS_VERSION_1)
    if (fh = fs_open(paths, "r"))
#else
    if (fh = fs_open_read(paths))
#endif
    {
        account_transfer_busy = 1;
        char* line, * key, * val;

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
        }
        fs_close(fh);

        account_transfer_busy = 0;
    }
}

void account_transfer_save(const char* playername)
{
    fs_file fh;

    SDL_assert(SDL_WasInit(SDL_INIT_VIDEO));
    
#if defined(_WIN32) && !defined(__EMSCRIPTEN__) && !defined(DISABLE_SAFECPY)
    char paths[MAXSTR]; sprintf_s(paths, dstSize, "Accounts_transfer/%s", playername);
#else
    char paths[MAXSTR]; sprintf(paths, "Accounts_transfer/%s", playername);
#endif

    if (!account_transfer_is_init)
    {
        log_errorf("Failure to save account transfer file! Account transfer must be initialized!");
        return;
    }

#if defined(FS_VERSION_1)
    if (fh = fs_open(paths, "w"))
#else
    if (fh = fs_open_write(paths))
#endif
    {
        account_transfer_busy = 1;
        int i;

        /* Write out integer options. */

        for (i = 0; i < ARRAYSIZE(account_transfer_d); i++)
        {
            const char* s = NULL;

            if (s)
                fs_printf(fh, "%-25s %s\n", account_transfer_d[i].name, s);
            else
                fs_printf(fh, "%-25s %d\n", account_transfer_d[i].name, account_transfer_d[i].cur);
        }

        /* Write out string options. */

        for (i = 0; i < ARRAYSIZE(account_transfer_s); i++)
            fs_printf(fh, "%-25s %s\n", account_transfer_s[i].name, account_transfer_s[i].cur);

        fs_close(fh);
        fs_persistent_sync();
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

void account_transfer_set_s(int i, const char* src)
{
    account_transfer_busy = 1;

    if (account_transfer_s[i].cur)
        free(account_transfer_s[i].cur);

    account_transfer_s[i].cur = strdup(src);

    account_transfer_busy = 0;
}

const char* account_transfer_get_s(int i)
{
    return account_transfer_s[i].cur;
}

/*---------------------------------------------------------------------------*/