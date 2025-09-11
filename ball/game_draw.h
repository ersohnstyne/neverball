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

#ifndef GAME_DRAW_H
#define GAME_DRAW_H

#include "solid_draw.h"
#include "game_common.h"

/*---------------------------------------------------------------------------*/

extern float gd_rotate_roll;

struct game_draw
{
    int state;

    struct s_vary vary;
    struct s_draw draw;

    struct s_full back;

    struct game_tilt tilt;              /* Floor rotation                            */
    struct game_view view;              /* Current view                              */

    int   goal_e;                       /* Goal enabled flag                         */
    float goal_k;                       /* Goal animation                            */

    int   jump_e;                       /* Jumping enabled flag                      */
    int   jump_b;                       /* Jump-in-progress flag                     */
    float jump_dt;                      /* Jump duration                             */

#ifdef MAPC_INCLUDES_CHKP
    int   chkp_e;                       /* New: Checkpoints; Checkpoint enabled flag */
    int   chkp_k;                       /* New: Checkpoints; Checkpoint animation    */
#endif

    int   fade_disabled;
    float fade_k;                       /* Fade in/out level                         */
    float fade_d;                       /* Fade in/out direction                     */

    /*
     * FIXME: Let Mojang done one of these!
     */

    int   mojang_death_enabled_flags;
    float mojang_death_time_now;
    float mojang_death_time_percent;
    float mojang_death_view_angle;
};

/* FIXME: this is just for POSE_* constants. */
#include "game_client.h"

void game_draw(struct game_draw *, int, float);

void game_draw_set_maxspeed(float, int);

/*---------------------------------------------------------------------------*/

struct game_lerp
{
    float alpha;                        /* Interpolation factor              */

    struct s_lerp lerp;

    struct game_tilt tilt[2];
    struct game_view view[2];

    float timer[2];                     /* Clock time                        */
    float goal_k[2];
    float jump_dt[2];
    float chkp_k[2];
};

void game_lerp_init(struct game_lerp *, struct game_draw *);
void game_lerp_free(struct game_lerp *);
void game_lerp_copy(struct game_lerp *);
void game_lerp_apply(struct game_lerp *, struct game_draw *);

struct game_lerp_pose {
    float pose_point_x;
    float pose_point_y;
    float pose_point_smooth_x;
    float pose_point_smooth_y;
};

void game_lerp_pose_point_tick(float);
void game_lerp_pose_point_init(void);
void game_lerp_pose_point(int, int);
void game_lerp_pose_point_reset(void);

/*---------------------------------------------------------------------------*/

#endif
