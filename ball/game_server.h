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

#ifndef GAME_SERVER_H
#define GAME_SERVER_H

/*---------------------------------------------------------------------------*/

enum
{
    SERVER_CAMDIRECTION_N,
    SERVER_CAMDIRECTION_NE,
    SERVER_CAMDIRECTION_E,
    SERVER_CAMDIRECTION_SE,
    SERVER_CAMDIRECTION_S,
    SERVER_CAMDIRECTION_SW,
    SERVER_CAMDIRECTION_W,
    SERVER_CAMDIRECTION_NW
};

#define RESPONSE    0.05f              /* Input smoothing time               */
#define ANGLE_BOUND 20.0f              /* Angle limit of floor tilting       */
#define AUTO_FORW   17.5f              /* Automatic forward of floor tilting */
#define VIEWR_BOUND 10.0f              /* Maximum rate of view rotation      */

#define AUTO_ROTATE_SPEED 0.025f        /* New: Automatic camera              */

/*---------------------------------------------------------------------------*/

int   game_server_init(const char *, int, int);
void  game_server_free(const char *);
void  game_server_step(float);
float game_server_blend(void);

void  game_set_goal(void);

void  game_set_ang(float, float);
void  game_set_pos(int, int);
void  game_set_pos_max_speed(int, int);
void  game_set_x  (float);
void  game_set_z  (float);
void  game_set_cam(int);
void  game_set_rot(float);

float game_get_ballspeed(void);        /* New: Speedometer                   */

void game_extend_time(float);          /* New: Time Extension                */

void game_set_zoom(float);             /* New: Zoom                          */

/*---------------------------------------------------------------------------*/

#endif
