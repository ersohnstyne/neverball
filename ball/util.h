/*
 * Copyright (C) 2026 Microsoft / Neverball authors / Jānis Rūcis
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

#if _WIN32
#define _CRT_NB_UTIL_DEPRECATED(_Type, _Params, _Func, _Replaces) \
    __declspec(deprecated(                                        \
        "This function or variable has been superseded by "       \
        "newer game UI or game logic functionality. Consider "    \
        "using " #_Replaces " instead."                           \
    )) _Type _Func _Params
#else
#define _CRT_NB_UTIL_DEPRECATED(_Type, _Params, _Func, _Replaces) \
    _Type _Func _Params                                           \
    __attribute__ ((deprecated(                                   \
        "This function or variable has been superseded by "       \
        "newer game UI or game logic functionality. Consider "    \
        "using " #_Replaces " instead."                           \
    )))
#endif

#include "set.h"

/*---------------------------------------------------------------------------*/

#define NB_FRAMERATE_MIN 25

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

/*
 * This stats for campaign will be replaced into the gui_levelgroup_stats.
 * Your functions will be replaced using same parameters.
 */
_CRT_NB_UTIL_DEPRECATED(void, (const struct level *), gui_campaign_stats, gui_levelgroup_stats);

/*
 * This stats for level set will be replaced into the gui_levelgroup_stats.
 * Your functions will be replaced using same parameters.
 */
_CRT_NB_UTIL_DEPRECATED(void, (const struct level *), gui_set_stats, gui_levelgroup_stats);

void gui_levelgroup_stats(const struct level *);

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

/*
 * This function for button with icon will be replaced into the gui_text_icon_button.
 * Your functions will be replaced using eight parameters.
 */
_CRT_NB_UTIL_DEPRECATED(int, (int, int), gui_start_button, gui_text_icon_button);

/*
 * This function name will be redirected to gui_text_icon_button() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `gui_text_icon_button()` to `gui_back_button()`.
 */
#define gui_back_button(_pd) \
    (gui_text_icon_button(_pd, _("Back"), GUI_CROSS, gui_red, GUI_BACK, 0, 1, 0))

int gui_text_icon_button(int pd,
                         const char *text, const char *icon, const unsigned char *c0,
                         const int tok, const int val,
                         const int enabled, const int destructive);

int gui_text_icon_button_LH(int pd,
                            const char *text, const char *icon, const unsigned char *c0,
                            const int tok, const int val,
                            const int enabled, const int destructive);

void gui_scoreboard_free(void);

/*---------------------------------------------------------------------------*/

#endif
