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

#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

 /*
 * HACK: Used with console version
 */
#include "console_control_gui.h"

#if NB_HAVE_PB_BOTH==1
#include "networking.h"
#include "accessibility.h"
#include "campaign.h"
#include "account.h"
#endif

#include "hud.h"
#include "gui.h"
#include "transition.h"
#include "audio.h"
#include "config.h"
#include "progress.h"
#include "video.h"
#include "key.h"

#include "activity_services.h"

#include "game_server.h"
#include "game_client.h"
#include "game_common.h"

#if NB_HAVE_PB_BOTH==1
#include "st_playmodes.h"
#endif

#include "st_common.h"
#include "st_set.h"
#include "st_level.h"
#include "st_shared.h"
#include "st_play.h"

#if defined(__WII__)
/* We're using SDL 1.2 on Wii, which has SDLKey instead of SDL_Keycode. */
typedef SDLKey SDL_Keycode;
#endif

/*---------------------------------------------------------------------------*/

#if NB_HAVE_PB_BOTH!=1
static int switchball_useable(void)
{
    const SDL_Keycode k_auto = config_get_d(CONFIG_KEY_CAMERA_TOGGLE);
    const SDL_Keycode k_cam1 = config_get_d(CONFIG_KEY_CAMERA_1);
    const SDL_Keycode k_cam2 = config_get_d(CONFIG_KEY_CAMERA_2);
    const SDL_Keycode k_cam3 = config_get_d(CONFIG_KEY_CAMERA_3);
    const SDL_Keycode k_caml = config_get_d(CONFIG_KEY_CAMERA_L);
    const SDL_Keycode k_camr = config_get_d(CONFIG_KEY_CAMERA_R);

    SDL_Keycode k_arrowkey[4] = {
        config_get_d(CONFIG_KEY_FORWARD),
        config_get_d(CONFIG_KEY_LEFT),
        config_get_d(CONFIG_KEY_BACKWARD),
        config_get_d(CONFIG_KEY_RIGHT)
    };

    if (k_auto == SDLK_c && k_cam1 == SDLK_3 && k_cam2 == SDLK_1 && k_cam3 == SDLK_2 &&
        k_caml == SDLK_RIGHT && k_camr == SDLK_LEFT &&
        k_arrowkey[0] == SDLK_w && k_arrowkey[1] == SDLK_a && k_arrowkey[2] == SDLK_s && k_arrowkey[3] == SDLK_d)
        return 1;
    else if (k_auto == SDLK_c && k_cam1 == SDLK_3 && k_cam2 == SDLK_1 && k_cam3 == SDLK_2 &&
             k_caml == SDLK_d && k_camr == SDLK_a &&
             k_arrowkey[0] == SDLK_UP && k_arrowkey[1] == SDLK_LEFT && k_arrowkey[2] == SDLK_DOWN && k_arrowkey[3] == SDLK_RIGHT)
        return 1;

    /*
     * If the Switchball input preset is not detected,
     * Try it with the Neverball by default.
     */

    return 0;
}
#endif

/*---------------------------------------------------------------------------*/

struct state st_hardcore_start;

/*---------------------------------------------------------------------------*/

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN

