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

#ifndef ST_SHOP_H
#define ST_SHOP_H

#include "state.h"

extern struct state st_shop;
extern struct state st_shop_unlocked;

int goto_shop_rename(struct state *, struct state *, unsigned int);
int goto_shop_iap(struct state *, struct state *,
                  int (*new_ok_fn)(struct state *), int (*new_cancel_fn)(struct state *),
                  int, int, int);

int goto_shop_activate(struct state* ok, struct state* cancel,
                       int (*new_ok_fn)(struct state*), int (*new_cancel_fn)(struct state*));

#endif
