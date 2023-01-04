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

#include "gui.h"
#include "vec3.h"
#include "glext.h"
#include "state.h"
#include "accessibility.h"
#include "config.h"
#include "video.h"
#include "common.h"
#include "hmd.h"
#include "geom.h"

/*---------------------------------------------------------------------------*/

#define STICK_MAX 32
#define STICK_HOLD_TIME 0.5f
#define STICK_REPEAT_TIME 0.2f

struct stick_cache
{
    int a;                              /* Axis index */
    float v, p;                         /* Axis value */
    float t;                            /* Repeat time */
};

static struct stick_cache stick_cache[STICK_MAX];
static int                stick_count;

static void cache_stick(int a, float v, float t)
{
    int i;

    /* Cache new values. */

    for (i = 0; i < stick_count; i++)
    {
        struct stick_cache *sc = &stick_cache[i];

        if (sc->a == a)
        {
            sc->p = sc->v;
            sc->v = v;

            if (fabsf(v) >= 0.5f && sc->t == 0.0f)
                sc->t = t;
            else if (fabsf(v) < 0.5f)
                sc->t = 0.0f;

            return;
        }
    }

    /* Cache new axis. */

    if (stick_count < STICK_MAX)
    {
        struct stick_cache *sc = &stick_cache[stick_count];

        sc->a = a;
        sc->p = 0.0f;
        sc->v = v;

        if (fabsf(v) >= 0.5f)
            sc->t = t;
        else if (fabsf(v) < 0.5f)
            sc->t = 0.0f;

        stick_count++;
    }
}

static int bump_stick(int a)
{
    int i;

    for (i = 0; i < stick_count; i++)
    {
        struct stick_cache *sc = &stick_cache[i];

        if (sc->a == a)
        {
            /* Note the transition from centered to leaned position. */

            return ((-0.5f <= sc->p && sc->p <= +0.5f) &&
                    (sc->v < -0.5f || +0.5f < sc->v));
        }
    }

    return 0;
}

/*---------------------------------------------------------------------------*/

/* SCREEN ANIMATIONS */
#define state_frame_smooth (1.f / 25.f) * 1000
#define state_anim_speed 6.f
static float alpha = 0;
static int intransition = 0;

static float         state_time;
static int           state_drawn;
static struct state *state;
static struct state *anim_next_state;

static int anim_queue = 0;

struct state *curr_state(void)
{
    return state;
}

float time_state(void)
{
    return state_time;
}

void init_state(struct state *st)
{
    state = st;
    alpha = 1.0f;
    video_clear();
    video_swap();
}

int goto_state(struct state *st)
{
    return goto_state_full(st, 0, 0, 0);
}

int goto_state_full(struct state *st, int fromdirection, int todirection, int noanimation)
{
    Uint32 currtime, prevtime, dt;
    struct state *prev = state;

    prevtime = SDL_GetTicks();

    if (intransition && anim_queue) return 1;

    anim_queue = 1;

    if (intransition == 0 &&
        (!noanimation && config_get_d(CONFIG_SCREEN_ANIMATIONS)))
    {
        intransition = 1;
        while (alpha > 0.01)
        {
            currtime = SDL_GetTicks();
            dt = MAX(currtime - prevtime, 0);
            alpha = alpha - ((config_get_d(CONFIG_SMOOTH_FIX) ? MIN(state_frame_smooth, dt) : MIN(100.f, dt)) * state_anim_speed) * 0.001;
            if (state)
            {
                if (state->fade != NULL) state->fade(alpha);
                gui_set_alpha(state->gui_id, alpha, fromdirection);
            }
#ifndef __EMSCRIPTEN__
            xbox_control_gui_set_alpha(alpha);
#endif

            if (accessibility_get_d(ACCESSIBILITY_SLOWDOWN) < 20) accessibility_set_d(ACCESSIBILITY_SLOWDOWN, 20);
            if (accessibility_get_d(ACCESSIBILITY_SLOWDOWN) > 100) accessibility_set_d(ACCESSIBILITY_SLOWDOWN, 100);
            float speedPercent = (float) accessibility_get_d(ACCESSIBILITY_SLOWDOWN) / 100;

            st_timer((0.001f * (config_get_d(CONFIG_SMOOTH_FIX) ? MIN(state_frame_smooth, dt) : dt)) * speedPercent);
            hmd_step();

            if (viewport_wireframe == 2 || viewport_wireframe == 3)
            {
                video_render_fill_or_line(1);
                st_paint(0.001f * currtime, 1);
                video_render_fill_or_line(0);
                st_paint(0.001f * currtime, 0);
            }
            else
                st_paint(0.001f * currtime, 1);

            video_swap();
            prevtime = currtime;
        }
    }

    alpha = 0;
    if (state)
    {
        if (state->fade != NULL) state->fade(alpha);
        gui_set_alpha(state->gui_id, alpha, fromdirection);
    }
#ifndef __EMSCRIPTEN__
    xbox_control_gui_set_alpha(alpha);
#endif

    if (state && state->leave)
        state->leave(state, st, state->gui_id);

    state       = st;
    state_time  = 0;
    state_drawn = 0;

    memset(&stick_cache, 0, sizeof (stick_cache));
    stick_count = 0;

    anim_queue = 0;

    if (state && state->enter)
        state->gui_id = state->enter(state, prev);

    intransition = 0;

    if (intransition == 0 &&
        (!noanimation && config_get_d(CONFIG_SCREEN_ANIMATIONS)))
    {
        intransition = 1;
        while (alpha < 0.99)
        {
            currtime = SDL_GetTicks();
            dt = MAX(currtime - prevtime, 0);
            alpha = alpha + ((config_get_d(CONFIG_SMOOTH_FIX) ? MIN(state_frame_smooth, dt) : MIN(100.f, dt)) * state_anim_speed) * 0.001;
            if (state)
            {
                if (state->fade != NULL) state->fade(alpha);
                gui_set_alpha(state->gui_id, alpha, todirection);
            }
#ifndef __EMSCRIPTEN__
            xbox_control_gui_set_alpha(alpha);
#endif

            if (accessibility_get_d(ACCESSIBILITY_SLOWDOWN) < 20) accessibility_set_d(ACCESSIBILITY_SLOWDOWN, 20);
            if (accessibility_get_d(ACCESSIBILITY_SLOWDOWN) > 100) accessibility_set_d(ACCESSIBILITY_SLOWDOWN, 100);
            float speedPercent = (float) accessibility_get_d(ACCESSIBILITY_SLOWDOWN) / 100;

            st_timer((0.001f * (config_get_d(CONFIG_SMOOTH_FIX) ? MIN(state_frame_smooth, dt) : dt)) * speedPercent);
            hmd_step();

            if (viewport_wireframe == 2 || viewport_wireframe == 3)
            {
                video_render_fill_or_line(1);
                st_paint(0.001f * currtime, 1);
                video_render_fill_or_line(0);
                st_paint(0.001f * currtime, 0);
            }
            else
                st_paint(0.001f * currtime, 1);

            video_swap();
            prevtime = currtime;
        }
    }

    alpha = 1.0f;
    if (state)
    {
        if (state->fade != NULL) state->fade(alpha);
        gui_set_alpha(state->gui_id, alpha, todirection);
    }
#ifndef __EMSCRIPTEN__
    xbox_control_gui_set_alpha(alpha);
#endif

    intransition = 0;

    return 1;
}

