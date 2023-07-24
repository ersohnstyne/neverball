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

#if _WIN32 && __MINGW32__
#include <SDL3/SDL.h>
#else
#include <SDL.h>
#endif

#include "networking.h"
#include "common.h"

#if NEVERBALL_EDITION>=10000
#if NEVERBALL_EDITION>10002
#error Präprozessordefinition NEVERBALL_EDITION muss zwischen 10000 (Server Essentials) und 10002 (Server Datacenter) liegen
#endif
#else
#if NEVERBALL_EDITION<-1 || NEVERBALL_EDITION>3
#error Präprozessordefinition NEVERBALL_EDITION muss zwischen -1 (Open-Source) und 3 (Education) liegen
#endif
#endif

#if NEVERBALL_EDITION==-1 && ENABLE_DEDICATED_SERVER==0

/*---------------------------------------------------------------------------*/

void server_policy_set_d(int i, int d);

/*---------------------------------------------------------------------------*/

int network_result = 0;
int networking_busy;

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

static int standalone;
static int connected;
static int net_error;

/* This is the custom editions. DO NOT EDIT!!! */
static struct
{
    int        *sym;
    const char *name;
    const int   def;
    int         cur;
} server_policy_d[] = {
    { &SERVER_POLICY_EDITION, "edition", -1 },
    { &SERVER_POLICY_LEVELGROUP_ONLY_CAMPAIGN, "levelgroup_only_campaign", 0 },
    { &SERVER_POLICY_LEVELGROUP_ONLY_LEVELSET, "levelgroup_only_levelset", 1 },

    { &SERVER_POLICY_LEVELGROUP_UNLOCKED_LEVELSET, "levelgroup_unlocked_levelset", 1 },

    { &SERVER_POLICY_LEVELSET_ENABLED_BONUS, "levelset_enabled_bonus", 1 },
    { &SERVER_POLICY_LEVELSET_ENABLED_CUSTOMSET, "levelset_enabled_customset", 1 },
    { &SERVER_POLICY_LEVELSET_UNLOCKED_BONUS, "levelset_unlocked_bonus", 0 },

    { &SERVER_POLICY_PLAYMODES_ENABLED, "playmodes_enabled", 1 },
    { &SERVER_POLICY_PLAYMODES_ENABLED_MODE_CAREER, "playmodes_enabled_mode_career", 1 },
    { &SERVER_POLICY_PLAYMODES_ENABLED_MODE_CHALLENGE, "playmodes_enabled_mode_challenge", 1 },
    { &SERVER_POLICY_PLAYMODES_ENABLED_MODE_HARDCORE, "playmodes_enabled_mode_hardcore", 0 },
    { &SERVER_POLICY_PLAYMODES_UNLOCKED_MODE_CAREER, "playmodes_unlocked_career", 1 },
    { &SERVER_POLICY_PLAYMODES_UNLOCKED_MODE_HARDCORE, "playmodes_unlocked_hardcore", 0 },

    { &SERVER_POLICY_SHOP_ENABLED, "shop_enabled", 0 },
    { &SERVER_POLICY_SHOP_ENABLED_IAP, "shop_enabled_iap", 0 },
    { &SERVER_POLICY_SHOP_ENABLED_MANAGED, "shop_enabled_managed", 0 },
    { &SERVER_POLICY_SHOP_ENABLED_CONSUMABLES, "shop_enabled_consumables", 0 },
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

int networking_init(int support_online)
{
    standalone = 1;
    net_error = 0;
    network_result = 1;

    server_policy_init();

    connected = 1;

    return network_result;
}

void networking_quit(void)
{
    connected = 0;
}

int networking_connected(void)
{
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

#endif