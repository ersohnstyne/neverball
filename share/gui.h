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

#ifndef GUI_H
#define GUI_H

#if _WIN32
#define _CRT_NB_GUI_DEPRECATED(_Type, _Params, _Func, _Replaces) \
    __declspec(deprecated(                                       \
        "This function or variable has been superceded by "      \
        "newer UI parts functionality. Consider using "          \
         #_Replaces " instead."                                  \
    )) _Type _Func _Params
#else
#define _CRT_NB_GUI_DEPRECATED(_Type, _Params, _Func, _Replaces) \
    _Type _Func _Params                                          \
    __attribute__ ((deprecated(                                  \
        "This function or variable has been superceded by "      \
        "newer UI parts functionality. Consider using "          \
         #_Replaces " instead."                                  \
    )))
#endif

#if NB_HAVE_PB_BOTH==1
#define SWITCHBALL_GUI
#endif

#include "glext.h"
#include "video.h"
#include "font.h"

/*---------------------------------------------------------------------------*/

#define GUI_ELLIPSIS       "…"
#define GUI_AIRPLANE       "✈"
#define GUI_ARROW_DN       "⬇"
#define GUI_TRIANGLE_LEFT  "◀️"
#define GUI_TRIANGLE_RIGHT "▶️"
#define GUI_CHECKMARK      "✔"
#define GUI_BALLOT_X       "✘"
#define GUI_STAR           "✯"
#define GUI_COIN           "⛁"
#define GUI_CROWN          "♕"
#define GUI_DIAMOND        "♦"
#define GUI_CIRCLE_ARROW   "⟳"
#define GUI_CROSS          "×"
#define GUI_ROMAN_2        "Ⅱ"
#define GUI_FISHEYE        "◉"

#define GUI_GAMEPAD        "🎮"
#define GUI_BATTERY        "🔋"
#define GUI_GOAL           "🏁"
#define GUI_TIMEOUT        "⏰"
#define GUI_HEARTBROKEN    "💔"

/*
 * DejaVu Sans V1: DejaVuSans-Bold.ttf
 * DejaVu Sans V2: DejaVuSans-Bold-v2.ttf
 * Fiolex: FiolexGirls.ttf
 */
#define GUI_FACE "ttf/DejaVuSans-Bold.ttf"

#define GUI_TNY FONT_SIZE_TNY
#define GUI_XS  FONT_SIZE_XS
#define GUI_SML FONT_SIZE_SML
#define GUI_TCH FONT_SIZE_TCH
#define GUI_MED FONT_SIZE_MED
#define GUI_LRG FONT_SIZE_LRG

/* Sides */

#define GUI_N    1
#define GUI_E    2
#define GUI_S    4
#define GUI_W    8

/* Corners */

#define GUI_NW   (GUI_N | GUI_W)
#define GUI_SW   (GUI_S | GUI_W)
#define GUI_NE   (GUI_N | GUI_E)
#define GUI_SE   (GUI_S | GUI_E)

/* Animation flags */

#define GUI_BACKWARD     (GUI_W << 1)
#define GUI_FLING        (GUI_W << 2)
#define GUI_REMOVE       (GUI_W << 3)
#define GUI_EASE_ELASTIC (GUI_W << 4)
#define GUI_EASE_BACK    (GUI_W << 5)
#define GUI_EASE_BOUNCE  (GUI_W << 6)

/* Rotation animation flags */

#define GUI_ROTATE                  (GUI_W << 7)
#define GUI_ROTATE_IN_EASE_ELASTIC  (GUI_W << 8)
#define GUI_ROTATE_OUT_EASE_ELASTIC (GUI_W << 9)
#define GUI_ROTATE_IN_EASE_BACK     (GUI_W << 10)
#define GUI_ROTATE_OUT_EASE_BACK    (GUI_W << 11)

/* Multiple corners */

#define GUI_LFT  (GUI_NW  | GUI_SW)
#define GUI_RGT  (GUI_NE  | GUI_SE)
#define GUI_TOP  (GUI_NW  | GUI_NE)
#define GUI_BOT  (GUI_SW  | GUI_SE)
#define GUI_ALL  (GUI_TOP | GUI_BOT)

/* Animation directions */

#define GUI_ANIMATION_NONE 0x0000
#define GUI_ANIMATION_N_LINEAR 0xF000
#define GUI_ANIMATION_E_LINEAR 0x0F00
#define GUI_ANIMATION_S_LINEAR 0x00F0
#define GUI_ANIMATION_W_LINEAR 0x000F

#define GUI_ANIMATION_N_CURVE 0x8000
#define GUI_ANIMATION_E_CURVE 0x0800
#define GUI_ANIMATION_S_CURVE 0x0080
#define GUI_ANIMATION_W_CURVE 0x0008

/* Default gradient colors */

#if NB_HAVE_PB_BOTH==1
#define GUI_COLOR_DEFAULT gui_pnk, gui_red
#else
#define GUI_COLOR_DEFAULT gui_yel, gui_red
#endif

