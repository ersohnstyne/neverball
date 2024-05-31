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

#include <stdlib.h>
#include <string.h>

#if NB_HAVE_PB_BOTH==1
#include "account.h"
#endif

#include "vec3.h"
#include "glext.h"
#include "config.h"
#include "common.h"

#include "solid_draw.h"
#include "solid_sim.h"

/*---------------------------------------------------------------------------*/

#define F_PENDULUM   1
#define F_DRAWBACK   2
#define F_DRAWCLIP   4
#define F_DEPTHMASK  8
#define F_DEPTHTEST 16

#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_MULTIBALLS)

/*
 * HACK: Does it mean, have we multiple balls in the
 * marble ball selection of 'em?
 *
 *   - Center: huge
 *   - Semi-side: small
 *   - Side: small
 */
struct ball_full
{
    int multi_has_solid;
    int multi_has_inner;
    int multi_has_outer;

    struct s_full multi_solid;
    struct s_full multi_inner;
    struct s_full multi_outer;

    int multi_solid_flags;
    int multi_inner_flags;
    int multi_outer_flags;

    int allow_edit_model;               /* Allows you to edit selected model */
};

static struct ball_full ball_full_v[5];

static int ball_idx = 2;
static int tmp_ball_idx = 0;

#define has_solid ball_full_v[ball_idx].multi_has_solid
#define has_inner ball_full_v[ball_idx].multi_has_inner
#define has_outer ball_full_v[ball_idx].multi_has_outer

#define solid ball_full_v[ball_idx].multi_solid
#define inner ball_full_v[ball_idx].multi_inner
#define outer ball_full_v[ball_idx].multi_outer

#define solid_flags ball_full_v[ball_idx].multi_solid_flags
#define inner_flags ball_full_v[ball_idx].multi_inner_flags
#define outer_flags ball_full_v[ball_idx].multi_outer_flags

#else

static int has_solid = 0;
static int has_inner = 0;
static int has_outer = 0;

static struct s_full solid;
static struct s_full inner;
static struct s_full outer;

static int solid_flags;
static int inner_flags;
static int outer_flags;

#endif

/*---------------------------------------------------------------------------*/

#define SET(B, v, b) ((v) ? ((B) | (b)) : ((B) & ~(b)))

static int ball_opts(const struct s_base *base)
{
    int flags = F_DEPTHTEST;
    int di;

    for (di = 0; di < base->dc; ++di)
    {
        char *k = base->av + base->dv[di].ai;
        char *v = base->av + base->dv[di].aj;

        if (strcmp(k, "pendulum")  == 0)
            flags = SET(flags, atoi(v), F_PENDULUM);
        if (strcmp(k, "drawback")  == 0)
            flags = SET(flags, atoi(v), F_DRAWBACK);
        if (strcmp(k, "drawclip")  == 0)
            flags = SET(flags, atoi(v), F_DRAWCLIP);
        if (strcmp(k, "depthmask") == 0)
            flags = SET(flags, atoi(v), F_DEPTHMASK);
        if (strcmp(k, "depthtest") == 0)
            flags = SET(flags, atoi(v), F_DEPTHTEST);
    }

    return flags;
}

#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT) && defined(CONFIG_INCLUDES_MULTIBALLS)
void ball_multi_init(void)
{
    for (int i = 0; i < 5; i++)
    {
        char *model_name;

        switch (i) {
            case 0:  model_name = strdup(account_get_s(ACCOUNT_BALL_FILE_LL)); break;
            case 1:  model_name = strdup(account_get_s(ACCOUNT_BALL_FILE_L));  break;
            case 2:  model_name = strdup(account_get_s(ACCOUNT_BALL_FILE_C));  break;
            case 3:  model_name = strdup(account_get_s(ACCOUNT_BALL_FILE_R));  break;
            case 4:  model_name = strdup(account_get_s(ACCOUNT_BALL_FILE_RR)); break;
            default: model_name = strdup(account_get_s(ACCOUNT_BALL_FILE_C));
        }

        /* Can't load with empty model path */

        if (strcmp(model_name, "ball/"))
            continue;

        char *solid_file = concat_string(model_name,
                                         "-solid.sol", NULL);
        char *inner_file = concat_string(model_name,
                                         "-inner.sol", NULL);
        char *outer_file = concat_string(model_name,
                                         "-outer.sol", NULL);

        ball_full_v[i].multi_solid_flags = 0;
        ball_full_v[i].multi_inner_flags = 0;
        ball_full_v[i].multi_outer_flags = 0;

        if ((ball_full_v[i].multi_has_solid = sol_load_full(&ball_full_v[i].multi_solid, solid_file, 0)))
            ball_full_v[i].multi_solid_flags = ball_opts(&ball_full_v[i].multi_solid.base);

        if ((ball_full_v[i].multi_has_inner = sol_load_full(&ball_full_v[i].multi_inner, inner_file, 0)))
            ball_full_v[i].multi_inner_flags = ball_opts(&ball_full_v[i].multi_inner.base);

        if ((ball_full_v[i].multi_has_outer = sol_load_full(&ball_full_v[i].multi_outer, outer_file, 0)))
            ball_full_v[i].multi_outer_flags = ball_opts(&ball_full_v[i].multi_outer.base);

        free(solid_file); solid_file = NULL;
        free(inner_file); inner_file = NULL;
        free(outer_file); outer_file = NULL;
    }
}

