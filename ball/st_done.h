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

#ifndef ST_DONE_H
#define ST_DONE_H

#include "state.h"

/*
 * Player wants to save and open the Leaderboard.
 */
#if NB_HAVE_PB_BOTH==1
#define LEADERBOARD_ALLOWANCE
#endif

extern struct state st_done;
extern struct state st_capital;

#endif
