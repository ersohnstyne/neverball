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

#include "progress.h"

#include "vec3.h"
#include "glext.h"
#include "ball.h"
#include "part.h"
#include "geom.h"
#include "config.h"
#include "video.h"

#if NB_HAVE_PB_BOTH==1
#include "solid_chkp.h"
#endif
#include "solid_all.h"
#include "solid_draw.h"

#include "game_draw.h"

#if NB_HAVE_PB_BOTH==1 && !defined(MAPC_INCLUDES_CHKP)
#error Security compilation error: Please enable checkpoints after joined PB+NB Discord Server!
#endif

/*---------------------------------------------------------------------------*/

static int draw_chnk_highaltitude;

static void game_draw_chnk_floor(struct s_rend *rend,
                                 const struct game_draw *gd,
                                 const float *bill_M, float t)
{
    float c[4] = DRAW_COLOR4FV_CNF_MOTIONBLUR;
    const float SCL = (draw_chnk_highaltitude ? 20.0f : 10.0f);

    const struct s_base *base =  gd->vary.base;
    const struct s_vary *vary = &gd->vary;

    float Y = vary->uv[0].p[1] - vary->uv[0].r;

    if (base->vc > 0) Y = base->vv[0].p[1];

    int i, j;

    for (int i = -4; i < 4; i++)
    {
        for (int j = -4; j < 4; j++)
        {
            glPushMatrix();
            {
                glScalef    (SCL, 1, SCL);
                glTranslatef((i * SCL) * (draw_chnk_highaltitude ? 800 : 400),
                             Y,
                             (j * SCL) * (draw_chnk_highaltitude ? 800 : 400));

                glColor4ub(ROUND(c[0] * 255),
                           ROUND(c[1] * 255),
                           ROUND(c[2] * 255),
                           ROUND(c[3] * 255));
                chnk_pane_draw(rend);
            }
            glPopMatrix();
        }
    }
}

static void game_draw_chnk_rings(struct s_rend *rend,
                                 const struct game_draw *gd,
                                 const float *bill_M, float t)
{
    float c[4] = DRAW_COLOR4FV_CNF_MOTIONBLUR;
    const float SCL = 0.25f;

    const struct s_base *base =  gd->vary.base;
    const struct s_vary *vary = &gd->vary;

    float Y = vary->uv[0].p[1] - vary->uv[0].r;

    if (base->vc > 0) Y = base->vv[0].p[1];

    for (int i = 0; i < base->zc; i++)
    {
        float chnk_p[3];

        sol_entity_p(chnk_p, vary, vary->zv[i].mi, vary->zv[i].mj);

        for (int j = 1; j < 100; j++)
        {
            glPushMatrix();
            {
                glScalef(j * SCL, 1, j * SCL);
                glTranslatef(chnk_p[0],
                             Y - 20,
                             chnk_p[2]);
                glTranslatef(base->zv[i].p[0] / (j * SCL),
                             0,
                             base->zv[i].p[2] / (j * SCL));

                glColor4ub(ROUND(c[0] * 255),
                           ROUND(c[1] * 255),
                           ROUND(c[2] * 255),
                           ROUND(c[3] * 255));
                chnk_ring_draw(rend, 1);
            }
            glPopMatrix();

            glPushMatrix();
            {
                glScalef(j * SCL, 1, j * SCL);
                glTranslatef(chnk_p[0] ,
                             Y - 10,
                             chnk_p[2]);
                glTranslatef(base->zv[i].p[0] / (j * SCL),
                             0,
                             base->zv[i].p[2] / (j * SCL));

                glColor4ub(ROUND(c[0] * 255),
                           ROUND(c[1] * 255),
                           ROUND(c[2] * 255),
                           ROUND(c[3] * 255));
                chnk_ring_draw(rend, 0);
            }
            glPopMatrix();
        }
    }
}

static void game_draw_chnk_balls(struct s_rend *rend,
                                 const struct game_draw *gd,
                                 const float *bill_M, float t)
{
    float c[4] = DRAW_COLOR4FV_CNF_MOTIONBLUR;

    const struct s_base *base =  gd->vary.base;
    const struct s_vary *vary = &gd->vary;

    /* New: Floor border damage. */

    float Y = -65536;

    if (base->vc > 0) Y = base->vv[0].p[1];

    if (base->uv[0].p[1] < Y) return;

    glPushMatrix();
    {
        glTranslatef(base->uv[0].p[0],
                     base->uv[0].p[1] - vary->uv[0].r + BALL_FUDGE,
                     base->uv[0].p[2]);

        glColor4ub(ROUND(c[0] * 255),
                   ROUND(c[1] * 255),
                   ROUND(c[2] * 255),
                   ROUND(c[3] * 255));
        chnk_ball_draw(rend);
    }
    glPopMatrix();
}

