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

#ifndef JOY_H
#define JOY_H

int  joy_init(void);
void joy_quit(void);

void joy_add(int device);
void joy_remove(int instance);
int  joy_button(int instance, int b, int d);
void joy_axis(int instance, int a, float v);

void joy_active_cursor(int instance, int active);
int  joy_get_active_cursor(int instance);

int  joy_get_gamepad_action_index(void);
int  joy_get_cursor_actions(int instance);

int  joy_connected(int instance, int *battery_level, int *wired);

#if NB_PB_WITH_XBOX==1
void joy_update(void);
#endif

#endif