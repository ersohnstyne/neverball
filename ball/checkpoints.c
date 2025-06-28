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

#if NB_HAVE_PB_BOTH==1 && !defined(MAPC_INCLUDES_CHKP)
#error Security compilation error: Please enable checkpoints after joined PB+NB Discord Server!
#endif

/*---------------------------------------------------------------------------*/

#ifdef MAPC_INCLUDES_CHKP

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
struct chkp_goal     last_chkp_goal[1024];
struct chkp_jump     last_chkp_jump[1024];
struct chkp_bill     last_chkp_bill[1024];
struct chkp_chkp     last_chkp_chkp[1024];
struct chkp_view     last_view[1024];

float last_pos[1024][3];

float last_time_elapsed;
float last_time_limit;
int   last_coins;

float respawn_time_elapsed = 0;
float respawn_time_limit   = 0;
int   respawn_coins        = 0;

/*---------------------------------------------------------------------------*/

void checkpoints_save_spawnpoint(struct s_vary saved_vary,
                                 struct game_view saved_view,
                                 int ui)
{
    /* Phase 1: Activate the checkpoints. */

    last_active = 1;

    /* Phase 2: Backup the camera view */

    memset(&last_view, 0, sizeof (saved_view));
    last_view[ui].a = saved_view.a;

    while (last_view[ui].a > 180.0f)
        last_view[ui].a -= 180.0f;

    while (last_view[ui].a < -180.0f)
        last_view[ui].a += 180.0f;

    /* Phase 3: Backup all SOL/SOLX data's simulation. */

    /* Backed up from the gameplay */
    for (int backupidx = 0; backupidx < saved_vary.uc; backupidx++)
    {
        struct v_ball    *up   = saved_vary.uv + backupidx;
        struct chkp_ball *c_up = last_chkp_ball + backupidx;

        e_cpy(c_up->e, up->e);
        v_cpy(c_up->p, up->p);
        e_cpy(c_up->E, up->E);

        c_up->r     = up->r;
        c_up->r_vel = up->r_vel;
        c_up->size  = up->size;
    }

    /* Backed up from the gameplay (path) */
    for (int backupidx = 0; backupidx < saved_vary.pc; backupidx++)
    {
        struct v_path    *pp   = saved_vary.pv + backupidx;
        struct chkp_path *c_pp = last_chkp_path + backupidx;

        c_pp->f = pp->f;

        c_pp->mi = pp->mi;
        c_pp->mj = pp->mj;
    }

    /* Backed up from the gameplay (body) */
    for (int backupidx = 0; backupidx < saved_vary.bc; backupidx++)
    {
        struct v_body    *bp   = saved_vary.bv + backupidx;
        struct chkp_body *c_bp = last_chkp_body + backupidx;

        c_bp->mi = bp->mi;
        c_bp->mj = bp->mj;
    }

    /* Backed up from the gameplay (mover) (no saving SOL/SOLX to files) */
    for (int backupidx = 0; backupidx < saved_vary.mc; backupidx++)
    {
        struct v_move    *mp   = saved_vary.mv + backupidx;
        struct chkp_move *c_mp = last_chkp_move + backupidx;

        c_mp->t  = mp->t;
        c_mp->tm = mp->tm;
        c_mp->pi = mp->pi;

        c_mp->pos.x = mp->pos.x;
        c_mp->pos.y = mp->pos.y;
        c_mp->pos.z = mp->pos.z;

        c_mp->rot.x = mp->rot.x;
        c_mp->rot.y = mp->rot.y;
        c_mp->rot.z = mp->rot.z;
        c_mp->rot.w = mp->rot.w;

        c_mp->dirty = mp->dirty;
    }

    /* Backed up from the gameplay (coins) */
    for (int backupidx = 0; backupidx < saved_vary.hc; backupidx++)
    {
        struct v_item    *hp   = saved_vary.hv + backupidx;
        struct chkp_item *c_hp = last_chkp_item + backupidx;

        v_cpy(c_hp->p, hp->p);
        c_hp->t = hp->t;
        c_hp->n = hp->n;

        c_hp->mi = hp->mi;
        c_hp->mj = hp->mj;
    }

    /* Backed up from the gameplay (switch) */
    for (int backupidx = 0; backupidx < saved_vary.xc; backupidx++)
    {
        struct v_swch    *xp   = saved_vary.xv + backupidx;
        struct chkp_swch *c_xp = last_chkp_swch + backupidx;

        c_xp->f  = xp->f;
        c_xp->t  = xp->t;
        c_xp->tm = xp->tm;

        c_xp->mi = xp->mi;
        c_xp->mj = xp->mj;
    }

    /* Backed up from the gameplay (goal) */
    for (int backupidx = 0; backupidx < saved_vary.zc; backupidx++)
    {
        struct v_goal    *zp   = saved_vary.zv + backupidx;
        struct chkp_goal *c_zp = last_chkp_goal + backupidx;

        c_zp->mi = zp->mi;
        c_zp->mj = zp->mj;
    }

    /* Backed up from the gameplay (jump) */
    for (int backupidx = 0; backupidx < saved_vary.jc; backupidx++)
    {
        struct v_jump    *jp   = saved_vary.jv + backupidx;
        struct chkp_jump *c_jp = last_chkp_jump + backupidx;

        c_jp->mi = jp->mi;
        c_jp->mj = jp->mj;
    }

    /* Backed up from the gameplay (bill) */
    for (int backupidx = 0; backupidx < saved_vary.rc; backupidx++)
    {
        struct v_bill    *rp   = saved_vary.rv + backupidx;
        struct chkp_bill *c_rp = last_chkp_bill + backupidx;

        c_rp->mi = rp->mi;
        c_rp->mj = rp->mj;
    }

    /* Backed up from the gameplay (chkp) */
    for (int backupidx = 0; backupidx < saved_vary.cc; backupidx++)
    {
        struct v_chkp    *cp   = saved_vary.cv + backupidx;
        struct chkp_chkp *c_cp = last_chkp_chkp + backupidx;

        c_cp->mi = cp->mi;
        c_cp->mj = cp->mj;
    }

#ifdef _DEBUG
    log_printf("Gameplay data has been backed up!\n");
#endif
    v_cpy(last_pos[ui], saved_vary.uv[ui].p);
}

