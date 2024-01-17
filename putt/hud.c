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

#include <math.h>

#include "gui.h"
#include "hud.h"
#include "hole.h"
#include "config.h"
#include "video.h"

/*---------------------------------------------------------------------------*/

static int Lhud_id;
static int Rhud_id;
static int fps_id;

static int touch_id;

static int stroke_type_id;

static int is_init;

/*---------------------------------------------------------------------------*/

void hud_init(void)
{
    if (is_init) return;

    static const GLubyte *color[5] =
    {
        gui_wht,
        gui_red,
        gui_grn,
        gui_blu,
        gui_yel
    };

    int i = curr_player();

    if ((stroke_type_id = gui_label(0, _(curr_stroke_type_name()), GUI_SML, color[i], gui_wht)))
    {
        gui_set_rect(stroke_type_id, GUI_BOT);
        gui_layout(stroke_type_id, 0, 1);
    }

    if ((Lhud_id = gui_hstack(0)))
    {
        gui_label(Lhud_id, curr_scr(), GUI_SML, color[i], gui_wht);
        gui_label(Lhud_id, _("Score"), GUI_SML,  gui_wht,  gui_wht);
        gui_set_rect(Lhud_id, GUI_NE);
        gui_layout(Lhud_id, -1, -1);
    }
    if ((Rhud_id = gui_hstack(0)))
    {
        gui_label(Rhud_id, curr_par(), GUI_SML,  color[i], gui_wht);
        gui_label(Rhud_id, _("Par"),   GUI_SML, gui_wht,  gui_wht);
        gui_set_rect(Rhud_id, GUI_NW);
        gui_layout(Rhud_id, +1, -1);
    }

    /* Let Mojang done these */
#if defined(__ANDROID__) || defined(__IOS__) || defined(__EMSCRIPTEN__)
    if ((touch_id = gui_vstack(0)))
    {
        gui_space(touch_id);

        if ((id = gui_hstack(touch_id)))
        {
            gui_state(id, GUI_ROMAN_2, GUI_TCH, GUI_BACK, 0);

            gui_space(id);

            gui_state(id, GUI_FISHEYE, GUI_TCH, GUI_CAMERA, 0);

            gui_space(id);
        }

        gui_layout(touch_id, -1, +1);
    }
#endif

    if ((fps_id = gui_count(0, 1000, GUI_SML)))
    {
        gui_set_rect(fps_id, GUI_SE);
        gui_layout(fps_id, -1, +1);
    }

    is_init = 1;
}

void hud_free(void)
{
    if (!is_init) return;
    gui_delete(stroke_type_id);
    gui_delete(Lhud_id);
    gui_delete(Rhud_id);
#if defined(__ANDROID__) || defined(__IOS__) || defined(__EMSCRIPTEN__)
    gui_delete(touch_id);
#endif
    gui_delete(fps_id);

    is_init = 0;
}

/*---------------------------------------------------------------------------*/

void hud_paint(void)
{
    if (config_get_d(CONFIG_FPS))
    {
        gui_set_count(fps_id, video_perf());
        gui_paint(fps_id);
    }

    gui_paint(stroke_type_id);
    gui_paint(Rhud_id);
    gui_paint(Lhud_id);
    gui_paint(touch_id);
}

#if defined(__ANDROID__) || defined(__IOS__) || defined(__EMSCRIPTEN__)
int hud_touch(const SDL_TouchFingerEvent *e)
{
    if (e->type == SDL_FINGERUP)
    {
        const int x = (int) ((float) video.device_w * e->x);
        const int y = (int) ((float) video.device_h * (1.0f - e->y));

        return gui_point(touch_id, x, y);
    }

    return 0;
}
#endif

void hud_set_alpha(float a)
{
    gui_set_alpha(fps_id,         a,
                                  GUI_ANIMATION_N_CURVE | GUI_ANIMATION_W_CURVE);
    gui_set_alpha(stroke_type_id, a,
                                  GUI_ANIMATION_N_CURVE);
    gui_set_alpha(Rhud_id,        a,
                                  GUI_ANIMATION_S_CURVE | GUI_ANIMATION_E_CURVE);
    gui_set_alpha(Lhud_id,        a,
                                  GUI_ANIMATION_S_CURVE | GUI_ANIMATION_W_CURVE);
#if defined(__ANDROID__) || defined(__IOS__) || defined(__EMSCRIPTEN__)
    gui_set_alpha(touch_id,       a,
                                  GUI_ANIMATION_N_CURVE | GUI_ANIMATION_E_CURVE);
#endif
}

/*---------------------------------------------------------------------------*/
