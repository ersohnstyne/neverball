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

#define ACCOUNT_WGCL_RESTART_POST_ATTEMPT_THEN_GOTOSTATE(next_state, from, to)  \
    if (account_wgcl_restart_attempt()) goto_state(next_state, from, to, 0); \

int  account_wgcl_init(void);
void account_wgcl_quit(void);
int  account_wgcl_exists(void);
void account_wgcl_load(void);
void account_wgcl_save(void);

int  account_wgcl_reload(void);
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