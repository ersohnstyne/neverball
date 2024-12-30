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

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
#if NB_HAVE_PB_BOTH==1
#include <PB_Network_Client.h>
#else
#include <NB_Network_Client.h>
#endif

#include "networking.h"
#include "config.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>

#if ENABLE_DEDICATED_SERVER==1
#if _WIN32
#if !_MSC_VER
#error Security compilation error: Use the combined library, \
       that you've compiled from Microsoft Visual Studio!
#else
#pragma message(__FILE__ ": Neverball Dedicated Network for Microsoft Visual Studio")
#endif
#pragma comment(lib, "neverball_net_client.lib")

#pragma comment(lib, "ws2_32.lib")
#else
#error Security compilation error: Dedicated networks requires Windows x86 or x64!
#endif
#endif

#if NB_HAVE_PB_BOTH==1
#define NETCLIENT_TNAME "PBNetClient"
#else
#define NETCLIENT_TNAME "NBNetClient"
#endif

/*---------------------------------------------------------------------------*/

#if ENABLE_DEDICATED_SERVER==1

int SERVER_POLICY_EDITION;
int SERVER_POLICY_LEVELGROUP_ONLY_CAMPAIGN;
int SERVER_POLICY_LEVELGROUP_ONLY_LEVELSET;
int SERVER_POLICY_LEVELGROUP_UNLOCKED_LEVELSET;
int SERVER_POLICY_LEVELSET_ENABLED_BONUS;
int SERVER_POLICY_LEVELSET_ENABLED_CUSTOMSET;
int SERVER_POLICY_LEVELSET_UNLOCKED_BONUS;
int SERVER_POLICY_PLAYMODES_ENABLED;
int SERVER_POLICY_PLAYMODES_ENABLED_MODE_CAREER;
int SERVER_POLICY_PLAYMODES_ENABLED_MODE_CHALLENGE;
int SERVER_POLICY_PLAYMODES_ENABLED_MODE_HARDCORE;
int SERVER_POLICY_PLAYMODES_UNLOCKED_MODE_CAREER;
int SERVER_POLICY_PLAYMODES_UNLOCKED_MODE_HARDCORE;
int SERVER_POLICY_SHOP_ENABLED;
int SERVER_POLICY_SHOP_ENABLED_IAP;
int SERVER_POLICY_SHOP_ENABLED_MANAGED;
int SERVER_POLICY_SHOP_ENABLED_CONSUMABLES;

static int netlib_init;
static int standalone;
static int connected;
static int net_error;

int network_result = 0;
int networking_busy;

