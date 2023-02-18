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

#include <assert.h>

#if NB_HAVE_PB_BOTH==1
#ifndef __EMSCRIPTEN__
#include "console_control_gui.h"
#endif

#include "campaign.h"
#include "checkpoints.h" // New: Checkpoints
#include "powerup.h"
#include "mediation.h"
#include "account.h"
#endif

#include "gui.h"
#include "config.h"
#include "video.h"
#include "progress.h"
#include "demo.h"
#include "level.h"
#include "audio.h"
#include "hud.h"

#include "game_common.h"
#include "game_server.h"
#include "game_client.h"

#include "st_play.h"
#include "st_level.h"
#include "st_pause.h"
#include "st_shared.h"
#include "st_conf.h"

/*---------------------------------------------------------------------------*/

struct state st_pause_quit;

/*---------------------------------------------------------------------------*/

enum
{
    PAUSE_CONF = GUI_LAST,
    PAUSE_ZEN_SWITCH,
    PAUSE_RESPAWN,
    PAUSE_CONTINUE,
    PAUSE_RESTART,
    PAUSE_EXIT
};

static struct state *st_continue;

static int quit_uses_resetpuzzle = 0;
static int quit_uses_restart = 0;

/*---------------------------------------------------------------------------*/

int goto_pause(struct state *returnable)
{
    st_continue = returnable;

    /* Set it up some those states? */
    if (st_continue == &st_play_ready
        || st_continue == &st_play_set
        || st_continue == &st_play_loop
        || st_continue == &st_look)
    {
        if (st_continue == &st_play_set)
            st_continue = &st_play_ready;
        if (st_continue == &st_look)
            st_continue = &st_play_loop;
    }

    return goto_state(&st_pause);
}

static int pause_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
    case PAUSE_CONF:
        return goto_conf(&st_pause, 1, 0);
        break;
    case PAUSE_CONTINUE:
        if (curr_state() == &st_pause)
        {
            audio_music_fade_in(0.5f);

            if (st_continue != &st_level)
                video_set_grab(0);

            return goto_state(st_continue);
        }
        else
        {
            quit_uses_resetpuzzle = 0;
            quit_uses_restart = 0;
            return goto_state(&st_pause);
        }
        break;

    case PAUSE_RESPAWN:
#ifdef MAPC_INCLUDES_CHKP
        if (progress_same_avail() && last_active)
        {
            if (quit_uses_resetpuzzle)
            {
                quit_uses_resetpuzzle = 0;
                checkpoints_respawn();
                if (progress_same())
                {
                    audio_music_fade_in(0.5f);
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                    return goto_state(campaign_used() ? &st_play_ready : &st_level);
#else
                    return goto_state(&st_play_ready);
#endif
                }
            }
            else if (!quit_uses_resetpuzzle)
            {
                quit_uses_resetpuzzle = 1;
                return goto_state(&st_pause_quit);
            }
        }
#endif
        break;

    case PAUSE_ZEN_SWITCH:
#if defined(LEVELGROUPS_INCLUDES_ZEN) && defined(CONFIG_INCLUDES_ACCOUNT)
        if (progress_same_avail())
        {
            mediation_init();
            progress_init(MODE_ZEN);
        }

        if (progress_same())
        {
            audio_music_fade_in(0.5f);
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            return goto_state(campaign_used() ? &st_play_ready : &st_level);
#else
            return goto_state(&st_play_ready);
#endif
        }
#endif
        break;

    case PAUSE_RESTART:
        if (progress_same_avail())
        {
            if (quit_uses_restart || !campaign_used())
            {
                quit_uses_restart = 0;
#ifdef MAPC_INCLUDES_CHKP
                checkpoints_stop();
#endif
                if (progress_same())
                {
                    audio_music_fade_in(0.5f);
                    video_set_grab(1);
                    return goto_state(&st_play_ready);
                }
            }
            else if (!quit_uses_restart)
            {
                quit_uses_restart = 1;
                return goto_state(&st_pause_quit);
            }
        }
        break;

    case PAUSE_EXIT:
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        if (curr_mode() == MODE_NONE || campaign_used()
            || curr_times() > 0)
        {
            if (curr_state() == &st_pause_quit)
            {
                if (campaign_hardcore())
                {
                    if (campaign_hardcore_norecordings()) demo_play_stop(1);
                    campaign_hardcore_quit();
                }
                if (curr_status() == GAME_NONE) progress_stat(GAME_NONE);
                progress_stop();
                if (curr_mode() != MODE_NONE) audio_music_stop();
                return goto_exit();
            }
            else
                return goto_state(&st_pause_quit);
        }
        else
        {
            if (curr_status() == GAME_NONE) progress_stat(GAME_NONE);
            progress_stop();
            audio_music_stop();
            return goto_exit();
        }
