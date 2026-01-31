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

#ifndef NDEBUG
#include <assert.h>
#elif defined(_MSC_VER) && defined(_AFXDLL)
#include <afx.h>
/**
 * HACK: assert() for Microsoft Windows Apps in Release builds
 * will be replaced to VERIFY() - Ersohn Styne
 */
#define assert VERIFY
#else
#define assert(_x) (_x)
#endif

#if NB_HAVE_PB_BOTH==1
#include "solid_chkp.h"
#endif
#ifdef MAPC_INCLUDES_CHKP
#include "checkpoints.h"
#endif
#if ENABLE_MOON_TASKLOADER!=0
#include "moon_taskloader.h"
#endif

#include "game_common.h"
#include "vec3.h"
#include "config.h"
#include "solid_vary.h"
#include "hmd.h"
#include "common.h"

#if _DEBUG && _MSC_VER
#ifndef _CRTDBG_MAP_ALLOC
#pragma message(__FILE__": Missing _CRT_MAP_ALLOC, recreate: _CRTDBG_MAP_ALLOC + crtdbg.h")
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

#if NB_HAVE_PB_BOTH==1 && !defined(MAPC_INCLUDES_CHKP)
#error Security compilation error: Please enable checkpoints after joined PB+NB Discord Server!
#endif

/*---------------------------------------------------------------------------*/

#if ENABLE_MOON_TASKLOADER!=0
struct game_moon_taskloader_info *game_create_mtli()
{
    struct game_moon_taskloader_info *pfi = malloc(sizeof (*pfi));

    return pfi;
}

void game_free_mtli(struct game_moon_taskloader_info *pfi)
{
    if (pfi)
    {
        free(pfi);
        pfi = NULL;
    }
}

int game_mtli_execute(void *data, void *execute_data)
{
    struct game_moon_taskloader_info *mtli = (struct game_moon_taskloader_info *) data;

    if (mtli && mtli->callback.execute)
        return mtli->callback.execute(mtli->callback.data, execute_data);

    return 0;
}

void game_mtli_progress(void *data, void *progress_data)
{
    struct game_moon_taskloader_info *mtli = (struct game_moon_taskloader_info *) data;

    if (mtli && mtli->callback.progress)
        mtli->callback.progress(mtli->callback.data, progress_data);
}

void game_mtli_done(void *data, void *done_data)
{
    struct game_moon_taskloader_info *mtli = (struct game_moon_taskloader_info *) data;

    if (mtli && mtli->callback.done)
    {
        if (mtli->callback.done)
            mtli->callback.done(mtli->callback.data, done_data);

        game_free_mtli(mtli);
    }
}
#endif

/*---------------------------------------------------------------------------*/

const char *status_to_str(int s)
{
    switch (s)
    {
        case GAME_NONE: return _("Aborted");
        case GAME_TIME: return _("Time-out");
        case GAME_GOAL: return _("Success");
        case GAME_FALL: return _("Fall-out");
        default:        return _("Unknown");
    }
}

/*---------------------------------------------------------------------------*/