static void game_draw_chnk_jumps(struct s_rend *rend,
                                 const struct game_draw *gd,
                                 const float *bill_M, float t)
{
    float c[4] = DRAW_COLOR4FV_CNF_MOTIONBLUR;

    const struct s_base *base =  gd->vary.base;
    const struct s_vary *vary = &gd->vary;

    /* New: Floor border damage. */

    float Y = -65536;

    if (base->vc > 0) Y = base->vv[0].p[1];

    if (base->uv[0].p[1] < Y) return;

    for (int i = 0; i < base->jc; i++)
    {
        float chnk_p[3];

        sol_entity_p(chnk_p, vary, vary->jv[i].mi, vary->jv[i].mj);

        if (chnk_p[1] < Y) continue;

        const float view_angle = V_DEG(fatan2f(gd->view.e[2][0], gd->view.e[2][2]));

        glPushMatrix();
        {
            glTranslatef(chnk_p[0],
                         chnk_p[1] + 0.001f,
                         chnk_p[2]);
            glTranslatef(base->jv[i].p[0],
                         base->jv[i].p[1],
                         base->jv[i].p[2]);
            glRotatef(view_angle, 0.0f, 1.0f, 0.0f);

            glColor4ub(ROUND(c[0] * 255),
                       ROUND(c[1] * 255),
                       ROUND(c[2] * 255),
                       ROUND(c[3] * 255));
            chnk_jump_draw(rend);
        }
        glPopMatrix();
    }
}

static void game_draw_chnk_goals(struct s_rend *rend,
                                 const struct game_draw *gd,
                                 const float *bill_M, float t)
{
    float c[4] = DRAW_COLOR4FV_CNF_MOTIONBLUR;

    const struct s_base *base =  gd->vary.base;
    const struct s_vary *vary = &gd->vary;

    /* New: Floor border damage. */

    float Y = -65536;

    if (base->vc > 0) Y = base->vv[0].p[1];

    if (base->uv[0].p[1] < Y) return;

    for (int i = 0; i < base->zc; i++)
    {
        float chnk_p[3];

        sol_entity_p(chnk_p, vary, vary->zv[i].mi, vary->zv[i].mj);

        if (chnk_p[1] < Y) continue;

        const float view_angle = V_DEG(fatan2f(gd->view.e[2][0], gd->view.e[2][2]));

        glPushMatrix();
        {
            glTranslatef(chnk_p[0],
                         chnk_p[1] + 0.001f,
                         chnk_p[2]);
            glTranslatef(base->zv[i].p[0],
                         base->zv[i].p[1],
                         base->zv[i].p[2]);
            glRotatef(view_angle, 0.0f, 1.0f, 0.0f);

            glColor4ub(ROUND(c[0] * 255),
                       ROUND(c[1] * 255),
                       ROUND(c[2] * 255),
                       ROUND(c[3] * 255));
            chnk_goal_draw(rend);
        }
        glPopMatrix();
    }
}

static void game_draw_chnk_swchs(struct s_rend *rend,
                                 const struct game_draw *gd,
                                 const float *bill_M, float t)
{
    float c[4] = DRAW_COLOR4FV_CNF_MOTIONBLUR;

    const struct s_base *base =  gd->vary.base;
    const struct s_vary *vary = &gd->vary;

    /* New: Floor border damage. */

    float Y = -65536;

    if (base->vc > 0) Y = base->vv[0].p[1];

    if (base->uv[0].p[1] < Y) return;

    for (int i = 0; i < base->xc; i++)
    {
        float chnk_p[3];

        sol_entity_p(chnk_p, vary, vary->xv[i].mi, vary->xv[i].mj);

        if (chnk_p[1] < Y) continue;

        const float view_angle = V_DEG(fatan2f(gd->view.e[2][0], gd->view.e[2][2]));

        glPushMatrix();
        {
            glTranslatef(chnk_p[0],
                         chnk_p[1] + 0.001f,
                         chnk_p[2]);
            glTranslatef(base->xv[i].p[0],
                         base->xv[i].p[1],
                         base->xv[i].p[2]);
            glRotatef(view_angle, 0.0f, 1.0f, 0.0f);

            glColor4ub(ROUND(c[0] * 255),
                       ROUND(c[1] * 255),
                       ROUND(c[2] * 255),
                       ROUND(c[3] * 255));
            chnk_swch_draw(rend);
        }
        glPopMatrix();
    }
}

#ifdef MAPC_INCLUDES_CHKP
static void game_draw_chnk_chkps(struct s_rend *rend,
                                 const struct game_draw *gd,
                                 const float *bill_M, float t)
{
    float c[4] = DRAW_COLOR4FV_CNF_MOTIONBLUR;

    const struct s_base *base =  gd->vary.base;
    const struct s_vary *vary = &gd->vary;

    /* New: Floor border damage. */

    float Y = -65536;

    if (base->vc > 0) Y = base->vv[0].p[1];

    if (base->uv[0].p[1] < Y) return;

    for (int i = 0; i < base->cc; i++)
    {
        float chnk_p[3];

        sol_entity_p(chnk_p, vary, vary->cv[i].mi, vary->cv[i].mj);

        if (chnk_p[1] < Y) continue;

        const float view_angle = V_DEG(fatan2f(gd->view.e[2][0], gd->view.e[2][2]));

        glPushMatrix();
        {
            glTranslatef(chnk_p[0],
                         chnk_p[1] + 0.001f,
                         chnk_p[2]);
            glTranslatef(base->cv[i].p[0],
                         base->cv[i].p[1],
                         base->cv[i].p[2]);
            glRotatef(view_angle, 0.0f, 1.0f, 0.0f);

            glColor4ub(ROUND(c[0] * 255),
                       ROUND(c[1] * 255),
                       ROUND(c[2] * 255),
                       ROUND(c[3] * 255));
            chnk_chkp_draw(rend);
        }
        glPopMatrix();
    }
}
#endif

/*---------------------------------------------------------------------------*/

static int   max_speed_enabled = 0;     /* New: Max speed indicator          */
static float max_speed_angle = 0.0f;  /* New: Max speed indicator          */

