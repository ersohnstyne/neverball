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

#ifndef NETWORKING_H
#define NETWORKING_H

/* Should be use within server file manager ? */
#define ENABLE_NETWORKING

// Should be required as connected server?
//#define NETWORKING_NO_STANDALONE

#define CHECK_ACCOUNT_ENABLED (networking_error() > -3)

extern int networking_busy;

extern int SERVER_POLICY_EDITION;
extern int SERVER_POLICY_LEVELGROUP_ONLY_CAMPAIGN;
extern int SERVER_POLICY_LEVELGROUP_ONLY_LEVELSET;
extern int SERVER_POLICY_LEVELGROUP_UNLOCKED_LEVELSET;
extern int SERVER_POLICY_LEVELSET_ENABLED_BONUS;
extern int SERVER_POLICY_LEVELSET_ENABLED_CUSTOMSET;
extern int SERVER_POLICY_LEVELSET_UNLOCKED_BONUS;
extern int SERVER_POLICY_PLAYMODES_ENABLED;
extern int SERVER_POLICY_PLAYMODES_ENABLED_MODE_CAREER;
extern int SERVER_POLICY_PLAYMODES_ENABLED_MODE_CHALLENGE;
extern int SERVER_POLICY_PLAYMODES_ENABLED_MODE_HARDCORE;
extern int SERVER_POLICY_PLAYMODES_UNLOCKED_MODE_CAREER;
extern int SERVER_POLICY_PLAYMODES_UNLOCKED_MODE_HARDCORE;
extern int SERVER_POLICY_SHOP_ENABLED;
extern int SERVER_POLICY_SHOP_ENABLED_IAP;
extern int SERVER_POLICY_SHOP_ENABLED_MANAGED;
extern int SERVER_POLICY_SHOP_ENABLED_CONSUMABLES;

#if ENABLE_DEDICATED_SERVER==1
void networking_init_dedicated_event(
    void (*dispatch_event)(void *),
    void (*error_dispatch_event)(int)
);
int networking_reinit_dedicated_event(void);
#endif

int networking_init(int);
void networking_quit(void);
int networking_connected(void);
int networking_standalone(void);
int networking_error(void);

int server_policy_get_d(int);

#if ENABLE_DEDICATED_SERVER==1
int networking_dedicated_refresh_login(const char *);
int networking_dedicated_levelstatus_send(const char *, int, float *);
int networking_dedicated_buyballs_send(int);
#endif

#endif