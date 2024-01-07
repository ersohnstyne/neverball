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

#include <assert.h>
#include "cmd.h"


#include "game_common.h"
#include "game_client.h"
#include "game_draw.h"
#include "game_server.h"

/*
 * Checkpoints discovered by: PennySchloss
 * Thanks for the Youtuber: PlayingWithMahWii
 */
#include "checkpoints.h"

#if NB_HAVE_PB_BOTH==1
#include "solid_chkp.h"
#endif
#include "solid_vary.h"
#include "vec3.h"

#ifdef MAPC_INCLUDES_CHKP

/*---------------------------------------------------------------------------*/

int last_active = 0;
int checkpoints_busy;

/* Pult-o-meter cache. */

struct chkp_ballsize last_chkp_ballsize[1024];
struct chkp_ball     last_chkp_ball[1024];
struct chkp_path     last_chkp_path[2048];
struct chkp_body     last_chkp_body[1024];
struct chkp_move     last_chkp_move[1024];
struct chkp_item     last_chkp_item[2048];
struct chkp_swch     last_chkp_swch[1024];
struct chkp_view     last_view[1024];

float last_pos[1024][3];

float last_timer;
int   last_coins;
int   last_goal;

int last_gained;

int   respawn_coins        = 0;
float respawn_time_elapsed = 0;
int   respawn_gained       = 0;

/*---------------------------------------------------------------------------*/

