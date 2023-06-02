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

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
#include "console_control_gui.h"
#endif

#include "audio.h"
#include "config.h"
#include "geom.h"
#include "gui.h"
#include "lang.h"
#include "state.h"

#include "game_client.h"
#include "game_common.h"
#include "game_proxy.h"
#include "game_server.h"

#include "st_conf.h"
#include "st_shared.h"

/*---------------------------------------------------------------------------*/

static int name_id;
static int beam_index;

static void set_curr_beam(void)
{
    config_set_d(CONFIG_ACCOUNT_BEAM_STYLE, beam_index);

    geom_free();
    geom_init();

    char *beam_version_name = "";
    
    switch (beam_index)
    {
    case 0:
        beam_version_name = "Remastered version (1.7)";
        break;
    case 1:
        beam_version_name = "Standard version (1.6.0)";
        break;
    case 2:
        beam_version_name = "Standard version (1.5.4)";
        break;
    case 3:
        beam_version_name = "Standard version (1.5.3)";
        break;
    }

    config_save();
    gui_set_label(name_id, beam_version_name);
}

static int beam_style_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
    case GUI_BACK:
        game_fade(+4.0);
        return goto_state(&st_conf_account);
        break;
    case GUI_PREV:
        beam_index--;
        if (beam_index < 0) beam_index = 3;

        set_curr_beam();
        break;
    case GUI_NEXT:
        beam_index++;
        if (beam_index > 3) beam_index = 0;

        set_curr_beam();
        break;
    }

    return 1;
}

static int beam_style_gui(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        if ((jd = gui_hstack(id)))
        {
            gui_label(jd, _("Beam Style"), GUI_SML, 0, 0);
            gui_filler(jd);
            gui_space(jd);

#if NB_HAVE_PB_BOTH==1
#ifndef __EMSCRIPTEN__
            if (current_platform == PLATFORM_PC)
                gui_start(jd, _("Back"), GUI_SML, GUI_BACK, 0);
            else
#endif
                gui_filler(jd);
#else
            gui_start(jd, _("Back"), GUI_SML, GUI_BACK, 0);
#endif
        }

        gui_space(id);

        if ((jd = gui_hstack(id)))
        {
#ifndef __EMSCRIPTEN__
            if (!xbox_show_gui())
#endif
#ifdef SWITCHBALL_GUI
                gui_maybe_img(jd, "gui/navig/arrow_right_disabled.png", "gui/navig/arrow_right.png", GUI_NEXT, GUI_NONE, 1);
#else
                gui_maybe(jd, GUI_TRIANGLE_RIGHT, GUI_NEXT, GUI_NONE, 1);
#endif

            name_id = gui_label(jd, "very-long-beam-style-name", GUI_SML,
                gui_wht, gui_wht);

            gui_set_trunc(name_id, TRUNC_TAIL);
            gui_set_fill(name_id);

#ifndef __EMSCRIPTEN__
            if (!xbox_show_gui())
#endif
#ifdef SWITCHBALL_GUI
                gui_maybe_img(jd, "gui/navig/arrow_left_disabled.png", "gui/navig/arrow_left.png", GUI_PREV, GUI_NONE, 1);
#else
                gui_maybe(jd, GUI_TRIANGLE_LEFT, GUI_PREV, GUI_NONE, 1);
#endif
        }
    }

    for (int i = 0; i < 12; i++)
        gui_space(id);

    gui_layout(id, 0, 0);

    beam_index = config_get_d(CONFIG_ACCOUNT_BEAM_STYLE);

    char *beam_version_name = "";

    switch (beam_index)
    {
    case 0:
        beam_version_name = "Remastered version (1.7)";
        break;
    case 1:
        beam_version_name = "Standard version (1.6.0)";
        break;
    case 2:
        beam_version_name = "Standard version (1.5.4)";
        break;
    case 3:
        beam_version_name = "Standard version (1.5.3)";
        break;
    }

    gui_set_label(name_id, beam_version_name);

    return id;
}

static int beam_style_enter(struct state *st, struct state *prev)
{
    if (game_client_init("gui/beam-style.sol"))
    {
        union cmd cmd;

        cmd.type = CMD_GOAL_OPEN;
        game_proxy_enq(&cmd);
        game_client_sync(NULL);

        game_client_fly(1.0f);

        game_fade(-4.0);
        if (!config_get_d(CONFIG_SCREEN_ANIMATIONS))
            game_kill_fade();

        game_client_toggle_show_balls(0);

        return beam_style_gui();
    }

    /* Ignore this, we don't have any map files. */
    return beam_style_action(GUI_BACK, 0);
}

static void beam_style_leave(struct state *st, struct state *next, int id)
{
    gui_delete(id);
}

static void beam_style_paint(int id, float t)
{
    video_push_persp((float) config_get_d(CONFIG_VIEW_FOV), 0.1f, FAR_DIST);
    video_pop_matrix();

    game_client_draw(0, t);

    gui_paint(id);
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
    xbox_control_beam_style_gui_paint();
#endif
}

static void beam_style_timer(int id, float dt)
{
    gui_timer(id, dt);
    geom_step(dt);
    game_step_fade(dt);
}

static int beam_style_keybd(int c, int d)
{
    if (d)
    {
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        if (c == KEY_EXIT && current_platform == PLATFORM_PC)
#else
        if (c == KEY_EXIT)
#endif
            return beam_style_action(GUI_BACK, 0);
    }
    return 1;
}

static int beam_style_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return beam_style_action(gui_token(active), gui_value(active));

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return beam_style_action(GUI_BACK, 0);

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L1, b))
            return beam_style_action(GUI_PREV, gui_value(active));

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_R1, b))
            return beam_style_action(GUI_NEXT, gui_value(active));
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

struct state st_beam_style =
{
    beam_style_enter,
    beam_style_leave,
    beam_style_paint,
    beam_style_timer,
    shared_point,
    shared_stick,
    NULL,
    shared_click,
    beam_style_keybd,
    beam_style_buttn
};