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

#ifndef ST_PLAY_H
#define ST_PLAY_H

extern struct state st_play_ready;
extern struct state st_play_set;
extern struct state st_play_loop;
extern struct state st_look;

int play_pause_goto(struct state *);

void wgcl_play_touch_pause(void);
void wgcl_play_touch_cammode(void);
void wgcl_play_touch_rotate_camera(int, int);
void wgcl_play_touch_zoom_camera(int);
void wgcl_play_devicemotion_tilt(int, int);

#endif
