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

#include <stdlib.h>
#include <assert.h>

#include "solid_vary.h"
#include "common.h"
#include "vec3.h"

/*---------------------------------------------------------------------------*/

int sol_load_vary(struct s_vary *fp, struct s_base *base)
{
    int i;

    memset(fp, 0, sizeof (*fp));

    fp->base = base;

    if (fp->base->pc)
    {
        fp->pc = fp->base->pc;
        fp->pv = calloc(fp->pc, sizeof (*fp->pv));
        if (fp->pv == 0) return 0;

        for (i = 0; i < fp->base->pc; i++)
        {
            struct v_path *pp = fp->pv + i;
            struct b_path *pq = fp->base->pv + i;

            pp->base = pq;
            pp->f    = pq->f;
        }
    }

    if (fp->base->bc)
    {
        struct alloc mv;

        fp->bc = fp->base->bc;
        fp->bv = calloc(fp->bc, sizeof (*fp->bv));
        if (fp->bv == 0) return 0;

        alloc_new(&mv, sizeof (*fp->mv), (void **) &fp->mv, &fp->mc);

        for (i = 0; i < fp->base->bc; i++)
        {
            struct b_body *bbody = fp->base->bv + i;
            struct v_body *vbody = fp->bv + i;
            struct v_move *vmove;

            vbody->base = bbody;

            vbody->mi = -1;
            vbody->mj = -1;

            if (bbody->pi >= 0 && (vmove = alloc_add(&mv)))
            {
                memset(vmove, 0, sizeof (*vmove));

                vbody->mi = fp->mc - 1;
                vmove->pi = bbody->pi;
            }

            if (bbody->pj == bbody->pi)
                vbody->mj = vbody->mi;
            else if (bbody->pj >= 0 && (vmove = alloc_add(&mv)))
            {
                memset(vmove, 0, sizeof (*vmove));

                vbody->mj = fp->mc - 1;
                vmove->pi = bbody->pj;
            }
        }
    }

    if (fp->base->hc)
    {
        fp->hc = fp->base->hc;
        fp->hv = calloc(fp->hc, sizeof (*fp->hv));
        if (fp->hv == 0) return 0;

        for (i = 0; i < fp->base->hc; i++)
        {
            struct v_item *hp = fp->hv + i;
            struct b_item *hq = fp->base->hv + i;

            v_cpy(hp->p, hq->p);

            hp->t = hq->t;
            hp->n = hq->n;
        }
    }

    if (fp->base->xc)
    {
        fp->xc = fp->base->xc;
        fp->xv = calloc(fp->xc, sizeof (*fp->xv));
        if (fp->xv == 0) return 0;

        for (i = 0; i < fp->base->xc; i++)
        {
            struct v_swch *xp = fp->xv + i;
            struct b_swch *xq = fp->base->xv + i;

            xp->base = xq;
            xp->t    = xq->t;
            xp->tm   = xq->tm;
            xp->f    = xq->f;
        }
    }

    if (fp->base->uc)
    {
        fp->uc = fp->base->uc;
        fp->uv = calloc(fp->uc, sizeof (*fp->uv));
        if (fp->uv == 0) return 0;

        for (i = 0; i < fp->base->uc; i++)
        {
            struct v_ball *up = fp->uv + i;
            struct b_ball *uq = fp->base->uv + i;

            v_cpy(up->p, uq->p);

            up->r = uq->r;

            up->E[0][0] = up->e[0][0] = 1.0f;
            up->E[0][1] = up->e[0][1] = 0.0f;
            up->E[0][2] = up->e[0][2] = 0.0f;

            up->E[1][0] = up->e[1][0] = 0.0f;
            up->E[1][1] = up->e[1][1] = 1.0f;
            up->E[1][2] = up->e[1][2] = 0.0f;

            up->E[2][0] = up->e[2][0] = 0.0f;
            up->E[2][1] = up->e[2][1] = 0.0f;
            up->E[2][2] = up->e[2][2] = 1.0f;
        }
    }

#ifdef MAPC_INCLUDES_CHKP
    if (fp->base->cc)
    {
        fp->cc = fp->base->cc;
        fp->cv = calloc(fp->cc, sizeof (*fp->cv));
        if (fp->cv == 0) return 0;

        for (i = 0; i < fp->base->cc; i++)
        {
            struct v_chkp *cp = fp->cv + i;
            struct b_chkp *cq = fp->base->cv + i;

            cp->base = cq;
        }
    }
#endif

    return 1;
}