/*---------------------------------------------------------------------------*/

void st_paint(float t, int allow_clear)
{
    state_drawn = 1;

    if (state && state->paint)
    {
        if (allow_clear)
            video_clear();

        if (hmd_stat())
        {
            hmd_prep_left();

            if (allow_clear)
                video_clear();

            state->paint(state->gui_id, t);

            hmd_prep_right();

            if (allow_clear)
                video_clear();

            state->paint(state->gui_id, t);
        }
        else
            state->paint(state->gui_id, t);
    }
}

void st_timer(float dt)
{
    int i;

    if (!state_drawn)
        return;

    state_time += dt;

    if (state && state->timer)
        state->timer(state->gui_id, dt);

    for (i = 0; i < stick_count; i++)
    {
        struct stick_cache *sc = &stick_cache[i];

        if (sc->t > 0.0f && state_time >= sc->t)
        {
            if (state)
                state->stick(state->gui_id, sc->a, sc->v, 1);
            sc->t = state_time + STICK_REPEAT_TIME;
        }
    }
}

void st_point(int x, int y, int dx, int dy)
{
    if (state && state->point)
    {
        if (hmd_stat())
            state->point(state->gui_id, x * 2, y, dx, dy);
        else
            state->point(state->gui_id, x,     y, dx, dy);
    }
}

void st_stick(int a, float v)
{
    static struct
    {
        const int *num;
        const int *inv;
    } axes[] = {
        { &CONFIG_JOYSTICK_AXIS_X0, &CONFIG_JOYSTICK_AXIS_X0_INVERT },
        { &CONFIG_JOYSTICK_AXIS_Y0, &CONFIG_JOYSTICK_AXIS_Y0_INVERT },
        { &CONFIG_JOYSTICK_AXIS_X1, &CONFIG_JOYSTICK_AXIS_X1_INVERT },
        { &CONFIG_JOYSTICK_AXIS_Y1, &CONFIG_JOYSTICK_AXIS_Y1_INVERT }
    };

    int i;

    for (i = 0; i < ARRAYSIZE(axes); i++)
        if (config_tst_d(*axes[i].num, a) && config_get_d(*axes[i].inv))
        {
            v = -v;
            break;
        }

    if (state && state->stick)
    {
        cache_stick(a, v, state_time + STICK_HOLD_TIME);

        state->stick(state->gui_id, a, v, bump_stick(a));
    }
}

void st_angle(float x, float z)
{
    if (state && state->angle)
        state->angle(state->gui_id, x, z);
}


void st_wheel(int x, int y)
{
    if (state && state->wheel)
        state->wheel(x, y);
}

/*---------------------------------------------------------------------------*/

int st_click(int b, int d)
{
    return (state && state->click) ? state->click(b, d) : 1;
}

int st_keybd(int c, int d)
{
    return (state && state->keybd) ? state->keybd(c, d) : 1;
}

int st_buttn(int b, int d)
{
    return (state && state->buttn) ? state->buttn(b, d) : 1;
}

int st_touch(const SDL_TouchFingerEvent *event)
{
    int d = 1;

    /* If the state can handle it, do it. */

    if (state && state->touch)
        return state->touch(event);

    /* Otherwise, emulate mouse events. */

    st_point(video.device_w * event->x,
        video.device_h * (1.0f - event->y),
        video.device_w * event->dx,
        video.device_h * -event->dy);

    if (event->type == SDL_FINGERDOWN)
        d = st_click(SDL_BUTTON_LEFT, 1);
    else if (event->type == SDL_FINGERUP)
        d = st_click(SDL_BUTTON_LEFT, 0);

    return d;
}

/*---------------------------------------------------------------------------*/
