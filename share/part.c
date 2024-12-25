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
#include <math.h>

#include "config.h"
#include "glext.h"
#include "part.h"
#include "vec3.h"
#include "image.h"
#include "geom.h"
#include "hmd.h"
#include "video.h"

/*---------------------------------------------------------------------------*/

struct part
{
    /**
     * Particle mode
     *
     * @param m -
     * 0 = None;
     * 1 = Classic burst;
     * 2 = Linear in + out; 3 = Linear in; 4 = Linear out
     */
    int m;

    float f[2];                    /* Particle fade                          */

    float g_i[3];                  /* Initial gravity                        */
    float v_i;                     /* Initial velocity                       */
    float v_ri;                    /* Initial random velocity                */
    float w_i;                     /* Initial angular velocity (degrees)     */
    float w_ri;                    /* Initial random angular velocity (degrees)*/
    float dv_i;                    /* Initial drag                           */
    float dv_ri;                   /* Initial random drag                    */

    float c_i[3];                  /* Initial color                          */
    float c_ri[3];                 /* Initial random color                   */
    float p_i[3];                  /* Initial position                       */
    float p_ri[3];                 /* Initial random position                */
    float s_i;                     /* Initial scale                          */
    float s_ri;                    /* Initial random scale                   */

    float t_i;                     /* Initial lifetime                       */
    float t_ri;                    /* Initial random lifetime                */

    float v[3];                    /* Velocity                               */
    float w;                       /* Angular velocity (degrees)             */
    float dv;                      /* Drag                                   */

    float p[3];                    /* Position                               */
    float s;                       /* Scale                                  */
    float c[3];                    /* Color                                  */

    float t;                       /* Time until death. Doubles as opacity.  */
    float t_r;                     /* Random lifetime. Doubles as opacity.   */

    int  loaded;                   /* Check whether particle file is loaded  */
    char file[64];                 /* Particle filename                      */
};

static GLuint part_vbo;
static GLuint part_ebo;

static struct part coin_part[PART_MAX_COIN];
static struct part goal_part[PART_MAX_GOAL];

static int coin_part_grav = 1;
static int goal_part_grav = 0;

/*---------------------------------------------------------------------------*/

static struct b_mtrl part_base_mtrl =
{
    { 0.8f, 0.8f, 0.8f, 1.0f },
    { 0.2f, 0.2f, 0.2f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f }, 0.0f, M_TRANSPARENT, IMG_PART_STAR
};

static int coin_mtrl;
static int goal_mtrl;

/*---------------------------------------------------------------------------*/

#define PI 3.1415927f

static float rnd(float l, float h)
{
    return l + (h - l) * rand() / (float) RAND_MAX;
}

/*---------------------------------------------------------------------------*/

#define CURR 0
#define PREV 1

struct part_lerp
{
    float p[2][3];
};

static struct part_lerp part_lerp_coin[PART_MAX_COIN];
static struct part_lerp part_lerp_goal[PART_MAX_GOAL];

static void part_lerp_copy(void)
{
    int i;

    for (i = 0; i < PART_MAX_COIN; i++)
        v_cpy(part_lerp_coin[i].p[PREV],
              part_lerp_coin[i].p[CURR]);

    for (i = 0; i < PART_MAX_GOAL; i++)
        v_cpy(part_lerp_goal[i].p[PREV],
              part_lerp_goal[i].p[CURR]);
}

void part_lerp_apply(float a)
{
    int i;

    for (i = 0; i < PART_MAX_COIN; i++)
        if (coin_part[i].t > 0.0f)
            v_lerp(coin_part[i].p,
                   part_lerp_coin[i].p[PREV],
                   part_lerp_coin[i].p[CURR], a);

    for (i = 0; i < PART_MAX_GOAL; i++)
        if (goal_part[i].t > 0.0f)
            v_lerp(goal_part[i].p,
                   part_lerp_goal[i].p[PREV],
                   part_lerp_goal[i].p[CURR], a);
}

/*---------------------------------------------------------------------------*/

void part_reset(void)
{
    int i;

    for (i = 0; i < PART_MAX_COIN; i++)
        coin_part[i].t = 0;

    for (i = 0; i < PART_MAX_GOAL; i++)
        goal_part[i].t = 0;
}