int sol_respawn_vary(struct s_vary *fp, struct s_vary *last_fp)
{
    int i;

    memset(fp, 0, sizeof (*fp));

    fp->base = last_fp->base;

    if (fp->base->pc)
    {
        fp->pc = fp->base->pc;
        fp->pv = calloc(fp->base->pc, sizeof (*fp->base->pv));
        if (fp->pv == 0) return 0;
        
        for (i = 0; i < fp->base->pc; i++)
        {
            struct v_path *last_pp = last_fp->pv + i;
            struct v_path *pp      = fp->pv + i;

            pp->base = last_pp->base;
            pp->f    = last_pp->f;
        }
    }

    if (fp->base->bc)
    {
        struct alloc mv;

        fp->bc = fp->base->bc;
        fp->bv = calloc(fp->base->pc, sizeof (*fp->base->bv));
        if (fp->bv == 0) return 0;

        alloc_new(&mv, sizeof (*fp->base->mv), (void**) &fp->base->mv, &fp->base->mc);

        for (i = 0; i < fp->base->bc; i++)
        {
            struct v_body *last_vbody = last_fp->bv + i;
            struct v_body *vbody      = fp->bv + i;
            struct v_move *last_vmove = last_fp->mv + i;
            struct v_move *vmove;

            vbody->base = last_vbody->base;

            vbody->mi = -1;
            vbody->mj = -1;

            if (last_vbody->base->pi >= 0 && (vmove = alloc_add(&mv)))
            {
                memset(vmove, 0, sizeof (*vmove));

                /*
                 * We do not guranteed, where was saved from the data:
                 * bbody->pi
                 */

                vbody->mi = last_fp->mc - 1;
                vmove->pi = last_vmove->pi;
                vmove->t  = last_vmove->t;
                vmove->tm = last_vmove->tm;
            }

            if (last_vbody->base->pj == last_vbody->base->pi)
                vbody->mj = vbody->mi;
            else if (last_vbody->base->pj >= 0 && (vmove = alloc_add(&mv)))
            {
                memset(vmove, 0, sizeof (*vmove));

                vbody->mj = last_fp->mc - 1;
                vmove->pi = last_vbody->base->pj;
            }
        }
    }

    if (fp->base->mc)
    {
        fp->mc = fp->base->mc;
        fp->mv = calloc(fp->base->mc, sizeof (*fp->base->mv));
        if (fp->mv == 0) return 0;

        for (i = 0; i < fp->base->mc; i++)
        {
            struct v_move *last_vmove = last_fp->mv + i;
            struct v_move *vmove      = fp->mv + i;

            vmove->t  = last_vmove->t;
            vmove->tm = last_vmove->tm;
            vmove->pi = last_vmove->pi;
        }
    }

    if (fp->base->hc)
    {
        fp->hc = fp->base->hc;
        fp->hv = calloc(fp->base->hc, sizeof (*fp->base->hv));
        if (fp->hv == 0) return 0;

        for (i = 0; i < fp->base->hc; i++)
        {
            struct v_item *last_hp = last_fp->hv + i;
            struct v_item *hp      = fp->hv + i;

            v_cpy(hp->p, last_hp->p);

            hp->t = last_hp->t;
            hp->n = last_hp->n;
        }
    }

    if (fp->base->xc)
    {
        fp->xc = fp->base->xc;
        fp->xv = calloc(fp->base->xc, sizeof (*fp->base->xv));
        if (fp->xv == 0) return 0;

        for (i = 0; i < fp->base->xc; i++)
        {
            struct v_swch *last_xp = last_fp->xv + i;
            struct v_swch *xp      = fp->xv + i;

            xp->base = last_xp->base;
            xp->t    = last_xp->t;
            xp->tm   = last_xp->tm;
            xp->f    = last_xp->f;
        }
    }

    if (fp->base->uc)
    {
        fp->uc = fp->base->uc;
        fp->uv = calloc(fp->base->uc, sizeof (*fp->base->uv));
        if (fp->uv == 0) return 0;

        for (i = 0; i < fp->base->uc; i++)
        {
            struct v_ball *last_up = last_fp->uv + i;
            struct v_ball *up      = fp->uv + i;

            v_cpy(up->p, last_up->p);

            up->r = last_up->r;

            v_cpy(up->e[0], last_up->e[0]);
            v_cpy(up->e[1], last_up->e[1]);
            v_cpy(up->e[2], last_up->e[2]);
            v_cpy(up->E[0], last_up->E[0]);
            v_cpy(up->E[1], last_up->E[1]);
            v_cpy(up->E[2], last_up->E[2]);
        }
    }

#ifdef MAPC_INCLUDES_CHKP
    if (fp->base->cc)
    {
        fp->cc = fp->base->cc;
        fp->cv = calloc(fp->base->cc, sizeof (*fp->base->cv));
        if (fp->cv == 0) return 0;

        for (i = 0; i < fp->base->cc; i++)
        {
            struct v_chkp *last_cp = last_fp->cv + i;
            struct v_chkp *cp      = fp->cv + i;

            cp->base = last_cp->base;
            cp->f    = last_cp->f;
            cp->e    = last_cp->e;
        }
    }
#endif

    return 1;
}