void checkpoints_save_last_data(float time_elapsed, float time_limit, int coins)
{
    /* Set the last level data */

    last_time_elapsed = time_elapsed;
    last_time_limit   = time_limit;
    last_coins        = coins;
}

int checkpoints_last_coins(void)
{
    return last_coins;
}

float checkpoints_last_time_elapsed(void)
{
    return last_time_elapsed;
}

float checkpoints_last_time_limit(void)
{
    return last_time_limit;
}

/*---------------------------------------------------------------------------*/

int checkpoints_load(void)
{
    if (last_active)
    {
        respawn_time_elapsed = last_time_elapsed;
        respawn_time_limit   = last_time_limit;
        respawn_coins        = last_coins;

        return 1;
    }

    return 0;
}

void checkpoints_respawn(struct s_vary *vary, cmd_fn_chkp cmd_func, int *ci)
{
    checkpoints_busy = 1;

    /* Restore SOL/SOLX data's simulation from checkpoint. */

    /* Restored from the checkpoints */
    for (int resetidx = 0; resetidx < vary->uc; resetidx++)
    {
        struct chkp_ball *c_up = last_chkp_ball + resetidx;
        struct v_ball    *up   = vary->uv + resetidx;

        e_cpy(up->e, c_up->e);
        v_cpy(up->p, c_up->p);
        e_cpy(up->E, c_up->E);

        up->r     = c_up->r;
        up->r_vel = c_up->r_vel;
        up->size  = c_up->size;

        if (cmd_func)
        {
            union cmd cmd = { CMD_BALL_POSITION };
            v_cpy(cmd.ballpos.p, up->p);
            cmd_func(&cmd);

            cmd.type = CMD_BALL_BASIS;
            v_cpy(cmd.ballbasis.e[0], up->e[0]);
            v_cpy(cmd.ballbasis.e[1], up->e[1]);
            cmd_func(&cmd);

            cmd.type = CMD_BALL_PEND_BASIS;
            v_cpy(cmd.ballpendbasis.E[0], up->E[0]);
            v_cpy(cmd.ballpendbasis.E[1], up->E[1]);
            cmd_func(&cmd);

            cmd.type = CMD_BALL_RADIUS;
            cmd.ballradius.r = up->r;
            cmd_func(&cmd);
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

            pp->mi = last_pp->mi;
            pp->mj = last_pp->mj;
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

    /* Restored from the checkpoints (mover) (no loading SOL/SOLX from files) */
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

            mp->pos.x = last_mp->pos.x;
            mp->pos.y = last_mp->pos.y;
            mp->pos.z = last_mp->pos.z;

            mp->rot.x = last_mp->rot.x;
            mp->rot.y = last_mp->rot.y;
            mp->rot.z = last_mp->rot.z;
            mp->rot.w = last_mp->rot.w;

            mp->dirty = last_mp->dirty;

            if (mp->tm >= pp->base->tm)
            {
                mp->t = 0;
                mp->tm = 0;
                mp->pi = pp->base->pi;
            }

            if (cmd_func)
            {
                union cmd cmd   = { CMD_MOVE_TIME };
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

    /* Restored from the checkpoints (goal) */
    for (int resetidx = 0; resetidx < vary->zc; resetidx++)
    {
        struct chkp_goal *last_zp = last_chkp_goal + resetidx;
        struct v_goal    *zp      = vary->zv + resetidx;

        zp->mi = last_zp->mi;
        zp->mj = last_zp->mj;
    }

    /* Restored from the checkpoints (jump) */
    for (int resetidx = 0; resetidx < vary->jc; resetidx++)
    {
        struct chkp_jump *last_jp = last_chkp_jump + resetidx;
        struct v_jump    *jp      = vary->jv + resetidx;

        jp->mi = last_jp->mi;
        jp->mj = last_jp->mj;
    }

    /* Restored from the checkpoints (bill) */
    for (int resetidx = 0; resetidx < vary->rc; resetidx++)
    {
        struct chkp_bill *last_rp = last_chkp_bill + resetidx;
        struct v_bill    *rp      = vary->rv + resetidx;

        rp->mi = last_rp->mi;
        rp->mj = last_rp->mj;
    }

    /* Restored from the checkpoints (chkp) */
    for (int resetidx = 0; resetidx < vary->cc; resetidx++)
    {
        struct chkp_chkp *last_cp = last_chkp_chkp + resetidx;
        struct v_chkp    *cp      = vary->cv + resetidx;

        if (ci && resetidx == *ci)
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

        cp->mi = last_cp->mi;
        cp->mj = last_cp->mj;
    }

    /* Finishing up loading checkpoint. */

    checkpoints_busy = 0;

#ifdef _DEBUG
    log_printf("Gameplay data has been restored!\n");
#endif
}

int checkpoints_respawn_coins(void)
{
    return respawn_coins;
}

float checkpoints_respawn_time_elapsed(void)
{
    return respawn_time_elapsed;
}

float checkpoints_respawn_time_limit(void)
{
    return respawn_time_limit;
}

/*---------------------------------------------------------------------------*/

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

    last_time_elapsed = 0;
    last_time_limit = 0;
    last_coins = 0;

    respawn_coins        = 0;
    respawn_time_elapsed = 0;
    respawn_time_limit   = 0;
}

#endif

/*---------------------------------------------------------------------------*/