void game_draw_set_maxspeed(float a, int f)
{
    max_speed_enabled = f;
    max_speed_angle = a;
}

static void game_draw_maxspeed(struct s_rend *rend,
                               const struct game_draw *gd)
{
    float c[4] = DRAW_COLOR4FV_CNF_MOTIONBLUR;

    const struct s_vary *vary = &gd->vary;

    const float view_angle = V_DEG(fatan2f(gd->view.e[2][0], gd->view.e[2][2]));

    glPushMatrix();

    if (max_speed_enabled)
    {
        glTranslatef(vary->uv[0].p[0],
            vary->uv[0].p[1] + BALL_FUDGE,
            vary->uv[0].p[2]);
        glRotatef(max_speed_angle + view_angle,
            0.0f, 90.0f, 0.0f);
        glScalef(vary->uv[0].r,
            vary->uv[0].r,
            vary->uv[0].r);

        glColor4ub(ROUND(c[0] * 255),
            ROUND(c[1] * 255),
            ROUND(c[2] * 255),
            ROUND(c[3] * 255));
        maxspeed_draw(rend);
    }

    glPopMatrix();
}

/*---------------------------------------------------------------------------*/

static void game_draw_balls(struct s_rend *rend,
                            const struct s_vary *vary,
                            const float *bill_M, float t)
{
    float c[4] = DRAW_COLOR4FV_CNF_MOTIONBLUR;

    float ball_M[16];
    float pend_M[16];

    m_basis(ball_M, vary->uv[0].e[0], vary->uv[0].e[1], vary->uv[0].e[2]);
    m_basis(pend_M, vary->uv[0].E[0], vary->uv[0].E[1], vary->uv[0].E[2]);

    glPushMatrix();
    {
        glTranslatef(vary->uv[0].p[0],
                     vary->uv[0].p[1] + BALL_FUDGE,
                     vary->uv[0].p[2]);
        glScalef(vary->uv[0].r,
                 vary->uv[0].r,
                 vary->uv[0].r);

        glColor4ub(ROUND(c[0] * 255),
                   ROUND(c[1] * 255),
                   ROUND(c[2] * 255),
                   ROUND(c[3] * 255));
#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_MULTIBALLS)
        ball_multi_draw_single(0, rend, ball_M, pend_M, bill_M, t);
#else
        ball_draw(rend, ball_M, pend_M, bill_M, t);
#endif
    }
    glPopMatrix();
}

static void game_draw_items(struct s_rend *rend,
                            const struct s_vary *vary,
                            const float *bill_M, float t)
{
    /* New: Floor border damage. */

    const struct s_base *base = vary->base;

    float Y = -65536;

    if (base->vc > 0) Y = base->vv[0].p[1];

    for (int hi = 0; hi < vary->hc; hi++)
    {
        struct v_item *hp = &vary->hv[hi];

        float item_p[3], item_e[4], u[3], a;

        /* Skip picked up items. */

        if (hp->t == ITEM_NONE || hp->p[1] < Y)
            continue;

        sol_entity_p(item_p, vary, hp->mi, hp->mj);
        sol_entity_e(item_e, vary, hp->mi, hp->mj);

        q_as_axisangle(item_e, u, &a);

        /* Draw model. */

        glPushMatrix();
        {
            glTranslatef(item_p[0], item_p[1], item_p[2]);
            glRotatef(V_DEG(a), u[0], u[1], u[2]);
            glTranslatef(hp->p[0], hp->p[1], hp->p[2]);
            item_draw(rend, hp, bill_M, t);
        }
        glPopMatrix();
    }
}

