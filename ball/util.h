/*
 * Copyright (C) 2023 Microsoft / Neverball authors
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

#ifndef UTIL_H
#define UTIL_H

#include "set.h"

/*---------------------------------------------------------------------------*/

#define GUI_SCORE_COIN  0x1
#define GUI_SCORE_TIME  0x2
#define GUI_SCORE_GOAL  0x4
#define GUI_SCORE_ALL  (GUI_SCORE_COIN | GUI_SCORE_TIME | GUI_SCORE_GOAL)

#define GUI_SCORE_NEXT(s) \
    ((((s) << 1) & GUI_SCORE_ALL) ? (s) << 1 : GUI_SCORE_COIN)

#define GUI_SCORE_PREV(s) \
    ((((s) >> 1) & GUI_SCORE_ALL) ? (s) >> 1 : GUI_SCORE_GOAL)

void gui_score_set(int);
int  gui_score_get(void);

void gui_score_board(int, unsigned int, int, int);
void set_score_board(const struct score *, int,
                     const struct score *, int,
                     const struct score *, int);

void gui_keyboard(int);
void gui_keyboard_lock(void);
void gui_keyboard_en(int);
void gui_keyboard_lock_en(void);
void gui_keyboard_de(int);
void gui_keyboard_lock_de(void);
char gui_keyboard_char(char);

/*---------------------------------------------------------------------------*/

#endif
