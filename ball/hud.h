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

#ifndef HUD_H
#define HUD_H

#include "SDL_events.h"

/*---------------------------------------------------------------------------*/

void toggle_hud_visibility(int);
void toggle_hud_visibility_expected(int);
int  hud_visibility(void);

void hud_init(void);
void hud_free(void);

void hud_set_alpha(float);
void hud_paint(void);
void hud_timer(float);
void hud_update(int, float);
void hud_update_camera_direction(float);

void hud_show(float delay);
void hud_hide(void);

<<<<<<< HEAD
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__)
=======
>>>>>>> 99ad14964f477526e558bffa1be6a7732d5d3a83
int hud_touch(const SDL_TouchFingerEvent *);
#endif

void hud_speedup_reset(void);
void hud_speedup_pulse(void);
void hud_speedup_timer(float);
void hud_speedup_paint(void);

void hud_lvlname_campaign(const char *, int);
void hud_lvlname_set     (const char *, int);
void hud_lvlname_set_ana (const char *, int);
void hud_lvlname_paint   (void);

void hud_cam_pulse(int);
void hud_cam_timer(float);
void hud_cam_paint(void);

void hud_speed_pulse(int);
void hud_speed_timer(float);
void hud_speed_paint(void);

void hud_touch_timer(float);
void hud_touch_paint(void);

/*---------------------------------------------------------------------------*/

#endif