void part_init(void)
{
    static const GLfloat verts[4][5] = {
        { -0.5f, -0.5f, 0.0f, 0.0f, 0.0f },
        { +0.5f, -0.5f, 0.0f, 1.0f, 0.0f },
        { -0.5f, +0.5f, 0.0f, 0.0f, 1.0f },
        { +0.5f, +0.5f, 0.0f, 1.0f, 1.0f },
    };

    static const GLushort elems[4] = {
        0u, 1u, 2u, 3u
    };

    coin_mtrl = mtrl_cache(&part_base_mtrl);
    goal_mtrl = mtrl_cache(&part_base_mtrl);

    memset(coin_part, 0, sizeof(coin_part));
    memset(goal_part, 0, sizeof(goal_part));

    glGenBuffers_(1,              &part_vbo);
    glBindBuffer_(GL_ARRAY_BUFFER, part_vbo);
    glBufferData_(GL_ARRAY_BUFFER, sizeof (verts), verts, GL_STATIC_DRAW);
    glBindBuffer_(GL_ARRAY_BUFFER, 0);

    glGenBuffers_(1,                      &part_ebo);
    glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER, part_ebo);
    glBufferData_(GL_ELEMENT_ARRAY_BUFFER, sizeof (elems), elems, GL_STATIC_DRAW);
    glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER, 0);

    part_load();
}

int part_load(void)
{
    part_reset();

    int i, j, k;

    for (i = 0; i < PART_MAX_COIN; i++)
        for (j = 0; j < 2; j++)
            for (k = 0; k < 3; k++)
            {
                coin_part[i].t           = 0;
                coin_part[i].m           = 0;

                coin_part[i].f    [j]    = j == 0 ? 0.01f : 0.5f;

                coin_part[i].g_i     [k] = 0.0f;
                coin_part[i].v_i         = 4.0f;
                coin_part[i].v_ri        = 0.0f;
                coin_part[i].w_i         = 0.0f;
                coin_part[i].w_ri        = 4.0f;
                coin_part[i].dv_i        = 0.0f;
                coin_part[i].dv_ri       = 0.0f;

                coin_part[i].c_i     [k] = 1.0f;
                coin_part[i].c_ri    [k] = 0.0f;
                coin_part[i].p_i     [k] = 0.0f;
                coin_part[i].p_ri    [k] = 0.0f;
                coin_part[i].s_i         = 1.0f;
                coin_part[i].s_ri        = 0.0f;

                coin_part[i].t_i         = 1.0f;
                coin_part[i].t_ri        = 0.0f;
            }

    for (i = 0; i < PART_MAX_GOAL; i++)
        for (j = 0; j < 2; j++)
            for (k = 0; k < 3; k++)
            {
                goal_part[i].t           = 0;
                goal_part[i].m           = 0;

                goal_part[i].f    [j]    = j == 0 ? 0.01f : 0.5f;

                goal_part[i].g_i     [k] = 0.0f;
                goal_part[i].v_i         = 10.0f;
                goal_part[i].v_ri        = 7.0f;
                goal_part[i].w_i         = 0.0f;
                goal_part[i].w_ri        = 4.0f;
                goal_part[i].dv_i        = 0.0f;
                goal_part[i].dv_ri       = 0.0f;

                goal_part[i].c_i     [k] = 1.0f;
                goal_part[i].c_ri    [k] = 0.0f;
                goal_part[i].p_i     [k] = 0.0f;
                goal_part[i].p_ri    [k] = 0.0f;
                goal_part[i].s_i         = 2.0f;
                goal_part[i].s_ri        = 1.0f;

                goal_part[i].t_i         = 1.5f;
                goal_part[i].t_ri        = 0.5f;
            }

    /* TODO: Load particle file. */

    return 1;
}

void part_free(void)
{
    part_reset();

    glDeleteBuffers_(1, &part_vbo);
    glDeleteBuffers_(1, &part_ebo);

    mtrl_free(coin_mtrl); coin_mtrl = 0;
    mtrl_free(goal_mtrl); goal_mtrl = 0;
}

/*---------------------------------------------------------------------------*/

