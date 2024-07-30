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

#include "progress.h"

#include "video_dualdisplay.h"
#include "common.h"
#include "gui.h"

/*---------------------------------------------------------------------------*/

#if ENABLE_DUALDISPLAY==1 && !defined(__GAMECUBE__) && !defined(__WII__)

static int ddpy_state;

static int ddpy_monitor_allowed;
static int ddpy_curr_heart;

static int ddpy_root_id;

static int ddpy_bg_id;
static int ddpy_time_id;

/*---------------------------------------------------------------------------*/

int game_dualdisplay_init(void)
{
    if (ddpy_state || video_dualdisplay_is_init())
    {
        ddpy_state = 1;
        return 1;
    }

    return (ddpy_state = video_dualdisplay_init()) != 0;
}

void game_dualdisplay_quit(void)
{
    video_dualdisplay_quit();

    ddpy_state = 0;
}

/*---------------------------------------------------------------------------*/

void game_dualdisplay_gui_init(void)
{
    if (!ddpy_state) return;

    int w = video_ddpy.ddpy_device_w;
    int h = video_ddpy.ddpy_device_h;

    const int ww_widescr = h * MIN(w, h) / 16;
    const int hh_widescr = ww_widescr / 16 * 9;

    const int ww = h * MIN(w, h) / 4;
    const int hh = ww / 4 * 3;

    if ((ddpy_root_id = gui_root()))
    {
        if ((ddpy_bg_id = gui_image(ddpy_root_id, "gui/hud/ui_newhud_ball_bg.png", ww_widescr, hh_widescr)))
        {
            gui_clr_rect(ddpy_bg_id);
            gui_layout(ddpy_bg_id, 0, 0);
        }

        if ((ddpy_time_id = gui_clock(ddpy_root_id, 8640000, GUI_MED)))
        {
            gui_set_rect(ddpy_time_id, GUI_BOT);
            gui_layout(ddpy_time_id, 0, +1);
        }
    }
}

void game_dualdisplay_gui_free(void)
{
    if (!ddpy_state) return;

    gui_delete(ddpy_time_id); ddpy_time_id = NULL;
    gui_delete(ddpy_bg_id);   ddpy_bg_id   = NULL;
    gui_delete(ddpy_root_id); ddpy_root_id = NULL;
}

void game_dualdisplay_set_mode(int m)
{
    if (!ddpy_state) return;

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    ddpy_monitor_allowed = m != MODE_NORMAL     &&
                           m != MODE_STANDALONE &&
                           m != MODE_CAMPAIGN;
#else
    ddpy_monitor_allowed = m != MODE_NORMAL     &&
                           m != MODE_STANDALONE;
#endif
}

void game_dualdisplay_set_time(int hsecond)
{
    if (!ddpy_state) return;

    gui_set_clock(ddpy_time_id, hsecond);
}

void game_dualdisplay_set_heart(int heart)
{
    if (!ddpy_state) return;

    ddpy_curr_heart = heart;
}

void game_dualdisplay_draw(void)
{
    if (!ddpy_state) return;

    if (ddpy_bg_id) gui_paint(ddpy_bg_id);

    if (ddpy_monitor_allowed)
    {
        /* Shall we monitoring them? */
        if (ddpy_time_id) gui_paint(ddpy_time_id);
    }
}

#endif
