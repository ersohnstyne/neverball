/*
 * Copyright (C) 2026 Microsoft / Neverball authors / Jānis Rūcis
 *
 * NEVERPUTT is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */

#ifndef HUD_H
#define HUD_H

#if NB_HAVE_PB_BOTH==1 && NB_PB_SDL3==1
#include <SDL3/SDL_Events.h>
#else
#include "SDL_events.h"
#endif

/*---------------------------------------------------------------------------*/

void putt_toggle_hud_visibility(int);
int  putt_hud_visibility(void);

void putt_hud_init(void);
void putt_hud_free(void);

void putt_hud_paint(void);
void putt_hud_timer(float);
void putt_hud_set_alpha(float);
void putt_hud_show(float);
void putt_hud_hide(void);

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__)
int putt_hud_touch(const SDL_TouchFingerEvent *e);
#endif

/*---------------------------------------------------------------------------*/

#endif
