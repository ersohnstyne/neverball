/*
 * Copyright (C) 2024 Microsoft / Neverball authors
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

/*---------------------------------------------------------------------------*/

void toggle_hud_visibility(int);
int  hud_visibility(void);

void hud_init(void);
void hud_free(void);

void hud_paint(void);
void hud_timer(float);
void hud_set_alpha(float);
void hud_show(float);
void hud_hide(void);

/*---------------------------------------------------------------------------*/

#endif
