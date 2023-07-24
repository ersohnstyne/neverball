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
#include "solid_draw.h"

#include "game_draw.h"

/*---------------------------------------------------------------------------*/

static void game_draw_balls(struct s_rend *rend,
                            const struct s_vary *vary,
                            const float *bill_M, float t)
{
    float c[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

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
        ball_draw(rend, ball_M, pend_M, bill_M, t);
    }
    glPopMatrix();
}

static void game_draw_items(struct s_rend *rend,
                            const struct s_vary *vary,
                            const float *bill_M, float t)
{
    int hi;

    for (hi = 0; hi < vary->hc; hi++)
    {
        struct v_item *hp = &vary->hv[hi];

        /* Skip picked up items. */

        if (hp->t == ITEM_NONE)
            continue;

        /* Draw model. */

        glPushMatrix();
        {
            glTranslatef(hp->p[0],
                         hp->p[1],
                         hp->p[2]);
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

    /* Checkpoint beams */
    static const GLfloat chkp_c[2][2][4] = {{{ 0.0f, 0.0f, 1.0f, 0.5f },
                                             { 0.0f, 0.0f, 1.0f, 0.8f }},
                                            {{ 1.0f, 0.0f, 1.0f, 0.5f },
                                             { 1.0f, 0.0f, 1.0f, 0.8f }}};

    const struct s_base *base =  gd->vary.base;
    const struct s_vary *vary = &gd->vary;

    /* Goal beams */

    if (gd->goal_e)
        for (int i = 0; i < base->zc; i++)
            beam_draw(rend, base->zv[i].p, goal_c,
                base->zv[i].r, gd->goal_k * 3.0f);

    /* Jump beams */

    for (int i = 0; i < base->jc; i++)
        beam_draw(rend, base->jv[i].p, jump_c[gd->jump_e ? 0 : 1],
            base->jv[i].r, 2.0f);

    /* Switch beams */

    for (int i = 0; i < base->xc; i++)
        if (!vary->xv[i].base->i)
            beam_draw(rend, base->xv[i].p, swch_c[vary->xv[i].f][vary->xv[i].e],
                base->xv[i].r, 2.0f);

    /* Checkpoint beams */

#ifdef MAPC_INCLUDES_CHKP
    if (gd->chkp_e)
        for (int i = 0; i < base->cc; i++)
            beam_draw(rend, base->cv[i].p, chkp_c[vary->cv[i].f][vary->cv[i].e],
                base->cv[i].r, gd->chkp_k * 2.0f);
#endif
}

static void game_draw_goals(struct s_rend *rend,
                            const struct game_draw *gd, float t)
{
    const struct s_base *base = gd->vary.base;

    int i;

    if (gd->goal_e)
        for (i = 0; i < base->zc; i++)
            goal_draw(rend, base->zv[i].p, base->zv[i].r, gd->goal_k, t);
}

static void game_draw_jumps(struct s_rend *rend,
                            const struct game_draw *gd, float t)
{
    const struct s_base *base = gd->vary.base;

    int i;

    for (i = 0; i < base->jc; i++)
        jump_draw(rend, base->jv[i].p, base->jv[i].r, 1.0f, t);
}

static void game_draw_chkps(struct s_rend *rend,
                            const struct game_draw *gd, float t)
{
    const struct s_base *base = gd->vary.base;

    int i;

#ifdef MAPC_INCLUDES_CHKP
    for (i = 0; i < base->cc; i++)
        chkp_draw(rend, base->cv[i].p, base->cv[i].r, 1.0f);
#endif
}

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

static void game_refl_all(struct s_rend *rend, const struct game_draw *gd, int flip)
{
    glPushMatrix();
    {
        game_draw_tilt(gd, 1, flip);

        /* Draw the floor. */

        sol_refl(&gd->draw, rend);
    }
    glPopMatrix();
}

/*---------------------------------------------------------------------------*/

//#define LIGHT_FIXED_INSTALLATION

static void game_draw_light(const struct game_draw *gd, int d, float t)
{
    GLfloat p[4];

    /* Configure the lighting. */

    light_conf();

    /* Overrride light 2 position. */

    p[0] = cosf(t);
    p[1] = 0.0f;
    p[2] = sinf(t);
    p[3] = 0.0f;

    glLightfv(GL_LIGHT2, GL_POSITION, p);

    /* Enable scene lights. */

    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
}

static void game_draw_back(struct s_rend *rend,
                           const struct game_draw *gd,
                           int pose, int d, float t, int flip)
{
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

            if (!flip)
                v_reflect(axis, axis, Y);

            /* See Git-issues #167, which you don't include tilting the floor. */
            if (config_get_d(CONFIG_TILTING_FLOOR)
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
             && !(campaign_used()
              || (curr_mode() == MODE_CAMPAIGN
               || curr_mode() == MODE_HARDCORE))
#endif
                )
                glRotatef(V_DEG(angle), axis[0], axis[1], axis[2]);
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
    GLfloat r, c[3], pz[4], nz[4];

    /* Compute the plane giving the front of the ball, as seen from view.p. */

    c[0] = p[0];
    c[1] = p[1] * d;
    c[2] = p[2];

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

        if (d < 0)
            glEnable(GL_CLIP_PLANE0);

        switch (pose)
        {
        case POSE_LEVEL:
            game_draw_items(rend, draw->vary, M, t);
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
                sol_draw(draw, rend, 0, 1);
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

            break;
        }

        glDepthMask(GL_FALSE);
        {
            /* Draw the billboards, entity beams, and coin particles. */

            glDisable(GL_LIGHTING);
            {
                sol_bill(draw, rend, M, t);

                game_draw_beams(rend, gd);
                part_draw_coin(rend);
            }
            glEnable(GL_LIGHTING);

            /* Draw the entity particles using only the sparkle light. */

            glDisable(GL_LIGHT0);
            glDisable(GL_LIGHT1);
            glEnable (GL_LIGHT2);
            {
                game_draw_goals(rend, gd, t);
                game_draw_jumps(rend, gd, t);
                game_draw_chkps(rend, gd, t);
            }
            glDisable(GL_LIGHT2);
            glEnable (GL_LIGHT1);
            glEnable (GL_LIGHT0);
        }
        glDepthMask(GL_TRUE);

        if (d < 0)
            glDisable(GL_CLIP_PLANE0);
    }
    glPopMatrix();
}

static void game_draw_fog()
{
    GLfloat fog_color[4]; fog_color[0] = 1.0f; fog_color[1] = 1.0f; fog_color[2] = 1.0f; fog_color[3] = 1.0f;
    glDisable(GL_FOG);
    glFogfv(GL_FOG_COLOR, fog_color);
    glFogf(GL_FOG_MODE, GL_EXP2);
    glFogf(GL_FOG_DENSITY, 0.0125);
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

    //if (gd->jump_b) fov *= 2.f * fabsf(gd->jump_dt - 0.5f);
    if (gd->jump_b) fov *= (fcosf(gd->jump_dt * (2 * V_PI)) / 2) + 0.5f;

    if (gd->state)
    {
        const struct game_view *view = &gd->view;
        struct s_rend rend;

        gd->draw.shadow_ui = 0;

        game_draw_fog();

        game_shadow_conf(pose, 1);
        r_draw_enable(&rend);

        video_push_persp(fov, 0.1f, FAR_DIST);
        glPushMatrix();
        {
#if ENABLE_EARTHQUAKE==1
            game_randomize_earthquake_shake();
            float earthquake_rotation[3];
            earthquake_rotation[0] = game_get_earthquake_shake()[0];
            earthquake_rotation[1] = game_get_earthquake_shake()[1];
            earthquake_rotation[2] = game_get_earthquake_shake()[2];
            glRotatef(1.0f, earthquake_rotation[0], earthquake_rotation[1], earthquake_rotation[2]);
#endif

            if (&game_lerp_pose_v)
            {
                glRotatef(game_lerp_pose_v.pose_point_smooth_x, 0.0f, 1.0f, 0.0f);
                glRotatef(game_lerp_pose_v.pose_point_smooth_y, 1.0f, 0.0f, 0.0f);
            }

            glRotatef(gd_rotate_roll, 0.0f, 0.0f, 1.0f);

            float T[16], U[16], M[16], v[3];

            /* Compute direct and reflected view bases. */

            v[0] = +view->p[0];
            v[1] = -view->p[1];
            v[2] = +view->p[2];

            video_calc_view(T, view->c, view->p, view->e[1]);
            video_calc_view(U, view->c, v,       view->e[1]);
            m_xps(M, T);

            /* Apply the current view. */

            v_sub(v, view->c, view->p);

            glTranslatef(0.f, 0.f, -v_len(v));
            glMultMatrixf(M);
            glTranslatef(-view->c[0], -view->c[1], -view->c[2]);

            /* Draw the background. */

            game_draw_back(&rend, gd, pose, +1, t, 0);

            /* Draw the reflection. */

            game_draw_light(gd, 1, t);

            if (gd->draw.reflective && config_get_d(CONFIG_REFLECTION))
            {
                glEnable(GL_STENCIL_TEST);
                {
                    /* Draw the mirrors only into the stencil buffer. */

                    glStencilFunc(GL_ALWAYS, 1, 0xFFFFFFFF);
                    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
                    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                    glDepthMask(GL_FALSE);

                    game_refl_all(&rend, gd, 0);

                    glDepthMask(GL_TRUE);
                    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
                    glStencilFunc(GL_EQUAL, 1, 0xFFFFFFFF);

                    /* Draw the scene reflected into color and depth buffers. */

                    glFrontFace(GL_CW);
                    glPushMatrix();
                    {
                        glScalef(+1.0f, -1.0f, +1.0f);

                        game_draw_light(gd, -1, t);
                        
                        game_draw_back(&rend, gd, pose,    -1, t, 1);
                        if (!config_cheat())
                            glEnable(GL_FOG);
                        game_draw_fore(&rend, gd, pose, U, -1, t, 1);
                        glDisable(GL_FOG);
                    }
                    glPopMatrix();
                    glFrontFace(GL_CCW);

                    glStencilFunc(GL_ALWAYS, 0, 0xFFFFFFFF);
                }
                glDisable(GL_STENCIL_TEST);
            }

            /* Ready the lights for foreground rendering. */

            game_draw_light(gd, 1, t);

            /* When reflection is disabled, mirrors must be rendered opaque  */
            /* to prevent the background from showing.                       */

            if (gd->draw.reflective && !config_get_d(CONFIG_REFLECTION))
            {
                r_color_mtrl(&rend, 1);
                {
                    glColor4ub(0, 0, 0, 0xFF);
                    game_refl_all(&rend, gd, 0);
                    glColor4ub(0xFF, 0xFF, 0xFF, 0xFF);
                }
                r_color_mtrl(&rend, 0);
            }

            /* Draw the mirrors and the rest of the foreground. */

            game_refl_all (&rend, gd, 0);
            if (!config_cheat())
                glEnable(GL_FOG);
            game_draw_fore(&rend, gd, pose, T, +1, t, 0);
            glDisable(GL_FOG);
        }
        glPopMatrix();
        video_pop_matrix();

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
    if (&game_lerp_pose_v)
    {
        game_lerp_pose_v.pose_point_x += dx;
        game_lerp_pose_v.pose_point_x = CLAMP(-60, game_lerp_pose_v.pose_point_x, 60);
        game_lerp_pose_v.pose_point_y += dy * (config_get_d(CONFIG_MOUSE_INVERT) ? 1 : -1);
        game_lerp_pose_v.pose_point_y = CLAMP(-60, game_lerp_pose_v.pose_point_y, 60);
    }
}

void game_lerp_pose_point_reset(void)
{
    if (&game_lerp_pose_v)
    {
        game_lerp_pose_v.pose_point_x = 0;
        game_lerp_pose_v.pose_point_y = 0;
    }
}

/*---------------------------------------------------------------------------*/
