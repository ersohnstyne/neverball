/*
 * Copyright (C) 2023 Microsoft / Neverball authors
 *
 * PENNYBALL is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */

#ifndef ST_TRANSFER_H
#define ST_TRANSFER_H

#include "state.h"

#define ENABLE_GAME_TRANSFER 0
#define GAME_TRANSFER_TARGET 1

#if GAME_TRANSFER_TARGET==0
void transfer_add_dispatch_event(void (*request_addreplay_dispatch_event)(int status_limit));
void transfer_addreplay(const char *path);
void transfer_addreplay_exceeded(void);
#endif

#if ENABLE_GAME_TRANSFER==1
extern struct state st_transfer;
#endif

#endif