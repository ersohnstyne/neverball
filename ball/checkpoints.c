/*
 * Copyright (C) 2022 Microsoft / Neverball authors
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

/*
 * Checkpoints discovered by: PennySchloss
 * Thanks for the Youtuber: PlayingWithMahWii
 */
#include "checkpoints.h"

#include <assert.h>

#include "game_client.h"
#include "game_common.h"
#include "game_draw.h"
#include "game_server.h"

#include "solid_chkp.h"
#include "solid_base.h"
#include "solid_vary.h"
#include "solid_draw.h"
#include "vec3.h"

#ifdef MAPC_INCLUDES_CHKP

/*---------------------------------------------------------------------------*/

int last_active = 0;
int checkpoints_busy;

/* Pult-o-meter cache. */

struct chkp_ballsize last_chkp_ballsize[1024];
struct chkp_ball last_chkp_ball[1024];
struct chkp_path last_chkp_path[2048];
struct chkp_body last_chkp_body[1024];
struct chkp_move last_chkp_move[1024];
struct chkp_item last_chkp_item[2048];
struct chkp_swch last_chkp_swch[1024];
struct game_view last_view[1024];

float last_pos[1024][3];

float last_time;
int last_coins;
int last_goal;

int last_timer_down;
int last_gained;

int respawn_coins;
float respawn_timer;

/*---------------------------------------------------------------------------*/

void checkpoints_save_spawnpoint(struct s_vary saved_vary,
                                 struct game_view saved_view,
                                 int ui)
{
    last_active = 1;
    memset(&last_view, 0, sizeof(saved_view));
    last_view[ui].a = saved_view.a;

    while (last_view[ui].a > 180.0f)
        last_view[ui].a -= 180.f;

    while (last_view[ui].a < -180.0f)
        last_view[ui].a += 180.f;

    /* Backed up from the gameplay */
    for (int backupidx = 0; backupidx < saved_vary.uc; backupidx++)
    {
        struct v_ball* up = saved_vary.uv + backupidx;
        struct chkp_ball* c_up = last_chkp_ball + backupidx;

        e_cpy(c_up->e, up->e);
        v_cpy(c_up->p, up->p);
        e_cpy(c_up->E, up->E);
        c_up->r = up->r;
    }

    /* Backed up from the gameplay (path) */
    for (int backupidx = 0; backupidx < saved_vary.pc; backupidx++)
    {
        struct v_path    *pp   = saved_vary.pv + backupidx;
        struct chkp_path *c_pp = last_chkp_path + backupidx;

        c_pp->f = pp->f;
    }

    /* Backed up from the gameplay (body) */
    for (int backupidx = 0; backupidx < saved_vary.bc; backupidx++)
    {
        struct v_body    *bp   = saved_vary.bv + backupidx;
        struct chkp_body *c_bp = last_chkp_body + backupidx;

        c_bp->mi = bp->mi;
        c_bp->mj = bp->mj;
    }

    /* Backed up from the gameplay (mover) (no saving SOL to files) */
    for (int backupidx = 0; backupidx < saved_vary.mc; backupidx++)
    {
        struct v_move    *mp   = saved_vary.mv + backupidx;
        struct chkp_move *c_mp = last_chkp_move + backupidx;

        c_mp->t = mp->t;
        c_mp->tm = mp->tm;
        c_mp->pi = mp->pi;
    }

    /* Backed up from the gameplay (coins) */
    for (int backupidx = 0; backupidx < saved_vary.hc; backupidx++)
    {
        struct v_item    *hp   = saved_vary.hv + backupidx;
        struct chkp_item *c_hp = last_chkp_item + backupidx;

        v_cpy(c_hp->p, hp->p);
        c_hp->t = hp->t;
        c_hp->n = hp->n;
    }

    /* Backed up from the gameplay (switch) */
    for (int backupidx = 0; backupidx < saved_vary.xc; backupidx++)
    {
        struct v_swch    *xp   = saved_vary.xv + backupidx;
        struct chkp_swch *c_xp = last_chkp_swch + backupidx;

        c_xp->f = xp->f;
        c_xp->t = xp->t;
        c_xp->tm = xp->tm;
    }

#ifndef NDEBUG
    log_printf("Gameplay data has been backed up!\n");
#endif
    v_cpy(last_pos[ui], saved_vary.uv[ui].p);
}

void checkpoints_respawn(void)
{
    checkpoints_busy = 1;
}

void checkpoints_respawn_done(void)
{
    checkpoints_busy = 0;
    respawn_coins = last_coins;
    respawn_timer = last_time;
}

int checkpoints_respawn_coins(void)
{
    return respawn_coins;
}

int checkpoints_respawn_timer(void)
{
    return respawn_timer * 100.f;
}

/*
 * Please disable all checkpoints,
 * before you attempt to restart or exit the level.
 */
void checkpoints_stop(void)
{
    if (last_active)
    {
        for (int ui = 0; ui < 1024; ui++)
            last_view[ui].a = 0;
    }

    checkpoints_busy = 0;

    last_active = 0;

    last_time = 0;
    last_coins = 0;
    last_goal = 0;

    last_timer_down = 0;
    last_gained = 0;

    respawn_coins = 0;
    respawn_timer = 0;

    float resetpos[3]; resetpos[0] = 0.0f; resetpos[1] = 0.0f; resetpos[2] = 0.0f;
}

void checkpoints_set_last_data(float time, int downward, int coins)
{
    /* Set the last level data */
    last_time = time;
    last_timer_down = downward;
    last_coins = coins;
}

#endif