/* This is the custom editions. DO NOT EDIT!!! */
static struct
{
    int        *sym;
    const char *name;
    const int   def;
    int         cur;
} server_policy_d[] = {
    { &SERVER_POLICY_EDITION,                  "edition",                  NEVERBALL_EDITION },
    { &SERVER_POLICY_LEVELGROUP_ONLY_CAMPAIGN, "levelgroup_only_campaign", 0 },
    { &SERVER_POLICY_LEVELGROUP_ONLY_LEVELSET, "levelgroup_only_levelset", NEVERBALL_EDITION == -1 ? 1 : 0 },

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    { &SERVER_POLICY_LEVELGROUP_UNLOCKED_LEVELSET, "levelgroup_unlocked_levelset", NEVERBALL_EDITION == 0 || NEVERBALL_EDITION == 1 ? 0 : 1 },
#else
    { &SERVER_POLICY_LEVELGROUP_UNLOCKED_LEVELSET, "levelgroup_unlocked_levelset", 1 },
#endif

    { &SERVER_POLICY_LEVELSET_ENABLED_BONUS,     "levelset_enabled_bonus",     1 },
    { &SERVER_POLICY_LEVELSET_ENABLED_CUSTOMSET, "levelset_enabled_customset", NEVERBALL_EDITION != 0 },
    { &SERVER_POLICY_LEVELSET_UNLOCKED_BONUS,    "levelset_unlocked_bonus",    NEVERBALL_EDITION > 1 },

    { &SERVER_POLICY_PLAYMODES_ENABLED,                "playmodes_enabled",                NEVERBALL_EDITION > -1 },
    { &SERVER_POLICY_PLAYMODES_ENABLED_MODE_CAREER,    "playmodes_enabled_mode_career",    NEVERBALL_EDITION > -1 },
    { &SERVER_POLICY_PLAYMODES_ENABLED_MODE_CHALLENGE, "playmodes_enabled_mode_challenge", NEVERBALL_EDITION != 0 },
    { &SERVER_POLICY_PLAYMODES_ENABLED_MODE_HARDCORE,  "playmodes_enabled_mode_hardcore",  NEVERBALL_EDITION > 0 },
    { &SERVER_POLICY_PLAYMODES_UNLOCKED_MODE_CAREER,   "playmodes_unlocked_career",        NEVERBALL_EDITION > 1 },
    { &SERVER_POLICY_PLAYMODES_UNLOCKED_MODE_HARDCORE, "playmodes_unlocked_hardcore",      NEVERBALL_EDITION > 2 },

    { &SERVER_POLICY_SHOP_ENABLED,             "shop_enabled",             NEVERBALL_EDITION != -1 },
    { &SERVER_POLICY_SHOP_ENABLED_IAP,         "shop_enabled_iap",         NEVERBALL_EDITION > -1 },
    { &SERVER_POLICY_SHOP_ENABLED_MANAGED,     "shop_enabled_managed",     NEVERBALL_EDITION > -1 },
    { &SERVER_POLICY_SHOP_ENABLED_CONSUMABLES, "shop_enabled_consumables", NEVERBALL_EDITION > 0 ? 1 : 0 },
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

struct networking_event
{
    void (*callback)(void *, void *);
    void *callback_data;
    void *extra_data;
};

/* Neverball netfunc system */

static void (*networking_dispatch_event)       (void *) = NULL;
static void (*networking_error_dispatch_event) (int)    = NULL;

/* End Neverball netfunc system */

static SDL_mutex  *networking_mutex;
static SDL_Thread *networking_thread;

static SDL_atomic_t networking_thread_running;

void server_policy_init(void)
{
    /*
     * Store index of each option in its associated config symbol and
     * initialise current values with defaults.
     */

    for (int i = 0; i < ARRAYSIZE(server_policy_d); i++)
    {
        *server_policy_d[i].sym = i;
        server_policy_set_d(i, server_policy_d[i].def);
    }
}

#define CLIENT_IPADDR config_get_s(CONFIG_DEDICATED_IPADDRESS)
#define CLIENT_PORT config_get_s(CONFIG_DEDICATED_IPPORT)

#define CHECK_IPNUM_RANGE(n) \
    (n[0] >= 0 && n[0] <= 255) && \
    (n[1] >= 0 && n[1] <= 255) && \
    (n[2] >= 0 && n[2] <= 255) && \
    (n[3] >= 0 && n[3] <= 255)

static int authenticate_networking()
{
    connected = 0;

    if (text_length(config_get_s(CONFIG_PLAYER)) < 3) return 0;

    int net_port = atoi(CLIENT_PORT);

    if (net_port <= 0 || net_port > 65535) net_port = 5000;

    connected = -1;

    char net_ipv4[MAXSTR];
    SAFECPY(net_ipv4, CLIENT_IPADDR);

    if (PBNetwork_Connect(net_ipv4, net_port))
    {
        if (text_length(config_get_s(CONFIG_PLAYER)) > 2)
            PBNetwork_Login(config_get_s(CONFIG_PLAYER), 0);

        connected = 1;
    }
    else
    {
        log_errorf("Can't connect to server: %s:%d",
                   CLIENT_IPADDR, CLIENT_PORT);

        PBNetwork_Quit();
        connected = 0;
    }

    return (connected = PBNetwork_IsConnected());
}

static int networking_thread_func(void *data)
{
    log_printf("Starting networking thread\n");

    SDL_AtomicSet(&networking_thread_running, 1);

    Uint32 last_time = SDL_GetTicks();
    Uint32 start_time = last_time;

    while (SDL_AtomicGet(&networking_thread_running))
    {
        UINT32 curr_time = SDL_GetTicks();
        int result = PBNetwork_IsConnected();
        int res_connected = 0;

        if (result == 0)
        {
            SDL_LockMutex(networking_mutex);
            PBNetwork_IsChallengePlayable(&res_connected);
            SDL_UnlockMutex(networking_mutex);
            net_error = res_connected;
            networking_error_dispatch_event(res_connected);
            SDL_LockMutex(networking_mutex);
            networking_dispatch_event(0);
            SDL_UnlockMutex(networking_mutex);

            SDL_AtomicSet(&networking_thread_running, 0);
        }
        else if (connected)
        {
            if ((curr_time - last_time) > 0)
            {
                SDL_LockMutex(networking_mutex);
                PBNetwork_Update((float) ((curr_time - start_time) * 0.001f),
                                 (float) ((curr_time - last_time) * 0.001f));
                networking_error_dispatch_event(0);
                SDL_UnlockMutex(networking_mutex);
            }
            SDL_LockMutex(networking_mutex);
            networking_dispatch_event(0);
            SDL_UnlockMutex(networking_mutex);
        }

        last_time = curr_time;
    }

    PBNetwork_Quit();

    log_printf("Stopping networking thread\n");

    return 0;
}

void networking_init_dedicated_event(
    void (*dispatch_event)(void *),
    void (*error_dispatch_event)(int)
)
{
    if (netlib_init == 1)
        networking_quit();

    networking_dispatch_event       = dispatch_event;
    networking_error_dispatch_event = error_dispatch_event;

    SDL_AtomicSet(&networking_thread_running, 1);
    networking_mutex = SDL_CreateMutex();
    networking_thread = SDL_CreateThread(networking_thread_func,
                                         NETCLIENT_TNAME, NULL);
}

int networking_reinit_dedicated_event(void)
{
    if (netlib_init == 1)
        networking_quit();

    connected = -1;
    SDL_AtomicSet(&networking_thread_running, 1);
    networking_mutex = SDL_CreateMutex();
    networking_thread = SDL_CreateThread(networking_thread_func,
                                         NETCLIENT_TNAME, NULL);

    authenticate_networking();

    return (connected = PBNetwork_IsConnected());
}

int networking_init(int support_online)
{
    if (netlib_init == 1)
        networking_quit();

    standalone = 1;
    net_error = 0;

    networking_busy = 1;

#ifndef __EMSCRIPTEN__
    server_policy_init();
#endif
    connected = -1;

    int tmp_res = authenticate_networking();

    if (tmp_res)
        netlib_init = 1;

    networking_busy = 0;

    return tmp_res;
}

void networking_quit(void)
{
    /* Stop the networking! */

    if (netlib_init == 0) return;

    networking_busy = 1;

    PBNetwork_Quit();

    SDL_AtomicSet(&networking_thread_running, 0);

    SDL_WaitThread(networking_thread, NULL);
    networking_thread = NULL;

    SDL_DestroyMutex(networking_mutex);
    networking_mutex = NULL;

    connected = 0;

    networking_busy = 0;

    netlib_init = 0;
}

int networking_connected(void)
{
    connected = PBNetwork_IsConnected() ? 1 : 0;

    if (connected == 0)
        networking_quit();
    else if (PBNetwork_IsWaitingForLogin())
        connected = 2;

    return connected;
}

int networking_standalone(void)
{
    return standalone;
}

int networking_error(void)
{
    return net_error;
}

/*---------------------------------------------------------------------------*/

void server_policy_set_d(int i, int d)
{
    networking_busy = 1;
    server_policy_d[i].cur = d;
    networking_busy = 0;
}

int server_policy_get_d(int i)
{
    return server_policy_d[i].cur;
}

/*---------------------------------------------------------------------------*/

int networking_dedicated_refresh_login(const char *name)
{
    if (PBNetwork_IsConnected())
    {
        if (text_length(config_get_s(CONFIG_PLAYER)) > 2)
            PBNetwork_Login(config_get_s(CONFIG_PLAYER), 0);

        return 1;
    }

    return 0;
}

int networking_dedicated_levelstatus_send(const char *levelmap, int status, float *p)
{
    if (!PBNetwork_IsConnected())
        return 0;

    PBNetwork_PlaceMarker(levelmap, p, status);
    return 1;
}

int networking_dedicated_buyballs_send(int amount)
{
    return 0;
}

#endif
#endif