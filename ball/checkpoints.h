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
    float size_orig;
    int size_state;
};

struct chkp_ball
{
    float e[3][3];
    float p[3];
    float E[3][3];
    float r;
};

struct chkp_path
{
    int f;
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
};

struct chkp_item
{
    float p[3];
    int t;
    int n;
};

struct chkp_swch
{
    float t;
    int   tm;
    int   f;
};

extern int last_active;
extern int checkpoints_busy;

extern struct chkp_ballsize last_chkp_ballsize[1024];
extern struct chkp_ball last_chkp_ball[1024];
// extern struct chkp_path last_chkp_path[2048];  /* Deprecated. DO NOT USE! */
// extern struct chkp_body last_chkp_body[1024];  /* Deprecated. DO NOT USE! */
// extern struct chkp_move last_chkp_move[1024];  /* Deprecated. DO NOT USE! */
// extern struct chkp_item last_chkp_item[2048];  /* Deprecated. DO NOT USE! */
// extern struct chkp_swch last_chkp_swch[1024];  /* Deprecated. DO NOT USE! */

extern struct chkp_view last_view[1024];

extern float last_pos[1024][3];

extern float last_timer;
extern int   last_timer_down;

extern int   respawn_coins;
extern float respawn_timer;
extern int   respawn_gained;

/*---------------------------------------------------------------------------*/

typedef void (*cmd_fn_chkp)(const union cmd *);

void checkpoints_save_spawnpoint(struct s_vary,
                                 struct game_view,
                                 int);

int checkpoints_load(void);

void checkpoints_respawn(struct s_vary *, cmd_fn_chkp, int *ci);

int checkpoints_respawn_coins(void);
int checkpoints_respawn_timer(void);

/*
 * Please disable all checkpoints,
 * before you attempt to restart the level.
 */
void checkpoints_stop(void);

void checkpoints_set_last_data(float, int, int, int);

#endif
#endif
