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

#ifndef ST_SETUP_H
#define ST_SETUP_H

int goto_game_setup(struct state *start_state,
                    int (*new_goto_name_fn) (struct state *,
                                             int         (*new_finish_fn) (struct state *)),
                    int (*new_goto_ball_fn) (struct state *));
int goto_game_setup_finish(struct state *);
int check_game_setup(void);

int game_setup_process(void);

#endif