static void game_draw_beams(struct s_rend *rend, const struct game_draw *gd)
{
    /* Goal beams */
    static const GLfloat goal_c[4]       =   { 1.0f, 1.0f, 0.0f, 0.5f };

    /* Jump beams */
    static const GLfloat jump_c[2][4]    =  {{ 0.7f, 0.5f, 1.0f, 0.5f },
                                             { 0.7f, 0.5f, 1.0f, 0.8f }};

    /* Switch beams */
    static const GLfloat swch_c[2][2][4] = {{{ 1.0f, 0.0f, 0.0f, 0.5f },
                                             { 1.0f, 0.0f, 0.0f, 0.8f }},
                                            {{ 0.0f, 1.0f, 0.0f, 0.5f },
                                             { 0.0f, 1.0f, 0.0f, 0.8f }}};

#ifdef MAPC_INCLUDES_CHKP
    /* Checkpoint beams */
    static const GLfloat chkp_c[2][2][4] = {{{ 0.0f, 0.0f, 1.0f, 0.5f },
                                             { 0.0f, 0.0f, 1.0f, 0.8f }},
                                            {{ 1.0f, 0.0f, 1.0f, 0.5f },
                                             { 1.0f, 0.0f, 1.0f, 0.8f }}};
#endif

    const struct s_base *base =  gd->vary.base;
    const struct s_vary *vary = &gd->vary;

    /* New: Floor border damage. */

    float Y = -65536;
    if (base->vc > 0) Y = base->vv[0].p[1];

    /* Goal beams */

    float beam_p[3], beam_e[4], u[3], a;

    if (gd->goal_e)
        for (int i = 0; i < base->zc; i++)
            if (base->zv[i].p[1] >= Y)
            {
                sol_entity_p(beam_p, vary, vary->zv[i].mi, vary->zv[i].mj);
                sol_entity_e(beam_e, vary, vary->zv[i].mi, vary->zv[i].mj);

                q_as_axisangle(beam_e, u, &a);

                glPushMatrix();
                {
                    glTranslatef(beam_p[0], beam_p[1], beam_p[2]);
                    glRotatef(V_DEG(a), u[0], u[1], u[2]);
                    beam_draw(rend, base->zv[i].p, goal_c, base->zv[i].r, gd->goal_k * 3.0f);
                }
                glPopMatrix();
            }

    /* Jump beams */

    for (int i = 0; i < base->jc; i++)
        if (base->jv[i].p[1] >= Y)
        {
            sol_entity_p(beam_p, vary, vary->jv[i].mi, vary->jv[i].mj);
            sol_entity_e(beam_e, vary, vary->jv[i].mi, vary->jv[i].mj);

            q_as_axisangle(beam_e, u, &a);

            glPushMatrix();
            {
                glTranslatef(beam_p[0], beam_p[1], beam_p[2]);
                glRotatef(V_DEG(a), u[0], u[1], u[2]);
                beam_draw(rend, base->jv[i].p, jump_c[gd->jump_e ? 0 : 1], base->jv[i].r, 2.0f);
            }
            glPopMatrix();
        }

    /* Switch beams */

    for (int i = 0; i < base->xc; i++)
        if (!vary->xv[i].base->i && base->xv[i].p[1] >= Y)
        {
            sol_entity_p(beam_p, vary, vary->xv[i].mi, vary->xv[i].mj);
            sol_entity_e(beam_e, vary, vary->xv[i].mi, vary->xv[i].mj);

            q_as_axisangle(beam_e, u, &a);

            glPushMatrix();
            {
                glTranslatef(beam_p[0], beam_p[1], beam_p[2]);
                glRotatef(V_DEG(a), u[0], u[1], u[2]);
                beam_draw(rend, base->xv[i].p, swch_c[vary->xv[i].f][vary->xv[i].e], base->xv[i].r, 2.0f);
            }
            glPopMatrix();
        }

    /* Checkpoint beams */

#ifdef MAPC_INCLUDES_CHKP
    if (gd->chkp_e)
        for (int i = 0; i < base->cc; i++)
            if (base->cv[i].p[1] >= Y)
            {
                sol_entity_p(beam_p, vary, vary->cv[i].mi, vary->cv[i].mj);
                sol_entity_e(beam_e, vary, vary->cv[i].mi, vary->cv[i].mj);

                q_as_axisangle(beam_e, u, &a);

                glPushMatrix();
                {
                    glTranslatef(beam_p[0], beam_p[1], beam_p[2]);
                    glRotatef(V_DEG(a), u[0], u[1], u[2]);
                    beam_draw(rend, base->cv[i].p, chkp_c[vary->cv[i].f][vary->cv[i].e], base->cv[i].r, 2.0f);
                }
                glPopMatrix();
            }
#endif
}

static void game_draw_goals(struct s_rend *rend,
                            const struct game_draw *gd, float t)
{
    const struct s_base *base = gd->vary.base;

    /* New: Floor border damage. */

    float Y = -65536;

    if (base->vc > 0) Y = base->vv[0].p[1];

    const struct s_vary *vary = &gd->vary;

    float goal_p[3], goal_e[4], u[3], a;

    if (gd->goal_e)
        for (int zi = 0; zi < base->zc; zi++)
            if (base->zv[zi].p[1] >= Y)
            {
                sol_entity_p(goal_p, vary, vary->zv[zi].mi, vary->zv[zi].mj);
                sol_entity_e(goal_e, vary, vary->zv[zi].mi, vary->zv[zi].mj);

                q_as_axisangle(goal_e, u, &a);

                glPushMatrix();
                {
                    glTranslatef(goal_p[0], goal_p[1], goal_p[2]);
                    glRotatef(V_DEG(a), u[0], u[1], u[2]);
                    goal_draw(rend, base->zv[zi].p, base->zv[zi].r, gd->goal_k, t);
                }
                glPopMatrix();
            }
}

static void game_draw_jumps(struct s_rend *rend,
                            const struct game_draw *gd, float t)
{
    const struct s_base *base = gd->vary.base;

    /* New: Floor border damage. */

    float Y = -65536;

    if (base->vc > 0) Y = base->vv[0].p[1];

    const struct s_vary *vary = &gd->vary;

    float jump_p[3], jump_e[4], u[3], a;

    for (int ji = 0; ji < base->jc; ji++)
        if (base->jv[ji].p[1] >= Y)
        {
            sol_entity_p(jump_p, vary, vary->jv[ji].mi, vary->jv[ji].mj);
            sol_entity_e(jump_e, vary, vary->jv[ji].mi, vary->jv[ji].mj);

            q_as_axisangle(jump_e, u, &a);

            glPushMatrix();
            {
                glTranslatef(jump_p[0], jump_p[1], jump_p[2]);
                glRotatef(V_DEG(a), u[0], u[1], u[2]);
                jump_draw(rend, base->jv[ji].p, base->jv[ji].r, 1.0f, t);
            }
            glPopMatrix();
        }
}