void sol_free_vary(struct s_vary *fp)
{
    free(fp->pv);
    free(fp->bv);
    free(fp->mv);
    free(fp->hv);
    free(fp->xv);
    free(fp->uv);
#ifdef MAPC_INCLUDES_CHKP
    free(fp->cv);
#endif

    memset(fp, 0, sizeof (*fp));
}

/*---------------------------------------------------------------------------*/

int sol_vary_cmd(struct s_vary *fp, struct cmd_state *cs, const union cmd *cmd)
{
    struct v_ball *up;
    int rc = 0;

    switch (cmd->type)
    {
    case CMD_MAKE_BALL:
        if ((up = realloc(fp->uv, sizeof (*up) * (fp->uc + 1))))
        {
            fp->uv = up;
            cs->curr_ball = fp->uc;
            fp->uc++;
            rc = 1;
        }
        break;

    case CMD_CLEAR_BALLS:
        free(fp->uv);
        fp->uv = NULL;
        fp->uc = 0;
        break;

    default:
        break;
    }

    return rc;
}

/*---------------------------------------------------------------------------*/

#define CURR 0
#define PREV 1

int sol_lerp_cmd(struct s_lerp *fp, struct cmd_state *cs, const union cmd *cmd)
{
    struct l_ball (*uv)[2];
    struct l_ball *up;

    int idx, mi, i;
    int rc = 0;

    switch (cmd->type)
    {
    case CMD_MAKE_BALL:
        if ((uv = realloc(fp->uv, sizeof (*uv) * (fp->uc + 1))))
        {
            fp->uv = uv;

            /* Update varying state. */

            if (sol_vary_cmd(fp->vary, cs, cmd))
            {
                fp->uc++;
                rc = 1;
            }
        }
        break;

    case CMD_MOVE_PATH:
        if ((mi = cmd->movepath.mi) >= 0 && mi < fp->mc)
        {
            /* Be extra paranoid. */

            if ((idx = cmd->movepath.pi) >= 0 && idx < fp->vary->base->pc)
                fp->mv[mi][CURR].pi = idx;
        }
        break;

    case CMD_MOVE_TIME:
        if ((mi = cmd->movetime.mi) >= 0 && mi < fp->mc)
        {
            fp->mv[mi][CURR].t = cmd->movetime.t;
        }
        break;

    case CMD_BODY_PATH:
        /* Backward compatibility: update linear mover only. */

        if ((idx = cmd->bodypath.bi) >= 0 && idx < fp->vary->bc &&
            (mi = fp->vary->bv[idx].mi) >= 0)
        {
            /* Be EXTRA paranoid. */

            if ((idx = cmd->bodypath.pi) >= 0 && idx < fp->vary->base->pc)
                fp->mv[mi][CURR].pi = idx;
        }
        break;

    case CMD_BODY_TIME:
        /* Same as CMD_BODY_PATH. */

        if ((idx = cmd->bodytime.bi) >= 0 && idx < fp->vary->bc &&
            (mi = fp->vary->bv[idx].mi) >= 0)
        {
            fp->mv[mi][CURR].t = cmd->bodytime.t;
        }
        break;

    case CMD_BALL_RADIUS:
        fp->uv[cs->curr_ball][CURR].r = cmd->ballradius.r;
        break;

    case CMD_CLEAR_BALLS:
        free(fp->uv);
        fp->uv = NULL;
        fp->uc = 0;

        sol_vary_cmd(fp->vary, cs, cmd);

        break;

    case CMD_BALL_POSITION:
        up = &fp->uv[cs->curr_ball][CURR];
        v_cpy(up->p, cmd->ballpos.p);
        break;

    case CMD_BALL_BASIS:
        up = &fp->uv[cs->curr_ball][CURR];
        v_cpy(up->e[0], cmd->ballbasis.e[0]);
        v_cpy(up->e[1], cmd->ballbasis.e[1]);
        v_crs(up->e[2], up->e[0], up->e[1]);
        break;

    case CMD_BALL_PEND_BASIS:
        up = &fp->uv[cs->curr_ball][CURR];
        v_cpy(up->E[0], cmd->ballpendbasis.E[0]);
        v_cpy(up->E[1], cmd->ballpendbasis.E[1]);
        v_crs(up->E[2], up->E[0], up->E[1]);
        break;

    case CMD_STEP_SIMULATION:
        /*
         * Step each mover ahead. This  way we cut down on replay size
         * significantly  while  still  keeping  things in  sync  with
         * occasional CMD_MOVE_PATH and CMD_MOVE_TIME.
         */

        for (i = 0; i < fp->mc; i++)
        {
            struct l_move *mp = &fp->mv[i][CURR];

            if (mp->pi >= 0 && fp->vary->pv[mp->pi].f)
                mp->t += cmd->stepsim.dt;
        }
        break;

    default:
        break;
    }

    return rc;
}