void ball_multi_free(void)
{
    for (int i = 0; i < 5; i++)
    {
        if (ball_full_v[i].multi_has_outer)
            sol_free_full(&ball_full_v[i].multi_outer);
        if (ball_full_v[i].multi_has_inner)
            sol_free_full(&ball_full_v[i].multi_inner);
        if (ball_full_v[i].multi_has_solid)
            sol_free_full(&ball_full_v[i].multi_solid);

        ball_full_v[i].multi_has_solid = ball_full_v[i].multi_has_inner
                                       = ball_full_v[i].multi_has_outer
                                       = 0;

        ball_full_v[i].allow_edit_model = 0;
    }
}

void ball_multi_step(float dt)
{
    for (int i = 0; i < 5; i++)
    {
        if (ball_full_v[i].multi_has_solid)
            sol_move(&ball_full_v[i].multi_solid.vary, NULL, dt);
        if (ball_full_v[i].multi_has_inner)
            sol_move(&ball_full_v[i].multi_inner.vary, NULL, dt);
        if (ball_full_v[i].multi_has_outer)
            sol_move(&ball_full_v[i].multi_outer.vary, NULL, dt);
    }
}

void ball_multi_equip(int id)
{
    ball_idx = id;
}

int ball_multi_curr(void)
{
    return ball_idx;
}
#endif

#ifndef CONFIG_INCLUDES_MULTIBALLS
void ball_init(void)
{
#if NB_HAVE_PB_BOTH==1
    const char *model_name = account_get_s(ACCOUNT_BALL_FILE);
#else
    const char *model_name = config_get_s(CONFIG_BALL_FILE);
#endif

    char *solid_file = concat_string(model_name,
                                     "-solid.sol", NULL);
    char *inner_file = concat_string(model_name,
                                     "-inner.sol", NULL);
    char *outer_file = concat_string(model_name,
                                     "-outer.sol", NULL);

    solid_flags = 0;
    inner_flags = 0;
    outer_flags = 0;

    if ((has_solid = sol_load_full(&solid, solid_file, 0)))
        solid_flags = ball_opts(&solid.base);

    if ((has_inner = sol_load_full(&inner, inner_file, 0)))
        inner_flags = ball_opts(&inner.base);

    if ((has_outer = sol_load_full(&outer, outer_file, 0)))
        outer_flags = ball_opts(&outer.base);

    free(solid_file); solid_file = NULL;
    free(inner_file); inner_file = NULL;
    free(outer_file); outer_file = NULL;
}

void ball_free(void)
{
    if (has_outer) sol_free_full(&outer);
    if (has_inner) sol_free_full(&inner);
    if (has_solid) sol_free_full(&solid);

    has_solid = has_inner = has_outer = 0;
}

void ball_step(float dt)
{
    if (has_solid) sol_move(&solid.vary, NULL, dt);
    if (has_inner) sol_move(&inner.vary, NULL, dt);
    if (has_outer) sol_move(&outer.vary, NULL, dt);
}
#endif

/*---------------------------------------------------------------------------*/

static void ball_draw_solid(struct s_rend *rend,
                            const float *ball_M,
                            const float *ball_bill_M, float t)
{
    if (has_solid)
    {
        const int mask = (solid_flags & F_DEPTHMASK);
        const int test = (solid_flags & F_DEPTHTEST);

        glPushMatrix();
        {
            /* Apply the ball rotation. */

            glMultMatrixf(ball_M);

            /* Draw the solid billboard geometry. */

            if (solid.base.rc)
            {
                if (test == 0) glDisable(GL_DEPTH_TEST);
                if (mask == 0) glDepthMask(GL_FALSE);
                {
                    sol_bill(&solid.draw, rend, ball_bill_M, t);
                }
                if (mask == 0) glDepthMask(GL_TRUE);
                if (test == 0) glEnable(GL_DEPTH_TEST);
            }

            /* Draw the solid opaque and transparent geometry. */

            sol_draw(&solid.draw, rend, mask, test);
        }
        glPopMatrix();
    }
}