static int playmodes_state(int id, int tok, int val, int unlocked,
                           const char *title,
                           const char *unlock_desc, const char *lock_desc)
{
    int jd;

    gui_space(id);

    if ((jd = gui_vstack(id)))
    {
        gui_multi(jd, title, GUI_SML,
                  unlocked ? gui_wht : gui_gry, unlocked ? gui_yel : gui_red);
        gui_multi(jd, unlocked ? unlock_desc : lock_desc,
                      GUI_SML, GUI_COLOR_WHT);

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
            return exit_state(&st_campaign);

        case PLAYMODES_CAREER_MODE:
            config_set_d(CONFIG_LOCK_GOALS,
                         !config_get_d(CONFIG_LOCK_GOALS));

            career_changed = 1;

            audio_play(AUD_SWITCH, 1.0f);

            if (config_get_d(CONFIG_LOCK_GOALS))
                audio_play(AUD_GOAL, 1.0f);

            config_save();

            return goto_state(&st_playmodes);

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
            if (current_platform == PLATFORM_PC && !console_gui_shown())
#endif
                gui_back_button(jd);
        }

        int         career_unlocked     = (server_policy_get_d(SERVER_POLICY_PLAYMODES_UNLOCKED_MODE_CAREER) ||
                                           campaign_career_unlocked());
        const char *career_title        = config_get_d(CONFIG_LOCK_GOALS) ?
                                          N_("Career Mode (Currently ENABLED!)") :
                                          N_("Career Mode (Currently disabled)");
        const char *career_title_locked = N_("Career Mode");
        const char *career_text         = N_("Toggle career mode in the entire game.\n"
                                             "Compatible with Level Set.");
        const char* career_text_locked  = server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_CAREER) ?
                                          N_("Complete the game to unlock.") :
                                          N_("Career mode is not available\n"
                                             "with server group policy.");

        playmodes_state(id, PLAYMODES_CAREER_MODE, 0,
                        career_unlocked &&
                        server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_CAREER),
                        career_unlocked &&
                        server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_CAREER) ?
                        _(career_title) : _(career_title_locked),
                        _(career_text),
                        _(career_text_locked));

        int hardc_unlocked = (server_policy_get_d(SERVER_POLICY_PLAYMODES_UNLOCKED_MODE_HARDCORE)
                           || campaign_hardcore_unlocked());
        int hardc_requirement = accessibility_get_d(ACCESSIBILITY_SLOWDOWN) >= 100 &&
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
               !config_cheat() &&
#endif
               (!config_get_d(CONFIG_SMOOTH_FIX) || video_perf() >= NB_FRAMERATE_MIN);

        if (!CHECK_ACCOUNT_ENABLED)
        {
            playmodes_state(id, GUI_NONE, 0, 0,
                            mode_to_str(MODE_HARDCORE, 1), "",
                            _("Hardcore Mode is not available.\n"
                              "Please check your account settings!"));
        }
        else if (CHECK_ACCOUNT_BANKRUPT)
        {
            playmodes_state(id, GUI_NONE, 0, 0,
                            mode_to_str(MODE_HARDCORE, 1), "",
                            _("Your player account is bankrupt.\n"
                              "Restore from the backup or delete the\n"
                              "local account and start over from scratch."));
        }
        else if (server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_HARDCORE))
        {
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
            const char *career_text_locked = !hardc_requirement ?
                                             N_("Hardcore Mode is not available\n"
                                                "with slowdown, cheat or smooth fix.") :
                                             N_("Achieve all Silver Medals or above in Best Time\n"
                                                "to unlock this Mode.");
#else
            const char *career_text_locked = hardc_requirement ?
                                             N_("Hardcore Mode is not available\n"
                                                "with slowdown or smooth fix.") :
                                             N_("Achieve all Silver Medals or above in Best Time\n"
                                                "to unlock this Mode.");
#endif

            playmodes_state(id, PLAYMODES_HARDCORE, 0, hardc_unlocked && hardc_requirement,
                            mode_to_str(MODE_HARDCORE, 1),
                            _("Play the entire game without dying once."),
                            career_text_locked);
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
                                _("Hardcore Mode is not available\n"
                                  "with Server Group Policy."));
        }

        gui_layout(id, 0, 0);
    }

    return id;
}

static int playmodes_enter(struct state *st, struct state *prev, int intent)
{
#if NB_HAVE_PB_BOTH==1
    audio_music_fade_to(0.5f, "bgm/inter_local.ogg", 1);
#else
    audio_music_fade_to(0.0f, switchball_useable() ? "bgm/title-switchball.ogg" :
                                                     BGM_TITLE_CONF_LANGUAGE, 1);
#endif

    if (&st_campaign == prev || &st_hardcore_start == prev)
        career_changed = 0;

    if (&st_hardcore_start == prev) game_kill_fade();

    if (prev == &st_playmodes)
        return playmodes_gui();

#if NB_HAVE_PB_BOTH==1
    /* HACK: These two transition directions will be merged! */

    if (prev == &st_campaign)
        return transition_slide_full(playmodes_gui(), 1, GUI_SE, GUI_SE);
#endif

    return transition_slide(playmodes_gui(), 1, intent);
}

