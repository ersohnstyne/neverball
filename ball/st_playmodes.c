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

#if NB_HAVE_PB_BOTH==1
#ifndef __EMSCRIPTEN__
#include "console_control_gui.h"
#endif

#include "networking.h"
#include "accessibility.h"
#include "campaign.h"
#include "account.h"
#endif

#include "hud.h"
#include "gui.h"
#include "audio.h"
#include "config.h"
#include "progress.h"
#include "video.h"

#include "game_client.h"
#include "game_common.h"

#if NB_HAVE_PB_BOTH==1
#include "st_playmodes.h"
#endif

#include "st_set.h"
#include "st_level.h"
#include "st_shared.h"
#include "st_play.h"

/*---------------------------------------------------------------------------*/

static int switchball_useable(void)
{
    const SDL_Keycode k_auto    = config_get_d(CONFIG_KEY_CAMERA_TOGGLE);
    const SDL_Keycode k_cam1    = config_get_d(CONFIG_KEY_CAMERA_1);
    const SDL_Keycode k_cam2    = config_get_d(CONFIG_KEY_CAMERA_2);
    const SDL_Keycode k_cam3    = config_get_d(CONFIG_KEY_CAMERA_3);
    const SDL_Keycode k_restart = config_get_d(CONFIG_KEY_RESTART);
    const SDL_Keycode k_caml    = config_get_d(CONFIG_KEY_CAMERA_L);
    const SDL_Keycode k_camr    = config_get_d(CONFIG_KEY_CAMERA_R);

    SDL_Keycode k_arrowkey[4];
    k_arrowkey[0] = config_get_d(CONFIG_KEY_FORWARD);
    k_arrowkey[1] = config_get_d(CONFIG_KEY_LEFT);
    k_arrowkey[2] = config_get_d(CONFIG_KEY_BACKWARD);
    k_arrowkey[3] = config_get_d(CONFIG_KEY_RIGHT);

    if (k_auto == SDLK_c && k_cam1 == SDLK_3 && k_cam2 == SDLK_1 && k_cam3 == SDLK_2
        && k_caml == SDLK_RIGHT && k_camr == SDLK_LEFT
        && k_arrowkey[0] == SDLK_w && k_arrowkey[1] == SDLK_a && k_arrowkey[2] == SDLK_s && k_arrowkey[3] == SDLK_d)
        return 1;
    else if (k_auto == SDLK_c && k_cam1 == SDLK_3 && k_cam2 == SDLK_1 && k_cam3 == SDLK_2
             && k_caml == SDLK_d && k_camr == SDLK_a
             && k_arrowkey[0] == SDLK_UP && k_arrowkey[1] == SDLK_LEFT && k_arrowkey[2] == SDLK_DOWN && k_arrowkey[3] == SDLK_RIGHT)
        return 1;

    /*
     * If the Switchball input preset is not detected,
     * Try it with the Neverball by default.
     */

    return 0;
}

/*---------------------------------------------------------------------------*/

struct state st_hardcore_start;

/*---------------------------------------------------------------------------*/

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN

static int playmodes_state(int id, int tok, int val, int unlocked,
                           char *title, char *unlock_desc, char *lock_desc)
{
    int jd;

    gui_space(id);

    if ((jd = gui_vstack(id)))
    {
        gui_multi(jd, title, GUI_SML,
                  unlocked ? gui_wht : gui_gry, unlocked ? gui_yel : gui_red);
        gui_multi(jd, unlocked ? unlock_desc : lock_desc, GUI_SML,
                  gui_wht, gui_wht);

        gui_set_rect(jd, GUI_ALL);
        gui_set_state(jd, unlocked ? tok : GUI_NONE, val);
    }

    return jd;
}

/*---------------------------------------------------------------------------*/

static int career_changed = 0;

enum {
    PLAYMODES_CAREER_MODE = GUI_LAST,
    PLAYMODES_HARDCORE
};

static int playmodes_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
    case GUI_BACK:
        return goto_state_full(&st_campaign,
                               GUI_ANIMATION_S_CURVE,
                               GUI_ANIMATION_N_CURVE,
                               0);
    case PLAYMODES_CAREER_MODE:
        config_set_d(CONFIG_LOCK_GOALS,
                     !config_get_d(CONFIG_LOCK_GOALS));

        career_changed = 1;

        audio_play(AUD_SWITCH, 1.0f);

        if (config_get_d(CONFIG_LOCK_GOALS))
            audio_play(AUD_GOAL, 1.0f);

        config_save();

        return goto_state_full(&st_playmodes, 0, 0, 1);
    case PLAYMODES_HARDCORE:
        return goto_state(&st_hardcore_start);
    }
    return 1;
}

