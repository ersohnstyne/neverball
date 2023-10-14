/*
 * Copyright (C) 2023 Microsoft / Valve / Neverball Authors
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

/*
 * HACK: Remembering the code file differences:
 * Developers  who  programming  C++  can see more bedrock declaration
 * than C.  Developers  who  programming  C  can  see  few  procedural
 * declaration than  C++.  Keep  in  mind  when making  sure that your
 * extern code must associated. The valid file types are *.c and *.cpp,
 * so it's always best when making cross C++ compiler to keep both.
 * - Ersohn Styne
 */

#if _WIN32 && __MINGW32__
#include <SDL3/SDL.h>
#else
#include <SDL.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#if NB_STEAM_API==1 && NB_EOS_SDK==0

#include <steam/steam_api.h>
#include <steam/isteamuser.h>

#if _WIN32 && _WIN64
#pragma comment(lib, "D:\\source_ersohn\\win10_libraries\\steam\\redistributable_bin\\win64\\steam_api64.lib")
#elif _WIN32
#pragma comment(lib, "D:\\source_ersohn\\win10_libraries\\steam\\redistributable_bin\\steam_api.lib")
#endif

extern "C"
{
#include "accessibility.h"
#include "account.h"
#include "config.h"
#include "networking.h"
#include "text.h"

#include "common.h"
#include "fs.h"

#include "binary.h"
}

/*---------------------------------------------------------------------------*/

extern "C"
{
    int account_is_init = 0;
    int account_changeable = 0;
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
}

/*---------------------------------------------------------------------------*/

extern "C"
{
    static struct
    {
        int curr;
    } steam_account_d[11];

    static struct
    {
        char *curr;
    } steam_account_s[2];

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
        { &ACCOUNT_CONSUMEABLE_EXTRALIVES, "consumeable_extralives", 0 }
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
        { &ACCOUNT_BALL_FILE, "ball_file", "ball/basic-ball/basic-ball" }
    };

    static int dirty = 0;
}

/*---------------------------------------------------------------------------*/

extern "C" int account_init(void)
{
    if (!SteamUser()->BLoggedOn())
        return 0;

    SteamFriends()->ActivateGameOverlay("Friends");

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

    for (int i = 0; i < ARRAYSIZE(account_d); i++)
    {
        steam_account_d[i].curr = account_d[i].cur;
        dirty = 1;
    }
    
    for (int i = 0; i < ARRAYSIZE(account_s); i++)
    {
        steam_account_s[i].curr = account_s[i].cur;
        dirty = 1;
    }

    account_busy = 0;

    config_set_s(CONFIG_PLAYER, SteamFriends()->GetPersonaName());
    config_save();

    account_is_init = 1;

    return 1;
}

extern "C" void account_quit(void)
{
    if (!account_is_init) return;

    assert(!account_busy);

    int i;

    for (i = 0; i < ARRAYSIZE(steam_account_s); i++)
    {
        if (steam_account_s[i].curr)
        {
            free(steam_account_s[i].curr);
            steam_account_s[i].curr = NULL;
        }
    }

    for (i = 0; i < ARRAYSIZE(account_s); i++)
    {
        if (account_s[i].cur)
        {
            free(account_s[i].cur);
            account_s[i].cur = NULL;
        }
    }

    account_is_init = 0;
}

