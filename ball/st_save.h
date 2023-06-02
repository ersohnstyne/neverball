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

#ifndef ST_SAVE_H
#define ST_SAVE_H

#include "state.h"

/*
 * HACK: Does not makes any sense
 * 
 * extern struct state st_save;
 * extern struct state st_clobber;
 * extern struct state st_lockdown;
 * extern struct state st_save_error;
 */

int goto_save(struct state *, struct state *);

#endif
