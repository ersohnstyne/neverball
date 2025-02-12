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

#ifndef ACCOUNT_TRANSFER_H
#define ACCOUNT_TRANSFER_H

#define CONFIG_INCLUDES_ACCOUNT

#include "base_config.h"

extern int account_transfer_busy;

extern int ACCOUNT_TRANSFER_DATA_WALLET_COINS;
extern int ACCOUNT_TRANSFER_DATA_WALLET_GEMS;
extern int ACCOUNT_TRANSFER_PRODUCT_LEVELS;
extern int ACCOUNT_TRANSFER_PRODUCT_BALLS;
extern int ACCOUNT_TRANSFER_PRODUCT_BONUS;
extern int ACCOUNT_TRANSFER_PRODUCT_MEDIATION;
extern int ACCOUNT_TRANSFER_SET_UNLOCKS;
extern int ACCOUNT_TRANSFER_CONSUMEABLE_EARNINATOR;
extern int ACCOUNT_TRANSFER_CONSUMEABLE_FLOATIFIER;
extern int ACCOUNT_TRANSFER_CONSUMEABLE_SPEEDIFIER;
extern int ACCOUNT_TRANSFER_CONSUMEABLE_EXTRALIVES;

extern int ACCOUNT_TRANSFER_PLAYER;
extern int ACCOUNT_TRANSFER_BALL_FILE;

int  account_transfer_init(void);
void account_transfer_quit(void);
int  account_transfer_exists(void);
void account_transfer_load(const char *paths);
void account_transfer_load_externals(const char *paths);
void account_transfer_save(const char *playername);

void account_transfer_set_d(int, int);
void account_transfer_tgl_d(int);
int  account_transfer_tst_d(int, int);
int  account_transfer_get_d(int);

void        account_transfer_set_s(int, const char *);
const char *account_transfer_get_s(int);

#endif