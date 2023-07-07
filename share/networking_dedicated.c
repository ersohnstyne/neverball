/*
 * Copyright (C) 2023 Microsoft / Neverball authors
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

#include <NB_Network_Client.h>
#include <PB_Network_Client.h>

#include "networking.h"
#include "config.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>

#if ENABLE_DEDICATED_SERVER==1
#if _WIN32
#if !_MSC_VER
#error Use the combined library, that you've compiled from Microsoft Visual Studio!
#else
#pragma message("Pennyball + Neverball Dedicated Network for Microsoft Visual Studio")
#endif
#pragma comment(lib, "neverball_net_client.lib")

#pragma comment(lib, "ws2_32.lib")
#else
#error Dedicated networks requires Windows x86 or x64!
#endif
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
    { &SERVER_POLICY_EDITION, "edition", PENNYBALL_EDITION },
    { &SERVER_POLICY_LEVELGROUP_ONLY_CAMPAIGN, "levelgroup_only_campaign", 0 },
    { &SERVER_POLICY_LEVELGROUP_ONLY_LEVELSET, "levelgroup_only_levelset", PENNYBALL_EDITION==-1 ? 1 : 0 },

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    { &SERVER_POLICY_LEVELGROUP_UNLOCKED_LEVELSET, "levelgroup_unlocked_levelset", PENNYBALL_EDITION==0 || PENNYBALL_EDITION==1 ? 0 : 1 },
#else
    { &SERVER_POLICY_LEVELGROUP_UNLOCKED_LEVELSET, "levelgroup_unlocked_levelset", 1 },
#endif

    { &SERVER_POLICY_LEVELSET_ENABLED_BONUS, "levelset_enabled_bonus", 1 },
    { &SERVER_POLICY_LEVELSET_ENABLED_CUSTOMSET, "levelset_enabled_customset", PENNYBALL_EDITION!=0 },
    { &SERVER_POLICY_LEVELSET_UNLOCKED_BONUS, "levelset_unlocked_bonus", PENNYBALL_EDITION>1 },

    { &SERVER_POLICY_PLAYMODES_ENABLED, "playmodes_enabled", PENNYBALL_EDITION>-1 },
    { &SERVER_POLICY_PLAYMODES_ENABLED_MODE_CAREER, "playmodes_enabled_mode_career", PENNYBALL_EDITION>-1 },
    { &SERVER_POLICY_PLAYMODES_ENABLED_MODE_CHALLENGE, "playmodes_enabled_mode_challenge", PENNYBALL_EDITION!=0 },
    { &SERVER_POLICY_PLAYMODES_ENABLED_MODE_HARDCORE, "playmodes_enabled_mode_hardcore", PENNYBALL_EDITION>0 },
    { &SERVER_POLICY_PLAYMODES_UNLOCKED_MODE_CAREER, "playmodes_unlocked_career", PENNYBALL_EDITION>1 },
    { &SERVER_POLICY_PLAYMODES_UNLOCKED_MODE_HARDCORE, "playmodes_unlocked_hardcore", PENNYBALL_EDITION>2 },

    { &SERVER_POLICY_SHOP_ENABLED, "shop_enabled", PENNYBALL_EDITION!=-1 },
    { &SERVER_POLICY_SHOP_ENABLED_IAP, "shop_enabled_iap", PENNYBALL_EDITION>-1 },
    { &SERVER_POLICY_SHOP_ENABLED_MANAGED, "shop_enabled_managed", PENNYBALL_EDITION>-1 },
    { &SERVER_POLICY_SHOP_ENABLED_CONSUMABLES, "shop_enabled_consumables", PENNYBALL_EDITION>0 ? 1 : 0 },
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

struct networking_event
{
    void (*callback)(void *, void *);
    void *callback_data;
    void *extra_data;
};

/* Neverball netfunc system */

static void (*networking_dispatch_event)(void *) = NULL;
static void (*networking_error_dispatch_event)(int) = NULL;

/* End Neverball netfunc system */

static SDL_mutex *networking_mutex;
static SDL_Thread *networking_thread;

static SDL_atomic_t networking_thread_running;

void server_policy_set_d(int i, int d);

static void server_policy_init(void)
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

    if (strlen(config_get_s(CONFIG_PLAYER)) < 3) return 0;

    int net_port = atoi(CLIENT_PORT);

    if (net_port <= 0 || net_port > 65535) net_port = 5000;

    connected = -1;