void part_burst(const float *p, const float *c)
{
    int i, n = 0;

    for (i = 0; n < 10 && i < PART_MAX_COIN; i++)
        if (coin_part[i].t <= 0.0f)
        {
            float a = rnd(-1.0f * PI, +1.0f * PI);
            float b = rnd(+0.3f * PI, +0.5f * PI);
            float w = coin_part[i].w_i + rnd(-coin_part[i].w_ri * PI, +coin_part[i].w_ri * PI);

            v_cpy(coin_part[i].c, coin_part[i].c_i);
            coin_part[i].c[0] *= CLAMP(0.0f, c[0], 1.0f);
            coin_part[i].c[1] *= CLAMP(0.0f, c[1], 1.0f);
            coin_part[i].c[2] *= CLAMP(0.0f, c[2], 1.0f);
            coin_part[i].c[0]  = CLAMP(0.0f, coin_part[i].c[0] + rnd(-coin_part[i].c_ri[0], coin_part[i].c_ri[0]), 1.0f);
            coin_part[i].c[1]  = CLAMP(0.0f, coin_part[i].c[1] + rnd(-coin_part[i].c_ri[1], coin_part[i].c_ri[1]), 1.0f);
            coin_part[i].c[2]  = CLAMP(0.0f, coin_part[i].c[2] + rnd(-coin_part[i].c_ri[2], coin_part[i].c_ri[2]), 1.0f);

            v_cpy(coin_part[i].p, p);
            v_add(coin_part[i].p, coin_part[i].p, coin_part[i].p_i);
            coin_part[i].p[0] += rnd(-coin_part[i].p_ri[0], coin_part[i].p_ri[0]);
            coin_part[i].p[1] += rnd(-coin_part[i].p_ri[1], coin_part[i].p_ri[1]);
            coin_part[i].p[2] += rnd(-coin_part[i].p_ri[2], coin_part[i].p_ri[2]);

            coin_part[i].s = coin_part[i].s_i + rnd(-coin_part[i].s_ri, coin_part[i].s_ri);

            coin_part[i].v[0] = (coin_part[i].v_i + rnd(-coin_part[i].v_ri, coin_part[i].v_ri)) * fcosf(a) * fcosf(b);
            coin_part[i].v[1] = (coin_part[i].v_i + rnd(-coin_part[i].v_ri, coin_part[i].v_ri)) *            fsinf(b);
            coin_part[i].v[2] = (coin_part[i].v_i + rnd(-coin_part[i].v_ri, coin_part[i].v_ri)) * fsinf(a) * fcosf(b);

            coin_part[i].w = V_DEG(w);

            coin_part[i].dv = coin_part[i].dv_i + rnd(-coin_part[i].dv_ri, coin_part[i].dv_ri);

            coin_part[i].t_r = rnd(-coin_part[i].t_ri, coin_part[i].t_ri);
            coin_part[i].t   = coin_part[i].t_i + coin_part[i].t_r;

            v_cpy(part_lerp_coin[i].p[PREV], coin_part[i].p);
            v_cpy(part_lerp_coin[i].p[CURR], coin_part[i].p);

            n++;
        }
}

void part_goal(const float *p)
{
    int i, n = 0;

    for (i = 0; n < 500 && i < PART_MAX_GOAL; i++)
        if (goal_part[i].t <= 0.0f)
        {
            float a = rnd(-1.0f * PI, +1.0f * PI);
            float b = rnd(-0.5f * PI, +0.5f * PI);
            float w = goal_part[i].w_i + rnd(-goal_part[i].w_ri * PI, +goal_part[i].w_ri * PI);

            v_cpy(goal_part[i].c, goal_part[i].c_i);
            goal_part[i].c[0] *= 1.0f;
            goal_part[i].c[1] *= 0.25f;
            goal_part[i].c[2] *= 1.0f;
            goal_part[i].c[0]  = CLAMP(0.0f, goal_part[i].c[0] + rnd(-goal_part[i].c_ri[0], goal_part[i].c_ri[0]), 1.0f);
            goal_part[i].c[1]  = CLAMP(0.0f, goal_part[i].c[1] + rnd(-goal_part[i].c_ri[1], goal_part[i].c_ri[1]), 1.0f);
            goal_part[i].c[2]  = CLAMP(0.0f, goal_part[i].c[2] + rnd(-goal_part[i].c_ri[2], goal_part[i].c_ri[2]), 1.0f);

            v_cpy(goal_part[i].p, p);
            v_add(goal_part[i].p, goal_part[i].p, goal_part[i].p_i);
            goal_part[i].p[0] += rnd(-goal_part[i].p_ri[0], goal_part[i].p_ri[0]);
            goal_part[i].p[1] += rnd(-goal_part[i].p_ri[1], goal_part[i].p_ri[1]);
            goal_part[i].p[2] += rnd(-goal_part[i].p_ri[2], goal_part[i].p_ri[2]);

            goal_part[i].s = goal_part[i].s_i + rnd(-goal_part[i].s_ri, goal_part[i].s_ri);

            goal_part[i].v[0] = (goal_part[i].v_i + rnd(-goal_part[i].v_ri, goal_part[i].v_ri)) * fcosf(a) * fcosf(b);
            goal_part[i].v[1] = (goal_part[i].v_i + rnd(-goal_part[i].v_ri, goal_part[i].v_ri)) *            fsinf(b);
            goal_part[i].v[2] = (goal_part[i].v_i + rnd(-goal_part[i].v_ri, goal_part[i].v_ri)) * fsinf(a) * fcosf(b);

            goal_part[i].w = V_DEG(w);

            goal_part[i].dv = goal_part[i].dv_i + rnd(-goal_part[i].dv_ri, goal_part[i].dv_ri);

            goal_part[i].t_r = rnd(-goal_part[i].t_ri, goal_part[i].t_ri);
            goal_part[i].t   = goal_part[i].t_i + goal_part[i].t_r;

            v_cpy(part_lerp_goal[i].p[PREV], goal_part[i].p);
            v_cpy(part_lerp_goal[i].p[CURR], goal_part[i].p);

            n++;
        }
}