#else
        if (curr_status() == GAME_NONE) progress_stat(GAME_NONE);
        progress_stop();
        audio_music_stop();
        return goto_exit();
#endif

        break;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/

int pause_button_width(int allow_zen, int reset_puzzle)
{
    int targ_width = 0;
    int btn_width;

    if (allow_zen)
    {
        btn_width = gui_measure(_("Switch to Zen"), GUI_SML).w;
        if (btn_width > targ_width) targ_width = btn_width;
    }

    if (reset_puzzle)
    {
        btn_width = gui_measure(_("Reset Puzzle"), GUI_SML).w;
        if (btn_width > targ_width) targ_width = btn_width;
    }

    btn_width = gui_measure(_("Continue"), GUI_SML).w;
    if (btn_width > targ_width) targ_width = btn_width;

    btn_width = gui_measure(_("Restart"), GUI_SML).w;
    if (btn_width > targ_width) targ_width = btn_width;

    btn_width = gui_measure(_("Quit"), GUI_SML).w;
    if (btn_width > targ_width) targ_width = btn_width;

    return targ_width;
}

static int pause_gui(void)
{
    int id, jd, title_id;

    /* Build the pause GUI. */

    if ((id = gui_vstack(0)))
    {
        int drastic = gui_measure(_("Paused"), GUI_LRG).w >= video.device_w;
        title_id = gui_title_header(id, _("Paused"), GUI_LRG, gui_gry, gui_red);

        gui_space(id);

#ifdef MAPC_INCLUDES_CHKP
#if defined(CONFIG_INCLUDES_ACCOUNT) && defined(LEVELGROUPS_INCLUDES_ZEN)
        if (curr_mode() == MODE_NORMAL && account_get_d(ACCOUNT_PRODUCT_MEDIATION) && last_active)
            drastic = pause_button_width(1, 1) * 5 > config_get_d(CONFIG_WIDTH);
        else if (curr_mode() == MODE_NORMAL && account_get_d(ACCOUNT_PRODUCT_MEDIATION))
            drastic = pause_button_width(1, 0) * 4 > config_get_d(CONFIG_WIDTH);
        else
#endif
        if (last_active)
            drastic = pause_button_width(0, 1) * 4 > config_get_d(CONFIG_WIDTH);
#else
#if defined(CONFIG_INCLUDES_ACCOUNT) && defined(LEVELGROUPS_INCLUDES_ZEN)
        if (curr_mode() == MODE_NORMAL && account_get_d(ACCOUNT_PRODUCT_MEDIATION) && last_active)
            drastic = pause_button_width(1, 1) * 4 > config_get_d(CONFIG_WIDTH);
        else if (curr_mode() == MODE_NORMAL && account_get_d(ACCOUNT_PRODUCT_MEDIATION))
            drastic = pause_button_width(1, 0) * 3 > config_get_d(CONFIG_WIDTH);
#endif
#endif

        gui_state(id, _("Options"), GUI_SML, PAUSE_CONF, 0);
        gui_space(id);
        
        /*
         * If the wide button is drastic from width pixels,
         * stack more buttons.
         */

        if (drastic)
        {
            if ((jd = gui_harray(id)))
            {
#ifdef MAPC_INCLUDES_CHKP
                if (progress_same_avail() && last_active)
#else
                if (progress_same_avail())
#endif
                    gui_state(jd, _("Reset Puzzle"), GUI_SML, PAUSE_RESPAWN, 0);

#if defined(CONFIG_INCLUDES_ACCOUNT) && defined(LEVELGROUPS_INCLUDES_ZEN)
                if (curr_mode() == MODE_NORMAL &&
                    account_get_d(ACCOUNT_PRODUCT_MEDIATION) &&
                    !mediation_enabled()
                    )
                    gui_state(jd, _("Switch to Zen"), GUI_SML, PAUSE_ZEN_SWITCH, 0);
#endif
            }

            gui_space(id);
        }

        if ((jd = gui_harray(id)))
        {
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
            if (current_platform == PLATFORM_PC)
#endif
            {
                gui_state(jd, _("Quit"), GUI_SML, PAUSE_EXIT, 0);

                if (progress_same_avail())
                    gui_state(jd, _("Restart"), GUI_SML, PAUSE_RESTART, 0);

                gui_start(jd, _("Continue"), GUI_SML, PAUSE_CONTINUE, 0);
            }
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
            else
            {
                gui_state(jd, _("Quit"), GUI_SML, PAUSE_EXIT, 0);
                if (progress_same_avail())
                    gui_start(jd, _("Restart"), GUI_SML, PAUSE_RESTART, 0);
            }
#endif

            /* If the wide button is not drastic, give more wide grid. */

            if (!drastic)
            {
#ifdef MAPC_INCLUDES_CHKP
                if (progress_same_avail() && last_active)
                    gui_state(jd, _("Reset Puzzle"), GUI_SML, PAUSE_RESPAWN, 0);
#endif

#if defined(CONFIG_INCLUDES_ACCOUNT) && defined(LEVELGROUPS_INCLUDES_ZEN)
                if (curr_mode() == MODE_NORMAL && account_get_d(ACCOUNT_PRODUCT_MEDIATION)
                    && !mediation_enabled())
                    gui_state(jd, _("Switch to Zen"), GUI_SML, PAUSE_ZEN_SWITCH, 0);
#endif
            }
        }

        gui_pulse(title_id, 1.2f);
        gui_layout(id, 0, 0);
    }

    return id;
}

