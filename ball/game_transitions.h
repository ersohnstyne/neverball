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

#ifndef GAME_TRANSITIONS_H
#define GAME_TRANSITIONS_H

/*---------------------------------------------------------------------------*/

int  game_transitions_init(void);
void game_transitions_quit(void);

int  game_transitions_available(void);

/*
 * Step alpha fade for each ticks: 1000ms = 1s
 */
void game_transitions_step_fade(float);

void game_transitions_fade_color(float, float, float);
void game_transitions_fade(float);
void game_transitions_fade_in(float);

void game_transitions_draw(struct s_rend *);

int game_transitions_fadeout_finished(void);

/*
 * Set z-order overlay for game transition
 *
 * 0 = Game Scene Only
 * 1 = All
 */
void game_transitions_set_zorder(int);
int  game_transitions_get_zorder(void);

void game_transitions_prep_scene(void);
void game_transitions_post_scene(void);

#endif