/*---------------------------------------------------------------------------*/

static void part_fall(const float *g, float dt)
{
    int   i;
    float g_total[3] = { 0.0f, 0.0f, 0.0f };

    for (i = 0; i < PART_MAX_COIN; i++)
        if (coin_part[i].t > 0.f)
        {
            coin_part[i].t -= dt;

            if (coin_part_grav)
            {
                v_add(g_total, g, coin_part[i].g_i);
                v_mad(coin_part[i].v, coin_part[i].v, g_total, dt);
            }
            else v_mad(coin_part[i].v, coin_part[i].v, coin_part[i].g_i, dt);

            if (coin_part[i].dv > 0.01f)
                v_scl(coin_part[i].v, coin_part[i].v, CLAMP(0.0f, 1.0f / MAX(1.0f, coin_part[i].dv + 1.0f), 1.0f));

            v_mad(part_lerp_coin[i].p[CURR], part_lerp_coin[i].p[CURR], coin_part[i].v, dt);
        }
        else coin_part[i].t = 0.0f;


    for (i = 0; i < PART_MAX_GOAL; i++)
        if (goal_part[i].t > 0.f)
        {
            goal_part[i].t -= dt;

            if (goal_part_grav)
            {
                v_add(g_total, g, goal_part[i].g_i);
                v_mad(goal_part[i].v, goal_part[i].v, g_total, dt);
            }
            else v_mad(goal_part[i].v, goal_part[i].v, goal_part[i].g_i, dt);

            if (goal_part[i].dv > 0.01f)
                v_scl(goal_part[i].v, goal_part[i].v, CLAMP(0.0f, 1.0f / MAX(1.0f, goal_part[i].dv + 1.0f), 1.0f));

            v_mad(part_lerp_goal[i].p[CURR], part_lerp_goal[i].p[CURR], goal_part[i].v, dt);
        }
        else goal_part[i].t = 0.0f;
}

void part_step(const float *g, float dt)
{
    part_lerp_copy();
    part_fall(g, dt);
}

/*---------------------------------------------------------------------------*/

