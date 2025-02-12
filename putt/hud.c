/*
 * Copyright (C) 2025 Microsoft / Neverball authors / Jānis Rūcis
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
#include "lang.h"

/*---------------------------------------------------------------------------*/

static int Lhud_id;
static int Rhud_id;
static int fps_id;

static int Touch_id;

static int stroke_type_id;

static float touch_timer;

static int is_init;

static int show_hud;
static int show_hud_expected;

static float show_hud_alpha;
static float show_hud_total_alpha;

void toggle_hud_visibility(int active)
{
    show_hud = active;
}

int hud_visibility(void)
{
    return show_hud && show_hud_expected;
}

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
    gui_paint(Touch_id);
}

void hud_timer(float dt)
{
    if (touch_timer > 0.0f)
    {
        touch_timer -= dt;

        if (touch_timer <= 0.0f)
        {
            touch_timer = 0.0f;
            gui_slide(Touch_id, GUI_N | GUI_EASE_BACK | GUI_BACKWARD, 0, 0.3f, 0);
        }
    }

    gui_timer(fps_id, dt);
    gui_timer(stroke_type_id, dt);
    gui_timer(Lhud_id, dt);
    gui_timer(Rhud_id, dt);

    gui_timer(Touch_id, dt);
}

void hud_show(float delay)
{
    if (show_hud_expected == 1) return;

    show_hud_expected = 1;
    
    gui_slide(fps_id,         GUI_NW | GUI_EASE_BACK, delay + 0.0f, 0.3f, 0);
    gui_slide(Lhud_id,        GUI_SW | GUI_EASE_BACK, delay + 0.0f, 0.3f, 0);
    gui_slide(stroke_type_id, GUI_N  | GUI_EASE_BACK, delay + 0.1f, 0.3f, 0);
    gui_slide(Rhud_id,        GUI_SE | GUI_EASE_BACK, delay + 0.2f, 0.3f, 0);
}

void hud_hide(void)
{
    if (show_hud_expected == 0) return;

    show_hud_expected = 0;

    gui_slide(fps_id,         GUI_NW | GUI_EASE_BACK, 0, 0.3f, 0);
    gui_slide(Lhud_id,        GUI_SW | GUI_EASE_BACK, 0, 0.3f, 0);
    gui_slide(stroke_type_id, GUI_N  | GUI_EASE_BACK, 0, 0.3f, 0);
    gui_slide(Rhud_id,        GUI_SE | GUI_EASE_BACK, 0, 0.3f, 0);
}

#if defined(__ANDROID__) || defined(__IOS__) || defined(__EMSCRIPTEN__)
int hud_touch(const SDL_TouchFingerEvent *e)
{
    if (touch_timer == 0.0f)
        gui_slide(Touch_id, GUI_N | GUI_EASE_BACK, 0, 0.3f, 0);

    touch_timer = 5.0f;

    if (e->type == SDL_FINGERUP)
    {
        const int x = (int) ((float) video.device_w * e->x);
        const int y = (int) ((float) video.device_h * (1.0f - e->y));

        return gui_point(touch_id, x, y);
    }

    return 0;
}
#endif


static void hud_update_alpha(void)
{
    float standard_hud_alpha   = show_hud_alpha *
                                 show_hud_total_alpha;
    float cam_hud_alpha        = show_hud_total_alpha;
    float replay_hud_alpha     = show_hud_alpha *
                                 show_hud_total_alpha;

    gui_set_alpha(fps_id,          standard_hud_alpha,
                                   GUI_ANIMATION_N_CURVE);
#if NB_HAVE_PB_BOTH==1 && defined(LEVELGROUPS_INCLUDES_CAMPAIGN)
    gui_set_alpha(camcompass_id,   standard_hud_alpha,
                                   GUI_ANIMATION_N_CURVE);
#endif
    gui_set_alpha(Touch_id,        standard_hud_alpha,
                                   GUI_ANIMATION_N_CURVE | GUI_ANIMATION_E_CURVE);
    gui_set_alpha(Lhud_id,         standard_hud_alpha,
                                   GUI_ANIMATION_S_CURVE | GUI_ANIMATION_W_CURVE);
    gui_set_alpha(Rhud_id,         standard_hud_alpha,
                                   GUI_ANIMATION_S_CURVE | GUI_ANIMATION_E_CURVE);
}

void hud_set_alpha(float a)
{
    show_hud_total_alpha = a;
    hud_update_alpha();
}

/*---------------------------------------------------------------------------*/
