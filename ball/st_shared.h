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

#ifndef ST_SHARED_H
#define ST_SHARED_H

#include "state.h"
#include "util.h"

void shared_leave(struct state *, struct state *next, int id);
void shared_paint(int id, float st);
void shared_timer(int id, float dt);
int  shared_point_basic(int id, int x, int y);
void shared_point(int id, int x, int y, int dx, int dy);
int  shared_stick_basic(int id, int a, float v, int bump);
void shared_stick(int id, int a, float v, int bump);
void shared_angle(int id, float x, float z);
int  shared_click_basic(int b, int d);
int  shared_click(int b, int d);

#endif