void part_draw_coin(const struct s_draw *draw, struct s_rend *rend, const float *M, float t)
{
    int i;

    r_apply_mtrl(rend, coin_mtrl);

    glBindBuffer_(GL_ARRAY_BUFFER,         part_vbo);
    glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER, part_ebo);

    glDisableClientState(GL_NORMAL_ARRAY);
    {
        glVertexPointer  (3, GL_FLOAT, sizeof (GLfloat) * 5, (GLvoid *) (                   0u));
        glTexCoordPointer(2, GL_FLOAT, sizeof (GLfloat) * 5, (GLvoid *) (sizeof (GLfloat) * 3u));

        for (i = 0; i < PART_MAX_COIN; ++i)
            if (coin_part[i].t > 0.0f)
            {
                const float part_time_length =  coin_part[i].t_i + coin_part[i].t_r;
                const float part_time_curr   = (coin_part[i].t_i + coin_part[i].t_r) - coin_part[i].t;

                float part_fade_alpha     = 2.0f;
                float part_fade_alpha_out = 2.0f;

                if (coin_part[i].f[1] > 0.01f)
                    part_fade_alpha_out = flerp(coin_part[i].f[1], 0, part_time_curr);

                if (coin_part[i].f[0] > 0.01f)
                    part_fade_alpha = flerp(0, 1 / coin_part[i].f[0], part_time_curr);

                part_fade_alpha = MIN(part_fade_alpha_out, part_fade_alpha);

                glColor4f(coin_part[i].c[0],
                          coin_part[i].c[1],
                          coin_part[i].c[2],
#if ENABLE_MOTIONBLUR!=0
                          config_get_d(CONFIG_MOTIONBLUR) ? (CLAMP(0, part_fade_alpha, 1) * video_motionblur_alpha_get()) : CLAMP(0, part_fade_alpha, 1));
#else
                          CLAMP(0, part_fade_alpha, 1));
#endif

                glPushMatrix();
                {
                    glTranslatef(coin_part[i].p[0], coin_part[i].p[1], coin_part[i].p[2]);

                    if (M) glMultMatrixf(M);

                    glScalef(PART_SIZE * (coin_part[i].s * 2.0f), PART_SIZE * (coin_part[i].s * 2.0f), 1.0f);
                    glRotatef((part_time_length - part_time_curr) * coin_part[i].w, 0.0f, 0.0f, 1.0f);

                    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, (GLvoid *) 0u);
                }
                glPopMatrix();
            }
    }
    glEnableClientState(GL_NORMAL_ARRAY);

    glBindBuffer_(GL_ARRAY_BUFFER,         0);
    glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER, 0);

    glColor4f(1.0f, 1.0f, 1.0f,
#if ENABLE_MOTIONBLUR!=0
              config_get_d(CONFIG_MOTIONBLUR) ? (1.0f * video_motionblur_alpha_get()) : 1.0f);
#else
              1.0f);
#endif
}

void part_draw_goal(const struct s_draw *draw, struct s_rend *rend, const float *M, float t)
{
    int i;

    r_apply_mtrl(rend, goal_mtrl);

    glBindBuffer_(GL_ARRAY_BUFFER,         part_vbo);
    glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER, part_ebo);

    glDisableClientState(GL_NORMAL_ARRAY);
    {
        glVertexPointer  (3, GL_FLOAT, sizeof (GLfloat) * 5, (GLvoid *) (                   0u));
        glTexCoordPointer(2, GL_FLOAT, sizeof (GLfloat) * 5, (GLvoid *) (sizeof (GLfloat) * 3u));

        for (i = 0; i < PART_MAX_GOAL; ++i)
            if (goal_part[i].t > 0.0f)
            {
                const float part_time_length =  goal_part[i].t_i + goal_part[i].t_r;
                const float part_time_curr   = (goal_part[i].t_i + goal_part[i].t_r) - goal_part[i].t;

                float part_fade_alpha     = 2.0f;
                float part_fade_alpha_out = 2.0f;

                if (goal_part[i].f[1] > 0.01f)
                    part_fade_alpha_out = flerp(0, goal_part[i].f[1], goal_part[i].t);

                if (goal_part[i].f[0] > 0.01f)
                    part_fade_alpha = flerp(0, 1 / goal_part[i].f[0], part_time_curr);

                part_fade_alpha = MIN(part_fade_alpha_out, part_fade_alpha);

                glColor4f(goal_part[i].c[0],
                          goal_part[i].c[1],
                          goal_part[i].c[2],
#if ENABLE_MOTIONBLUR!=0
                          config_get_d(CONFIG_MOTIONBLUR) ? (CLAMP(0, part_fade_alpha, 1) * video_motionblur_alpha_get()) : CLAMP(0, part_fade_alpha, 1));
#else
                          CLAMP(0, part_fade_alpha, 1));
#endif

                glPushMatrix();
                {
                    glTranslatef(goal_part[i].p[0], goal_part[i].p[1], goal_part[i].p[2]);

                    if (M) glMultMatrixf(M);

                    glScalef(PART_SIZE * (goal_part[i].s * 2.0f), PART_SIZE * (goal_part[i].s * 2.0f), 1.0f);
                    glRotatef((part_time_length - part_time_curr) * goal_part[i].w, 0.0f, 0.0f, 1.0f);

                    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, (GLvoid *) 0u);
                }
                glPopMatrix();
            }
    }
    glEnableClientState(GL_NORMAL_ARRAY);

    glBindBuffer_(GL_ARRAY_BUFFER,         0);
    glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER, 0);

    glColor4f(1.0f, 1.0f, 1.0f,
#if ENABLE_MOTIONBLUR!=0
              config_get_d(CONFIG_MOTIONBLUR) ? (1.0f * video_motionblur_alpha_get()) : 1.0f);
#else
              1.0f);
#endif
}

/*---------------------------------------------------------------------------*/