#ifdef MAPC_INCLUDES_CHKP
static void game_draw_chkps(struct s_rend *rend,
                            const struct game_draw *gd, float t)
{
    const struct s_base *base = gd->vary.base;

    /* New: Floor border damage. */

    float Y = -65536;

    if (base->vc > 0) Y = base->vv[0].p[1];

    const struct s_vary *vary = &gd->vary;

    float chkp_p[3], chkp_e[4], u[3], a;

    for (int ci = 0; ci < base->cc; ci++)
        if (base->cv[ci].p[1] >= Y)
        {
            sol_entity_p(chkp_p, vary, vary->cv[ci].mi, vary->cv[ci].mj);
            sol_entity_e(chkp_e, vary, vary->cv[ci].mi, vary->cv[ci].mj);

            q_as_axisangle(chkp_e, u, &a);

            glPushMatrix();
            {
                glTranslatef(chkp_p[0], chkp_p[1], chkp_p[2]);
                glRotatef(V_DEG(a), u[0], u[1], u[2]);
                chkp_draw(rend, base->cv[ci].p, base->cv[ci].r, 1.0f);
            }
            glPopMatrix();
        }
}
#endif

/*---------------------------------------------------------------------------*/

static void game_draw_tilt(const struct game_draw *gd, int d, int flip)
{
    static const float Y[3] = { 0.0f, 1.0f, 0.0f };

    const struct game_tilt *tilt = &gd->tilt;
    const float *ball_p = gd->vary.uv[0].p;

    float axis[3], angle;

    q_as_axisangle(tilt->q, axis, &angle);

    if (!flip)
        v_reflect(axis, axis, Y);

    /*
     * Rotate the environment about the position of the ball.
     * See Git-issues #167, which you don't include tilting the floor.
     */
    if (config_get_d(CONFIG_TILTING_FLOOR)
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
     && !(campaign_used()
      || (curr_mode() == MODE_CAMPAIGN
       || curr_mode() == MODE_HARDCORE))
#endif
        )
    {
        glTranslatef(+ball_p[0], +ball_p[1] * d, +ball_p[2]);

        glRotatef(V_DEG(angle), axis[0], axis[1], axis[2]);

        glTranslatef(-ball_p[0], -ball_p[1] * d, -ball_p[2]);
    }
}

static void game_refl_all(struct s_rend *rend, const struct game_draw *gd)
{
    glPushMatrix();
    {
        game_draw_tilt(gd, 1, 0);

        /* Draw the floor. */

        sol_refl(&gd->draw, rend);
    }
    glPopMatrix();
}

/*---------------------------------------------------------------------------*/

#if ENABLE_MOTIONBLUR==1
static int motionblur_refl_allow_draw_back;
#endif

static void game_draw_light(const struct game_draw *gd, int d, float t)
{
    /* Configure the lighting. */

    light_conf();

    /* Overrride light 2 position. */

    GLfloat p[4] = { cosf(t), 0.0f, sinf(t), 0.0f };

    glLightfv(GL_LIGHT2, GL_POSITION, p);

    /* Enable scene lights. */

    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
}

static void game_draw_back(struct s_rend *rend,
                           const struct game_draw *gd,
                           int pose, int d, float t, int flip)
{
#if ENABLE_MOTIONBLUR==1
    if (video_can_swap_window && !motionblur_refl_allow_draw_back)
        return;

    if (config_get_d(CONFIG_MOTIONBLUR))
        video_can_swap_window = 1;
#endif

    if (pose == POSE_BALL)
        return;

    glPushMatrix();
    {
        const struct game_view *view = &gd->view;

        if (d < 0)
        {
            const struct game_tilt *tilt = &gd->tilt;
            static const float Y[3] = { 0.0f, 1.0f, 0.0f };
            float axis[3], angle = 1;

            q_as_axisangle(tilt->q, axis, &angle);

            if (flip)
                v_reflect(axis, axis, Y);

            /* See Git-issues #167, which you don't include tilting the floor. */
            if (config_get_d(CONFIG_TILTING_FLOOR)
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
             && !(campaign_used()
               || (curr_mode() == MODE_CAMPAIGN
                || curr_mode() == MODE_HARDCORE))
#endif
                )
                glRotatef(V_DEG(-angle), axis[0], axis[1], axis[2]);
        }

        glTranslatef(view->p[0], view->p[1] * d, view->p[2]);

        back_draw(rend);

        if (config_get_d(CONFIG_BACKGROUND))
            sol_back(&gd->back.draw, rend, 0, FAR_DIST, t);
    }
    glPopMatrix();
}

static void game_clip_refl(int d)
{
    /* Fudge to eliminate the floor from reflection. */

    glClipPlane4f_(GL_CLIP_PLANE0, 0, 1, 0, -0.00001);
}

