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

#ifndef CHECKPOINTS_H
#define CHECKPOINTS_H

#if NB_HAVE_PB_BOTH==1
#include "solid_chkp.h"
#include "game_common.h"
#endif

#ifdef MAPC_INCLUDES_CHKP

struct chkp_view
{
    float dc;                           /* Ideal view center distance above ball */
    float dp;                           /* Ideal view position distance above ball */
    float dz;                           /* Ideal view position distance behind ball */

    float c[3];                         /* Current view center               */
    float p[3];                         /* Current view position             */
    float e[3][3];                      /* Current view reference frame      */

    float a;                            /* Ideal view rotation about Y axis  */
};

struct chkp_ballsize
{
    int curr_size;
};

struct chkp_ball
{
    float e[3][3];
    float p[3];
    float E[3][3];
    float r;
    float r_vel;
    float sizes[3];
    short size;
};

struct chkp_path
{
    int f;

    int mi;
    int mj;
};

struct chkp_body
{
    int mi, mj;
};

struct chkp_move
{
    float t;
    int tm;

    int pi;

    struct vec3 pos;                           /* cached position            */
    struct vec4 rot;                           /* cached orientation         */

    unsigned int dirty : 1;
};

struct chkp_item
{
    float p[3];
    int   t;
    int   n;

    int mi;
    int mj;
};

struct chkp_swch
{
    float t;
    int   tm;
    int   f;

    int mi;
    int mj;
};

struct chkp_goal
{
    int mi;
    int mj;
};

struct chkp_jump
{
    int mi;
    int mj;
};

struct chkp_bill
{
    int mi;
    int mj;
};

struct chkp_chkp
{
    int mi;
    int mj;
};

extern int last_active;
extern int checkpoints_busy;

extern struct chkp_ball last_chkp_ball[1024];

extern struct chkp_view last_view[1024];

extern float last_pos[1024][3];

/*---------------------------------------------------------------------------*/

typedef void (*cmd_fn_chkp)(const union cmd *);

void checkpoints_save_spawnpoint(struct s_vary,
                                 struct game_view,
                                 int);

void checkpoints_save_last_data(float, float, int);

int   checkpoints_last_coins(void);
float checkpoints_last_time_elapsed(void);
float checkpoints_last_time_limit(void);

/*---------------------------------------------------------------------------*/

int checkpoints_load(void);

void checkpoints_respawn(struct s_vary *, cmd_fn_chkp, int *ci);

int   checkpoints_respawn_coins(void);
float checkpoints_respawn_time_elapsed(void);
float checkpoints_respawn_time_limit(void);

/*---------------------------------------------------------------------------*/

/*
 * Please disable all checkpoints,
 * before you attempt to restart the level.
 */
void checkpoints_stop(void);

#endif
#endif
