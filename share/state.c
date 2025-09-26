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

/*
 * HACK: Used with console version
 */
#include "console_control_gui.h"

#include "activity_services.h"

#include "vec3.h"
#include "glext.h"
#include "state.h"
#include "accessibility.h"
#include "config.h"
#include "video.h"
#if ENABLE_DUALDISPLAY==1
#include "video_dualdisplay.h"
#endif
#include "common.h"
#include "hmd.h"
#include "geom.h"
#include "gui.h"
#include "transition.h"
#include "log.h"

/*---------------------------------------------------------------------------*/

#define STICK_MAX         32
#define STICK_HOLD_TIME   0.5f
#define STICK_REPEAT_TIME 0.2f

struct stick_cache
{
    int   a;                            /* Axis index  */
    float v, p;                         /* Axis value  */
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

#define LOOP_DURING_SCREENANIMATE   \
    do { if (!st_global_loop()) {   \
        SDL_Event e = { SDL_QUIT }; \
        SDL_PushEvent(&e);          \
    } } while (0)

/*---------------------------------------------------------------------------*/

/* SCREEN ANIMATIONS */
#define state_frame_smooth (1.0f / 25.0f) * 1000
#define state_anim_speed 6.0f

static int   anim_queue = 0;
static int   anim_done  = 0;

static float         state_time;
static int           state_drawn;
static struct state *state;

static struct state *anim_queue_state;
static int           anim_queue_allowskip;
static int           anim_queue_intent;
static int           anim_queue_directions[2];

struct state *curr_state(void)
{
    return state;
}

struct state *queue_state(void)
{
    return anim_queue_state;
}

float time_state(void)
{
    return state_time;
}

void init_state(struct state *st)
{
    state = st;
}

int goto_state_intent(struct state *st, int intent)
{
    return goto_state_full_intent(st, 0, 0, 0, intent);
}

int goto_state_full_intent(struct state *st,
                           int fromdirection, int todirection, int noanimation, int intent)
{
    int r           = 1;
    int prev_gui_id = 0;
    struct state *prev = state;

    anim_queue_state         = st;
    anim_queue_directions[0] = fromdirection;
    anim_queue_directions[1] = todirection;
    anim_queue_allowskip     = noanimation;

    if (anim_queue) return 1;

    anim_done  = 0;
    anim_queue = 1;

    if (state)
    {
        if (state->fade != NULL) state->fade(1.0f);

        gui_set_alpha(state->gui_id, 1.0f, fromdirection);
    }

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    console_gui_set_alpha(1.0f);
    console_gui_slide(GUI_S | GUI_EASE_ELASTIC | GUI_BACKWARD);
#endif

    if (state && state->leave)
    {
        prev_gui_id   = state->gui_id;
        state->gui_id = state->leave(state, st, state->gui_id, intent);

        if (!state->gui_id)
            gui_remove(prev_gui_id);
    }

    state       = anim_queue_state;
    state_time  = 0;
    state_drawn = 0;

    anim_queue_state = NULL;

    memset(&stick_cache, 0, sizeof (stick_cache));
    stick_count = 0;

    if (state && state->enter)
        state->gui_id = state->enter(state, prev, intent);

    if (state)
    {
        if (state->fade != NULL) state->fade(1.0f);

        gui_set_alpha(state->gui_id, 1.0f, todirection);
    }

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    console_gui_set_alpha(1.0f);
    console_gui_slide(GUI_S | GUI_EASE_ELASTIC);
#endif

    anim_done  = 1;

#ifndef NDEBUG
    if (!(r && state))
        log_errorf("(r && state) returned 0!\n");
#endif

    anim_queue = 0;

    if (anim_queue_state != NULL)
    {
        int r = goto_state_full_intent(anim_queue_state,
                                       anim_queue_directions[0],
                                       anim_queue_directions[1],
                                       anim_queue_allowskip,
                                       anim_queue_intent);

        anim_queue_state         = NULL;
        anim_queue_directions[0] = 0;
        anim_queue_directions[1] = 0;
        anim_queue_allowskip     = 0;
        anim_queue_intent        = -1;

        if (!r) {
            SDL_Event e = { SDL_QUIT };
            SDL_PushEvent(&e);
        }
    }

    return r && state;
}

int goto_state(struct state *st)
{
    return goto_state_intent(st, INTENT_FORWARD);
}

int exit_state(struct state *st)
{
    return goto_state_intent(st, INTENT_BACK);
}

int st_global_animating(void)
{
    return anim_queue;
}

/*---------------------------------------------------------------------------*/

void st_paint(float t, int allow_clear)
{
    state_drawn = 1;

    if (state && state->paint)
    {
        if (allow_clear)
            video_clear();

#if ENABLE_MOTIONBLUR!=0
        if (config_get_d(CONFIG_MOTIONBLUR))
            video_motionblur_prep();
#endif

        if (hmd_stat())
        {
            hmd_prep_left();

            if (allow_clear)
                video_clear();

            state->paint(state->gui_id, t);

            transition_paint();

            hmd_prep_right();

            if (allow_clear)
                video_clear();

            state->paint(state->gui_id, t);

            transition_paint();
        }
        else
        {
            state->paint(state->gui_id, t);

            transition_paint();
        }
    }
}

void st_timer(float dt)
{
    int i;

    if (!state_drawn)
        return;

    transition_timer(dt);

    console_gui_timer(dt);

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

    if (fabsf(v) < 0.05f)
        v = 0.0f;

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
    if (console_gui_shown()) return;

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    console_gui_toggle(0);
#endif

    if (state && state->wheel)
        state->wheel(x, y);
}

/*---------------------------------------------------------------------------*/

int st_click(int b, int d)
{
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    console_gui_toggle(0);
#endif

    return (state && state->click) ? state->click(b, d) : 1;
}

int st_keybd(int c, int d)
{
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    console_gui_toggle(0);
#endif

    return (state && state->keybd) ? state->keybd(c, d) : 1;
}

int st_buttn(int b, int d)
{
    return (state && state->buttn) ? state->buttn(b, d) : 1;
}

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__)
int st_touch(const SDL_TouchFingerEvent *event)
{
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    console_gui_toggle(0);
#endif

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
#endif

int st_dpad(int b, int d, int *p)
{
    /* If the state can handle it, do it. */

    if (state && state->dpad)
        return state->dpad(state->gui_id, b, d);

    /* Otherwise, convert D-pad button events into joystick axis motion. */

    const int X = config_get_d(CONFIG_JOYSTICK_AXIS_X0);
    const int Y = config_get_d(CONFIG_JOYSTICK_AXIS_Y0);

    if      (p[0] && !p[1]) st_stick(X, -1.0f);
    else if (p[1] && !p[0]) st_stick(X, +1.0f);
    else                    st_stick(X,  0.0f);

    if      (p[2] && !p[3]) st_stick(Y, -1.0f);
    else if (p[3] && !p[2]) st_stick(Y, +1.0f);
    else                    st_stick(Y,  0.0f);

    return 1;
}

/*---------------------------------------------------------------------------*/
