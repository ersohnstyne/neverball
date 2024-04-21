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

#ifndef STATE_H
#define STATE_H

#if _WIN32
#define _CRT_NB_SCREENSTATE_DEPRECATED(_Type, _Params, _Func, _Replaces) \
    __declspec(deprecated(                                               \
        "This function or variable has been superceded by newer UI "     \
        "animations functionality. Consider using " #_Replaces           \
        " instead."                                                      \
    )) _Type _Func _Params
#else
#define _CRT_NB_SCREENSTATE_DEPRECATED(_Type, _Params, _Func, _Replaces) \
    _Type _Func _Params                                                  \
    __attribute__ ((deprecated(                                          \
        "This function or variable has been superceded by newer UI "     \
        "animations functionality. Consider using " #_Replaces           \
        " instead."                                                      \
    )))
#endif

#if _WIN32 && __MINGW32__
#include <SDL2/SDL_events.h>
#else
#include <SDL_events.h>
#endif

/*---------------------------------------------------------------------------*/

#define CHECK_GAMESPEED(a, b) \
    if (accessibility_get_d(ACCESSIBILITY_SLOWDOWN) < a \
        || accessibility_get_d(ACCESSIBILITY_SLOWDOWN) > b) \
        accessibility_set_d(ACCESSIBILITY_SLOWDOWN, CLAMP(a, accessibility_get_d(ACCESSIBILITY_SLOWDOWN), b));

extern int st_global_loop(void);

/*---------------------------------------------------------------------------*/

/*
 * Things throttles, just find the keyword regex
 * with "struct state [^*]+= {\n"
 */

struct state
{
    int  (*enter) (struct state *, struct state *prev);
    void (*leave) (struct state *, struct state *next, int id);
    void (*paint) (int id, float t);
    void (*timer) (int id, float dt);
    void (*point) (int id, int x, int y, int dx, int dy);
    void (*stick) (int id, int a, float v, int bump);
    void (*angle) (int id, float x, float z);
    int  (*click) (int b,  int d);
    int  (*keybd) (int c,  int d);
    int  (*buttn) (int b,  int d);
    void (*wheel) (int x,  int y);
    int  (*touch) (const SDL_TouchFingerEvent *);
    void (*fade)  (float alpha);
    void (*exit)  (int);

    int gui_id;
};

struct state *curr_state(void);
struct state *queue_state(void);

float time_state(void);
void  init_state(struct state *);

/*
 * This screenstate transition will be replaced into the goto_state_full.
 * Your functions will be replaced using four parameters.
 */
_CRT_NB_SCREENSTATE_DEPRECATED(int, (struct state *st), goto_state, goto_state_full);

int  goto_state_full(struct state *st, int fromdirection, int todirection, int noanimation);

int  st_global_animating(void);

void st_paint(float, int);
void st_timer(float);
void st_point(int, int, int, int);
void st_stick(int, float);
void st_angle(float, float);
void st_wheel(int, int);
int  st_click(int, int);
int  st_keybd(int, int);
int  st_buttn(int, int);
int  st_touch(const SDL_TouchFingerEvent *);
void st_exit (void);

/*---------------------------------------------------------------------------*/

#endif