void checkpoints_save_spawnpoint(struct s_vary saved_vary,
                                 struct game_view saved_view,
                                 int ui)
{
    /* Phase 1: Activate the checkpoints. */
    
    last_active = 1;

    /* Phase 2: Backup the camera view */
    
    memset(&last_view, 0, sizeof(saved_view));
    last_view[ui].a = saved_view.a;

    while (last_view[ui].a > 180.0f)
        last_view[ui].a -= 180.f;

    while (last_view[ui].a < -180.0f)
        last_view[ui].a += 180.f;

    /* Phase 3: Backup all SOL data's simulation. */

    /* Backed up from the gameplay */
    for (int backupidx = 0; backupidx < saved_vary.uc; backupidx++)
    {
        struct v_ball    *up = saved_vary.uv + backupidx;
        struct chkp_ball *c_up = last_chkp_ball + backupidx;

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

#ifdef _DEBUG
    log_printf("Gameplay data has been backed up!\n");
#endif
    v_cpy(last_pos[ui], saved_vary.uv[ui].p);
}

int checkpoints_load()
{
    if (last_active)
    {
        respawn_coins  = last_coins;
        respawn_gained = last_gained;

        return 1;
    }

    return 0;
}

void checkpoints_respawn(struct s_vary *vary, cmd_fn_chkp cmd_func, int* ci)
{
    /* Phase 1: Obtain the checkpoint index. */

    assert(ci);

    /* Phase 2: Restore SOL data's simulation from checkpoint. */

    checkpoints_busy = 1;

    /* Restored from the checkpoints */
    for (int resetidx = 0; resetidx < vary->cc; resetidx++)
    {
        struct v_chkp *cp = vary->cv + resetidx;

        if (resetidx == *ci)
        {
            cp->e = 1;

            if (cmd_func)
            {
                union cmd cmd    = { CMD_CHKP_ENTER };
                cmd.chkpenter.ci = resetidx;
                cmd_func(&cmd);
            }

            if (cp->f != 1)
            {
                cp->f = 1;

                if (cmd_func)
                {
                    union cmd cmd     = { CMD_CHKP_TOGGLE };
                    cmd.chkptoggle.ci = resetidx;
                    cmd_func(&cmd);
                }

                if (*ci == -1)
                    *ci = resetidx;
            }
        }
    }

    /* Restored from the checkpoints (path) */
    for (int resetidx = 0; resetidx < vary->pc; resetidx++)
    {
        struct chkp_path *last_pp = last_chkp_path + resetidx;
        struct v_path    *pp      = vary->pv + resetidx;

        if (pp->f != last_pp->f)
        {
            pp->f = last_pp->f;

            if (cmd_func)
            {
                union cmd cmd   = { CMD_PATH_FLAG };
                cmd.pathflag.pi = resetidx;
                cmd.pathflag.f  = pp->f;
                cmd_func(&cmd);
            }
        }
    }

    /* Restored from the checkpoints (body) */
    for (int resetidx = 0; resetidx < vary->bc; resetidx++)
    {
        struct chkp_body *c_bp = last_chkp_body + resetidx;
        struct v_body    *bp   = vary->bv + resetidx;

        bp->mi = c_bp->mi;
        bp->mj = c_bp->mj;
    }

    /* Restored from the checkpoints (mover) (no loading SOL from files) */
    for (int resetidx = 0; resetidx < vary->mc; resetidx++)
    {
        struct chkp_move *last_mp = last_chkp_move + resetidx;
        struct v_move    *mp      = vary->mv + resetidx;

        if (vary->pv[mp->pi].f)
        {
            struct v_path *pp = vary->pv + mp->pi;

            mp->t  = last_mp->t;
            mp->tm = last_mp->tm;
            mp->pi = last_mp->pi;

            if (mp->tm >= pp->base->tm)
            {
                mp->t = 0;
                mp->tm = 0;
                mp->pi = pp->base->pi;
            }

            if (cmd_func)
            {
                union cmd cmd;

                cmd.type        = CMD_MOVE_TIME;
                cmd.movetime.mi = resetidx;
                cmd.movetime.t  = mp->t;
                cmd_func(&cmd);

                cmd.type        = CMD_MOVE_PATH;
                cmd.movepath.mi = resetidx;
                cmd.movepath.pi = mp->pi;
                cmd_func(&cmd);
            }
        }
    }

    /* Restored from the checkpoints (coins) */
    for (int resetidx = 0; resetidx < vary->hc; resetidx++)
    {
        struct chkp_item *last_hp = last_chkp_item + resetidx;
        struct v_item    *hp      = vary->hv + resetidx;

        v_cpy(hp->p, last_hp->p);
        hp->t = last_hp->t;
        hp->n = last_hp->n;
    }

    /* Restored from the checkpoints (switch) */
    for (int resetidx = 0; resetidx < vary->xc; resetidx++)
    {
        struct chkp_swch *last_xp = last_chkp_swch + resetidx;
        struct v_swch    *xp      = vary->xv + resetidx;

        if (xp->f != last_xp->f)
        {
            xp->f = last_xp->f;

            if (cmd_func)
            {
                union cmd cmd     = { CMD_SWCH_TOGGLE };
                cmd.swchtoggle.xi = resetidx;
                cmd_func(&cmd);
            }
        }
    }

    if (cmd_func)
    {
        union cmd cmd = { CMD_CLEAR_ITEMS };
        cmd_func(&cmd);
    }

    /* Restored from the checkpoints (item) */
    for (int resetidx = 0; resetidx < vary->hc; resetidx++)
    {
        struct v_item    *hp   = vary->hv + resetidx;
        struct chkp_item *c_hp = last_chkp_item + resetidx;

        v_cpy(hp->p, c_hp->p);

        hp->t = c_hp->t;
        hp->n = c_hp->n;

        if (cmd_func)
        {
            union cmd cmd = { CMD_MAKE_ITEM };
            v_cpy(cmd.mkitem.p, hp->p);
            cmd.mkitem.t = hp->t;
            cmd.mkitem.n = hp->n;
            cmd_func(&cmd);
        }
    }

    /* Phase 3: Finishing up loading checkpoint. */

    checkpoints_busy = 0;

#ifdef _DEBUG
    log_printf("Gameplay data has been restored!\n");
#endif
}

int checkpoints_respawn_coins(void)
{
    return respawn_coins;
}

int checkpoints_respawn_time_elapsed(void)
{
    return respawn_time_elapsed * 100.f;
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

    last_timer = 0;
    last_coins = 0;
    last_goal  = 0;

    last_gained     = 0;

    respawn_coins        = 0;
    respawn_time_elapsed = 0;
    respawn_gained       = 0;

    float resetpos[3]; resetpos[0] = 0.0f; resetpos[1] = 0.0f; resetpos[2] = 0.0f;
}

void checkpoints_set_last_data(float time, int downward, int coins, int gained)
{
    /* Set the last level data */
    last_timer      = time;
    last_coins      = coins;
    last_gained     = gained;
}

#endif
