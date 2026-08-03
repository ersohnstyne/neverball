/*
 * Copyright (C) 2026 Microsoft / Neverball authors / Ersohn Styne
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

#ifndef GAME_SWITCHBALL_H
#define GAME_SWITCHBALL_H

int game_switchball_installed(void);

void game_switchball_set_fixed_altitude(float);
void game_switchball_set_speeding(int);
void game_switchball_ignore_speeding(void);

float game_switchball_altitude(void);
int   game_switchball_speeding(void);

void game_switchball_toggle_ticks(int);
int  game_switchball_haveticks(void);

#endif