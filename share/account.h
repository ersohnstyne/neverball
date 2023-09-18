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

#ifndef ACCOUNT_H
#define ACCOUNT_H

#define CONFIG_INCLUDES_ACCOUNT

#define ACCOUNT_WALLET_MAX_COINS 1000000

#if NB_STEAM_API==1 && NB_EOS_SDK==1
#error Steam API and EOS SDK should not built at same time!
#endif

#include "base_config.h"

extern int account_changeable;
extern int account_busy;

extern int ACCOUNT_DATA_WALLET_COINS;
extern int ACCOUNT_DATA_WALLET_GEMS;
extern int ACCOUNT_PRODUCT_LEVELS;
extern int ACCOUNT_PRODUCT_BALLS;
extern int ACCOUNT_PRODUCT_BONUS;
extern int ACCOUNT_PRODUCT_MEDIATION;
extern int ACCOUNT_SET_UNLOCKS;
extern int ACCOUNT_CONSUMEABLE_EARNINATOR;
extern int ACCOUNT_CONSUMEABLE_FLOATIFIER;
extern int ACCOUNT_CONSUMEABLE_SPEEDIFIER;
extern int ACCOUNT_CONSUMEABLE_EXTRALIVES;

extern int ACCOUNT_PLAYER;
extern int ACCOUNT_BALL_FILE;

#define CHECK_ACCOUNT_BANKRUPT \
    (account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES) < 0)

#define LOG_ACCOUNT_BANKRUPT \
    if (account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES) < 0) \
        log_errorf("Player account is bankrupt!: %s\n", account_get_s(ACCOUNT_PLAYER)); \
    else \
        log_printf("Player is not bankrupt!: %s\n", account_get_s(ACCOUNT_PLAYER))

int  account_init(void);
void account_quit(void);
int  account_exists(void);
void account_load(void);
void account_save(void);

void account_set_d(int, int);
void account_tgl_d(int);
int  account_tst_d(int, int);
int  account_get_d(int);

void        account_set_s(int, const char *);
const char *account_get_s(int);

#endif