/*
 * Copyright (C) 2022 Microsoft / Neverball authors
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

#if _WIN32 && __GNUC__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

extern "C"
{
#include "accessibility.h"
#include "account.h"
#include "config.h"
#include "networking.h"

#include "common.h"
#include "fs.h"
}

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
    { &ACCOUNT_PLAYER, "player", "" },
    { &ACCOUNT_BALL_FILE, "ball_file", "ball/legacy-ball/legacy-ball" }
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
    assert(!account_busy);

    int i;

    for (i = 0; i < ARRAYSIZE(account_s); i++)
        free(account_s[i].cur);

    account_is_init = 0;
}

int  account_exists(void)
{
    if (server_policy_get_d(SERVER_POLICY_EDITION) < 0) return 1;

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    char paths[MAXSTR]; sprintf_s(paths, dstSize, "Accounts/account-%s.dat", config_get_s(CONFIG_PLAYER));
#else
    char paths[MAXSTR]; sprintf(paths, "Accounts/account-%s.dat", config_get_s(CONFIG_PLAYER));
#endif

    assert(!networking_busy && !config_busy && !accessibility_busy && "This networking, accessibility or configuration is busy and cannot be edit there!");

#ifdef FS_VERSION_1
    fs_file input = fs_open(paths, "r");
#else
    fs_file input = fs_open_read(paths);
#endif

    account_busy = 1;
    if (input)
    {
        fs_close(input);
        account_busy = 0;
        return 1;
    }
    account_busy = 0;
    return 0;
}

void account_load(void)
{
    fs_file fh;

    assert(!networking_busy && !config_busy && !accessibility_busy && "This networking, accessibility or configuration is busy and cannot be edit there!");
    SDL_assert(SDL_WasInit(SDL_INIT_VIDEO));
    if (server_policy_get_d(SERVER_POLICY_EDITION) < 0) return;

    if (strlen(config_get_s(CONFIG_PLAYER)) < 1)
    {
        log_errorf("Cannot load account! No player name!\n");
        return;
    }

    for (int i = 0; i < strlen(config_get_s(CONFIG_PLAYER)); i++)
    {
        if (config_get_s(CONFIG_PLAYER)[i] == '\\' || config_get_s(CONFIG_PLAYER)[i] == '/' || config_get_s(CONFIG_PLAYER)[i] == ':' || config_get_s(CONFIG_PLAYER)[i] == '*' || config_get_s(CONFIG_PLAYER)[i] == '?' || config_get_s(CONFIG_PLAYER)[i] == '"' || config_get_s(CONFIG_PLAYER)[i] == '<' || config_get_s(CONFIG_PLAYER)[i] == '>' || config_get_s(CONFIG_PLAYER)[i] == '|')
        {
            log_errorf("Cannot load account! Can't accept other charsets!\n", config_get_s(CONFIG_PLAYER)[i]);
            return;
        }
    }

    if (strlen(config_get_s(CONFIG_PLAYER)) < 3)
    {
        log_errorf("Cannot load account! Too few characters!\n");
        return;
    }

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    char paths[MAXSTR]; sprintf_s(paths, dstSize, "Accounts/account-%s.txt", config_get_s(CONFIG_PLAYER));
#else
    char paths[MAXSTR]; sprintf(paths, "Accounts/account-%s.txt", config_get_s(CONFIG_PLAYER));
#endif

    if (!account_is_init)
    {
        log_errorf("Failure to load account file! Account must be initialized!");
#ifdef FS_VERSION_1
        if ((fh = fs_open(paths, "r")))
#else
        if ((fh = fs_open_read(paths)))
#endif
        {
            fs_close(fh);
            fs_remove(paths);
        }
        exit(1);
        return;
    }

#ifdef FS_VERSION_1
    if (dirty && (fh = fs_open(paths, "r")))
#else
    if (dirty && (fh = fs_open_read(paths)))
#endif
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
        }
        fs_close(fh);

        dirty = 0;
        account_busy = 0;
    }
}

void account_save(void)
{
    fs_file fh;

    assert(!networking_busy && !config_busy && !accessibility_busy && "This networking, accessibility or configuration is busy and cannot be edit there!");
    SDL_assert(SDL_WasInit(SDL_INIT_VIDEO));
    if (server_policy_get_d(SERVER_POLICY_EDITION) < 0) return;

    if (strlen(config_get_s(CONFIG_PLAYER)) < 1)
    {
        log_errorf("Cannot save account! No player name!\n");
        return;
    }

    for (int i = 0; i < strlen(config_get_s(CONFIG_PLAYER)); i++)
    {
        if (config_get_s(CONFIG_PLAYER)[i] == '\\' || config_get_s(CONFIG_PLAYER)[i] == '/' || config_get_s(CONFIG_PLAYER)[i] == ':' || config_get_s(CONFIG_PLAYER)[i] == '*' || config_get_s(CONFIG_PLAYER)[i] == '?' || config_get_s(CONFIG_PLAYER)[i] == '"' || config_get_s(CONFIG_PLAYER)[i] == '<' || config_get_s(CONFIG_PLAYER)[i] == '>' || config_get_s(CONFIG_PLAYER)[i] == '|')
        {
            log_errorf("Cannot save account! Can't accept other charsets!\n", config_get_s(CONFIG_PLAYER)[i]);
            return;
        }
    }

    if (strlen(config_get_s(CONFIG_PLAYER)) < 3)
    {
        log_errorf("Cannot save account! Too few characters!\n");
        return;
    }

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    char paths[MAXSTR]; sprintf_s(paths, dstSize, "Accounts/account-%s.txt", config_get_s(CONFIG_PLAYER));
#else
    char paths[MAXSTR]; sprintf(paths, "Accounts/account-%s.txt", config_get_s(CONFIG_PLAYER));
#endif

    if (!account_is_init)
    {
        log_errorf("Failure to save account file! Account must be initialized!");
#ifdef FS_VERSION_1
        if ((fh = fs_open(paths, "r")))
#else
        if ((fh = fs_open_read(paths)))
#endif
        {
            fs_close(fh);
            fs_remove(paths);
        }
        exit(1);
        return;
    }

#ifdef FS_VERSION_1
    if (dirty && (fh = fs_open(paths, "w")))
#else
    if (dirty && (fh = fs_open_write(paths)))
#endif
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
        log_errorf("Failure to save account file!: %s / %s\n", paths, fs_error());

    account_busy = 0;
}

/*---------------------------------------------------------------------------*/

void account_set_d(int i, int d)
{
    assert(!networking_busy && !config_busy && !accessibility_busy && "This networking, accessibility or configuration is busy and cannot be edit there!");
    account_busy = 1;
    account_d[i].cur = d;
    dirty = 1;
    account_busy = 0;
}

void account_tgl_d(int i)
{
    assert(!networking_busy && !config_busy && !accessibility_busy && "This networking, accessibility or configuration is busy and cannot be edit there!");
    account_busy = 1;
    account_d[i].cur = (account_d[i].cur ? 0 : 1);
    dirty = 1;
    account_busy = 0;
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
    assert(!networking_busy && !config_busy && !accessibility_busy && "This networking, accessibility or configuration is busy and cannot be edit there!");
    account_busy = 1;

    if (account_s[i].cur)
        free(account_s[i].cur);

    account_s[i].cur = dupe_string(src);

    dirty = 1;
    account_busy = 0;
}

const char *account_get_s(int i)
{
    return account_s[i].cur;
}

/*---------------------------------------------------------------------------*/