#define GUI_COLOR_WHT gui_wht, gui_wht
#define GUI_COLOR_YEL gui_yel, gui_yel
#define GUI_COLOR_CYA gui_cya, gui_cya
#define GUI_COLOR_TWI gui_twi, gui_twi
#define GUI_COLOR_VIO gui_vio, gui_vio
#define GUI_COLOR_PNK gui_pnk, gui_pnk
#define GUI_COLOR_RED gui_red, gui_red
#define GUI_COLOR_GRN gui_grn, gui_grn
#define GUI_COLOR_BLU gui_blu, gui_blu
#define GUI_COLOR_BRN gui_brn, gui_brn
#define GUI_COLOR_BLK gui_blk, gui_blk
#define GUI_COLOR_GRY gui_gry, gui_gry

#define GUI_COLOR_WHT2 gui_wht2, gui_wht2

extern const GLubyte gui_wht[4];
extern const GLubyte gui_yel[4];
extern const GLubyte gui_cya[4];
extern const GLubyte gui_twi[4];
extern const GLubyte gui_vio[4];
extern const GLubyte gui_pnk[4];
extern const GLubyte gui_red[4];
extern const GLubyte gui_blu[4];
extern const GLubyte gui_brn[4];
extern const GLubyte gui_grn[4];
extern const GLubyte gui_blk[4];
extern const GLubyte gui_gry[4];

extern const GLubyte gui_wht2[4];

enum trunc
{
    TRUNC_NONE,
    TRUNC_HEAD,
    TRUNC_TAIL
};

/*---------------------------------------------------------------------------*/

void gui_init(void);
void gui_free(void);

void gui_resize(void);

/*---------------------------------------------------------------------------*/

void gui_set_label (int, const char *);
void gui_set_image (int, const char *);
void gui_set_font  (int, const char *);
void gui_set_multi (int, const char *);
void gui_set_count (int, int);
void gui_set_clock (int, int);
void gui_set_color (int, const GLubyte *, const GLubyte *);
void gui_set_trunc (int, enum trunc);
void gui_set_fill  (int);
int  gui_set_state (int, int, int);
void gui_set_hilite(int, int);
void gui_set_rect  (int, int);
void gui_clr_rect  (int);
void gui_set_clip  (int);
void gui_clr_clip  (int);
void gui_set_cursor(int);
void gui_set_hidden(int id, int hidden);

/*---------------------------------------------------------------------------*/

int  gui_harray(int);
int  gui_varray(int);
int  gui_hstack(int);
int  gui_vstack(int);
int  gui_filler(int);
int  gui_root  (void);

int  gui_image(int, const char *, int, int);
int  gui_start(int, const char *, int, int, int);
int  gui_state(int, const char *, int, int, int);
int  gui_label(int, const char *, int, const GLubyte *, const GLubyte *);
int  gui_multi(int, const char *, int, const GLubyte *, const GLubyte *);
int  gui_title_header(int, const char *, int, const GLubyte *, const GLubyte *);
int  gui_count(int, int, int);
int  gui_clock(int, int, int);
int  gui_space(int);

/*---------------------------------------------------------------------------*/

void gui_dump(int, int);
void gui_layout(int, int, int);
int  gui_search(int, int, int);
int  gui_child(int id, int index);
int  gui_delete(int);
void gui_remove(int id);

/*---------------------------------------------------------------------------*/

void gui_set_alpha(int, float, int);
void gui_paint(int);
void gui_pulse(int, float);
void gui_timer(int, float);
int  gui_point(int, int, int);
int  gui_stick(int, int, float, int);
int  gui_click(int, int);
void gui_focus(int);
void gui_alpha(int, float);

int  gui_active(void);
int  gui_token(int);
int  gui_value(int);
void gui_toggle(int);

/*---------------------------------------------------------------------------*/

void gui_set_slide(int id, int flags, float delay, float t, float stagger);

void gui_slide(int id, int flags, float delay, float t, float stagger);

/*---------------------------------------------------------------------------*/

/*
 * Reserved GUI tokens. (Mostly Neverball specific.)
 */

enum
{
    GUI_NONE = 0,

    GUI_BACK,
    GUI_PREV,
    GUI_NEXT,
    GUI_BS,
    GUI_CL,
    GUI_CHAR,
    GUI_NAME,
    GUI_SCORE,
    GUI_CAMERA,

    GUI_LAST
};

_CRT_NB_GUI_DEPRECATED(int, (int id, int total, int first, int step), gui_navig, gui_navig_full);

int gui_navig_full(int id, int total, int first, int step, int back_disabled);
int gui_maybe_img(int, const char *, const char *, int, int, int);
int gui_maybe(int, const char *, int, int, int);

/*---------------------------------------------------------------------------*/

struct size
{
    int w, h;
};

struct size gui_measure(const char *text, int size);

/*---------------------------------------------------------------------------*/

#endif