#if _MSC_VER
    /*char hostname[MAXSTR], net_ipv4[MAXSTR];
    SAFECPY(hostname, CLIENT_IPADDR);

    int ipap[4];
    if (sscanf(CLIENT_IPADDR, "%d.%d.%d.%d", &ipap[0], &ipap[1], &ipap[2], &ipap[3]) != 4)
    {
        WSADATA wsData;
        int res;
        if ((res = WSAStartup(MAKEWORD(2, 2), &wsData)) != 0)
        {
            log_errorf("Can't launch the WSA\n");
            return (connected = 0);
        }

        struct hostent *host_info;
        struct in_addr addr;
        DWORD dw;

        host_info = gethostbyname(hostname);

        if (host_info == NULL)
        {
            dw = WSAGetLastError();
            if (dw != 0)
            {
                if (dw == WSAHOST_NOT_FOUND)
                {
                    log_errorf("Host is not found\n");
                    WSACleanup();
                    return (connected = 0);
                }
                else if (dw == WSANO_DATA)
                {
                    log_errorf("No data record is found");
                    WSACleanup();
                    return (connected = 0);
                }
                else
                {
                    log_errorf("Function failed with an error: %ul", dw);
                    WSACleanup();
                    return (connected = 0);
                }
            }
        }
        else
        {
            addr.s_addr = *(unsigned long *) host_info->h_addr_list[0];
            log_printf("Hostname: %s; IPv4: %s\n", host_info->h_name, inet_ntoa(addr));
            SAFECPY(net_ipv4, inet_ntoa(addr));
        }

        WSACleanup();
        Sleep(3000);
    }
    else if (CHECK_IPNUM_RANGE(ipap))
        SAFECPY(net_ipv4, hostname);
    else return (connected = 0);*/
#else
    char net_ipv4[MAXSTR];
    SAFECPY(net_ipv4, CLIENT_IPADDR);
#endif
    
    if (PBNetwork_Connect(net_ipv4, net_port) == 1)
    {
        Sleep(1000);

        if (strlen(config_get_s(CONFIG_PLAYER)) > 2)
            PBNetwork_Login(config_get_s(CONFIG_PLAYER), 0);

        connected = 1;
    }
    
    if (PBNetwork_IsConnected() == 0)
    {
        log_errorf("Can't connect to server: %s:%d",
                   CLIENT_IPADDR,
                   CLIENT_PORT);

        PBNetwork_Quit();
        connected = 0;
    }

    return (connected = PBNetwork_IsConnected());
}

static int networking_thread_func(void *data)
{
    SDL_AtomicSet(&networking_thread_running, authenticate_networking());

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
        else
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

    return 0;
}

void networking_init_dedicated_event(
    void (*dispatch_event)(void *),
    void (*error_dispatch_event)(int)
)
{
    if (netlib_init == 1)
        networking_quit();

    networking_dispatch_event = dispatch_event;
    networking_error_dispatch_event = error_dispatch_event;

    SDL_AtomicSet(&networking_thread_running, 1);
    networking_mutex = SDL_CreateMutex();
    networking_thread = SDL_CreateThread(networking_thread_func,
                                         "networking", NULL);
}

int networking_reinit_dedicated_event(void)
{
    if (netlib_init == 1)
        networking_quit();

    connected = -1;
    SDL_AtomicSet(&networking_thread_running, 1);
    networking_mutex = SDL_CreateMutex();
    networking_thread = SDL_CreateThread(networking_thread_func,
                                         "networking", NULL);

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

    server_policy_init();
    connected = -1;

    authenticate_networking();

    networking_busy = 0;

    int tmp_res = PBNetwork_IsConnected();

    if (tmp_res)
        netlib_init = 1;

    return tmp_res;
}

void networking_quit(void)
{
    /* Stop the networking! */

    if (netlib_init == 0) return;

    networking_busy = 1;

    PBNetwork_Quit();
    SDL_AtomicSet(&networking_thread_running, 0);

    if (networking_thread)
        SDL_WaitThread(networking_thread, NULL);
    networking_thread = NULL;
        
    if (networking_mutex)
        SDL_DestroyMutex(networking_mutex);
    networking_mutex = NULL;

    networking_busy = 0;

    netlib_init = 0;
    connected = 0;
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
        if (strlen(config_get_s(CONFIG_PLAYER)) > 2)
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