const char *cam_to_str(int c)
{
    static char str[64];

    int s = -1000;
    if (c == CAM_AUTO) return _("Automatic");

    s = cam_speed(c);

    if (s <    0) return _("Manual Camera");
    if (s ==   0) return _("Static Camera");
    if (s <= 100) return _("Lazy Camera");
    if (s <= 500) return _("Chase Camera");

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(str, 64,
#else
    sprintf(str,
#endif
            _("Camera %d"), c + 1);

    return str;
}

int cam_speed(int c)
{
    static const int *cfgs[] = {
        &CONFIG_CAMERA_1_SPEED,
        &CONFIG_CAMERA_2_SPEED,
        &CONFIG_CAMERA_3_SPEED
    };

    if (c >= 0 && c < ARRAYSIZE(cfgs))
        return config_get_d(*cfgs[c]);

    return 250;
}

/*---------------------------------------------------------------------------*/

const float GRAVITY_NY[]   = { 0.0f, -9.8f, 0.0f };
const float GRAVITY_BUSY[] = { 0.0f,  0.0f, 0.0f };

void game_tilt_init(struct game_tilt *tilt)
{
    tilt->x[0] = 1.0f;
    tilt->x[1] = 0.0f;
    tilt->x[2] = 0.0f;

    tilt->rx = 0.0f;

    tilt->z[0] = 0.0f;
    tilt->z[1] = 0.0f;
    tilt->z[2] = 1.0f;

    tilt->rz = 0.0f;

    tilt->q[0] = 1.0f;
    tilt->q[1] = 0.0f;
    tilt->q[2] = 0.0f;
    tilt->q[3] = 0.0f;
}

/*
 * Compute tilt orientation from tilt angles and the view basis.
 */
void game_tilt_calc(struct game_tilt *tilt, float view_e[3][3])
{
    float qz[4], qx[4];

    if (view_e)
    {
        v_cpy(tilt->x, view_e[0]);
        v_cpy(tilt->z, view_e[2]);
    }

    q_by_axisangle(qx, tilt->x, V_RAD(tilt->rx));
    q_by_axisangle(qz, tilt->z, V_RAD(tilt->rz));

    q_mul(tilt->q, qx, qz);

    q_nrm(tilt->q, tilt->q);
}

/*
 * Compute appropriate tilt axes from the view basis.
 */
void game_tilt_axes(struct game_tilt *tilt, float view_e[3][3])
{
    v_cpy(tilt->x, view_e[0]);
    v_cpy(tilt->z, view_e[2]);
}

void game_tilt_grav(float h[3], const float g[3], const struct game_tilt *tilt)
{
    q_rot(h, tilt->q, g);
}

/*---------------------------------------------------------------------------*/

float view_zoom_diff_curr = 0;
float view_zoom_diff_rate;
float view_zoom_diff_end;

int use_static_cam_view;
float pos_static_cam_view[3];

void game_view_set_static_cam_view(int activated, const float pos[3])
{
    use_static_cam_view = activated;

    if (activated)
        v_cpy(pos_static_cam_view, pos);
}

void game_view_init(struct game_view *view)
{
    /*
     * In VR, ensure the default view is level.
     */

    if (hmd_stat())
    {
        view->dp = 0.25f;
        view->dc = 0.25f;
        view->dz = 2.2f;
    }
    else
    {
        view->dp = 0.75f;
        view->dc = 0.25f;
        view->dz = 2.2f;
    }

    view->a = 0.0f;

    view->c[0] = 0.0f;
    view->c[1] = view->dc;
    view->c[2] = 0.0f;

    view->p[0] = 0.0f;
    view->p[1] = view->dp;
    view->p[2] = view->dz;

    view->e[0][0] = 1.0f; view->e[0][1] = 0.0f; view->e[0][2] = 0.0f;
    view->e[1][0] = 0.0f; view->e[1][1] = 1.0f; view->e[1][2] = 0.0f;
    view->e[2][0] = 0.0f; view->e[2][1] = 0.0f; view->e[2][2] = 1.0f;
}

void game_view_fly(struct game_view *view, const struct s_vary *vary, int ui, float k)
{
#ifdef MAPC_INCLUDES_CHKP
    float start_direction = last_active ? last_view[ui].a : 0.0f;
#else
    float start_direction = 0;
#endif

    const float vsin = fsinf(start_direction * V_PI / 180);
    const float vcos = fcosf(start_direction * V_PI / 180);

    float  x[3] = { vsin, 0.0f, 0.0f };
    float  y[3] = { 0.0f, 1.0f, 0.0f };
    float  z[3] = { 0.0f, 0.0f, vcos };
    float c0[3] = { 0.0f, 0.0f, 0.0f };
    float p0[3] = { 0.0f, 0.0f, 0.0f };
    float c1[3] = { 0.0f, 0.0f, 0.0f };
    float p1[3] = { 0.0f, 0.0f, 0.0f };
    float  v[3] = { 0.0f, 0.0f, 0.0f };

    game_view_init(view);

    /* k = 0.0 view is at the ball. */

    if (vary->uc > 0)
    {
        v_cpy(c0, vary->uv[0].p);
        v_cpy(p0, vary->uv[0].p);

        v_mad(p0, p0, y, view->dp);
        v_mad(c0, c0, y, view->dc);

        v_mad(p0, p0, z, view->dz);
        v_mad(p0, p0, x, view->dz);
    }

    if (use_static_cam_view)
    {
        v_cpy(p1, pos_static_cam_view);
        v_cpy(c1, vary->uv[0].p);

        v_sub(v, p1, p0);
        v_mad(p0, p0, v, 1.0f);

        v_sub(v, c1, c0);
        v_mad(c0, c0, v, 1.0f);

        //c0[1] += view->dc;
    }

#ifdef MAPC_INCLUDES_CHKP
    float chkp_campos[3]        = { 0.0f, 0.0f, 0.0f };
    float chkp_campos_center[3] = { 0.0f, 0.0f, 0.0f };

    if (last_active)
    {
        chkp_campos[0] = last_pos[ui][0] + (4 * fsinf(start_direction * V_PI / 180));
        chkp_campos[1] = last_pos[ui][1] + 2;
        chkp_campos[2] = last_pos[ui][2] + (4 * fcosf(start_direction * V_PI / 180));
        chkp_campos_center[0] = last_pos[ui][0];
        chkp_campos_center[1] = last_pos[ui][1] + 0.5f;
        chkp_campos_center[2] = last_pos[ui][2];
    }
#endif

    if (!vary->base) return;

    /* k = +1.0 view is s_view 0 */

    if (k >= 0)
    {
        if (vary->base->wc > 0)
        {
#ifdef MAPC_INCLUDES_CHKP
            v_cpy(p1, last_active ? chkp_campos        : vary->base->wv[0].p);
            v_cpy(c1, last_active ? chkp_campos_center : vary->base->wv[0].q);
#else
            v_cpy(p1, vary->base->wv[0].p);
            v_cpy(c1, vary->base->wv[0].q);
#endif
        }
        else if (vary->base->uc > 0)
        {
            v_cpy(p1, vary->base->uv[0].p);
            v_cpy(c1, vary->base->uv[0].p);

            p1[1] += 10.0f;
            p1[2] += 10.0f;
        }
    }

    /* k = -1.0 view is s_view 1 */

    if (k <= 0)
    {
        if (vary->base->wc > 1)
        {
#ifdef MAPC_INCLUDES_CHKP
            v_cpy(p1, last_active ? chkp_campos        : vary->base->wv[1].p);
            v_cpy(c1, last_active ? chkp_campos_center : vary->base->wv[1].q);
#else
            v_cpy(p1, vary->base->wv[1].p);
            v_cpy(c1, vary->base->wv[1].q);
#endif
        }
        else if (vary->base->uc > 0)
        {
            v_cpy(p1, vary->base->uv[0].p);
            v_cpy(c1, vary->base->uv[0].p);

            p1[1] += 10.0f;
            p1[2] += 10.0f;
        }
    }

    /* Interpolate the views. */

    v_sub(v, p1, p0);
    v_mad(view->p, p0, v, k * k);

    v_sub(v, c1, c0);
    v_mad(view->c, c0, v, k * k);

    /* Orthonormalize the view basis. */

    v_sub(view->e[2], view->p, view->c);
    e_orthonrm_xz(view->e);
}

void game_view_death(struct game_view *view, const float p[3], int ui, float angle)
{
    const float vsin = fsinf(angle * V_PI / 180);
    const float vcos = fcosf(angle * V_PI / 180);

    float  x[3] = { vsin, 0.0f, 0.0f };
    float  y[3] = { 0.0f, 1.0f, 0.0f };
    float  z[3] = { 0.0f, 0.0f, vcos };
    float c0[3] = { 0.0f, 0.0f, 0.0f };
    float p0[3] = { 0.0f, 0.0f, 0.0f };
    float c1[3] = { 0.0f, 0.0f, 0.0f };
    float p1[3] = { 0.0f, 0.0f, 0.0f };
    float  v[3] = { 0.0f, 0.0f, 0.0f };

    game_view_init(view);

    v_cpy(c0, p);
    v_cpy(p0, p);

    v_mad(p0, p0, y, 5);
    v_mad(c0, c0, y, view->dc);

    v_mad(p0, p0, z, 9);
    v_mad(p0, p0, x, 9);

    v_cpy(view->p, p0);
    v_cpy(view->c, c0);

    /* Orthonormalize the view basis. */

    v_sub(view->e[2], view->p, view->c);
    e_orthonrm_xz(view->e);
}

void game_view_set_pos_and_target(struct game_view *view,
                                  const struct s_vary *vary,
                                  float pos[3], float center[3])
{
    if ((view && vary) &&
        (pos[0] != 0.0f || pos[1] != 0.0f || pos[2] != 0.0f) &&
        (center[0] != 0.0f || center[1] != 0.0f || center[2] != 0.0f))
    {
        float local_e[3][3] = {
            { 1, 0, 0 },
            { 0, 1, 0 },
            { 0, 0, 1 }
        };

        local_e[0][0] = 1.0f; local_e[0][1] = 0.0f; local_e[0][2] = 0.0f;
        local_e[1][0] = 0.0f; local_e[1][1] = 1.0f; local_e[1][2] = 0.0f;
        local_e[2][0] = 0.0f; local_e[2][1] = 0.0f; local_e[2][2] = 1.0f;

        /* Requires both, or it may not work! */

#ifndef NDEBUG
        assert(local_e && view->e);
        assert(local_e[2]);
#endif

        game_view_init(view);

        v_cpy(view->p, pos);
        v_cpy(view->c, center);

        /* Orthonormalize the view basis. */

        v_sub(local_e[2], view->p, view->c);
        e_orthonrm_xz(local_e);

        e_cpy(view->e, local_e);
    }
#ifndef NDEBUG
    else
        assert(0 && "No cam position or cam target selected!");
#endif
}

/*---------------------------------------------------------------------------*/

void lockstep_clr(struct lockstep *ls)
{
    ls->at = 0;
    ls->ts = 1.0f;
}

void lockstep_run(struct lockstep *ls, float dt)
{
    ls->at += dt * ls->ts;

    while (ls->at >= ls->dt)
    {
        ls->step(ls->dt);
        ls->at -= ls->dt;
    }
}

void lockstep_scl(struct lockstep *ls, float ts)
{
    ls->ts = ts;
}

/*---------------------------------------------------------------------------*/

/* See checkpoints.c */

struct s_base  game_base;
static char   *base_path;

int game_base_load(const char *path)
{
    if (base_path)
    {
        if (strcmp(base_path, path) == 0)
            return 1;

        sol_free_base(&game_base);

        free(base_path);
        base_path = NULL;
    }

    if (sol_load_base(&game_base, path))
    {
        base_path = strdup(path);
        return 1;
    }

    return 0;
}

void game_base_free(const char *next)
{
    if (base_path)
    {
        if (next && strcmp(base_path, next) == 0)
            return;

        sol_free_base(&game_base);

        free(base_path);
        base_path = NULL;
    }
}

/*---------------------------------------------------------------------------*/

float SPEED_FACTORS[SPEED_MAX] = {
    0.0f,
    1.0f / 128,
    1.0f / 64,
    1.0f / 32,
    1.0f / 16,
    1.0f / 8,
    1.0f / 4,
    1.0f / 2,
    1.0f,
    1.0f * 2,
    1.0f * 4,
    1.0f * 8,
    1.0f * 16,
    1.0f * 32,
    1.0f * 64,
    1.0f * 128,
};

/*---------------------------------------------------------------------------*/