static int playmodes_gui(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        if ((jd = gui_hstack(id)))
        {
            gui_label(jd, _("Play Modes"), GUI_SML, 0, 0);
            gui_filler(jd);

#ifndef __EMSCRIPTEN__
            if (current_platform == PLATFORM_PC)
#endif
                gui_start(jd, _("Back"), GUI_SML, GUI_BACK, 0);
        }

        int   career_unlocked = (server_policy_get_d(SERVER_POLICY_PLAYMODES_UNLOCKED_MODE_CAREER)
                              || campaign_career_unlocked());
        char *career_title    = config_get_d(CONFIG_LOCK_GOALS) ?
                                _("Career Mode (Currently ENABLED!)") :
                                _("Career Mode (Currently disabled)");
        char *career_text     = _("Toggle career mode in the entire game.\\"
                                  "Compatible with Level Set.");

        playmodes_state(id, PLAYMODES_CAREER_MODE, 0,
                        career_unlocked &&
                        server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_CAREER),
                        career_unlocked &&
                        server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_CAREER) ?
                        career_title : _("Career Mode"),
                        _(career_text),
                        server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_CAREER) ?
                        _("Complete the game to unlock.") :
                        _("Career mode is not available\\with server group policy."));

        int hardc_unlocked = (server_policy_get_d(SERVER_POLICY_PLAYMODES_UNLOCKED_MODE_HARDCORE)
                           || campaign_hardcore_unlocked());
        int hardc_requirement = accessibility_get_d(ACCESSIBILITY_SLOWDOWN) >= 100
#if NB_STEAM_API==0 && NB_EOS_SDK==0
            && !config_cheat()
#endif
            && (!config_get_d(CONFIG_SMOOTH_FIX) || video_perf() >= 25);

        if (!CHECK_ACCOUNT_ENABLED)
        {
            playmodes_state(id, GUI_NONE, 0, 0,
                            mode_to_str(MODE_HARDCORE, 1), "",
                            _("Hardcore Mode is not available.\\"
                              "Please check your account settings!"));
        }
        else if (CHECK_ACCOUNT_BANKRUPT)
        {
            playmodes_state(id, GUI_NONE, 0, 0,
                            mode_to_str(MODE_HARDCORE, 1), "",
                            _("Your player account is bankrupt.\\"
                              "Restore from the backup or delete the\\"
                              "local account and start over from scratch."));
        }
        else if (server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_HARDCORE))
        {
            playmodes_state(id, PLAYMODES_HARDCORE, 0, hardc_unlocked && hardc_requirement,
                            mode_to_str(MODE_HARDCORE, 1),
                            _("Play the entire game without dying once."),
                            !hardc_requirement ?
#if NB_STEAM_API==0 && NB_EOS_SDK==0
                            _("Hardcore Mode is not available\\"
                              "with slowdown, cheat or smooth fix.") :
#else
                            _("Hardcore Mode is not available\\"
                              "with slowdown or smooth fix.") :
#endif
                            _("Achieve all Silver Medals or above in Best Time\\"
                              "to unlock this Mode."));
        }
        else
        {
            if (server_policy_get_d(SERVER_POLICY_EDITION) == 0)
                playmodes_state(id, GUI_NONE, 0, 0,
                                mode_to_str(MODE_HARDCORE, 1), "",
                                _("Upgrade to Pro edition to play this Mode."));
            else if (server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_HARDCORE) == 0)
                playmodes_state(id, GUI_NONE, 0, 0,
                                mode_to_str(MODE_HARDCORE, 1), "",
                                _("Hardcore Mode is not available\\"
                                  "with Server Group Policy."));
        }
    }

    gui_layout(id, 0, 0);
    return id;
}

