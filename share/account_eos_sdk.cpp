/*
 * Copyright (C) 2023 Microsoft / Epic Games / Neverball Authors
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

#if NB_STEAM_API==0 && NB_EOS_SDK==1

#include <eos_sdk.h>
#include <eos_auth.h>

#if _WIN32 && _WIN64
#pragma comment(lib, "D:\\source_ersohn\\win10_libraries\\epicgames\\lib\\eossdk-win64-shipping.lib")
#elif _WIN32
#pragma comment(lib, "D:\\source_ersohn\\win10_libraries\\epicgames\\lib\\eossdk-win32-shipping.lib")
#endif

/*---------------------------------------------------------------------------*/

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

#include "log.h"
}

#if _DEBUG && _MSC_VER
#ifndef _CRTDBG_MAP_ALLOC
#pragma message(__FILE__": Missing _CRT_MAP_ALLOC, recreate: _CRTDBG_MAP_ALLOC + crtdbg.h")
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

/*---------------------------------------------------------------------------*/

static int eos_login_initialized = 0;
static int eos_login_result;

static EOS_EpicAccountId curr_account_id;
static EOS_HAuth auth_handle;

static void NB_EOS_SDK_LoginActionPerformed(const EOS_Auth_LoginCallbackInfo *data)
{
    eos_login_waiting = false;

    eos_login_result = data->ResultCode == EOS_EResult::EOS_Success;

    if (data->ResultCode == EOS_EResult::EOS_Success)
    {
        /* HACK: Shall avoiding changing player name? */

        account_changeable = 0;

        const int num_accounts = EOS_Auth_GetLoggedInAccountsCount(auth_handle);

        for (int i = 0; i < num_accounts; i++)
        {
            EOS_EpicAccountId account_id =
                EOS_Auth_GetLoggedInAccountByIndex(auth_handle, i);

            EOS_ELoginStatus login_status =
                EOS_Auth_GetLoginStatus(auth_handle, data->LocalUserId);

            log_printf("[EOS SDK] [%d] - Account ID: %ls, Status: %d",
                       i, account_id, login_status);
        }

        curr_account_id = data->LocalUserId;
    }
}

static void NB_EOS_SDK_LogoutActionPerformed(const EOS_Auth_LogoutCallbackInfo *data)
{
    if (data->ResultCode != EOS_EResult::EOS_Success)
    {
    }
}

/*---------------------------------------------------------------------------*/

extern "C"
{
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
} steam_account_s[7];

static struct
{
    int        *sym;
    const char *name;
    const int   def;
    int         cur;
} account_d[] = {
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
} account_s[] = {
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
}

/*---------------------------------------------------------------------------*/

extern "C" int account_init(void)
{
    account_busy = 1;

    /* TODO: Find Epic Games logged in account (required) */

    if (!eos_login_initialized)
    {
        EOS_Auth_Credentials cred = { 0 };
        cred.ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;
        cred.Type       = EOS_ELoginCredentialType::EOS_LCT_AccountPortal;

        EOS_Auth_LoginOptions login_opt = { 0 };
        login_opt.ApiVersion = EOS_AUTH_LOGIN_API_LATEST;
        login_opt.ScopeFlags = EOS_EAuthScopeFlags::EOS_AS_BasicProfile |
                               EOS_EAuthScopeFlags::EOS_AS_FriendsList  |
                               EOS_EAuthScopeFlags::EOS_AS_Presence;

        EOS_Auth_Login(auth_handle, &login_opt, 0,
                       NB_EOS_SDK_LoginActionPerformed);

        eos_login_initialized = 1;
    }

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
        steam_account_s[i].curr = strdup(account_s[i].cur);
        dirty = 1;
    }

    account_busy = 0;
    account_is_init = 1;

    return eos_login_result;
}

