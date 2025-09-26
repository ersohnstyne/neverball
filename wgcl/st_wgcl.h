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

#ifndef ST_WGCL_H
#define ST_WGCL_H

extern struct state st_wgcl_error_offline;

int goto_wgcl_login(struct state *back_state, int (*back_fn)(struct state *),
                    struct state *next_state, int (*next_fn)(struct state *));

int goto_wgcl_logout(struct state *);

int goto_wgcl_addons_login(int id, struct state *back_state, int (back_fn) (struct state *));

#endif
