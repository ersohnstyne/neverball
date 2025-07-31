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

#ifndef __EMSCRIPTEN__
#error Only Emscripten SDK to be compiled with it!
#endif

#include <SDL.h>

#include "state.h"
#include "config.h"

/*---------------------------------------------------------------------------*/

static int st_dpad_btn_left;
static int st_dpad_btn_right;
static int st_dpad_btn_up;
static int st_dpad_btn_down;

void st_init_emscripten(void)
{
    st_dpad_btn_left  = 0;
    st_dpad_btn_right = 0;
    st_dpad_btn_up    = 0;
    st_dpad_btn_down  = 0;

    config_set_d(CONFIG_JOYSTICK, 0);
}

void st_stick_emscripten(int a, int vh)
{
    int _a = -1;

    switch (a) {
        case 0: _a = 0; break;
        case 1: _a = 1; break;
        case 2: _a = 3; break;
        case 3: _a = 4; break;
    }

    if (_a != -1)
        st_stick(_a, (float) (vh / 100.f));
}

void st_buttn_emscripten(int b, int d)
{
    int _b = -1;

    switch (b) {
        case 0:  _b = 0;  break;
        case 1:  _b = 1;  break;
        case 2:  _b = 2;  break;
        case 3:  _b = 3;  break;
        case 4:  _b = 4;  break;
        case 5:  _b = 5;  break;
        case 6:  _b = 6;  break;
        case 7:  _b = 7;  break;
        case 8:  _b = 8;  break;
        case 9:  _b = 9;  break;
        case 10: _b = 10; break;
        case 11: _b = 11; break;
    }

    if (_b != -1)
        if (!st_buttn(b, d)) {
            SDL_Event e = { SDL_QUIT };
            SDL_PushEvent(&e);
        }
}

void st_dpad_emscripten(int b, int d)
{
    switch (b) {
        case 12: st_dpad_btn_up    = d; break;
        case 13: st_dpad_btn_down  = d; break;
        case 14: st_dpad_btn_left  = d; break;
        case 15: st_dpad_btn_right = d; break;
    }

    const int p[4] = { st_dpad_btn_left, st_dpad_btn_right, st_dpad_btn_up, st_dpad_btn_down };

    if (!st_dpad(b, d, p)) {
        SDL_Event e = { SDL_QUIT };
        SDL_PushEvent(&e);
    }
}
