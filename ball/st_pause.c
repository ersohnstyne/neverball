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

 /*
 * HACK: Used with console version
 */
#include "console_control_gui.h"

#if NB_HAVE_PB_BOTH==1
#include "campaign.h"
#include "powerup.h"
#include "mediation.h"
#include "account.h"
#endif

#ifdef MAPC_INCLUDES_CHKP
#include "checkpoints.h" /* New: Checkpoints */
#endif

#include "gui.h"
#include "transition.h"
#include "config.h"
#include "video.h"
#include "progress.h"
#include "demo.h"
#include "level.h"
#include "audio.h"
#include "hud.h"
#include "key.h"

#include "game_common.h"
#include "game_server.h"
#include "game_client.h"

#include "st_common.h"
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

#define PAUSED_ACTION_CONTINUE              \
    do {                                    \
        if (curr_state() == &st_pause) {    \
            audio_music_fade_in(0.5f);      \
            if (st_continue != &st_level)   \
                video_set_grab(1);          \
            return exit_state(st_continue); \
        } else {                            \
            quit_uses_resetpuzzle = 0;      \
            quit_uses_restart     = 0;      \
            return exit_state(&st_pause);   \
        }                                   \
    } while (0)

int goto_pause(struct state *returnable)
{
    audio_play("snd/2.2/game_pause.ogg", 1.0f);

    st_continue = returnable;

    /* Set it up some those states? */
    if (st_continue == &st_play_ready ||
        st_continue == &st_play_set ||
        st_continue == &st_play_loop ||
        st_continue == &st_look)
    {
        if (st_continue == &st_play_set)
            st_continue = &st_play_ready;
        if (st_continue == &st_look)
            st_continue = &st_play_loop;
    }

#if ENABLE_LIVESPLIT!=0
    progress_livesplit_pause(1);
#endif

    return goto_state(&st_pause);
}

static int pause_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
        case PAUSE_CONTINUE:
            PAUSED_ACTION_CONTINUE;
            break;

        case PAUSE_CONF:
            return goto_conf(&st_pause, 1, 0);
            break;

        case PAUSE_RESPAWN:
