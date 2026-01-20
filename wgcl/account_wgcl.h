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

#ifndef ACCOUNT_WGCL_H
#define ACCOUNT_WGCL_H

#include "state.h"

#define ACCOUNT_WGCL_RESTART_POST_ATTEMPT_THEN_GOTOSTATE(next_state, from, to)  \
    if (account_wgcl_restart_attempt()) goto_state(next_state, from, to, 0); \

int  account_wgcl_init(void);
void account_wgcl_quit(void);
int  account_wgcl_exists(void);
void account_wgcl_load(void);
void account_wgcl_save(void);

void account_wgcl_autokick_func_prepare(int (*kick_fn) (void));
void account_wgcl_autokick_state_prepare(struct state *st);
void account_wgcl_autokick_state_ignore(void);

int  account_wgcl_reload(void);
void account_wgcl_set_session_uuid4(const char *uuid4);
void account_wgcl_set_readonly_playername(const char *f);
int  account_wgcl_name_read_only(void);

int  account_wgcl_login(const char *name, const char *password);
int  account_wgcl_logout(void);

int account_wgcl_try_add(int w_coins, int w_gems,
                         int c_hp, int c_doublecash, int c_halfgrav, int c_doublespeed);
int account_wgcl_try_set(int w_coins, int w_gems,
                         int c_hp, int c_doublecash, int c_halfgrav, int c_doublespeed);
int account_wgcl_restart_attempt(void);

void account_wgcl_do_add(int w_coins, int w_gems,
                         int c_hp, int c_doublecash, int c_halfgrav, int c_doublespeed);
void account_wgcl_do_set(int w_coins, int w_gems,
                         int c_hp, int c_doublecash, int c_halfgrav, int c_doublespeed);
int  account_wgcl_do_buy(int w_coins_cost, int flags);

void account_wgcl_post_sync(const char *, const char *);

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
int account_wgcl_mapmarkers_place(const char *, int, int, int, int);
#endif

void WGCL_KickScreenState(void);

#endif