extern "C" int  account_exists(void)
{
    char paths[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(paths, dstSize,
#else
    sprintf(paths,
#endif
            "Accounts/account-%s.nbaccount", config_get_s(CONFIG_PLAYER));

    assert(!networking_busy && !config_busy && !accessibility_busy &&
           "This networking, accessibility or configuration is busy and cannot be edit there!");
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

extern "C" void account_load(void)
{
    fs_file fh;

    assert(!networking_busy && !config_busy && !accessibility_busy &&
           "This networking, accessibility or configuration is busy and cannot be edit there!");
    SDL_assert(SDL_WasInit(SDL_INIT_VIDEO));

    if (text_length(config_get_s(CONFIG_PLAYER)) < 1)
    {
        log_errorf("Cannot load account! No player name!\n");
        return;
    }

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
        {
            log_errorf("Cannot load account! Can't accept other charsets!\n");
            return;
        }
    }

    if (text_length(config_get_s(CONFIG_PLAYER)) < 3)
    {
        log_errorf("Cannot load account! Too few characters!\n");
        return;
    }

    char paths[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(paths, dstSize,
#else
    sprintf(paths,
#endif
            "Accounts/account-%s.nbaccount", config_get_s(CONFIG_PLAYER));

    if (!account_is_init)
    {
        log_errorf("Failure to load account file! Account must be initialized!\n");
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

        for (int i = 0; i < ARRAYSIZE(account_d); i++)
        {
            steam_account_d[i].curr = get_index(fh);
            account_d[i].cur = steam_account_d[i].curr;
        }

        for (int i = 0; i < ARRAYSIZE(account_s); i++)
        {
            get_string(fh, steam_account_s[i].curr, MAXSTR);
            account_s[i].cur = strdup(steam_account_s[i].curr);
        }

        fs_close(fh);

        dirty = 0;
        account_busy = 0;
    }
}

extern "C" void account_save(void)
{
    fs_file fh;

    assert(!networking_busy && !config_busy && !accessibility_busy &&
           "This networking, accessibility or configuration is busy and cannot be edit there!");
    SDL_assert(SDL_WasInit(SDL_INIT_VIDEO));
    
    if (text_length(config_get_s(CONFIG_PLAYER)) < 1)
    {
        log_errorf("Cannot save account! No player name!\n");
        return;
    }

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
        {
            log_errorf("Cannot save account! Can't accept other charsets!\n");
            return;
        }
    }

    if (text_length(config_get_s(CONFIG_PLAYER)) < 3)
    {
        log_errorf("Cannot save account! Too few characters!\n");
        return;
    }

    char paths[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(paths, dstSize,
#else
    sprintf(paths,
#endif
            "Accounts/account-%s.nbaccount", config_get_s(CONFIG_PLAYER));

    if (!account_is_init)
    {
        log_errorf("Failure to save account file! Account must be initialized!\n");
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

    account_set_s(ACCOUNT_PLAYER, config_get_s(CONFIG_PLAYER));

#ifdef FS_VERSION_1
    if (dirty && (fh = fs_open(paths, "w")))
#else
    if (dirty && (fh = fs_open_write(paths)))
#endif
    {
        account_busy = 1;

        for (int i = 0; i < ARRAYSIZE(account_d); i++)
        {
            steam_account_d[i].curr = account_d[i].cur;
            put_index(fh, steam_account_d[i].curr);
        }

        for (int i = 0; i < ARRAYSIZE(account_s); i++)
        {
            if (steam_account_s[i].curr)
            {
                free(steam_account_s[i].curr);
                steam_account_s[i].curr = NULL;
            }

            steam_account_s[i].curr = strdup(account_s[i].cur);
            put_string(fh, steam_account_s[i].curr);
        }

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

extern "C" void account_set_d(int i, int d)
{
    assert(!networking_busy && !config_busy && !accessibility_busy &&
           "This networking, accessibility or configuration is busy and cannot be edit there!");
    if (!networking_busy && !config_busy && !accessibility_busy)
    {
        account_busy = 1;
        account_d[i].cur = d;
        dirty = 1;
        account_busy = 0;
    }
}

extern "C" void account_tgl_d(int i)
{
    assert(!networking_busy && !config_busy && !accessibility_busy &&
           "This networking, accessibility or configuration is busy and cannot be edit there!");
    if (!networking_busy && !config_busy && !accessibility_busy)
    {
        account_busy = 1;
        account_d[i].cur = (account_d[i].cur ? 0 : 1);
        dirty = 1;
        account_busy = 0;
    }
}

extern "C" int account_tst_d(int i, int d)
{
    return (account_d[i].cur == d) ? 1 : 0;
}

extern "C" int account_get_d(int i)
{
    return account_d[i].cur;
}

/*---------------------------------------------------------------------------*/

extern "C" void account_set_s(int i, const char *src)
{
    assert(!networking_busy && !config_busy && !accessibility_busy &&
           "This networking, accessibility or configuration is busy and cannot be edit there!");
    if (!networking_busy && !config_busy && !accessibility_busy)
    {
        account_busy = 1;

        if (account_s[i].cur)
        {
            free(account_s[i].cur);
            account_s[i].cur = NULL;
        }

        account_s[i].cur = strdup(src);

        dirty = 1;
        account_busy = 0;
    }
}

extern "C" const char *account_get_s(int i)
{
    return account_s[i].cur;
}

#endif

/*---------------------------------------------------------------------------*/