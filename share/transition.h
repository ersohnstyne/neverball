/*
 * Copyright (C) 2024 Microsoft / Neverball Authors / Jānis Rūcis
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

#ifndef TRANSITION_H
#define TRANSITION_H

#define INTENT_FORWARD 0
#define INTENT_BACK 1

#if _WIN32
#define _CRT_NB_TRANSITION_DEPRECATED(_Type, _Params, _Func, _Replaces) \
    __declspec(deprecated(                                               \
        "This function or variable has been superceded by newer UI "     \
        "animations functionality. Consider using " #_Replaces           \
        " instead."                                                      \
    )) _Type _Func _Params
#else
#define _CRT_NB_TRANSITION_DEPRECATED(_Type, _Params, _Func, _Replaces) \
    _Type _Func _Params                                                  \
    __attribute__ ((deprecated(                                          \
        "This function or variable has been superceded by newer UI "     \
        "animations functionality. Consider using " #_Replaces           \
        " instead."                                                      \
    )))
#endif

void transition_init(void);
void transition_quit(void);

_CRT_NB_TRANSITION_DEPRECATED(void, (int id), transition_add, transition_add_full);

void transition_add_full(int id, int flags);
void transition_remove(int id);

void transition_timer(float dt);
void transition_paint(void);

/*
 * This screenstate transition will be replaced into the transition_slide_full.
 * Your functions will be replaced using four parameters.
 */
_CRT_NB_TRANSITION_DEPRECATED(int, (int id, int in, int intent), transition_slide, transition_slide_full);

int transition_slide_full(int id, int in,
                          int enter_flags, int exit_flags);

int transition_page(int id, int in, int intent);

#endif
