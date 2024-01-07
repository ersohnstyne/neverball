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

#ifndef SCORE_ONLINE_H
#define SCORE_ONLINE_H

#if NB_STEAM_API==1
void score_steam_init_hs(int, int);
int  score_steam_hs_loading(void);
int  score_steam_hs_online(void);

void score_steam_hs_save(int, int);
#endif

#endif