#ifdef MAPC_INCLUDES_CHKP
            if (progress_same_avail() && last_active)
            {
                if (quit_uses_resetpuzzle)
                {
                    quit_uses_resetpuzzle = 0;
                    if (checkpoints_load() && progress_same())
                    {
                        audio_music_fade_in(0.5f);
                        return goto_play_level();
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
                progress_reinit(MODE_ZEN);
            }

            if (progress_same())
            {
                audio_music_fade_in(0.5f);
                return goto_play_level();
            }
#endif
            break;

        case PAUSE_RESTART:
            if (progress_same_avail())
            {
                if (quit_uses_restart
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                 || !campaign_used()
#endif
                    )
                {
                    quit_uses_restart = 0;
#ifdef MAPC_INCLUDES_CHKP
                    checkpoints_stop();
#endif
                    if (progress_same())
                    {
                        audio_music_fade_in(0.5f);
                        return goto_play_level();
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
            if (curr_mode() == MODE_NONE || campaign_used() ||
                curr_times() > 0)
            {
                if (curr_state() == &st_pause_quit)
                {
                    if (campaign_hardcore())
                    {
                        if (campaign_hardcore_norecordings())
                            demo_play_stop(1);

                        campaign_hardcore_quit();
                    }
                    if (curr_status() == GAME_NONE)
                        progress_stat(GAME_NONE);
                    if (curr_mode() != MODE_NONE)
                        audio_music_stop();

                    return goto_exit();
                }
                else
                    return goto_state(&st_pause_quit);
            }
            else
            {
                if (curr_status() == GAME_NONE)
                    progress_stat(GAME_NONE);

                audio_music_stop();
                return goto_exit();
            }
#else
            if (curr_status() == GAME_NONE)
                progress_stat(GAME_NONE);

            return goto_exit();
#endif

        break;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/

static int pause_button_width(int allow_zen, int reset_puzzle)
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

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    const char *quit_btn_text = campaign_used() ? N_("Quit") : N_("Give Up");
#else
    const char *quit_btn_text = N_("Give Up");
#endif

    btn_width = gui_measure(_(quit_btn_text), GUI_SML).w;
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
        if      (curr_mode() == MODE_NORMAL &&
                 account_get_d(ACCOUNT_PRODUCT_MEDIATION) && last_active)
            drastic = pause_button_width(1, 1) * 5 > config_get_d(CONFIG_WIDTH);
        else if (curr_mode() == MODE_NORMAL &&
                 account_get_d(ACCOUNT_PRODUCT_MEDIATION))
            drastic = pause_button_width(1, 0) * 4 > config_get_d(CONFIG_WIDTH);
        else
#endif
        if      (last_active)
            drastic = pause_button_width(0, 1) * 4 > config_get_d(CONFIG_WIDTH);
#else
#if defined(CONFIG_INCLUDES_ACCOUNT) && defined(LEVELGROUPS_INCLUDES_ZEN)
        if      (curr_mode() == MODE_NORMAL &&
                 account_get_d(ACCOUNT_PRODUCT_MEDIATION) && last_active)
            drastic = pause_button_width(1, 1) * 4 > config_get_d(CONFIG_WIDTH);
        else if (curr_mode() == MODE_NORMAL &&
                 account_get_d(ACCOUNT_PRODUCT_MEDIATION))
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
                    gui_state(jd, _("Reset Puzzle"), GUI_SML, PAUSE_RESPAWN, 0);
#endif

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
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            const char *quit_btn_text = campaign_used() ? N_("Quit") : N_("Give Up");
#else
            const char *quit_btn_text = N_("Give Up");
#endif

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            if (current_platform == PLATFORM_PC && !console_gui_shown())
#endif
            {
                gui_state(jd, _(quit_btn_text), GUI_SML, PAUSE_EXIT, 0);

                if (progress_same_avail())
                    gui_state(jd, _("Restart"), GUI_SML, PAUSE_RESTART, 0);

                gui_start(jd, _("Continue"), GUI_SML, PAUSE_CONTINUE, 0);
            }
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            else
            {
                gui_state(jd, _(quit_btn_text), GUI_SML, PAUSE_EXIT, 0);
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
                if (curr_mode() == MODE_NORMAL &&
                    account_get_d(ACCOUNT_PRODUCT_MEDIATION) &&
                    !mediation_enabled())
                    gui_state(jd, _("Switch to Zen"), GUI_SML, PAUSE_ZEN_SWITCH, 0);
#endif
            }
        }

        gui_pulse(title_id, 1.2f);
        gui_layout(id, 0, 0);
    }

    return id;
}

static int pause_enter(struct state *st, struct state *prev, int intent)
{
    video_clr_grab();

    /* Cannot pause the game in home room. */
    if (curr_mode() != MODE_NONE       &&
        curr_mode() != MODE_CHALLENGE  &&
        curr_mode() != MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
     && curr_mode() != MODE_HARDCORE
#endif
        )
        audio_music_fade_out(1.0f);

    hud_update(config_get_d(CONFIG_SCREEN_ANIMATIONS), 0.0f);

    return transition_slide(pause_gui(), 1, intent);
}

static int pause_leave(struct state *st, struct state *next, int id, int intent)
{
    if (next == &st_null)
    {
        progress_stat(GAME_NONE);
        progress_exit();

        campaign_quit();
        set_quit();

        game_server_free(NULL);
        game_client_free(NULL);
    }

    return transition_slide(id, 0, intent);
}

static void pause_paint(int id, float t)
{
    game_client_draw(0, t);

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    if (console_gui_shown())
        console_gui_death_paint();
#endif
    if (hud_visibility() || config_get_d(CONFIG_SCREEN_ANIMATIONS))
    {
        hud_paint();
        hud_lvlname_paint();
    }

    gui_paint(id);
}

static void pause_timer(int id, float dt)
{
    /* Cannot pause the game in home room. */
    if (curr_mode() == MODE_NONE)
    {
        game_step_fade(dt);
        game_server_step(dt);
        game_client_sync(curr_mode() != MODE_NONE
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                      && !campaign_hardcore_norecordings()
#endif
                             ? demo_fp : NULL);
        game_client_blend(game_server_blend());
    }

    gui_timer(id, dt);
    hud_timer(dt);

    hud_update(config_get_d(CONFIG_SCREEN_ANIMATIONS), dt);
}

static int pause_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
         && current_platform == PLATFORM_PC
#endif
            )
        {
            if (!st_global_animating())
            {
                audio_play(AUD_BACK, 1.0f);
                PAUSED_ACTION_CONTINUE;
            }
            else
                audio_play(AUD_DISABLED, 1.0f);
        }

        if (config_tst_d(CONFIG_KEY_RESTART, c)
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
         && current_platform == PLATFORM_PC
#endif
            )
        {
            if (progress_same_avail() && progress_same())
            {
#if NB_HAVE_PB_BOTH==1
                powerup_stop();
#endif
                return goto_play_level();
            }
            else
            {
                /* Can't do yet, play buzzer sound. */

                audio_play(AUD_DISABLED, 1.0f);
            }
        }
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

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
        {
            if (!st_global_animating())
            {
                audio_play(AUD_BACK, 1.0f);
                PAUSED_ACTION_CONTINUE;
            }
            else
                audio_play(AUD_DISABLED, 1.0f);
        }
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int pause_quit_gui(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        const char *quit_header_text = campaign_used() ? N_("Quit") : N_("Give Up");
#else
        const char *quit_header_text = N_("Give Up");
#endif

        int warn_title_id = gui_label(id, _(quit_header_text), GUI_MED, GUI_COLOR_RED);

        if (quit_uses_restart)
            gui_set_label(warn_title_id, _("Restart"));

        if (quit_uses_resetpuzzle)
            gui_set_label(warn_title_id, _("Reset Puzzle"));

        gui_pulse(warn_title_id, 1.2f);

        gui_space(id);

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        if (campaign_used())
        {
            const char *quit_warn_campaign = campaign_hardcore() ? N_("Return to World selection?") :
                                             (curr_mode() == MODE_NONE ? N_("Return to main menu?") :
                                              (quit_uses_resetpuzzle ? N_("Are you sure?\n"
                                                                          "You will restart at the last checkpoint.") :
                                                                       N_("Are you sure?\n"
                                                                          "You will lose all progress on this level.")));
            gui_multi(id, _(quit_warn_campaign), GUI_SML, GUI_COLOR_WHT);
        }
        else
#endif
        if (quit_uses_resetpuzzle)
        {
            gui_multi(id, _("Are you sure?\n"
                            "You will restart at the last checkpoint."),
                          GUI_SML, GUI_COLOR_WHT);
        }
        else if (curr_times() > 0)
        {
            const char *quit_warn_set = N_("Are you sure?\n"
                                           "You will lose all progress on this level set.");
            gui_multi(id, _(quit_warn_set), GUI_SML, GUI_COLOR_WHT);
        }

        gui_space(id);

        if ((jd = gui_harray(id)))
        {
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            if (current_platform == PLATFORM_PC)
#endif
            {
                gui_start(jd, _("No"), GUI_SML, GUI_BACK, 0);
                gui_state(jd, _("Yes"), GUI_SML,
                          quit_uses_restart ? PAUSE_RESTART :
                                              (quit_uses_resetpuzzle ? PAUSE_RESPAWN :
                                                                       PAUSE_EXIT), 0);
            }
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            else
                gui_start(jd, _("Yes"), GUI_SML,
                          quit_uses_restart ? PAUSE_RESTART :
                                              (quit_uses_resetpuzzle ? PAUSE_RESPAWN :
                                                                       PAUSE_EXIT), 0);
#endif
        }

        gui_layout(id, 0, 0);
    }

    return id;
}

static int pause_quit_enter(struct state *st, struct state *prev, int intent)
{
    if (curr_mode() != MODE_NONE       &&
        curr_mode() != MODE_CHALLENGE  &&
        curr_mode() != MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
     && curr_mode() != MODE_HARDCORE
#endif
        )
        audio_music_fade_out(0.5f);

    audio_play(AUD_WARNING, 1.0f);

    return transition_slide(pause_quit_gui(), 1, intent);
}

/*---------------------------------------------------------------------------*/

struct state st_pause = {
    pause_enter,
    pause_leave,
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
    pause_leave,
    shared_paint,
    pause_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    pause_keybd,
    pause_buttn
};