static int playmodes_enter(struct state *st, struct state *prev)
{
#if NB_HAVE_PB_BOTH==1
    audio_music_fade_to(0.5f, "bgm/inter_local.ogg");
#else
    audio_music_fade_to(0.0f, switchball_useable() ? "bgm/title-switchball.ogg" :
                                                     BGM_TITLE_CONF_LANGUAGE);
#endif

    if (&st_campaign == prev || &st_hardcore_start == prev)
        career_changed = 0;

    if (&st_hardcore_start == prev) game_kill_fade();

    return playmodes_gui();
}

static void playmodes_paint(int id, float t)
{
    game_client_draw(0, t);

    gui_paint(id);
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
    if (xbox_show_gui())
        xbox_control_list_gui_paint();
#endif
}

static void playmodes_timer(int id, float dt)
{
    gui_timer(id, dt);
}

static int playmodes_keybd(int c, int d)
{
    if (d && (c == KEY_EXIT
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
           && current_platform == PLATFORM_PC
#endif
        ))
        return playmodes_action(GUI_BACK, 0);

    return 1;
}

static int playmodes_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return playmodes_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return playmodes_action(GUI_BACK, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int hardcore_start_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok) {
    case GUI_BACK:
        progress_init(MODE_CAMPAIGN);
        game_fade_color(0.0f, 0.0f, 0.0f);
        game_fade(-6.0f);
        return goto_state(&st_playmodes);
    case PLAYMODES_HARDCORE:
        progress_init(MODE_HARDCORE);
        audio_play(AUD_STARTGAME, 1.0f);
        if (progress_play(campaign_get_level(0)))
        {
            campaign_load_camera_box_trigger("1");
            config_set_d(CONFIG_SMOOTH_FIX, val);
            config_save();
            campaign_hardcore_play(!val);
            hud_update(0, 0.0f);
            game_client_fly(1.0f);
            return goto_state(&st_play_ready);
        }
        break;
    }
    return 1;
}

static int hardcore_start_gui(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        gui_label(id, _("Good luck!"), GUI_MED, 0, 0);
        gui_space(id);

        if (campaign_career_unlocked())
            gui_multi(id, _("You can't save in this mode.\\ \\"
                            "Would you like to disable the\\"
                            "recordings during the game?"),
                          GUI_SML, gui_wht, gui_wht);
        else
            gui_multi(id, _("You can't save in this mode\\"
                            "except unlocking levels.\\ \\"
                            "Would you like to disable the\\"
                            "recordings during the game?"),
                          GUI_SML, gui_wht, gui_wht);

        gui_space(id);

        if ((jd = gui_harray(id)))
        {
#ifndef __EMSCRIPTEN__
            if (current_platform == PLATFORM_PC)
#endif
            {
                gui_start(jd, _("Cancel"), GUI_SML, GUI_BACK, 0);
                gui_state(jd, _("No"), GUI_SML, PLAYMODES_HARDCORE, 1);
                gui_state(jd, _("Yes"), GUI_SML, PLAYMODES_HARDCORE, 0);
            }
#ifndef __EMSCRIPTEN__
            else
            {
                gui_state(jd, _("No"), GUI_SML, PLAYMODES_HARDCORE, 1);
                gui_start(jd, _("Yes"), GUI_SML, PLAYMODES_HARDCORE, 0);
            }
#endif
        }

        gui_layout(id, 0, 0);
    }

    return id;
}

static void hardcore_start_timer(int id, float dt)
{
    gui_timer(id, dt);
    game_step_fade(dt);
}

static int hardcore_start_enter(struct state *st, struct state *prev)
{
    progress_init(MODE_HARDCORE);
    audio_music_fade_to(0.5f, "gui/bgm/inter.ogg");
    game_fade_color(0.25f, 0.0f, 0.0f);
    game_fade(+0.333f);
    return hardcore_start_gui();
}

static int hardcore_start_keybd(int c, int d)
{
    if (d && (c == KEY_EXIT
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
           && current_platform == PLATFORM_PC
#endif
        ))
        return hardcore_start_action(GUI_BACK, 0);
    return 1;
}

static int hardcore_start_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return hardcore_start_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return hardcore_start_action(GUI_BACK, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

struct state st_playmodes = {
    playmodes_enter,
    shared_leave,
    playmodes_paint,
    playmodes_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    playmodes_keybd,
    playmodes_buttn
};

struct state st_hardcore_start = {
    hardcore_start_enter,
    shared_leave,
    shared_paint,
    hardcore_start_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    hardcore_start_keybd,
    hardcore_start_buttn
};

#endif