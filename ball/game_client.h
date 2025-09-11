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

#ifndef GAME_CLIENT_H
#define GAME_CLIENT_H

#include "fs.h"

/*---------------------------------------------------------------------------*/

enum
{
    POSE_NONE = 0,
    POSE_LEVEL,
    POSE_BALL
};

int game_client_init(const char *);

#if ENABLE_MOON_TASKLOADER!=0
int game_client_load_moon_taskloader(void *, void *);
int game_client_init_moon_taskloader(const char *,
                                     struct moon_taskloader_callback);
#endif

void  game_client_toggle_show_balls(int);
void  game_client_free(const char *);
void  game_client_sync(fs_file);
void  game_client_draw(int, float);
void  game_client_blend(float);
void  game_client_maxspeed(float, int);

int   curr_viewangle(void);
int   curr_clock(void);
int   curr_coins(void);
int   curr_status(void);
float curr_speedometer(void);

void  game_look(float, float);
void  game_look_v2(float, float, float, float, float);

void  game_disable_fade(int);
void  game_kill_fade(void);
void  game_step_fade(float);
void  game_fade(float);
void  game_fade_color(float, float, float);

void  game_client_fly(float);

void  game_client_step_studio(float);
int   game_client_init_studio(int);

int   game_client_get_jump_b(void);

/*---------------------------------------------------------------------------*/

extern int game_compat_map;
extern int game_compat_campaign;

/*---------------------------------------------------------------------------*/

#endif