extern "C" void account_quit(void)
{
    if (!account_is_init) return;

#ifndef NDEBUG
    assert(!account_busy);
#endif

    EOS_Auth_LogoutOptions logout_opt = {0};
    logout_opt.ApiVersion = EOS_AUTH_LOGIN_API_LATEST;
    logout_opt.LocalUserId = curr_account_id;

    EOS_Auth_Logout(auth_handle, &logout_opt, 0,
                    NB_EOS_SDK_LogoutActionPerformed);

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

extern "C" int account_exists(void)
{
    char paths[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(paths, MAXSTR,
#else
    sprintf(paths,
#endif
            "Accounts/account-%s.nbaccount", config_get_s(CONFIG_PLAYER));

#ifndef NDEBUG
    assert(!networking_busy && !config_busy && !accessibility_busy &&
           "This networking, accessibility or configuration is busy and cannot be edit there!");
#endif

    return fs_exists(paths);
}

extern "C" void account_load(void)
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
            "Accounts/account-%s.nbaccount", config_get_s(CONFIG_PLAYER));

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

        for (int i = 0; i < ARRAYSIZE(account_d); i++)
        {
            steam_account_d[i].curr = get_index(fh);
            account_d[i].cur = steam_account_d[i].curr;
        }

        for (int i = 0; i < ARRAYSIZE(account_s); i++)
        {
            char tmp_account_s[MAXSTR];
            get_string(fh, tmp_account_s, MAXSTR);
            account_set_s(i, tmp_account_s);
        }

        fs_close(fh);

        dirty = 0;
        account_busy = 0;
    }
}

extern "C" void account_save(void)
{
    fs_file fh;

#ifndef NDEBUG
    assert(!networking_busy && !config_busy && !accessibility_busy &&
           "This networking, accessibility or configuration is busy and cannot be edit there!");
    SDL_assert(SDL_WasInit(SDL_INIT_VIDEO));
#endif

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
    sprintf_s(paths, MAXSTR,
#else
    sprintf(paths,
#endif
            "Accounts/account-%s.nbaccount", config_get_s(CONFIG_PLAYER));

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

        for (int i = 0; i < ARRAYSIZE(account_d); i++)
            put_index(fh, account_d[i].cur);

        for (int i = 0; i < ARRAYSIZE(account_s); i++)
            put_string(fh, account_s[i].cur);

        fs_close(fh);

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
#ifndef NDEBUG
    assert(!networking_busy && !config_busy && !accessibility_busy &&
           "This networking, accessibility or configuration is busy and cannot be edit there!");
#endif
    if (!networking_busy && !config_busy && !accessibility_busy)
    {
        account_busy = 1;
        account_d[i].cur = d;
        steam_account_d[i].curr = d;
        dirty = 1;
        account_busy = 0;
    }
}

extern "C" void account_tgl_d(int i)
{
#ifndef NDEBUG
    assert(!networking_busy && !config_busy && !accessibility_busy &&
           "This networking, accessibility or configuration is busy and cannot be edit there!");
#endif
    if (!networking_busy && !config_busy && !accessibility_busy)
    {
        account_busy = 1;
        account_d[i].cur = (account_d[i].cur ? 0 : 1);
        steam_account_d[i].curr = (steam_account_d[i].curr ? 0 : 1);
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

/*
 * Set an account option from a string value.
 *
 * Works for both int and string options. Don't use this if you already have an int.
 */
extern "C" void account_set(const char *key, const char *value)
{
    if (key && *key)
    {
        int i;

        for (i = 0; i < ARRAYSIZE(account_s); ++i)
        {
            if (strcmp(account_s[i].name, key) == 0)
                account_set_s(i, value);
        }

        for (i = 0; i < ARRAYSIZE(account_d); ++i)
        {
            if (strcmp(account_d[i].name, key) == 0)
                account_set_d(i, atoi(value));
        }
    }
}

/*---------------------------------------------------------------------------*/

extern "C" void account_set_s(int i, const char *src)
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

        if (steam_account_s[i].curr)
        {
            free(steam_account_s[i].curr);
            steam_account_s[i].curr = NULL;
        }

        account_s[i].cur = strdup(src);
        steam_account_s[i].curr = strdup(src);

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