static void ball_draw_inner(struct s_rend *rend,
                            const float *pend_M,
                            const float *bill_M,
                            const float *pend_bill_M, float t)
{
    if (has_inner)
    {
        const int pend = (inner_flags & F_PENDULUM);
        const int mask = (inner_flags & F_DEPTHMASK);
        const int test = (inner_flags & F_DEPTHTEST);

        /* Apply the pendulum rotation. */

        if (pend)
        {
            glPushMatrix();
            glMultMatrixf(pend_M);
        }

        /* Draw the inner opaque and transparent geometry. */

        sol_draw(&inner.draw, rend, mask, test);

        /* Draw the inner billboard geometry. */

        if (inner.base.rc)
        {
            if (test == 0) glDisable(GL_DEPTH_TEST);
            if (mask == 0) glDepthMask(GL_FALSE);
            {
                if (pend)
                    sol_bill(&inner.draw, rend, pend_bill_M, t);
                else
                    sol_bill(&inner.draw, rend, bill_M,      t);
            }
            if (mask == 0) glDepthMask(GL_TRUE);
            if (test == 0) glEnable(GL_DEPTH_TEST);
        }

        if (pend)
            glPopMatrix();
    }
}

static void ball_draw_outer(struct s_rend *rend,
                            const float *pend_M,
                            const float *bill_M,
                            const float *pend_bill_M, float t)
{
    if (has_outer)
    {
        const int pend = (outer_flags & F_PENDULUM);
        const int mask = (outer_flags & F_DEPTHMASK);
        const int test = (outer_flags & F_DEPTHTEST);

        /* Apply the pendulum rotation. */

        if (pend)
        {
            glPushMatrix();
            glMultMatrixf(pend_M);
        }

        /* Draw the outer opaque and transparent geometry. */

        sol_draw(&outer.draw, rend, mask, test);

        /* Draw the outer billboard geometry. */

        if (outer.base.rc)
        {
            if (test == 0) glDisable(GL_DEPTH_TEST);
            if (mask == 0) glDepthMask(GL_FALSE);
            {
                if (pend)
                    sol_bill(&outer.draw, rend, pend_bill_M, t);
                else
                    sol_bill(&outer.draw, rend, bill_M,      t);
            }
            if (mask == 0) glDepthMask(GL_TRUE);
            if (test == 0) glEnable(GL_DEPTH_TEST);
        }

        if (pend)
            glPopMatrix();
    }
}

/*---------------------------------------------------------------------------*/

static void ball_pass_inner(struct s_rend *rend,
                            const float *ball_M,
                            const float *pend_M,
                            const float *bill_M,
                            const float *ball_bill_M,
                            const float *pend_bill_M, float t)
{
    /* Sort the inner ball using clip planes. */

    if      (inner_flags & F_DRAWCLIP)
    {
        glEnable(GL_CLIP_PLANE1);
        ball_draw_inner(rend, pend_M, bill_M, pend_bill_M, t);
        glDisable(GL_CLIP_PLANE1);

        glEnable(GL_CLIP_PLANE2);
        ball_draw_inner(rend, pend_M, bill_M, pend_bill_M, t);
        glDisable(GL_CLIP_PLANE2);
    }

    /* Sort the inner ball using face culling. */

    else if (inner_flags & F_DRAWBACK)
    {
        glCullFace(GL_FRONT);
        ball_draw_inner(rend, pend_M, bill_M, pend_bill_M, t);
        glCullFace(GL_BACK);
        ball_draw_inner(rend, pend_M, bill_M, pend_bill_M, t);
    }

    /* Draw the inner ball normally. */

    else
    {
        ball_draw_inner(rend, pend_M, bill_M, pend_bill_M, t);
    }
}

static void ball_pass_solid(struct s_rend *rend,
                            const float *ball_M,
                            const float *pend_M,
                            const float *bill_M,
                            const float *ball_bill_M,
                            const float *pend_bill_M, float t)
{
    /* Sort the solid ball with the inner ball using clip planes. */

    if      (solid_flags & F_DRAWCLIP)
    {
        glEnable(GL_CLIP_PLANE1);
        ball_draw_solid(rend, ball_M,                 ball_bill_M, t);
        glDisable(GL_CLIP_PLANE1);

        ball_pass_inner(rend, ball_M, pend_M, bill_M, ball_bill_M, pend_bill_M, t);

        glEnable(GL_CLIP_PLANE2);
        ball_draw_solid(rend, ball_M,                 ball_bill_M, t);
        glDisable(GL_CLIP_PLANE2);
    }

    /* Sort the solid ball with the inner ball using face culling. */

    else if (solid_flags & F_DRAWBACK)
    {
        glCullFace(GL_FRONT);
        ball_draw_solid(rend, ball_M,                 ball_bill_M, t);
        glCullFace(GL_BACK);

        ball_pass_inner(rend, ball_M, pend_M, bill_M, ball_bill_M, pend_bill_M, t);
        ball_draw_solid(rend, ball_M,                 ball_bill_M, t);
    }

    /* Draw the solid ball after the inner ball. */

    else
    {
        ball_pass_inner(rend, ball_M, pend_M, bill_M, ball_bill_M, pend_bill_M, t);
        ball_draw_solid(rend, ball_M,                 ball_bill_M, t);
    }
}

