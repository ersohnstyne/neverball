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

#ifndef ST_TRANSFER_H
#define ST_TRANSFER_H

#include "state.h"

//#define ENABLE_GAME_TRANSFER 1
#if NB_HAVE_PB_BOTH==1
#define GAME_TRANSFER_TARGET
#endif

#ifndef GAME_TRANSFER_TARGET
void transfer_add_dispatch_event(void (*request_addreplay_dispatch_event) (int status_limit));
void transfer_addreplay(const char *path);
void transfer_addreplay_unsupported(void);
void transfer_addreplay_exceeded(void);
#endif

#if ENABLE_GAME_TRANSFER==1
int goto_game_transfer(struct state*);
#endif

#endif