static int playmodes_leave(struct state *st, struct state *next, int id, int intent)
{
    if (next == &st_null)
    {
        progress_exit();

        campaign_quit();
        set_quit();

        game_server_free(NULL);
        game_client_free(NULL);
    }

    if (next == &st_playmodes)
    {
        gui_delete(id);
        return 0;
    }

#if NB_HAVE_PB_BOTH==1
    /* HACK: These two transition directions will be merged! */

    if (next == &st_campaign)
        return transition_slide_full(id, 0, GUI_SE, GUI_SE);
#endif

    return transition_slide(id, 0, intent);
}

static void playmodes_paint(int id, float t)
{
    game_client_draw(0, t);

    gui_paint(id);
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    if (console_gui_shown())
        console_gui_list_paint();
#endif
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

static int   start_play_level_pending;
static float start_play_level_state_time;

static int hardcore_start_play(void)
{
    struct level *l = campaign_get_level(0);

    if (!l) return 0;

#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    EM_ASM({
        Neverball.gamecore_levelmap_load($0, UTF8ToString($1), false);
    }, l->num_indiv_theme, l->song);

    start_play_level_state_time = time_state() + 0.2;
#else
    start_play_level_state_time = time_state();
#endif

    start_play_level_pending = 1;

    audio_play(AUD_STARTGAME, 1.0f);
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    return 0;
#else
    return progress_play(l);
#endif
}

static int hardcore_start_play_timer(float dt)
{
    if (start_play_level_pending &&
        time_state() >= start_play_level_state_time)
    {
        start_play_level_pending = 0;

        if (progress_play(campaign_get_level(0)))
        {
            activity_services_mode_update(AS_MODE_HARDCORE);

            return goto_play_level();
        }
    }

    return 0;
}

static int hardcore_start_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
            game_fade_color(0.0f, 0.0f, 0.0f);
            game_fade(-6.0f);
            return exit_state(&st_playmodes);

        case PLAYMODES_HARDCORE:
            config_set_d(CONFIG_SMOOTH_FIX, val);
            config_save();

            progress_exit();
            progress_init(MODE_HARDCORE);
            campaign_hardcore_play(!val);

            campaign_load_camera_box_trigger("1");

            if (hardcore_start_play())
            {
                activity_services_mode_update(AS_MODE_HARDCORE);

                hud_update(0, 0.0f);
                game_client_fly(1.0f);
                return goto_play_level();
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
        gui_label(id, _("Good luck!"), GUI_MED, GUI_COLOR_DEFAULT);
        gui_space(id);

        if (campaign_career_unlocked())
            gui_multi(id, _("You can't save in this mode.\n\n"
                            "Would you like to disable the\n"
                            "recordings during the game?"),
                          GUI_SML, GUI_COLOR_WHT);
        else
            gui_multi(id, _("You can't save in this mode\n"
                            "except unlocking levels.\n\n"
                            "Would you like to disable the\n"
                            "recordings during the game?"),
                          GUI_SML, GUI_COLOR_WHT);

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
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    hardcore_start_play_timer(dt);
#endif

    gui_timer(id, dt);
    game_step_fade(dt);
}

static int hardcore_start_enter(struct state *st, struct state *prev, int intent)
{
    audio_music_fade_to(0.5f, "gui/bgm/inter.ogg", 1);
    game_fade_color(0.25f, 0.0f, 0.0f);
    game_fade(+0.333f);

    return transition_slide(hardcore_start_gui(), 1, intent);
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
    playmodes_leave,
    playmodes_paint,
    shared_timer,
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