static void game_clip_ball(const struct game_draw *gd, int d, const float *p)
{
    /* Compute the plane giving the front of the ball, as seen from view.p. */

    GLfloat r,
            c [3] = { p[0], p[1] * d, p[2] },
            pz[4] = { 0.0f, 0.0f, 0.0f, 0.0f },
            nz[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    pz[0] = gd->view.p[0] - c[0];
    pz[1] = gd->view.p[1] - c[1];
    pz[2] = gd->view.p[2] - c[2];

    r = sqrt(pz[0] * pz[0] + pz[1] * pz[1] + pz[2] * pz[2]);

    pz[0] /= r;
    pz[1] /= r;
    pz[2] /= r;
    pz[3] = -(pz[0] * c[0] +
              pz[1] * c[1] +
              pz[2] * c[2]);

    /* Find the plane giving the back of the ball, as seen from view.p. */

    nz[0] = -pz[0];
    nz[1] = -pz[1];
    nz[2] = -pz[2];
    nz[3] = -pz[3];

    /* Reflect these planes as necessary, and store them in the GL state. */

    pz[1] *= d;
    nz[1] *= d;

    glClipPlane4f_(GL_CLIP_PLANE1, nz[0], nz[1], nz[2], nz[3]);
    glClipPlane4f_(GL_CLIP_PLANE2, pz[0], pz[1], pz[2], pz[3]);
}

static void game_draw_fore(struct s_rend *rend,
                           struct game_draw *gd,
                           int pose, const float *M,
                           int d, float t, int flip)
{
    const float *ball_p = gd->vary.uv[0].p;

    struct s_draw *draw = &gd->draw;

    glPushMatrix();
    {
        /* Rotate the environment about the position of the ball. */

        game_draw_tilt(gd, d, flip);

        /* Compute clipping planes for reflection and ball facing. */

        game_clip_refl(d);
        game_clip_ball(gd, d, ball_p);

        if (d < 0) glEnable(GL_CLIP_PLANE0);

#if !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__)
        if (!config_cheat()) glEnable(GL_FOG);
#endif

        if (draw && rend) switch (pose)
        {
            case POSE_LEVEL:
                sol_draw(draw, rend, 0, 1);
                break;

            case POSE_BALL:
                if (curr_tex_env == &tex_env_pose)
                {
                    /*
                     * We need the check above because otherwise the
                     * active texture env is set up in a way that makes
                     * level geometry visible, and we don't want that.
                     */

                    glDepthMask(GL_FALSE);
                    sol_draw   (draw, rend, 0, 1);
                    glDepthMask(GL_TRUE);
                }
                game_draw_balls(rend, draw->vary, M, t);
                break;

            case POSE_NONE:
                /* Draw the coins. */

                game_draw_items(rend, draw->vary, M, t);

                /* Draw the floor. */

                sol_draw(draw, rend, 0, 1);

                /* Draw the ball. */

                game_draw_balls(rend, draw->vary, M, t);

                game_draw_maxspeed(rend, gd);

                break;
        }

        glDepthMask(GL_FALSE);

        if (draw && rend && gd)
        {
            /* Draw the billboards, entity beams, and coin particles. */

            sol_bill(draw, rend, M, t);

            game_draw_beams(rend, gd);
            part_draw_coin(draw, rend, M, t);
            part_draw_goal(draw, rend, M, t);

            /* Draw the entity particles using only the sparkle light. */

            glDisable(GL_LIGHT0);
            glDisable(GL_LIGHT1);
            glEnable (GL_LIGHT2);
            {
                game_draw_goals(rend, gd, t);
                game_draw_jumps(rend, gd, t);
#ifdef MAPC_INCLUDES_CHKP
                game_draw_chkps(rend, gd, t);
#endif
            }
            glDisable(GL_LIGHT2);
            glEnable (GL_LIGHT1);
            glEnable (GL_LIGHT0);
        }

        glDepthMask(GL_TRUE);

#if !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__)
        if (!config_cheat()) glDisable(GL_FOG);
#endif

        if (d < 0) glDisable(GL_CLIP_PLANE0);
    }
    glPopMatrix();
}

static void game_draw_fore_chnk(struct s_rend *rend,
                                struct game_draw *gd,
                                int pose, const float *M,
                                int d, float t, int flip, int nofullmap)
{
    const float *ball_p = gd->vary.uv[0].p;

    struct s_draw* draw = &gd->draw;

    glPushMatrix();
    {
        /* Rotate the environment about the position of the ball. */

        game_draw_tilt(gd, d, flip);

        /* Compute clipping planes for reflection and ball facing. */

        game_clip_refl(d);
        game_clip_ball(gd, d, ball_p);

        if (d < 0) glEnable(GL_CLIP_PLANE0);

        if (draw && rend && !nofullmap) switch (pose)
        {
            case POSE_BALL:
                /* No render available for map chunk overview. */
                break;

            default:
                sol_draw(draw, rend, 0, 1);
        }

        glDepthMask(GL_FALSE);

        if (draw && rend && gd)
        {
            /* Draw the billboards only. */

            sol_bill(draw, rend, M, t);
        }

        glDepthMask(GL_TRUE);

        if (d < 0) glDisable(GL_CLIP_PLANE0);

        glDepthMask(GL_FALSE);

        if (rend && gd)
        {
            /* Draw the map chunk overview. */

            game_draw_chnk_rings(rend, gd, M, t);
            game_draw_chnk_floor(rend, gd, M, t);
            game_draw_chnk_balls(rend, gd, M, t);
            game_draw_chnk_jumps(rend, gd, M, t);
            game_draw_chnk_goals(rend, gd, M, t);
            game_draw_chnk_swchs(rend, gd, M, t);
#ifdef MAPC_INCLUDES_CHKP
            game_draw_chnk_chkps(rend, gd, M, t);
#endif
        }

        glDepthMask(GL_TRUE);
    }
    glPopMatrix();
}

static void game_draw_fog()
{
    GLfloat fog_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

#if !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__)
    glFogfv(GL_FOG_COLOR,   fog_color);
    glFogf (GL_FOG_MODE,    GL_EXP2);
    glFogf (GL_FOG_DENSITY, 0.0125);
#endif
}

/*---------------------------------------------------------------------------*/

static struct game_lerp_pose game_lerp_pose_v;

float gd_rotate_roll;

static void game_shadow_conf(int pose, int enable)
{
    if (enable && config_get_d(CONFIG_SHADOW))
    {
        switch (pose)
        {
            case POSE_LEVEL:
                /* No shadow. */
                tex_env_active(&tex_env_default);
                break;

            case POSE_BALL:
                /* Shadow only. */
                tex_env_select(&tex_env_pose,
                               &tex_env_default,
                               NULL);
                break;

            default:
                /* Regular shadow. */
                tex_env_select(&tex_env_shadow_clip,
                               &tex_env_shadow,
                               &tex_env_default,
                               NULL);
                break;
        }
    }
    else
    {
        tex_env_active(&tex_env_default);
    }
}

void game_draw(struct game_draw *gd, int pose, float t)
{
    float fov = (float) config_get_d(CONFIG_VIEW_FOV);

    /* if (gd->jump_b) fov *= 2.0f * fabsf(gd->jump_dt - 0.5f); */
    if (gd->jump_b) fov *= (fcosf(gd->jump_dt * (2 * V_PI)) / 2) + 0.5f;

    if (gd->state)
    {
#if ENABLE_MOTIONBLUR==1
        motionblur_refl_allow_draw_back = 0;
#endif

        float c[4] = DRAW_COLOR4FV_CNF_MOTIONBLUR;

        const struct game_view *view = &gd->view;
        struct s_rend rend;

        gd->draw.shadow_ui = 0;

        game_draw_fog();

        game_shadow_conf(pose, 1);
        r_draw_enable(&rend);
        
        const float effective_fov =
            CLAMP(30,
                  (fov / (gd->mojang_death_enabled_flags ? 1.25f : 1.0f)) +
                  (25 * (gd->mojang_death_time_percent / 100.f)), 110);

        video_set_perspective(effective_fov, 0.1f, FAR_DIST);
        glPushMatrix();
        {
            glRotatef(game_lerp_pose_v.pose_point_smooth_x, 0.0f, 1.0f, 0.0f);
            glRotatef(game_lerp_pose_v.pose_point_smooth_y, 1.0f, 0.0f, 0.0f);

            glRotatef(gd_rotate_roll, 0.0f, 0.0f, 1.0f);

            float T[16], U[16], M[16],
                  v[3] = { +view->p[0], -view->p[1], +view->p[2] }; /* Compute direct and reflected view bases. */

            video_calc_view(T, view->c, view->p, view->e[1]);
            video_calc_view(U, view->c, v,       view->e[1]);
            m_xps(M, T);

            /* Apply the current view. */

            v_sub(v, view->c, view->p);

            glTranslatef(0.0f, 0.0f, -v_len(v));
            glMultMatrixf(M);
            glTranslatef(-view->c[0], -view->c[1], -view->c[2]);

            /* Draw the background. */
            
            if (!(view->p[1] > gd->vary.uv[0].p[1] + 100 &&
                  view->p[1] > gd->vary.base->vv[0].p[1] + 100))
            {
                game_draw_back(&rend, gd, pose, +1, t, 0);
                draw_chnk_highaltitude = 0;
            }
            else draw_chnk_highaltitude = 1;

            /* If the view altitude is greater than 15 m from the surface,   */
            /* map chunk must be rendered from their overview.               */

            if (view->p[1] > gd->vary.uv[0].p[1] + 15 &&
                view->p[1] > gd->vary.base->vv[0].p[1] + 15)
            {
                game_draw_light(gd, 1, t);

                /* Draw the map chunk overlay. Must be opaqued for mirrors.  */

                r_color_mtrl(&rend, 1);
                {
                    glColor4ub   (0, 0, 0, ROUND(c[3] * 255));
                    game_refl_all(&rend, gd);
                    glColor4ub   (ROUND(c[0] * 255),
                                  ROUND(c[1] * 255),
                                  ROUND(c[2] * 255),
                                  ROUND(c[3] * 255));
                }
                r_color_mtrl(&rend, 0);

                game_refl_all(&rend, gd);

                game_draw_fore_chnk(&rend, gd, pose, U, +1, t, 0,
                                    draw_chnk_highaltitude);
            }
            else
            {
                /* Draw the reflection. */

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__)
                if (gd->draw.reflective && config_get_d(CONFIG_REFLECTION))
                {
#if ENABLE_MOTIONBLUR==1
                    if (config_get_d(CONFIG_MOTIONBLUR))
                        motionblur_refl_allow_draw_back = 1;
#endif

                    glEnable(GL_STENCIL_TEST);

                    /* Draw the mirrors only into the stencil buffer. */

                    glStencilFunc(GL_ALWAYS, 1, 0xFFFFFFFF);
                    glStencilOp  (GL_DECR, GL_DECR, GL_REPLACE);
                    glColorMask  (GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                    glDepthMask  (GL_FALSE);

                    game_refl_all(&rend, gd);

                    glDepthMask  (GL_TRUE);
                    glColorMask  (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                    glStencilOp  (GL_KEEP, GL_KEEP, GL_KEEP);
                    glStencilFunc(GL_EQUAL, 1, 0xFFFFFFFF);

                    /* Draw the scene reflected into color and depth buffers. */

                    glFrontFace(GL_CW);
                    glPushMatrix();
                    {
                        glScalef(+1.0f, -1.0f, +1.0f);

                        game_draw_light(gd, -1, t);

                        game_draw_back(&rend, gd, pose,    -1, t, 1);
                        game_draw_fore(&rend, gd, pose, U, -1, t, 1);
                    }
                    glPopMatrix();
                    glFrontFace(GL_CCW);

                    glStencilFunc(GL_ALWAYS, 0, 0xFFFFFFFF);
                    glDisable(GL_STENCIL_TEST);
                }
#endif

                /* Ready the lights for foreground rendering. */

                game_draw_light(gd, 1, t);

                /* When reflection is disabled, mirrors must be rendered opaque  */
                /* to prevent the background from showing.                       */

                if (gd->draw.reflective && !config_get_d(CONFIG_REFLECTION))
                {
                    r_color_mtrl(&rend, 1);
                    {
                        glColor4ub   (0, 0, 0, ROUND(c[3] * 255));
                        game_refl_all(&rend, gd);
                        glColor4ub   (ROUND(c[0] * 255),
                                      ROUND(c[1] * 255),
                                      ROUND(c[2] * 255),
                                      ROUND(c[3] * 255));
                    }
                    r_color_mtrl(&rend, 0);
                }

#if !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__)
                if (!config_cheat()) glEnable(GL_FOG);
#endif

                /* Draw the mirrors and the rest of the foreground. */

                game_refl_all (&rend, gd);
                game_draw_fore(&rend, gd, pose, T, +1, t, 0);

#if !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__)
                if (!config_cheat()) glDisable(GL_FOG);
#endif
            }
        }
        glPopMatrix();

        /* Draw the fade overlay. */

        sol_fade(&gd->draw, &rend, gd->fade_k);

        r_draw_disable(&rend);
        game_shadow_conf(pose, 0);
    }
}

/*---------------------------------------------------------------------------*/

#define CURR 0
#define PREV 1

void game_lerp_init(struct game_lerp *gl, struct game_draw *gd)
{
    game_lerp_pose_point_init();
    gl->alpha = 1.0f;

    sol_load_lerp(&gl->lerp, &gd->vary);

    gl->tilt[PREV] = gl->tilt[CURR] = gd->tilt;
    gl->view[PREV] = gl->view[CURR] = gd->view;

    gl->goal_k[PREV] = gl->goal_k[CURR] = gd->goal_k;
#ifdef MAPC_INCLUDES_CHKP
    gl->chkp_k[PREV] = gl->chkp_k[CURR] = gd->chkp_k;
#endif
    gl->jump_dt[PREV] = gl->jump_dt[CURR] = gd->jump_dt;
}

void game_lerp_free(struct game_lerp *gl)
{
    sol_free_lerp(&gl->lerp);
}

void game_lerp_copy(struct game_lerp *gl)
{
    sol_lerp_copy(&gl->lerp);

    gl->tilt[PREV] = gl->tilt[CURR];
    gl->view[PREV] = gl->view[CURR];

    gl->goal_k[PREV] = gl->goal_k[CURR];
#ifdef MAPC_INCLUDES_CHKP
    gl->chkp_k[PREV] = gl->chkp_k[CURR];
#endif
    gl->jump_dt[PREV] = gl->jump_dt[CURR];
}

void game_lerp_apply(struct game_lerp *gl, struct game_draw *gd)
{
    float a = gl->alpha;

    /* Solid. */

    sol_lerp_apply(&gl->lerp, a);

    /* Particles. */

    part_lerp_apply(a);

    /* Tilt. */

    v_lerp(gd->tilt.x, gl->tilt[PREV].x, gl->tilt[CURR].x, a);
    v_lerp(gd->tilt.z, gl->tilt[PREV].z, gl->tilt[CURR].z, a);

    q_slerp(gd->tilt.q, gl->tilt[PREV].q, gl->tilt[CURR].q, a);

    /* View. */

    v_lerp(gd->view.c, gl->view[PREV].c, gl->view[CURR].c, a);
    v_lerp(gd->view.p, gl->view[PREV].p, gl->view[CURR].p, a);
    e_lerp(gd->view.e, gl->view[PREV].e, gl->view[CURR].e, a);

    /* Effects. */

    gd->goal_k = flerp(gl->goal_k[PREV], gl->goal_k[CURR], a);
#ifdef MAPC_INCLUDES_CHKP
    gd->chkp_k = flerp(gl->chkp_k[PREV], gl->chkp_k[CURR], a);
#endif
    gd->jump_dt = flerp(gl->jump_dt[PREV], gl->jump_dt[CURR], a);
}

/*---------------------------------------------------------------------------*/

void game_lerp_pose_point_tick(float deltatime)
{
    if (&game_lerp_pose_v)
    {
        game_lerp_pose_v.pose_point_smooth_x = flerp(game_lerp_pose_v.pose_point_smooth_x, game_lerp_pose_v.pose_point_x, deltatime * 2);
        game_lerp_pose_v.pose_point_smooth_y = flerp(game_lerp_pose_v.pose_point_smooth_y, game_lerp_pose_v.pose_point_y, deltatime * 2);
    }
}

void game_lerp_pose_point_init(void)
{
    memset(&game_lerp_pose_v, 0, sizeof (struct game_lerp_pose));
}

void game_lerp_pose_point(int dx, int dy)
{
    game_lerp_pose_v.pose_point_x += dx;
    game_lerp_pose_v.pose_point_x = CLAMP(-60, game_lerp_pose_v.pose_point_x, 60);
    game_lerp_pose_v.pose_point_y += dy * (config_get_d(CONFIG_MOUSE_INVERT) ? 1 : -1);
    game_lerp_pose_v.pose_point_y = CLAMP(-60, game_lerp_pose_v.pose_point_y, 60);
}

void game_lerp_pose_point_reset(void)
{
    game_lerp_pose_v.pose_point_x = 0;
    game_lerp_pose_v.pose_point_y = 0;
}

/*---------------------------------------------------------------------------*/
