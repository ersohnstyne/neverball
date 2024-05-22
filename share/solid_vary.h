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

#ifndef SOLID_VARY_H
#define SOLID_VARY_H

#if NB_HAVE_PB_BOTH==1
#include "solid_chkp.h"
#endif
#include "base_config.h"
#include "solid_base.h"

/*
 * Varying solid data.
 */

#define GROW_TIME  0.5f                 /* sec for the ball to get to size.  */
#define GROW_BIG   1.5f                 /* large factor                      */
#define GROW_SMALL 0.5f                 /* small factor                      */

/*---------------------------------------------------------------------------*/

struct v_path
{
    const struct b_path *base;

    int f;                                     /* enable flag                */

    int mi;
    int mj;
};

struct v_body
{
    const struct b_body *base;

    int mi;
    int mj;
};

struct v_move
{
    float t;                                   /* time on current path       */
    int   tm;                                  /* milliseconds               */

    int pi;
};

struct v_item
{
    float p[3];                                /* position                   */
    int   t;                                   /* type                       */
    int   n;                                   /* value                      */

    int mi;
    int mj;
};

struct v_swch
{
    const struct b_swch* base;
    float t;                                   /* current timer              */
    int   tm;                                  /* milliseconds               */
    int   f;                                   /* current state              */
    int   e;                                   /* is a ball inside it?       */

    int mi;
    int mj;
};

struct v_goal
{
    int mi;
    int mj;
};

struct v_jump
{
    int mi;
    int mj;
};

struct v_bill
{
    int mi;
    int mj;
};

struct v_ball
{
    float e[3][3];                             /* basis of orientation       */
    float p[3];                                /* position vector            */
    float v[3];                                /* velocity vector            */
    float w[3];                                /* angular velocity vector    */
    float E[3][3];                             /* basis of pendulum          */
    float W[3];                                /* angular pendulum velocity  */
    float r;                                   /* current radius             */
    float r_vel;                               /* radius velocity            */
    float sizes[3];                            /* sizes (small, base, big)   */
    short size;                                /* current size (0, 1, 2)     */
};

#ifdef MAPC_INCLUDES_CHKP
/* New: Checkpoints */
struct v_chkp
{
    const struct b_chkp *base;

    int   f;                                   /* current state              */
    int   e;                                   /* is a ball inside it?       */

    int mi;
    int mj;
};
#endif

struct s_vary
{
    struct s_base *base;

    int pc;
    int bc;
    int mc;
    int hc;
    int zc;
    int jc;
    int xc;
    int rc;
    int uc;
#ifdef MAPC_INCLUDES_CHKP
    /* New: Checkpoints */
    int cc;
#endif

    struct v_path *pv;
    struct v_body *bv;
    struct v_move *mv;
    struct v_item *hv;
    struct v_goal *zv;
    struct v_jump *jv;
    struct v_swch *xv;
    struct v_bill *rv;
    struct v_ball *uv;
#ifdef MAPC_INCLUDES_CHKP
    /* New: Checkpoints */
    struct v_chkp *cv;
#endif

    /* Accumulator for tracking time in integer milliseconds. */

    float ms_accum;

    int sim_uses_px;
};

/*---------------------------------------------------------------------------*/

int  sol_load_vary(struct s_vary *, struct s_base *);
void sol_free_vary(struct s_vary *);

/*---------------------------------------------------------------------------*/

/*
 * Buffers changes to the varying SOL/SOLX data for interpolation purposes.
 */

struct l_move
{
    float t;                                   /* time on current path       */

    int pi;
};

struct l_ball
{
    float e[3][3];                             /* basis of orientation       */
    float p[3];                                /* position vector            */
    float E[3][3];                             /* basis of pendulum          */
    float r;                                   /* radius                     */
};

struct s_lerp
{
    struct s_vary *vary;

    int mc;
    int uc;

    struct l_move (*mv)[2];
    struct l_ball (*uv)[2];
};

/*---------------------------------------------------------------------------*/

#include "cmd.h"

int  sol_load_lerp(struct s_lerp *, struct s_vary *);
int  sol_respawn_lerp(struct s_lerp *, struct s_vary *);
void sol_free_lerp(struct s_lerp *);

void sol_lerp_copy(struct s_lerp *);
void sol_lerp_apply(struct s_lerp *, float);
int  sol_lerp_cmd(struct s_lerp *, struct cmd_state *, const union cmd *);

/*---------------------------------------------------------------------------*/

#endif