static void ball_pass_outer(struct s_rend *rend,
                            const float *ball_M,
                            const float *pend_M,
                            const float *bill_M,
                            const float *ball_bill_M,
                            const float *pend_bill_M, float t)
{
    /* Sort the outer ball with the solid ball using clip planes. */

    if      (outer_flags & F_DRAWCLIP)
    {
        glEnable(GL_CLIP_PLANE1);
        ball_draw_outer(rend,         pend_M, bill_M,              pend_bill_M, t);
        glDisable(GL_CLIP_PLANE1);

        ball_pass_solid(rend, ball_M, pend_M, bill_M, ball_bill_M, pend_bill_M, t);

        glEnable(GL_CLIP_PLANE2);
        ball_draw_outer(rend,         pend_M, bill_M,              pend_bill_M, t);
        glDisable(GL_CLIP_PLANE2);
    }

    /* Sort the outer ball with the solid ball using face culling. */

    else if (outer_flags & F_DRAWBACK)
    {
        glCullFace(GL_FRONT);
        ball_draw_outer(rend,         pend_M, bill_M,              pend_bill_M, t);
        glCullFace(GL_BACK);

        ball_pass_solid(rend, ball_M, pend_M, bill_M, ball_bill_M, pend_bill_M, t);
        ball_draw_outer(rend,         pend_M, bill_M,              pend_bill_M, t);
    }

    /* Draw the outer ball after the solid ball. */

    else
    {
        ball_pass_solid(rend, ball_M, pend_M, bill_M, ball_bill_M, pend_bill_M, t);
        ball_draw_outer(rend,         pend_M, bill_M,              pend_bill_M, t);
    }
}

/*---------------------------------------------------------------------------*/

void ball_draw(struct s_rend *rend,
               const float *ball_M,
               const float *pend_M,
               const float *bill_M, float t)
{
    /* Compute transforms for ball and pendulum billboards. */

    float ball_T[16], ball_bill_M[16];
    float pend_T[16], pend_bill_M[16];

    m_xps(ball_T, ball_M);
    m_xps(pend_T, pend_M);

    m_mult(ball_bill_M, ball_T, bill_M);
    m_mult(pend_bill_M, pend_T, bill_M);

    /* Go to GREAT pains to ensure all layers are drawn back-to-front. */

    ball_pass_outer(rend, ball_M, pend_M, bill_M, ball_bill_M, pend_bill_M, t);
}

#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_MULTIBALLS)
/* Those darn to render multiballs in the profile selection screen */
#define multi_draw_range(offset, ball_m, pend_m, bill_m) \
    do { \
        tmp_ball_idx = ball_idx; \
        ball_idx = ball_idx + offset; \
        if (ball_idx >= 0 && ball_idx < 5) \
        { \
             float ball_T[16], ball_bill_M[16]; \
             float pend_T[16], pend_bill_M[16]; \
             m_xps(ball_T, ball_m); m_xps(pend_T, pend_m); \
             m_mult(ball_bill_M, ball_T, bill_m); m_mult(pend_bill_M, pend_T, bill_m); \
             ball_pass_outer(rend, ball_m, pend_m, bill_m, ball_bill_M, pend_bill_M, t); \
        } \
        ball_idx = tmp_ball_idx; \
        CLAMP(0, ball_idx, 5); \
    } while (0)

void ball_multi_draw(struct s_rend *rend,
                     const float *ball_M,
                     const float *pend_M,
                     const float *bill_M, float t)
{
    multi_draw_range(-2, ball_M, pend_M, bill_M);
    multi_draw_range(-1, ball_M, pend_M, bill_M);
    multi_draw_range( 0, ball_M, pend_M, bill_M);
    multi_draw_range( 1, ball_M, pend_M, bill_M);
    multi_draw_range( 2, ball_M, pend_M, bill_M);
}

void ball_multi_draw_single(int offset, struct s_rend *rend,
                            const float *ball_M,
                            const float *pend_M,
                            const float *bill_M, float t)
{
    multi_draw_range(offset, ball_M, pend_M, bill_M);
}
#endif

/*---------------------------------------------------------------------------*/