void sol_lerp_copy(struct s_lerp *fp)
{
    int i;

    for (i = 0; i < fp->mc; i++)
        fp->mv[i][PREV] = fp->mv[i][CURR];

    for (i = 0; i < fp->uc; i++)
        fp->uv[i][PREV] = fp->uv[i][CURR];
}

void sol_lerp_apply(struct s_lerp *fp, float a)
{
    int i;

    for (i = 0; i < fp->mc; i++)
    {
        if (fp->mv[i][PREV].pi == fp->mv[i][CURR].pi)
            fp->vary->mv[i].t = flerp(fp->mv[i][PREV].t, fp->mv[i][CURR].t, a);
        else
            fp->vary->mv[i].t = fp->mv[i][CURR].t * a;

        fp->vary->mv[i].pi = fp->mv[i][CURR].pi;
    }

    for (i = 0; i < fp->uc; i++)
    {
        e_lerp(fp->vary->uv[i].e, fp->uv[i][PREV].e, fp->uv[i][CURR].e, a);
        v_lerp(fp->vary->uv[i].p, fp->uv[i][PREV].p, fp->uv[i][CURR].p, a);
        e_lerp(fp->vary->uv[i].E, fp->uv[i][PREV].E, fp->uv[i][CURR].E, a);

        fp->vary->uv[i].r = flerp(fp->uv[i][PREV].r, fp->uv[i][CURR].r, a);
    }
}

int sol_load_lerp(struct s_lerp *fp, struct s_vary *vary)
{
    int i;

    fp->vary = vary;

    if (fp->vary->mc)
    {
        fp->mv = calloc(fp->vary->mc, sizeof (*fp->mv));
        if (fp->mv == 0) return 0;
        fp->mc = fp->vary->mc;

        for (i = 0; i < fp->vary->mc; i++)
            fp->mv[i][CURR].pi = fp->vary->mv[i].pi;
    }

    if (fp->vary->uc)
    {
        fp->uv = calloc(fp->vary->uc, sizeof (*fp->uv));
        if (fp->uv == 0) return 0;
        fp->uc = fp->vary->uc;

        for (i = 0; i < fp->vary->uc; i++)
        {
            e_cpy(fp->uv[i][CURR].e, fp->vary->uv[i].e);
            v_cpy(fp->uv[i][CURR].p, fp->vary->uv[i].p);
            e_cpy(fp->uv[i][CURR].E, fp->vary->uv[i].E);

            fp->uv[i][CURR].r = fp->vary->uv[i].r;
        }
    }

    sol_lerp_copy(fp);

    return 1;
}

int  sol_respawn_lerp(struct s_lerp *fp, struct s_vary *vary)
{
    int i;

    fp->vary = vary;

    if (fp->vary->mc)
    {
        fp->mv = calloc(fp->vary->mc, sizeof (*fp->mv));
        if (fp->mv == 0) return 0;
        fp->mc = fp->vary->mc;

        for (i = 0; i < fp->vary->mc; i++)
            fp->mv[i][CURR].pi = fp->vary->mv[i].pi;
    }

    if (fp->vary->uc)
    {
        fp->uv = calloc(fp->vary->uc, sizeof (*fp->uv));
        if (fp->uv == 0) return 0;
        fp->uc = fp->vary->uc;

        for (i = 0; i < fp->vary->uc; i++)
        {
            e_cpy(fp->uv[i][CURR].e, fp->vary->uv[i].e);
            v_cpy(fp->uv[i][CURR].p, fp->vary->uv[i].p);
            e_cpy(fp->uv[i][CURR].E, fp->vary->uv[i].E);

            fp->uv[i][CURR].r = fp->vary->uv[i].r;
        }
    }

    sol_lerp_copy(fp);

    return 1;
}

void sol_free_lerp(struct s_lerp *fp)
{
    if (fp->mv) free(fp->mv);
    if (fp->uv) free(fp->uv);

    memset(fp, 0, sizeof (*fp));
}

/*---------------------------------------------------------------------------*/