static int pause_enter(struct state *st, struct state *prev)
{
    video_clr_grab();

    /* Cannot pause the game in home room. */
    if (curr_mode() != MODE_NONE)
        audio_music_fade_out(0.5f);

    hud_update(0, 0.f);

    return pause_gui();
}

static void pause_paint(int id, float t)
{
    shared_paint(id, t);

#ifndef __EMSCRIPTEN__
    if (xbox_show_gui())
        xbox_control_paused_gui_paint();
    else if (hud_visibility() || config_get_d(CONFIG_SCREEN_ANIMATIONS))
#endif
        hud_paint();
}

static void pause_timer(int id, float dt)
{
    /* Cannot pause the game in home room. */
    if (curr_mode() == MODE_NONE)
    {
        game_step_fade(dt);
        game_server_step(dt);
        game_client_sync(!campaign_hardcore_norecordings() && curr_mode() != MODE_NONE ? NULL : demo_fp);
        game_client_blend(game_server_blend());
    }

    gui_timer(id, dt);
    hud_timer(dt);

    hud_update(0, dt);
}

static int pause_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT)
            return pause_action(PAUSE_CONTINUE, 0);

        if (config_tst_d(CONFIG_KEY_RESTART, c) && progress_same_avail())
            return pause_action(PAUSE_RESTART, 0);
    }
    return 1;
}

static int pause_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return pause_action(gui_token(active), gui_value(active));

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b) ||
            config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
            return pause_action(PAUSE_CONTINUE, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int pause_quit_gui(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        int warn_title = gui_label(id, _("Quit"), GUI_MED, gui_red, gui_red);

        if (quit_uses_restart)
            gui_set_label(warn_title, _("Restart"));

        if (quit_uses_resetpuzzle)
            gui_set_label(warn_title, _("Reset Puzzle"));

        gui_pulse(warn_title, 1.2f);

        gui_space(id);

        char *quit_warn = campaign_hardcore() ? _("Return to World selection?") : _("Are you sure?\\You will lose all progress on this level.");
        if (curr_mode() == MODE_NONE)
            quit_warn = _("Return to main menu?");

        if (quit_uses_resetpuzzle)
            quit_warn = _("Are you sure?\\You will restart at the last checkpoint.");

        if (!campaign_used() && curr_times() > 0)
            quit_warn = _("Are you sure?\\You will lose all progress on this level set.");

        gui_multi(id, quit_warn, GUI_SML, gui_wht, gui_wht);

        gui_space(id);

        if ((jd = gui_harray(id)))
        {
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
            if (current_platform == PLATFORM_PC)
#endif
            {
                gui_start(jd, _("No"), GUI_SML, PAUSE_CONTINUE, 0);
                gui_state(jd, _("Yes"), GUI_SML,
                    quit_uses_restart ? PAUSE_RESTART :
                        quit_uses_resetpuzzle ? PAUSE_RESPAWN : PAUSE_EXIT, 0);
            }
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
            else
                gui_start(jd, _("Yes"), GUI_SML,
                    quit_uses_restart ? PAUSE_RESTART :
                    quit_uses_resetpuzzle ? PAUSE_RESPAWN : PAUSE_EXIT, 0);
#endif
        }
    }

    gui_layout(id, 0, 0);
    return id;
}

static int pause_quit_enter(struct state *st, struct state *prev)
{
    if (curr_mode() != MODE_NONE)
        audio_music_fade_out(0.5f);

    return pause_quit_gui();
}

/*---------------------------------------------------------------------------*/

struct state st_pause = {
    pause_enter,
    shared_leave,
    pause_paint,
    pause_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    pause_keybd,
    pause_buttn
};

struct state st_pause_quit = {
    pause_quit_enter,
    shared_leave,
    shared_paint,
    pause_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    pause_keybd,
    pause_buttn
};