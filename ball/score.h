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

#ifndef SCORE_H
#define SCORE_H

#include "base_config.h"

/*---------------------------------------------------------------------------*/

enum
{
    RANK_MIN = -1,

    RANK_HARD,
    RANK_MEDM,
    RANK_EASY,
    RANK_LAST,

    RANK_MAX
};

struct score
{
    char player[RANK_MAX][MAXSTR];

    int  timer[RANK_MAX];               /* Time elapsed                      */
    int  coins[RANK_MAX];               /* Coins collected                   */
};

/*---------------------------------------------------------------------------*/

void score_init_hs(struct score *, int, int);

void score_time_insert(struct score *, int *, const char *, int, int);
void score_coin_insert(struct score *, int *, const char *, int, int);

/*---------------------------------------------------------------------------*/

#endif
