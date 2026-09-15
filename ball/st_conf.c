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

/*
 * HACK: Used with console version
 */
#include "console_control_gui.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#if NB_HAVE_PB_BOTH==1
#include "account.h"
#include "account_wgcl.h"
#include "st_intro_covid.h"
#include "st_beam_style.h"
#include "networking.h"
#if _WIN32 && _MSC_VER
#include "mapmarkers.h"
#endif
#endif

#include "log.h"
#include "demo.h"
#include "demo_dir.h"
#include "gui.h"
#include "transition.h"
#include "hud.h"
#include "geom.h"
#include "ball.h"
#include "part.h"
#include "audio.h"
#include "config.h"
#include "text.h"
#include "video.h"
#if ENABLE_DUALDISPLAY==1
#include "video_dualdisplay.h"
#endif
#include "common.h"
#include "progress.h"
#ifndef VERSION
#include "version.h"
#endif
#include "lang.h"

#include "package_superwaifu.h"

#if ENABLE_DUALDISPLAY==1
#include "game_dualdisplay.h"
#endif
#include "game_common.h"
#include "game_client.h"
#include "game_server.h"
#include "game_switchball.h"

#if NB_HAVE_PB_BOTH==1
#include "game_transitions.h"
#endif

#ifndef ENABLE_GAME_TRANSFER
#include "st_transfer.h"
#endif

#include "st_conf.h"
#include "st_title.h"
#include "st_common.h"
#include "st_name.h"
#include "st_ball.h"
#include "st_shared.h"
#include "st_shop.h"
#if ENABLE_FETCH!=0
#include "st_package.h"
#endif

#if NB_HAVE_PB_BOTH==1
#include "st_wgcl.h"
#endif

#if NB_HAVE_PB_BOTH!=1 && \
    (defined(ENABLE_GAME_TRANSFER) || defined(GAME_TRANSFER_TARGET))
#error Security compilation error: Preprocessor definitions can be used it, \
       once you have transferred or joined into the target Discord Server, \
       and verified and promoted as Developer Role. \
       This invite link can be found under https://discord.gg/qnJR263Hm2/.
#endif

extern const char TITLE[];
extern const char ICON[];

#if defined(__WII__)
/* We're using SDL 1.2 on Wii, which has SDLKey instead of SDL_Keycode. */
typedef SDLKey SDL_Keycode;
#endif

/*---------------------------------------------------------------------------*/

struct state st_conf_social;
struct state st_conf_gameplay;
struct state st_conf_notification;
struct state st_conf_controls;
struct state st_conf_touch;
struct state st_conf_keybd;
struct state st_conf_controllers;
struct state st_conf_calibrate;
struct state st_conf_audio;

/*---------------------------------------------------------------------------*/

static struct state *conf_back;

static int ingame_demo = 0;
static int mainmenu_conf = 1;

int goto_conf(struct state *back_state, int using_game, int demo)
{
    conf_back = back_state;

    ingame_demo = demo;
    mainmenu_conf = !using_game;

    return goto_state(&st_conf);
}

static int conf_check_playername(const char *regname)
{
    for (int i = 0; i < text_length(regname); i++)
        if (regname[i] == '\\' || regname[i] == '/' || regname[i] == ':'  ||
            regname[i] == '*'  || regname[i] == '?' || regname[i] == '"'  ||
            regname[i] == '<'  || regname[i] == '>' || regname[i] == '|')
        {
            log_errorf("Can't accept other charsets!: %c\n", regname[i]);
            return 0;
        }

    return text_length(config_get_s(CONFIG_PLAYER)) >= 3;
}

/*---------------------------------------------------------------------------*/

static int conf_join_confirm = 0;

static struct state *st_conf_social_back;

/*
 * This variable name will be redirected to st_conf_social_back for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `st_conf_social_back` to `social_back`.
 */
#define social_back st_conf_social_back

static int conf_goto_social(struct state *back)
{
    st_conf_social_back = back;

    return goto_state(&st_conf_social);
}

enum
{
    CONF_SOCIAL_DISCORD = GUI_LAST,
};

static int conf_social_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    int r = 1;

#ifndef __EMSCRIPTEN__
    char linkstr_cmd[MAXSTR], linkstr_code[64];
#endif

    switch (tok)
    {
        case GUI_BACK:
            conf_join_confirm = 0;
            r = exit_state(social_back);
            social_back = NULL;
            return r;

        case CONF_SOCIAL_DISCORD:
#if NB_HAVE_PB_BOTH==1
            if (conf_join_confirm)
#endif
            {
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
#ifdef __EMSCRIPTEN__
#if NB_HAVE_PB_BOTH==1
                EM_ASM({ window.open("https://discord.gg/qnJR263Hm2"); });
#else
                EM_ASM({ window.open("https://discord.gg/HhMfr4N6H6"); });
#endif
#else
#if NB_HAVE_PB_BOTH==1
                SAFECPY(linkstr_code, "qnJR263Hm2");
#else
                SAFECPY(linkstr_code, "HhMfr4N6H6");
#endif

#if _WIN32
                SAFECPY(linkstr_cmd, "explorer https://discord.gg/");
#elif defined(__APPLE__)
                SAFECPY(linkstr_cmd, "open https://discord.gg/");
#elif defined(__linux__)
                SAFECPY(linkstr_cmd, "x-www-browser https://discord.gg/");
#endif

                SAFECAT(linkstr_cmd, linkstr_code);

                system(linkstr_cmd);

                /* bye! */

                return !mainmenu_conf ? exit_state(&st_conf) : 0;
#endif
#endif
            }
#if NB_HAVE_PB_BOTH==1
            else
            {
                conf_join_confirm = 1;
                return goto_state(curr_state());
            }
#endif
            break;
    }
    return r;
}

/*
 * This function name will be redirected to conf_social_action() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_social_action()` to `social_action()`.
 */
#define social_action conf_social_action

static int conf_social_gui(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("Join Discord?"), GUI_MED, GUI_COLOR_DEFAULT);

        gui_space(id);

        if ((jd = gui_vstack(id)))
        {
            if (!conf_join_confirm)
            {
                gui_label(jd, _("Inviting Discord server will allow to:"),
                              GUI_SML, gui_wht, gui_cya);

#if NB_HAVE_PB_BOTH==1
                gui_multi(jd, _("- Access the game edition to you\n"
                                "- Keep collected coins into your account\n"
                                "- Use powerups in Challenge mode\n"
                                "- Adds your checkpoints from map compiler\n"
                                "- Share any levels from MAPC for PB\n"),
                              GUI_SML, GUI_COLOR_WHT);
#else
                gui_multi(jd, _("- Access all Discord public channels\n"
                                "- Share any levels from MAPC for NB\n"),
                              GUI_SML, GUI_COLOR_WHT);
#endif
            }
            else
                gui_multi(jd, _("Please make sure that you've verified the\n"
                                "new members after joined, before send\n"
                                "community messages, connect voice chats\n"
                                "and watch streaming."),
                              GUI_SML, GUI_COLOR_WHT);

#if NB_HAVE_PB_BOTH==1
            if (mainmenu_conf && conf_join_confirm)
#endif
            {
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
                gui_label(jd, _("This may take a while, and the game will then exit."),
                              GUI_SML, gui_twi, gui_vio);
#else
                gui_label(jd, _("Join on Desktop: https://discord.gg/qnJR263Hm2"),
                              GUI_SML, gui_twi, gui_vio);
#endif
            }


            gui_set_rect(jd, GUI_ALL);
        }

        gui_space(id);

        if ((jd = gui_harray(id)))
        {
            gui_state(jd, _("Join"), GUI_SML, CONF_SOCIAL_DISCORD, 0);
            gui_state(jd, _("Cancel"), GUI_SML, GUI_BACK, 0);
        }
    }
    gui_layout(id, 0, 0);

    return id;
}

/*
 * This function name will be redirected to conf_social_gui() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_social_gui()` to `social_gui()`.
 */
#define social_gui conf_social_gui

static int conf_social_enter(struct state *st, struct state *prev, int intent)
{
    if (mainmenu_conf && !game_server_state() && !demo_state())
        game_client_free(NULL);

    conf_common_init(social_action, mainmenu_conf);
    return transition_slide(social_gui(), 1, intent);
}

/*
 * This function name will be redirected to conf_social_enter() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_social_enter()` to `social_enter()`.
 */
#define social_enter conf_social_enter

/*---------------------------------------------------------------------------*/

static int conf_covid_extended = 0;

#define CONF_ACCOUNT_DEMO_LOCKED_DESC_INGAME \
    _("You can't change save filters\n" \
      "during the game.")

#define CONF_ACCOUNT_DEMO_LOCKED_DESC_INTRODUCTIVE \
    _("Filters restricts some replays.\n" \
      "Locked level status: %s")

#define CONF_ACCOUNT_DEMO_LOCKED_DESC_HARDLOCK \
    _("Replays have locked down until\n" \
      "the next future update.")

#define CONF_ACCOUNT_DEMO_LOCKED_DESC_HIGHRISK \
    _("Replays have locked down\n" \
      "during high risks!")

#define CONF_ACCOUNT_DEMO_LOCKED_DESC_NIGHT \
    _("Replays have locked down between\n" \
      "16:00 - 8:00 (4:00 PM - 8:00 AM).")

#define CONF_ACCOUNT_DEMO_LOCKED_DESC_EXTREME_CASES \
    _("Replays have locked down\n" \
      "for extreme cases.")

#define CONF_ACCOUNT_DEMO_FILTER_CURR_OPTTION_1 \
    _("Only Finish")

#define CONF_ACCOUNT_DEMO_FILTER_CURR_OPTTION_2 \
    _("Keep on board")

#define CONF_ACCOUNT_DEMO_FILTER_CURR_OPTTION_3 \
    _("Always active")

static int save_id, load_id;
int online_mode;

enum
{
    CONF_ACCOUNT_SIGNIN = GUI_LAST,
    CONF_ACCOUNT_SIGNOUT,
    CONF_ACCOUNT_COVID_EXTEND,
    CONF_ACCOUNT_AUTOUPDATE,
    CONF_ACCOUNT_MAYHEM,
    CONF_ACCOUNT_PLAYER,
    CONF_ACCOUNT_PACKAGES,
#if NB_HAVE_PB_BOTH==1
    CONF_ACCOUNT_BALL,
    CONF_ACCOUNT_BEAM,
#endif
    CONF_ACCOUNT_SAVE,
    CONF_ACCOUNT_LOAD
};

static void account_refresh_packages_done(void *data1, void *data2)
{
    struct fetch_done *dn = data2;

    if (dn->success)
    {
#if NB_HAVE_PB_BOTH == 1
        goto_wgcl_addons_login(0, &st_conf_account, 0);
#else
        goto_package(0, &st_conf_account);
#endif
    }
    else audio_play("snd/uierror.ogg", 1.0f);
}

static unsigned int account_refresh_packages(void)
{
    package_change_category(PACKAGE_CATEGORY_ALL);

    struct fetch_callback callback = { 0 };

    callback.data = NULL;
    callback.done = account_refresh_packages_done;

    return package_refresh(callback);
}

static int conf_account_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
            return exit_state(&st_conf);

        case CONF_ACCOUNT_COVID_EXTEND:
            conf_covid_extended = 1;
            goto_state(&st_conf_account);
            break;

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__EMSCRIPTEN__)
        case CONF_ACCOUNT_SIGNIN:
            return goto_wgcl_login(&st_conf_account, 0,
                                   &st_conf_account, 0);
            break;

        case CONF_ACCOUNT_SIGNOUT:
            return goto_wgcl_logout(&st_conf_account);
            break;
#endif

#if ENABLE_FETCH==1
        case CONF_ACCOUNT_AUTOUPDATE:
            break;

        case CONF_ACCOUNT_PACKAGES:
            account_refresh_packages();
            break;
#endif

        case CONF_ACCOUNT_PLAYER:
#ifdef CONFIG_INCLUDES_ACCOUNT
            goto_shop_rename(&st_conf_account, &st_conf_account, 1);
#else
            goto_name(&st_conf_account, &st_conf_account, 0, 0, 1);
#endif
            break;

#if NB_HAVE_PB_BOTH==1
        case CONF_ACCOUNT_BALL:
            if ((fs_exists("gui/ball.sol") ||
                 fs_exists("gui/ball.solx")) &&
                fs_exists("gui/ball.nbr"))
            {
                game_fade(+6.0f);
                goto_state(&st_ball);
            }
            break;

        case CONF_ACCOUNT_BEAM:
            if ((fs_exists("gui/beam-style.sol") ||
                 fs_exists("gui/beam-style.solx")))
            {
                game_fade(+6.0f);
                goto_state(&st_beam_style);
            }
            break;
#endif

        case CONF_ACCOUNT_SAVE:
#ifndef DEMO_QUARANTINED_MODE
            if (config_get_d(CONFIG_ACCOUNT_SAVE) == 3)
            {
                gui_set_label(save_id, _("Off"));
                config_set_d(CONFIG_ACCOUNT_SAVE, 0);
            }
            else if (config_get_d(CONFIG_ACCOUNT_SAVE) == 2)
            {
                gui_set_label(save_id, CONF_ACCOUNT_DEMO_FILTER_CURR_OPTTION_3);
                config_set_d(CONFIG_ACCOUNT_SAVE, 3);
            }
            else if (config_get_d(CONFIG_ACCOUNT_SAVE) == 1)
            {
                gui_set_label(save_id, CONF_ACCOUNT_DEMO_FILTER_CURR_OPTTION_2);
                config_set_d(CONFIG_ACCOUNT_SAVE, 2);
            }
            else if (config_get_d(CONFIG_ACCOUNT_SAVE) == 0)
            {
                gui_set_label(save_id, CONF_ACCOUNT_DEMO_FILTER_CURR_OPTTION_1);
                config_set_d(CONFIG_ACCOUNT_SAVE, 1);
            }
#else
            if (config_get_d(CONFIG_ACCOUNT_SAVE) == 3)
            {
                gui_set_label(save_id, _("Off"));
                config_set_d(CONFIG_ACCOUNT_SAVE, 0);
            }
            else if (config_get_d(CONFIG_ACCOUNT_SAVE) == 2)
            {
                if (conf_covid_extended || config_cheat())
                {
                    gui_set_label(save_id, CONF_ACCOUNT_DEMO_FILTER_CURR_OPTTION_3);
                    config_set_d(CONFIG_ACCOUNT_SAVE, 3);
                }
                else
                {
                    gui_set_label(save_id, _("Off"));
                    config_set_d(CONFIG_ACCOUNT_SAVE, 0);
                }
            }
            else if (config_get_d(CONFIG_ACCOUNT_SAVE) == 1)
            {
                gui_set_label(save_id, CONF_ACCOUNT_DEMO_FILTER_CURR_OPTTION_2);
                config_set_d(CONFIG_ACCOUNT_SAVE, 2);
            }
            else if (config_get_d(CONFIG_ACCOUNT_SAVE) == 0)
            {
                gui_set_label(save_id, CONF_ACCOUNT_DEMO_FILTER_CURR_OPTTION_1);
                config_set_d(CONFIG_ACCOUNT_SAVE, 1);
            }
#endif
            audio_play(config_get_d(CONFIG_ACCOUNT_SAVE) != 0 ? "snd/2.2/game_button_down.ogg" : "snd/2.2/game_button_up.ogg", 1.0f);
            config_save();
            break;

        case CONF_ACCOUNT_LOAD:
#ifndef DEMO_QUARANTINED_MODE
            if (config_get_d(CONFIG_ACCOUNT_LOAD) == 3)
            {
                gui_set_label(load_id, CONF_ACCOUNT_DEMO_FILTER_CURR_OPTTION_1);
                config_set_d(CONFIG_ACCOUNT_LOAD, 1);
            }
            else if (config_get_d(CONFIG_ACCOUNT_LOAD) == 1)
            {
                gui_set_label(load_id, CONF_ACCOUNT_DEMO_FILTER_CURR_OPTTION_2);
                config_set_d(CONFIG_ACCOUNT_LOAD, 2);
            }
            else if (config_get_d(CONFIG_ACCOUNT_LOAD) == 2)
            {
                gui_set_label(load_id, CONF_ACCOUNT_DEMO_FILTER_CURR_OPTTION_3);
                config_set_d(CONFIG_ACCOUNT_LOAD, 3);
            }
#else
            if (config_get_d(CONFIG_ACCOUNT_LOAD) == 3)
            {
                gui_set_label(load_id, CONF_ACCOUNT_DEMO_FILTER_CURR_OPTTION_1);
                config_set_d(CONFIG_ACCOUNT_LOAD, 1);
            }
            else if (config_get_d(CONFIG_ACCOUNT_LOAD) == 1)
            {
                gui_set_label(load_id, CONF_ACCOUNT_DEMO_FILTER_CURR_OPTTION_2);
                config_set_d(CONFIG_ACCOUNT_LOAD, 2);
            }
            else if (config_get_d(CONFIG_ACCOUNT_LOAD) == 2)
            {
                if (conf_covid_extended || config_cheat())
                {
                    gui_set_label(load_id, CONF_ACCOUNT_DEMO_FILTER_CURR_OPTTION_3);
                    config_set_d(CONFIG_ACCOUNT_LOAD, 3);
                }
                else if (config_get_d(CONFIG_ACCOUNT_SAVE) <= 1)
                {
                    gui_set_label(load_id, CONF_ACCOUNT_DEMO_FILTER_CURR_OPTTION_1);
                    config_set_d(CONFIG_ACCOUNT_LOAD, 1);
                }
            }
#endif
            audio_play(config_get_d(CONFIG_ACCOUNT_LOAD) != 1 ? "snd/2.2/game_button_down.ogg" : "snd/2.2/game_button_up.ogg", 1.0f);
            config_save();
            break;
    }

    return 1;
}

/*
 * This function name will be redirected to conf_account_action() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_account_action()` to `account_action()`.
 */
#define account_action conf_account_action

static int time_remain_lbl_id;

static int conf_account_gui(void)
{
    int id;

    save_id     = 0;
    load_id     = 0;
    online_mode = 0;

    /* Initialize the configuration GUI. */

    if ((id = gui_vstack(0)))
    {
#ifndef __EMSCRIPTEN__
        int name_id = 0;
        const char *player = config_get_s(CONFIG_PLAYER);
#endif
#if defined(CONFIG_INCLUDES_ACCOUNT) && defined(CONFIG_INCLUDES_MULTIBALLS)
        const char *ball;
        switch (ball_multi_curr()) {
            case 0:  ball = account_get_s(ACCOUNT_BALL_FILE_LL); break;
            case 1:  ball = account_get_s(ACCOUNT_BALL_FILE_L);  break;
            case 2:  ball = account_get_s(ACCOUNT_BALL_FILE_C);  break;
            case 3:  ball = account_get_s(ACCOUNT_BALL_FILE_R);  break;
            case 4:  ball = account_get_s(ACCOUNT_BALL_FILE_RR); break;
            default: ball = account_get_s(ACCOUNT_BALL_FILE_C);
        }

        account_set_s(ACCOUNT_BALL_FILE, ball);
#elif defined(CONFIG_INCLUDES_ACCOUNT)
        const char *ball   = account_get_s(ACCOUNT_BALL_FILE);
#else
        const char *ball   = config_get_s(CONFIG_BALL_FILE);
#endif

        int ball_id = 0, beam_id = 0;

        int save = config_get_d(CONFIG_ACCOUNT_SAVE),
            load = config_get_d(CONFIG_ACCOUNT_LOAD);

        conf_header(id, _("Account"), GUI_BACK);

        if (!ingame_demo && !mainmenu_conf)
        {
            gui_multi(id, CONF_ACCOUNT_DEMO_LOCKED_DESC_INGAME,
                          GUI_SML, GUI_COLOR_RED);
            gui_space(id);
        }
        else
        {
#ifdef COVID_HIGH_RISK
            time_remain_lbl_id = gui_multi(id, CONF_ACCOUNT_DEMO_LOCKED_HIGHRISK_DESC,
                                               GUI_SML, GUI_COLOR_RED);
            gui_space(id);
#else
#ifdef DEMO_QUARANTINED_MODE
#ifdef DEMO_LOCKDOWN_COMPLETE
            time_remain_lbl_id = gui_multi(id, CONF_ACCOUNT_DEMO_LOCKED_DESC_HARDLOCK,
                                               GUI_SML, GUI_COLOR_RED);
            gui_space(id);
#else
            /* Lockdown duration time. DO NOT EDIT!*/
            int nolockdown; DEMO_LOCKDOWN_RANGE_NIGHT(
                nolockdown,
                DEMO_LOCKDOWN_RANGE_NIGHT_START_HOUR_DEFAULT,
                DEMO_LOCKDOWN_RANGE_NIGHT_END_HOUR_DEFAULT
            );

#if NB_HAVE_PB_BOTH==1
#if (_WIN32 && _MSC_VER)
            if (config_cheat() && CHECK_ACCOUNT_ENABLED) {
                gui_multi(id, "WGCL Operator enabled!\n"
                              "Some features will temporary accessible.",
                              GUI_SML, GUI_COLOR_RED);
            } else
#endif
            if (nolockdown && CHECK_ACCOUNT_ENABLED)
            {
                char filter_introductive_attr[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(filter_introductive_attr, MAXSTR,
#else
                sprintf(filter_introductive_attr,
#endif
                        CONF_ACCOUNT_DEMO_LOCKED_DESC_INTRODUCTIVE,
                        _(status_to_str(3)));

                time_remain_lbl_id = gui_multi(id, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n"
                                                   "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
                                                   GUI_SML, GUI_COLOR_RED);

                if (conf_covid_extended == 0)
                {
                    gui_space(id);
                    gui_state(id, _("Request Lift"),
                                  GUI_SML, CONF_ACCOUNT_COVID_EXTEND, 0);
                }

                gui_set_multi(time_remain_lbl_id, filter_introductive_attr);
            }
            else if (CHECK_ACCOUNT_ENABLED)
                gui_multi(id, CONF_ACCOUNT_DEMO_LOCKED_DESC_NIGHT,
                              GUI_SML, GUI_COLOR_RED);
            else
                gui_multi(id, CONF_ACCOUNT_DEMO_LOCKED_DESC_EXTREME_CASES,
                              GUI_SML, GUI_COLOR_RED);
#endif

            gui_space(id);
#endif
#endif
#endif
        }

        if (mainmenu_conf && !game_server_state() && !demo_state())
        {
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__EMSCRIPTEN__)
#if _WIN32 && _MSC_VER
            if (account_wgcl_name_read_only())
                gui_state(id, _("Sign out from Pennyball + Neverball WGCL"),
                              GUI_SML, CONF_ACCOUNT_SIGNOUT, 0);
            else
                gui_state(id, _("Sign in to Pennyball + Neverball WGCL"),
                              GUI_SML, CONF_ACCOUNT_SIGNIN, 0);

            gui_space(id);
#endif
#endif
        }
        
#ifndef __EMSCRIPTEN__
#if NB_HAVE_PB_BOTH==1
        if (server_policy_get_d(SERVER_POLICY_EDITION) != 0)
#endif
            if ((name_id = conf_state(id, _("Player Name"), "XXXXXXXXXXXXXX",
                                          CONF_ACCOUNT_PLAYER)))
            {
                gui_set_trunc(name_id, TRUNC_TAIL);
                gui_set_label(name_id, player);

#if NB_HAVE_PB_BOTH==1
#if NB_EOS_SDK==0 || NB_STEAM_API==0
                if (game_server_state() || demo_state() || account_wgcl_name_read_only() ||
#ifndef __EMSCRIPTEN__
                    config_playername_locked() ||
#endif
                    online_mode || !account_changeable)
#else
                if (game_server_state() || demo_state())
#endif
                {
                    /*
                     * If the account is signed in e.g. PB+NB WGCL or Steam,
                     * you cannot change the player name.
                     */

#ifndef __EMSCRIPTEN__
                    if (server_policy_get_d(SERVER_POLICY_EDITION) != 0)
                    {
                        gui_set_state(name_id, GUI_NONE, 0);
                        gui_set_color(name_id, GUI_COLOR_GRY);
                    }
#endif
                }
#endif
            }

#endif
        if (!demo_state()) {
#if NB_HAVE_PB_BOTH==1
            if ((ball_id = conf_state(id, _("Ball Model"), "XXXXXXXXXXXXXX",
                                          CONF_ACCOUNT_BALL)))
            {
                gui_set_trunc(ball_id, TRUNC_TAIL);
#if defined(CONFIG_INCLUDES_ACCOUNT) && defined(CONFIG_INCLUDES_MULTIBALLS)
                gui_set_label(ball_id, _("Manage"));
#else
                gui_set_label(ball_id, base_name(ball));
#endif
            }
#endif
        }

        if (mainmenu_conf && !game_server_state() && !demo_state())
        {
#if NB_HAVE_PB_BOTH==1 && ENABLE_FETCH==1 && \
    !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__)
            if (CHECK_ACCOUNT_ENABLED && config_get_d(CONFIG_ONLINE))
            {
                conf_state(id, _("Addons"), _("Manage"), CONF_ACCOUNT_PACKAGES);
                gui_space(id);
            }
#endif
        }

#if NB_HAVE_PB_BOTH==1
        if (mainmenu_conf && !game_server_state() && !demo_state() &&
            (server_policy_get_d(SERVER_POLICY_EDITION) != 0 ||
             account_wgcl_name_read_only())) {
#ifdef CONFIG_INCLUDES_ACCOUNT
            if ((beam_id = conf_state(id, _("Beam Style"), "XXXXXXXXXXXXXX",
                                          CONF_ACCOUNT_BEAM)))
            {
                gui_set_trunc(beam_id, TRUNC_TAIL);

                const char* beam_version_name = "";

                switch (config_get_d(CONFIG_ACCOUNT_BEAM_STYLE))
                {
                case 0:
                    beam_version_name = "Remastered (1.7)";
                    break;
                case 1:
                    beam_version_name = "Standard (1.6.0)";
                    break;
                case 2:
                    beam_version_name = "Standard (1.5.4)";
                    break;
                case 3:
                    beam_version_name = "Standard (1.5.3)";
                    break;
                }

                gui_set_label(beam_id, beam_version_name);
                gui_space(id);
            }
#endif
        }
#endif

        /* Those filters will use some replays */
        const char *savefilter = _("None");
        switch (save)
        {
            /* All save filters: Goal, Aborted, Time-out, Fall-out */
            case 3: savefilter = CONF_ACCOUNT_DEMO_FILTER_CURR_OPTTION_3; break;
            /* Keep filters: Goal, Aborted, Time-out */
            case 2: savefilter = CONF_ACCOUNT_DEMO_FILTER_CURR_OPTTION_2; break;
            /* Only Goal filters: Goal */
            case 1: savefilter = CONF_ACCOUNT_DEMO_FILTER_CURR_OPTTION_1; break;
            /* Disabled */
            case 0: savefilter = _("Off"); break;
        }

        const char *loadfilter = _("None");
        switch (load)
        {
            /* All save filters: Goal, Aborted, Time-out, Fall-out */
            case 3: loadfilter = CONF_ACCOUNT_DEMO_FILTER_CURR_OPTTION_3; break;
            /* Keep filters: Goal, Aborted, Time-out */
            case 2: loadfilter = CONF_ACCOUNT_DEMO_FILTER_CURR_OPTTION_2; break;
            /* Only Goal filters: Goal */
            case 1: loadfilter = CONF_ACCOUNT_DEMO_FILTER_CURR_OPTTION_1; break;
        }

        save_id = conf_state(id, _("Save Replay"), "XXXXXXXXXXXXXX",
                             (!ingame_demo && !mainmenu_conf) || game_server_state() ?
                             GUI_NONE : CONF_ACCOUNT_SAVE);
        load_id = conf_state(id, _("Replay Filter"), "XXXXXXXXXXXXXX",
                             CONF_ACCOUNT_LOAD);

        if ((!ingame_demo && !mainmenu_conf) || game_server_state())
            gui_set_color(save_id, GUI_COLOR_GRY);

        gui_set_trunc(save_id, TRUNC_TAIL);
        gui_set_trunc(load_id, TRUNC_TAIL);

        gui_set_label(save_id, savefilter);
        gui_set_label(load_id, loadfilter);

        gui_layout(id, 0, 0);
    }

    return id;
}

/*
 * This function name will be redirected to conf_account_gui() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_account_gui()` to `account_gui()`.
 */
#define account_gui conf_account_gui

static int conf_account_enter(struct state *st, struct state *prev, int intent)
{
    if (prev == &st_ball) game_fade(-6.0f);

    if (mainmenu_conf && !game_server_state() && !demo_state())
        game_client_free(NULL);

    conf_common_init(account_action, mainmenu_conf);
    return transition_slide(account_gui(), 1, intent);
}

/*
 * This function name will be redirected to conf_account_enter() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_account_enter()` to `account_enter()`.
 */
#define account_enter conf_account_enter

static void conf_account_timer(int id, float dt)
{
    game_step_fade(dt);
    gui_timer(id, dt);

    int sec;
    int nolockdown;

    DEMO_LOCKDOWN_RANGE_NIGHT_TIMELEFT(nolockdown,
                                       DEMO_LOCKDOWN_RANGE_NIGHT_START_HOUR_DEFAULT,
                                       DEMO_LOCKDOWN_RANGE_NIGHT_END_HOUR_DEFAULT,
                                       sec);

    if (conf_covid_extended != 0 && nolockdown)
    {
        sec = MAX(0, sec);

        static char cv19_infoattr[MAXSTR];

        int clock_hour = (int) MAX(0, (sec / 3600) % 24);
        int clock_min  = (int) MAX(0, (sec / 60) % 60);
        int clock_sec  = (int) MAX(0, (sec) % 60);

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(cv19_infoattr, MAXSTR,
#else
        sprintf(cv19_infoattr,
#endif
                _("Full access valid until locked down.\n"
                  "Time Remaining: %i h %i m %i s"),
                clock_hour, clock_min, clock_sec);

        if (time_remain_lbl_id != 0)
            gui_set_multi(time_remain_lbl_id, cv19_infoattr);
    }
    else if (conf_covid_extended != 0 && !nolockdown)
    {
        if (config_get_d(CONFIG_ACCOUNT_SAVE) > 2 && save_id)
        {
            config_set_d(CONFIG_ACCOUNT_SAVE, 2);
            gui_set_label(save_id, CONF_ACCOUNT_DEMO_FILTER_CURR_OPTTION_2);
        }

        if (config_get_d(CONFIG_ACCOUNT_LOAD) > 2 && load_id)
        {
            config_set_d(CONFIG_ACCOUNT_LOAD, 2);
            gui_set_label(load_id, CONF_ACCOUNT_DEMO_FILTER_CURR_OPTTION_2);
        }

        config_save();

        if (time_remain_lbl_id != 0)
            gui_set_multi(time_remain_lbl_id,
                          CONF_ACCOUNT_DEMO_LOCKED_DESC_NIGHT);
    }
}

/*
 * This function name will be redirected to conf_account_timer() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_account_timer()` to `account_timer()`.
 */
#define account_timer conf_account_timer

/*---------------------------------------------------------------------------*/

enum
{
    CONF_GAMEPLAY_AUTORETRY = GUI_LAST,
    CONF_GAMEPLAY_FASTERRESET,
    CONF_GAMEPLAY_TUTORIAL,
    CONF_GAMEPLAY_HINT,
    CONF_GAMEPLAY_SWITCHBALL_DROPSPEEDING,

    /* Increased gameplay settings logic! */

    CONF_GAMEPLAY_CAMERA_DEFAULT,
    CONF_GAMEPLAY_CAMERA_1_4,
    CONF_GAMEPLAY_CAMERA_1_5,
    CONF_GAMEPLAY_LOCK_GOALS
};

static int conf_gameplay_settings_entered = 0;
static int cam_preset_expected = CONF_GAMEPLAY_CAMERA_DEFAULT;

static int conf_gameplay_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
            conf_gameplay_settings_entered = 0;
            return exit_state(&st_conf);

        case CONF_GAMEPLAY_AUTORETRY:
            audio_play(val != 0 ? "snd/2.2/game_button_down.ogg" : "snd/2.2/game_button_up.ogg", 1.0f);
            config_set_d(CONFIG_ADVANCEDGAMING_GAMEPLAY_AUTORETRY, val);
            goto_state(curr_state());
            config_save();
            break;

        case CONF_GAMEPLAY_FASTERRESET:
            audio_play(val != 0 ? "snd/2.2/game_button_down.ogg" : "snd/2.2/game_button_up.ogg", 1.0f);
            config_set_d(CONFIG_ADVANCEDGAMING_GAMEPLAY_FASTERRESET, val);
            goto_state(curr_state());
            config_save();
            break;

        case CONF_GAMEPLAY_TUTORIAL:
            audio_play(val != 0 ? "snd/2.2/game_button_down.ogg" : "snd/2.2/game_button_up.ogg", 1.0f);
            config_set_d(CONFIG_ACCOUNT_TUTORIAL, val);
            config_save();
            goto_state(curr_state());
            break;

        case CONF_GAMEPLAY_HINT:
            audio_play(val != 0 ? "snd/2.2/game_button_down.ogg" : "snd/2.2/game_button_up.ogg", 1.0f);
            config_set_d(CONFIG_ACCOUNT_HINT, val);
            config_save();
            goto_state(curr_state());
            break;

        case CONF_GAMEPLAY_SWITCHBALL_DROPSPEEDING:
            audio_play(val != 0 ? "snd/2.2/game_button_down.ogg" : "snd/2.2/game_button_up.ogg", 1.0f);
            config_set_d(CONFIG_ADVANCEDGAMING_GAMEPLAY_SWITCHBALL_DROPSPEEDING, val);
            config_save();
            goto_state(curr_state());
            break;

        case CONF_GAMEPLAY_CAMERA_DEFAULT:
            cam_preset_expected = CAM_PRESET_DEFAULT;
            cam_preset_set(CAM_1, CAM_PRESET_DEFAULT);
            config_save();
            goto_state(&st_conf_gameplay);
            break;

        case CONF_GAMEPLAY_CAMERA_1_4:
            cam_preset_expected = CAM_PRESET_1_4;
            cam_preset_set(CAM_1, CAM_PRESET_1_4);
            config_save();
            goto_state(curr_state());
            break;

        case CONF_GAMEPLAY_CAMERA_1_5:
            cam_preset_expected = CAM_PRESET_1_5;
            cam_preset_set(CAM_1, CAM_PRESET_1_5);
            config_save();
            goto_state(curr_state());
            break;

        case CONF_GAMEPLAY_LOCK_GOALS:
            audio_play(val == 0 ? "snd/2.2/game_button_down.ogg" : "snd/2.2/game_button_up.ogg", 1.0f);
            config_set_d(CONFIG_LOCK_GOALS, val);
            config_save();
            goto_state(curr_state());
            break;
    }

    return 1;
}

/*
 * This function name will be redirected to conf_gameplay_action() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_gameplay_action()` to `gameplay_action()`.
 */
#define gameplay_action conf_gameplay_action

static int conf_gameplay_gui(void)
{
    int id, jd, kd, ld;
    int curr = cam_preset_get(CAM_1);

    /* Initialize the configuration GUI. */

    if ((id = gui_vstack(0)))
    {
        conf_header(id, _("Gameplay"), GUI_BACK);

        if (game_server_state() &&
            (curr_mode() == MODE_CHALLENGE ||
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
             curr_mode() == MODE_HARDCORE ||
#else
             curr_mode() == MODE_ROGUE ||
#endif
             curr_mode() == MODE_BOOST_RUSH ||
             curr_mode() == MODE_DAILY
             ))
        {
            gui_multi(id, _("Some options are disabled\nbecause you've selected this mode."),
                          GUI_SML, GUI_COLOR_RED);
            gui_space(id);
        }

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        if (!game_server_state() || (curr_mode() != MODE_HARDCORE && curr_balls() > 0))
#else
        if (!game_server_state() || curr_balls() > 0)
#endif
        {
#if NB_HAVE_PB_BOTH==1
            conf_toggle_simple(id, _("Auto-Retry"), CONF_GAMEPLAY_AUTORETRY,
                                   config_get_d(CONFIG_ADVANCEDGAMING_GAMEPLAY_AUTORETRY), 1, 0);

            if (config_get_d(CONFIG_ADVANCEDGAMING_GAMEPLAY_AUTORETRY))
                conf_toggle_simple(id, _("Faster Reset"), CONF_GAMEPLAY_FASTERRESET,
                                       config_get_d(CONFIG_ADVANCEDGAMING_GAMEPLAY_FASTERRESET), 1, 0);
#else
            conf_toggle(id, _("Auto-Retry"), CONF_GAMEPLAY_AUTORETRY,
                            config_get_d(CONFIG_ADVANCEDGAMING_GAMEPLAY_AUTORETRY), _("On"), 1, _("Off"), 0);

            if (config_get_d(CONFIG_ADVANCEDGAMING_GAMEPLAY_AUTORETRY))
                conf_toggle(id, _("Faster Reset"), CONF_GAMEPLAY_FASTERRESET,
                                config_get_d(CONFIG_ADVANCEDGAMING_GAMEPLAY_FASTERRESET), _("On"), 1, _("Off"), 0);
#endif

            gui_space(id);
        }

#if NB_HAVE_PB_BOTH==1
        conf_toggle_simple(id, _("Show Tutorial"), CONF_GAMEPLAY_TUTORIAL,
                               config_get_d(CONFIG_ACCOUNT_TUTORIAL), 1, 0);
        conf_toggle_simple(id, _("Show Hint"), CONF_GAMEPLAY_HINT,
                               config_get_d(CONFIG_ACCOUNT_HINT), 1, 0);
#else
        conf_toggle(id, _("Show Tutorial"), CONF_GAMEPLAY_TUTORIAL,
                        config_get_d(CONFIG_ACCOUNT_TUTORIAL), _("On"), 1, _("Off"), 0);
        conf_toggle(id, _("Show Hint"), CONF_GAMEPLAY_HINT,
                        config_get_d(CONFIG_ACCOUNT_HINT), _("On"), 1, _("Off"), 0);
#endif

        gui_space(id);

        if (game_switchball_installed())
        {
#if NB_HAVE_PB_BOTH==1
            conf_toggle_simple(id, _("Drop Speeding Alert"), CONF_GAMEPLAY_SWITCHBALL_DROPSPEEDING,
                                   config_get_d(CONFIG_ADVANCEDGAMING_GAMEPLAY_SWITCHBALL_DROPSPEEDING), 1, 0);
#else
            conf_toggle(id, _("Drop Speeding Alert"), CONF_GAMEPLAY_SWITCHBALL_DROPSPEEDING,
                            config_get_d(CONFIG_ADVANCEDGAMING_GAMEPLAY_SWITCHBALL_DROPSPEEDING), _("On"), 1, _("Off"), 0);
#endif
            gui_space(id);
        }

        if (!game_server_state() ||
            (curr_mode() != MODE_CHALLENGE &&
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
             curr_mode() != MODE_HARDCORE &&
#else
             curr_mode() != MODE_ROGUE &&
#endif
             curr_mode() != MODE_BOOST_RUSH &&
             curr_mode() != MODE_DAILY
             ))
        {
#if NB_HAVE_PB_BOTH==1
            conf_toggle_simple(id, _("Completed Levels"), CONF_GAMEPLAY_LOCK_GOALS,
                                   config_get_d(CONFIG_LOCK_GOALS), 0, 1);
#else
            conf_toggle(id, _("Completed Levels"), CONF_GAMEPLAY_SWITCHBALL_DROPSPEEDING,
                            config_get_d(CONFIG_LOCK_GOALS), _("Locked"), 1, _("Unlocked"), 0);
#endif
            gui_space(id);
        }

        if ((jd = gui_harray(id)) && (kd = gui_vstack(jd)) && (ld = gui_vstack(jd)))
        {
            int btn0 = gui_state(kd, _("Default"),     GUI_SML, CONF_GAMEPLAY_CAMERA_DEFAULT, 0);
            int btn1 = gui_state(kd, _("1.4 Classic"), GUI_SML, CONF_GAMEPLAY_CAMERA_1_4,     0);
            int btn2 = gui_state(kd, _("1.5 Classic"), GUI_SML, CONF_GAMEPLAY_CAMERA_1_5,     0);

            gui_set_hilite(btn0, (curr == CAM_PRESET_DEFAULT));
            gui_set_hilite(btn1, (curr == CAM_PRESET_1_4));
            gui_set_hilite(btn2, (curr == CAM_PRESET_1_5));

            gui_label(ld, _("Camera Preset"), GUI_SML, 0, 0);
            gui_filler(ld);
        }

        gui_layout(id, 0, 0);
    }

    return id;
}

/*
 * This function name will be redirected to conf_gameplay_gui() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_gameplay_gui()` to `gameplay_gui()`.
 */
#define gameplay_gui conf_gameplay_gui

static int conf_gameplay_enter(struct state *st, struct state *prev, int intent)
{
    if (!conf_gameplay_settings_entered) {
        cam_preset_expected = cam_preset_get(CAM_1);
        conf_gameplay_settings_entered = 1;
    }

    if (mainmenu_conf && !game_server_state() && !demo_state())
        game_client_free(NULL);

    conf_common_init(gameplay_action, mainmenu_conf);
    return transition_slide(gameplay_gui(), 1, intent);
}

/*
 * This function name will be redirected to conf_gameplay_enter() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_gameplay_enter()` to `gameplay_enter()`.
 */
#define gameplay_enter conf_gameplay_enter

/*---------------------------------------------------------------------------*/

/*
 * Should be set the preset keys as well?
 */
#define CONF_CONTROL_SET_PRESET_KEYS(cam_tgl, cam1, cam2,    \
                                     cam3, camL, camR, axYP, \
                                     axXN, axYN, axXP)       \
    do {                                                     \
        config_set_d(CONFIG_KEY_CAMERA_TOGGLE, cam_tgl);     \
        config_set_d(CONFIG_KEY_CAMERA_1,      cam1);        \
        config_set_d(CONFIG_KEY_CAMERA_2,      cam2);        \
        config_set_d(CONFIG_KEY_CAMERA_3,      cam3);        \
        config_set_d(CONFIG_KEY_CAMERA_L,      camL);        \
        config_set_d(CONFIG_KEY_CAMERA_R,      camR);        \
        config_set_d(CONFIG_KEY_FORWARD,       axYP);        \
        config_set_d(CONFIG_KEY_LEFT,          axXN);        \
        config_set_d(CONFIG_KEY_BACKWARD,      axYN);        \
        config_set_d(CONFIG_KEY_RIGHT,         axXP);        \
    } while (0)

enum
{
    CONF_CONTROLS_INPUT_PRESET = GUI_LAST,
    CONF_CONTROLS_TILTING_FLOOR,
    CONF_CONTROLS_CAMERA_ROTATE_MODE,
    CONF_CONTROLS_MOUSE_SENSE,
    CONF_CONTROLS_INVERT_MOUSE_Y,
    CONF_CONTROLS_INVERT_RS_Y,
    CONF_CONTROLS_CHANGEKEYBD,
    CONF_CONTROLS_CONTROLLERS,
    CONF_CONTROLS_AUTOCALIB_AXIS,
    CONF_CONTROLS_CONTROLLERS_CALIBRATE,
    CONF_CONTROLS_TOUCH
};

/*
 * This enum name will be redirected to CONF_CONTROLS_INPUT_PRESET for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `CONF_CONTROLS_INPUT_PRESET` to `CONTROLS_INPUT_PRESET`.
 */
#define CONTROLS_INPUT_PRESET CONF_CONTROLS_INPUT_PRESET

/*
 * This enum name will be redirected to CONF_CONTROLS_TILTING_FLOOR for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `CONF_CONTROLS_TILTING_FLOOR` to `CONTROLS_TILTING_FLOOR`.
 */
#define CONTROLS_TILTING_FLOOR CONF_CONTROLS_TILTING_FLOOR

/*
 * This enum name will be redirected to CONF_CONTROLS_CAMERA_ROTATE_MODE for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `CONF_CONTROLS_CAMERA_ROTATE_MODE` to `CONTROLS_CAMERA_ROTATE_MODE`.
 */
#define CONTROLS_CAMERA_ROTATE_MODE CONF_CONTROLS_CAMERA_ROTATE_MODE

/*
 * This enum name will be redirected to CONF_CONTROLS_MOUSE_SENSE for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `CONF_CONTROLS_MOUSE_SENSE` to `CONTROLS_MOUSE_SENSE`.
 */
#define CONTROLS_MOUSE_SENSE CONF_CONTROLS_MOUSE_SENSE

/*
 * This enum name will be redirected to CONF_CONTROLS_INVERT_MOUSE_Y for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `CONF_CONTROLS_INVERT_MOUSE_Y` to `CONTROLS_INVERT_MOUSE_Y`.
 */
#define CONTROLS_INVERT_MOUSE_Y CONF_CONTROLS_INVERT_MOUSE_Y

/*
 * This enum name will be redirected to CONF_CONTROLS_INVERT_RS_Y for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `CONF_CONTROLS_INVERT_RS_Y` to `CONTROLS_INVERT_RS_Y`.
 */
#define CONTROLS_INVERT_RS_Y CONF_CONTROLS_INVERT_RS_Y

/*
 * This enum name will be redirected to CONF_CONTROLS_CHANGEKEYBD for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `CONF_CONTROLS_CHANGEKEYBD` to `CONTROLS_KEYBD`.
 */
#define CONTROLS_KEYBD CONF_CONTROLS_CHANGEKEYBD

/*
 * This enum name will be redirected to CONF_CONTROLS_CONTROLLERS for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `CONF_CONTROLS_CONTROLLERS` to `CONTROLS_JOYSTICK`.
 */
#define CONTROLS_JOYSTICK CONF_CONTROLS_CONTROLLERS

/*
 * This enum name will be redirected to CONF_CONTROLS_AUTOCALIB_AXIS for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `CONF_CONTROLS_AUTOCALIB_AXIS` to `CONTROLS_JOYSTICK_AUTOCALIB_AXIS`.
 */
#define CONTROLS_JOYSTICK_AUTOCALIB_AXIS CONF_CONTROLS_AUTOCALIB_AXIS

/*
 * This enum name will be redirected to CONF_CONTROLS_CONTROLLERS_CALIBRATE for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `CONF_CONTROLS_CONTROLLERS_CALIBRATE` to `CONTROLS_JOYSTICK_CALIBRATE`.
 */
#define CONTROLS_JOYSTICK_CALIBRATE CONF_CONTROLS_CONTROLLERS_CALIBRATE

/*
 * This enum name will be redirected to CONF_CONTROLS_TOUCH for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `CONF_CONTROLS_TOUCH` to `CONTROLS_TOUCH`.
 */
#define CONTROLS_TOUCH CONF_CONTROLS_TOUCH

enum InputType
{
    CONTROL_NONE,

    CONTROL_NEVERBALL,
    CONTROL_SWITCHBALL_V1,
    CONTROL_SWITCHBALL_V2,

    CONTROL_MAX
};

static int preset_id;
static int key_preset_id;

#ifdef SWITCHBALL_GUI
static int camrot_mode_id;
static int mouse_id;
#else
static int mouse_id[11];
#endif

/*
 * This maps mouse_sense 300 (default) to the 7th of an 11 button
 * series. Effectively there are more options for a lower-than-default
 * sensitivity than for a higher one.
 */

#define MOUSE_RANGE_MIN  100
#define MOUSE_RANGE_INC  50
#define MOUSE_RANGE_MAX (MOUSE_RANGE_MIN + (MOUSE_RANGE_INC * 10))

/*
 * Map mouse_sense values to [0, 10]. A higher mouse_sense value means
 * lower sensitivity, thus counter-intuitively, 0 maps to the higher
 * value.
 */

#define MOUSE_RANGE_MAP(m) \
    CLAMP(0, (MOUSE_RANGE_MAX - m) / MOUSE_RANGE_INC, 10)

#define MOUSE_RANGE_UNMAP(i) \
    (MOUSE_RANGE_MAX - (i * MOUSE_RANGE_INC))

static int control_get_input(void)
{
    const SDL_Keycode k_auto    = config_get_d(CONFIG_KEY_CAMERA_TOGGLE);
    const SDL_Keycode k_cam1    = config_get_d(CONFIG_KEY_CAMERA_1);
    const SDL_Keycode k_cam2    = config_get_d(CONFIG_KEY_CAMERA_2);
    const SDL_Keycode k_cam3    = config_get_d(CONFIG_KEY_CAMERA_3);
    const SDL_Keycode k_caml    = config_get_d(CONFIG_KEY_CAMERA_L);
    const SDL_Keycode k_camr    = config_get_d(CONFIG_KEY_CAMERA_R);

    SDL_Keycode k_arrowkey[4] = { 0, 0, 0, 0 };
    k_arrowkey[0] = config_get_d(CONFIG_KEY_FORWARD);
    k_arrowkey[1] = config_get_d(CONFIG_KEY_LEFT);
    k_arrowkey[2] = config_get_d(CONFIG_KEY_BACKWARD);
    k_arrowkey[3] = config_get_d(CONFIG_KEY_RIGHT);

    if (k_auto == SDLK_c && k_cam1 == SDLK_3 && k_cam2 == SDLK_1 && k_cam3 == SDLK_2
        && k_caml == SDLK_RIGHT && k_camr == SDLK_LEFT
        && k_arrowkey[0] == SDLK_w && k_arrowkey[1] == SDLK_a && k_arrowkey[2] == SDLK_s && k_arrowkey[3] == SDLK_d)
        return CONTROL_SWITCHBALL_V2;
    if (k_auto == SDLK_c && k_cam1 == SDLK_3 && k_cam2 == SDLK_1 && k_cam3 == SDLK_2
        && k_caml == SDLK_d && k_camr == SDLK_a
        && k_arrowkey[0] == SDLK_UP && k_arrowkey[1] == SDLK_LEFT && k_arrowkey[2] == SDLK_DOWN && k_arrowkey[3] == SDLK_RIGHT)
        return CONTROL_SWITCHBALL_V1;
    else if (k_auto == SDLK_e && k_cam1 == SDLK_1 && k_cam2 == SDLK_2 && k_cam3 == SDLK_3
             && k_caml == SDLK_s && k_camr == SDLK_d
             && k_arrowkey[0] == SDLK_UP && k_arrowkey[1] == SDLK_LEFT && k_arrowkey[2] == SDLK_DOWN && k_arrowkey[3] == SDLK_RIGHT)
        return CONTROL_NEVERBALL;

    return CONTROL_MAX;
}

static void control_set_input()
{
    if (key_preset_id == CONTROL_SWITCHBALL_V1)
    {
        CONF_CONTROL_SET_PRESET_KEYS(SDLK_c, SDLK_3, SDLK_1, SDLK_2,
                                     SDLK_RIGHT, SDLK_LEFT,
                                     SDLK_w, SDLK_a, SDLK_s, SDLK_d);

        gui_set_label(preset_id, "Switchball HD");
        key_preset_id = CONTROL_SWITCHBALL_V2;
    }
    else if (key_preset_id == CONTROL_NEVERBALL)
    {
        CONF_CONTROL_SET_PRESET_KEYS(SDLK_c, SDLK_3, SDLK_1, SDLK_2,
                                     SDLK_d, SDLK_a,
                                     SDLK_UP, SDLK_LEFT, SDLK_DOWN, SDLK_RIGHT);

        gui_set_label(preset_id, "Switchball");
        key_preset_id = CONTROL_SWITCHBALL_V1;
    }
    else
    {
        CONF_CONTROL_SET_PRESET_KEYS(SDLK_e, SDLK_1, SDLK_2, SDLK_3,
                                     SDLK_s, SDLK_d,
                                     SDLK_UP, SDLK_LEFT, SDLK_DOWN, SDLK_RIGHT);

        gui_set_label(preset_id, "Neverball");
        key_preset_id = CONTROL_NEVERBALL;
    }

    audio_play(key_preset_id != CONTROL_NEVERBALL ? "snd/2.2/game_button_down.ogg" : "snd/2.2/game_button_up.ogg", 1.0f);
}

static int conf_controls_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    int mouse = MOUSE_RANGE_MAP(config_get_d(CONFIG_MOUSE_SENSE));

    switch (tok)
    {
        case GUI_BACK:
            exit_state(&st_null);
            return exit_state(&st_conf);

        case CONTROLS_INPUT_PRESET:
            control_set_input();
            config_save();
            break;

        case CONTROLS_TILTING_FLOOR:
            audio_play(val != 0 ? "snd/2.2/game_button_down.ogg" : "snd/2.2/game_button_up.ogg", 1.0f);
            config_set_d(CONFIG_TILTING_FLOOR, val);
            config_save();
            goto_state(&st_conf_controls);
            break;

        case CONTROLS_CAMERA_ROTATE_MODE:
#ifdef SWITCHBALL_GUI
            if (camrot_mode_id)
            {
                config_tgl_d(CONFIG_CAMERA_ROTATE_MODE);
                config_set_d(CONFIG_TOUCH_ROTATE_INVERT, config_get_d(CONFIG_CAMERA_ROTATE_MODE));

                const char *cam_rot_mode_text = config_get_d(CONFIG_CAMERA_ROTATE_MODE) == 1 ?
                                                N_("Inverted") : N_("Normal");

                gui_set_label(camrot_mode_id, _(cam_rot_mode_text));
                audio_play(config_get_d(CONFIG_CAMERA_ROTATE_MODE) != 0 ? "snd/2.2/game_button_down.ogg" : "snd/2.2/game_button_up.ogg", 1.0f);
            }
#endif
            break;

        case CONTROLS_MOUSE_SENSE:
            config_set_d(CONFIG_MOUSE_SENSE, MOUSE_RANGE_UNMAP(val));

#ifdef SWITCHBALL_GUI
            conf_set_slider_v2(mouse_id, val);
            goto_state(curr_state());
#else
            gui_toggle(mouse_id[val]);
            gui_toggle(mouse_id[mouse]);
#endif
            config_save();
            break;

        case CONTROLS_INVERT_MOUSE_Y:
            audio_play(val != 0 ? "snd/2.2/game_button_down.ogg" : "snd/2.2/game_button_up.ogg", 1.0f);
            config_set_d(CONFIG_MOUSE_INVERT, val);
            config_save();
            goto_state(&st_conf_controls);
            break;

        case CONTROLS_INVERT_RS_Y:
            audio_play(val != 0 ? "snd/2.2/game_button_down.ogg" : "snd/2.2/game_button_up.ogg", 1.0f);
            config_set_d(CONFIG_JOYSTICK_AXIS_Y1_INVERT, val);
            config_save();
            goto_state(&st_conf_controls);
            break;

        case CONTROLS_KEYBD:
            goto_state(&st_conf_keybd);
            break;

        case CONTROLS_JOYSTICK:
            goto_state(&st_conf_controllers);
            break;

        case CONTROLS_JOYSTICK_AUTOCALIB_AXIS:
            audio_play(val != 0 ? "snd/2.2/game_button_down.ogg" : "snd/2.2/game_button_up.ogg", 1.0f);
            config_set_d(CONFIG_JOYSTICK_AUTOCALIB_AXIS, val);
            config_save();
            goto_state(&st_conf_controls);
            break;

        case CONTROLS_JOYSTICK_CALIBRATE:
            goto_state(&st_conf_calibrate);
            break;

        case CONTROLS_TOUCH:
            goto_state(&st_conf_touch);
            break;
    }

    return 1;
}

/*
 * This function name will be redirected to conf_controls_action() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controls_action()` to `controls_action()`.
 */
#define controls_action conf_controls_action

static int conf_controls_gui(void)
{
    int id;

    preset_id      = 0;
#ifdef SWITCHBALL_GUI
    camrot_mode_id = 0;
    mouse_id       = 0;
#else
    for (int i = 0; i < 11; i++)
        mouse_id[i] = 0;
#endif

    /* Initialize the configuration GUI. */

    if ((id = gui_vstack(0)))
    {
        int mouse = MOUSE_RANGE_MAP(config_get_d(CONFIG_MOUSE_SENSE));

        conf_header(id, _("Controls"), GUI_BACK);

        if (opt_touch && !video_has_touch)
        {
            gui_multi(id, _("Emulated touch controls enabled!\n"
                            "Tilt angles may accelerate than expected."),
                          GUI_SML, GUI_COLOR_RED);

            gui_space(id);
        }

        preset_id = conf_state(id, _("Preset"), "XXXXXXXXXXXXX",
                                   CONTROLS_INPUT_PRESET);

        const char *presetname = N_("Custom");

        switch (control_get_input())
        {
            case CONTROL_NEVERBALL:
                key_preset_id = control_get_input();
                presetname = "Neverball";
                break;
            case CONTROL_SWITCHBALL_V1:
                key_preset_id = control_get_input();
                presetname = "Switchball";
                break;
            case CONTROL_SWITCHBALL_V2:
                key_preset_id = control_get_input();
                presetname = "Switchball HD";
                break;
        }

        gui_set_label(preset_id, _(presetname));
        gui_space(id);

#if NB_HAVE_PB_BOTH==1
        conf_toggle_simple(id, _("Tilting Floor"), CONTROLS_TILTING_FLOOR,
                               config_get_d(CONFIG_TILTING_FLOOR),
                               1, 0);
#else
        conf_toggle(id, _("Tilting Floor"), CONTROLS_TILTING_FLOOR,
                        config_get_d(CONFIG_TILTING_FLOOR),
                        _("On"), 1, _("Off"), 0);
#endif

#ifdef SWITCHBALL_GUI
        const char *camrot_mode_text = config_get_d(CONFIG_CAMERA_ROTATE_MODE) == 1 ?
                                       N_("Inverted") : N_("Normal");
        camrot_mode_id = conf_state(id, _("Camera rotate"), camrot_mode_text,
                                    CONTROLS_CAMERA_ROTATE_MODE);
#endif

        gui_space(id);

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        if (current_platform == PLATFORM_PC && !console_gui_shown())
#endif
        {

#ifdef SWITCHBALL_GUI
            mouse_id = conf_slider_v2(id, _("Mouse Sensitivity"), CONTROLS_MOUSE_SENSE,
                                      mouse);
#else
            conf_slider(id, _("Mouse Sensitivity"), CONF_CONTROLS_MOUSE_SENSE,
                            mouse, mouse_id, ARRAYSIZE(mouse_id));
#endif

#if NB_HAVE_PB_BOTH==1
            conf_toggle_simple(id, _("Invert Y Axis"), CONTROLS_INVERT_MOUSE_Y,
                                   config_get_d(CONFIG_MOUSE_INVERT),
                                   1, 0);
#else
            conf_toggle(id, _("Invert Y Axis"), CONF_CONTROLS_INVERT_MOUSE_Y,
                            config_get_d(CONFIG_MOUSE_INVERT),
                            _("On"), 1, _("Off"), 0);
#endif
            gui_space(id);
            conf_state(id, _("Keyboard"), _("Configure"), CONTROLS_KEYBD);
        }
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        else
#endif
        {
#if NB_HAVE_PB_BOTH==1
            conf_toggle_simple(id, _("Invert Y Axis (RS)"), CONTROLS_INVERT_RS_Y,
                                   config_get_d(CONFIG_JOYSTICK_AXIS_Y1_INVERT),
                                   1, 0);
#else
            conf_toggle(id, _("Invert Y Axis (RS)"), CONTROLS_INVERT_RS_Y,
                            config_get_d(CONFIG_JOYSTICK_AXIS_Y1_INVERT),
                            _("On"), 1, _("Off"), 0);
#endif
        }

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        if (current_platform != PLATFORM_PC || console_gui_shown())
#endif
        {
#if !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__)
            gui_space(id);

#if NB_HAVE_PB_BOTH==1
            conf_toggle_simple(id, _("Auto-Calibrate Axis"), CONTROLS_JOYSTICK_AUTOCALIB_AXIS,
                                   config_get_d(CONFIG_JOYSTICK_AUTOCALIB_AXIS),
                                   1, 0);
#else
            conf_toggle(id, _("Auto-Calibrate Axis"), CONTROLS_JOYSTICK_AUTOCALIB_AXIS,
                            config_get_d(CONFIG_JOYSTICK_AUTOCALIB_AXIS),
                            _("On"), 1, _("Off"), 0);
#endif
            gui_space(id);
            conf_state(id, _("Gamepad"), _("Configure"), CONTROLS_JOYSTICK);

            if (!config_get_d(CONFIG_JOYSTICK_AUTOCALIB_AXIS))
                conf_state(id, _("Axis"), _("Calibrate"), CONTROLS_JOYSTICK_CALIBRATE);
#endif
        }

#ifdef NDEBUG
        if (opt_touch || video_has_touch)
#endif
        {
            gui_space(id);
            conf_state(id, _("Touch"), _("Configure"), CONTROLS_TOUCH);
        }

        gui_layout(id, 0, 0);
    }

    return id;
}

/*
 * This function name will be redirected to conf_controls_gui() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controls_gui()` to `controls_gui()`.
 */
#define controls_gui conf_controls_gui

static int conf_controls_enter(struct state *st, struct state *prev, int intent)
{
    if (mainmenu_conf && !game_server_state() && !demo_state())
        game_client_free(NULL);

    conf_common_init(controls_action, mainmenu_conf);
    return transition_slide(controls_gui(), 1, intent);
}

/*
 * This function name will be redirected to conf_controls_enter() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controls_enter()` to `controls_enter()`.
 */
#define controls_enter conf_controls_enter

/*---------------------------------------------------------------------------*/

enum
{
    CONF_TOUCH_MODE = GUI_LAST,
    CONF_TOUCH_ROTATE_INVERT
};

/*
 * This enum name will be redirected to CONF_TOUCH_MODE for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `CONF_TOUCH_MODE` to `TOUCH_MODE`.
 */
#define TOUCH_MODE CONF_TOUCH_MODE

/*
 * This enum name will be redirected to CONF_TOUCH_ROTATE_INVERT for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `CONF_TOUCH_ROTATE_INVERT` to `TOUCH_ROTATE_INVERT`.
 */
#define TOUCH_ROTATE_INVERT CONF_TOUCH_ROTATE_INVERT

static struct state *conf_touch_back;

/*
 * This variable name will be redirected to conf_touch_back for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_touch_back` to `touch_back`.
 */
#define touch_back conf_touch_back

static int conf_touch_action(int tok, int val)
{
    int r = 1;

    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
    case GUI_BACK:
        r = exit_state(touch_back);
        touch_back = NULL;
        return r;

    case TOUCH_MODE:
        config_set_d(CONFIG_TOUCH_MODE, val);
        return goto_state(&st_conf_touch);

    case TOUCH_ROTATE_INVERT:
        audio_play(val != 0 ? "snd/2.2/game_button_down.ogg" : "snd/2.2/game_button_up.ogg", 1.0f);
        config_set_d(CONFIG_TOUCH_ROTATE_INVERT, val);
        config_set_d(CONFIG_CAMERA_ROTATE_MODE, val);
        return goto_state(&st_conf_touch);
    }

    return r;
}

/*
 * This function name will be redirected to conf_touch_enter() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_touch_enter()` to `touch_enter()`.
 */
#define touch_action conf_touch_action

static int conf_touch_gui(void)
{
    int id, jd, kd, ld;

    int curr = config_get_d(CONFIG_TOUCH_MODE);

    if ((id = gui_vstack(0)))
    {
        conf_header(id, _("Touch"), GUI_BACK);

        if ((jd = gui_harray(id)) && (kd = gui_vstack(jd)) && (ld = gui_vstack(jd)))
        {
            const char btn_texts[3][12] = { N_("Left Tilt"), N_("Right Tilt"), N_("Dynamic") };
            int btn_enum_touch_modes[3] = { TOUCH_MODE_LR, TOUCH_MODE_RL, TOUCH_MODE_DYNAMIC };
            int btns[3] = { 0, 0, 0 };

            for (int i = 0; i < 3; i++)
            {
                btns[i] = gui_state(kd, _(btn_texts[i]), GUI_SML, TOUCH_MODE, btn_enum_touch_modes[i]);
                gui_set_hilite(btns[i], (curr == btn_enum_touch_modes[i]));
            }

            gui_label(ld, _("Mode"), GUI_SML, 0, 0);
            gui_filler(ld);
        }

        gui_space(id);

#if NB_HAVE_PB_BOTH==1
        conf_toggle_simple(id, _("Invert Rotation"), TOUCH_ROTATE_INVERT,
                               config_get_d(CONFIG_TOUCH_ROTATE_INVERT),
                               1, 0);
#else
        conf_toggle(id, _("Invert Rotation"),
                        TOUCH_ROTATE_INVERT,
                        config_get_d(CONFIG_TOUCH_ROTATE_INVERT),
                        _("On"), 1, _("Off"), 0);
#endif

        gui_layout(id, 0, 0);
    }

    return id;
}

/*
 * This function name will be redirected to conf_touch_enter() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_touch_enter()` to `touch_enter()`.
 */
#define touch_gui conf_touch_gui

static int conf_touch_enter(struct state *st, struct state *prev, int intent)
{
    if (!touch_back)
        touch_back = prev;

    conf_common_init(touch_action, mainmenu_conf);
    return transition_slide(touch_gui(), 1, intent);
}

/*
 * This function name will be redirected to conf_touch_enter() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_touch_enter()` to `touch_enter()`.
 */
#define touch_enter conf_touch_enter

/*---------------------------------------------------------------------------*/

enum
{
    CONF_KEYBD_ASSIGN_KEY = GUI_LAST,
};

/*
 * This enum name will be redirected to CONF_KEYBD_ASSIGN_KEY for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `CONF_KEYBD_ASSIGN_KEY` to `KEYBD_ASSIGN_KEY`.
 */
#define KEYBD_ASSIGN_KEY CONF_KEYBD_ASSIGN_KEY

static struct state *conf_keybd_back;

/*
 * This variable name will be redirected to conf_keybd_back for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_keybd_back` to `keybd_back`.
 */
#define keybd_back conf_keybd_back

static int conf_keybd_modal_key_id;
static int conf_keybd_modal;
static int conf_keybd_option_index;

/*
 * This variable name will be redirected to conf_keybd_modal_key_id for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_keybd_modal_key_id` to `keybd_modal_key_id`.
 */
#define keybd_modal_key_id conf_keybd_modal_key_id

/*
 * This variable name will be redirected to conf_keybd_modal for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_keybd_modal` to `keybd_modal`.
 */
#define keybd_modal conf_keybd_modal

/*
 * This variable name will be redirected to conf_keybd_option_index for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_keybd_option_index` to `keybd_option_index`.
 */
#define keybd_option_index conf_keybd_option_index

static float keybd_modal_alpha = 0.0f;

static const char *conf_keybd_option_names[] = {
    N_("Auto-Camera"),
    "KEYBD_KEY_CAM_1",
    "KEYBD_KEY_CAM_2",
    "KEYBD_KEY_CAM_3",
    "",
    N_("Restart Level"),
    "",
    N_("Tilt/Roll Forward"),
    N_("Tilt/Roll Backward"),
    N_("Tilt/Roll Left"),
    N_("Tilt/Roll Right"),
    "",
    N_("Rotate Left"),
    N_("Rotate Right")
};

/*
 * This variable name will be redirected to conf_keybd_option_names[] for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_keybd_option_names[]` to `keybd_option_names[]`.
 */
#define keybd_option_names conf_keybd_option_names

static int *conf_keybd_options[] = {
    &CONFIG_KEY_CAMERA_TOGGLE,
    &CONFIG_KEY_CAMERA_1,
    &CONFIG_KEY_CAMERA_2,
    &CONFIG_KEY_CAMERA_3,
    NULL,
    &CONFIG_KEY_RESTART,
    NULL,
    &CONFIG_KEY_FORWARD,
    &CONFIG_KEY_BACKWARD,
    &CONFIG_KEY_LEFT,
    &CONFIG_KEY_RIGHT,
    NULL,
    &CONFIG_KEY_CAMERA_R,
    &CONFIG_KEY_CAMERA_L
};

/*
 * This variable name will be redirected to conf_keybd_options[] for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_keybd_options[]` to `conf_keybd_options[]`.
 */
#define keybd_options conf_keybd_options

static int conf_keybd_option_ids[ARRAYSIZE(keybd_options)];

/*
 * This variable name will be redirected to conf_keybd_option_ids[] for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_keybd_option_ids[]` to `conf_keybd_option_ids[]`.
 */
#define keybd_option_ids conf_keybd_option_ids

static void conf_keybd_set_label(int id, int value)
{
    gui_set_label(id, value ? SDL_GetKeyName(value) : _("Unassigned"));
}

/*
 * This function name will be redirected to conf_keybd_set_label() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_keybd_set_label()` to `keybd_set_label()`.
 */
#define keybd_set_label conf_keybd_set_label

static void conf_keybd_set_option(int index, int value)
{
    for (int i = 0; i < ARRAYSIZE(keybd_options); i++)
    {
        int option_id = *keybd_options[index];

        if (value == config_get_d(option_id))
        {
            config_set_d(option_id, 0);
            keybd_set_label(option_id, 0);
        }
    }

    if (index < ARRAYSIZE(keybd_options))
    {
        int option_new = *keybd_options[index];

        config_set_d(option_new, value);

        keybd_set_label(keybd_option_ids[index], value);

        /* Focus the next button. */

        if (index < ARRAYSIZE(keybd_options) - 1)
        {
            /* Skip over marker, if any. */

            if (index < ARRAYSIZE(keybd_options) - 2 &&
                keybd_options[index + 1] == NULL)
                gui_focus(keybd_option_ids[index + 2]);
            else
                gui_focus(keybd_option_ids[index + 1]);
        }
    }
}

/*
 * This function name will be redirected to conf_keybd_set_option() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_keybd_set_option()` to `keybd_set_option()`.
 */
#define keybd_set_option conf_keybd_set_option

static int conf_keybd_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
            if (keybd_modal)
                keybd_modal = 0;
            else
            {
                exit_state(keybd_back);
                while (curr_state() != keybd_back)
                {
                    exit_state(keybd_back);
                    keybd_back = NULL;
                }
            }
            break;

        case KEYBD_ASSIGN_KEY:
            keybd_modal        = tok;
            keybd_option_index = val;
            break;
    }

    return 1;
}

/*
 * This function name will be redirected to conf_keybd_action() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_keybd_action()` to `keybd_action()`.
 */
#define keybd_action conf_keybd_action

static int conf_keybd_gui(void)
{
    int id;

    /* Initialize the configuration GUI. */

    if ((id = gui_vstack(0)))
    {
        conf_header(id, _("Keyboard"), GUI_BACK);

        int btn_id;
        int value;

        for (int i = 0; i < ARRAYSIZE(keybd_option_names); i++)
        {
            if (keybd_options[i])
            {
                char tmp_opt_name[MAXSTR];

                if      (str_starts_with(keybd_option_names[i], "KEYBD_KEY_CAM_1"))
                    SAFECPY(tmp_opt_name, cam_to_str(CAM_1));
                else if (str_starts_with(keybd_option_names[i], "KEYBD_KEY_CAM_2"))
                    SAFECPY(tmp_opt_name, cam_to_str(CAM_2));
                else if (str_starts_with(keybd_option_names[i], "KEYBD_KEY_CAM_3"))
                    SAFECPY(tmp_opt_name, cam_to_str(CAM_3));
                else
                    SAFECPY(tmp_opt_name, _(keybd_option_names[i]));

                value = config_get_d(*keybd_options[i]);

                if ((btn_id = conf_state(id, tmp_opt_name,
                                             value ? SDL_GetKeyName(value) : _("Unassigned"),
                                             KEYBD_ASSIGN_KEY)))
                {
                    keybd_option_ids[i] = btn_id;

                    gui_set_state(btn_id, KEYBD_ASSIGN_KEY, i);

                    keybd_set_label(btn_id, value);
                }
            }
            else gui_space(id);
        }

        gui_layout(id, 0, 0);
    }

    return id;
}

/*
 * This function name will be redirected to conf_keybd_gui() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_keybd_gui()` to `keybd_gui()`.
 */
#define keybd_gui conf_keybd_gui

static int conf_keybd_modal_key_gui(void)
{
    int id;

#if NB_HAVE_PB_BOTH==1
    if ((id = gui_multi(0,
                        _("Press any key on the keyboard\n"
                          "to assign. Press ESC to cancel."),
                        GUI_SML, GUI_COLOR_WHT)))
#else
    if ((id = gui_title_header(0, _("Press any key..."), GUI_MED, GUI_COLOR_WHT)))
#endif
        gui_layout(id, 0, 0);

    return id;
}

/*
 * This function name will be redirected to conf_keybd_modal_key_gui() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_keybd_modal_key_gui()` to `keybd_modal_key_gui()`.
 */
#define keybd_modal_key_gui conf_keybd_modal_key_gui

static int conf_keybd_enter(struct state *st, struct state *prev, int intent)
{
    if (!keybd_back)
        keybd_back = prev;

    if (mainmenu_conf && !game_server_state() && !demo_state())
        game_client_free(NULL);

    conf_common_init(keybd_action, mainmenu_conf);

    keybd_modal        = 0;
    keybd_modal_key_id = keybd_modal_key_gui();

    return transition_slide(keybd_gui(), 1, intent);
}

/*
 * This function name will be redirected to conf_keybd_enter() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_keybd_enter()` to `keybd_enter()`.
 */
#define keybd_enter conf_keybd_enter

static int conf_keybd_leave(struct state *st, struct state *next, int id, int intent)
{
    conf_common_leave(st, next, id, intent);

    gui_delete(keybd_modal_key_id);
    keybd_modal_key_id = 0;

    keybd_modal_alpha = 0.0f;

    return transition_slide(id, 0, intent);
}

/*
 * This function name will be redirected to conf_keybd_leave() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_keybd_leave()` to `keybd_leave()`.
 */
#define keybd_leave conf_keybd_leave

static void conf_keybd_paint(int id, float t)
{
    if (mainmenu_conf && !game_server_state() && !demo_state())
    {
        video_set_perspective((float) config_get_d(CONFIG_VIEW_FOV), 0.1f, FAR_DIST);
        back_draw_easy();
    }
    else game_client_draw(0, t);

    gui_paint(id);

    if (conf_keybd_modal == KEYBD_ASSIGN_KEY)
        gui_paint(keybd_modal_key_id);

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    if (current_platform != PLATFORM_PC || console_gui_shown())
        console_gui_list_paint();
#endif
}

/*
 * This function name will be redirected to conf_keybd_paint() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_keybd_paint()` to `keybd_paint()`.
 */
#define keybd_paint conf_keybd_paint

static void conf_keybd_timer(int id, float dt)
{
    gui_timer(id, dt);
    gui_alpha(id, 1 - keybd_modal_alpha);

    if (conf_keybd_modal == KEYBD_ASSIGN_KEY)
        keybd_modal_alpha = keybd_modal_alpha + (dt * 4);
    else
        keybd_modal_alpha = keybd_modal_alpha - (dt * 4);

    keybd_modal_alpha = CLAMP(0.0f, keybd_modal_alpha, 1.0f);
    gui_alpha(keybd_modal_key_id, keybd_modal_alpha);
}

/*
 * This function name will be redirected to conf_keybd_timer() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_keybd_timer()` to `keybd_timer()`.
 */
#define keybd_timer conf_keybd_timer

static int conf_keybd_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT && conf_keybd_modal)
        {
            /* Allow backing out of other modal types with Escape key. */

            conf_keybd_modal = 0;
            return 1;
        }
        else if (conf_keybd_modal == KEYBD_ASSIGN_KEY)
        {
            keybd_set_option(keybd_option_index, c);
            conf_keybd_modal = 0;
            return 1;
        }
    }

    return common_keybd(c, d);
}

/*
 * This function name will be redirected to conf_keybd_keybd() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_keybd_keybd()` to `keybd_keybd()`.
 */
#define keybd_keybd conf_keybd_keybd

/*---------------------------------------------------------------------------*/

enum
{
    CONF_CONTROLLERS_ASSIGN_BUTTON = GUI_LAST,
    CONF_CONTROLLERS_ASSIGN_AXIS,
};

/*
 * This enum name will be redirected to CONF_CONTROLLERS_ASSIGN_BUTTON for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `CONF_CONTROLLERS_ASSIGN_BUTTON` to `JOYSTICK_ASSIGN_BUTTON`.
 */
#define JOYSTICK_ASSIGN_BUTTON CONF_CONTROLLERS_ASSIGN_BUTTON

/*
 * This enum name will be redirected to CONF_CONTROLLERS_ASSIGN_AXIS for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `CONF_CONTROLLERS_ASSIGN_AXIS` to `JOYSTICK_ASSIGN_AXIS`.
 */
#define JOYSTICK_ASSIGN_AXIS CONF_CONTROLLERS_ASSIGN_AXIS

static struct state *conf_controllers_back;

static int conf_controllers_modal_button_id;
static int conf_controllers_modal_axis_id;
static int conf_controllers_modal;
static int conf_controllers_option_index;

static float controllers_modal_alpha = 0.0f;

/*
 * This variable name will be redirected to conf_controllers_back for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_back` to `joystick_back`.
 */
#define joystick_back conf_controllers_back

/*
 * This variable name will be redirected to conf_controllers_modal_button_id for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_modal_button_id` to `joystick_modal_button_id`.
 */
#define joystick_modal_button_id conf_controllers_modal_button_id

/*
 * This variable name will be redirected to conf_controllers_modal_axis_id for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_modal_axis_id` to `joystick_modal_axis_id`.
 */
#define joystick_modal_axis_id conf_controllers_modal_axis_id

/*
 * This variable name will be redirected to conf_controllers_modal for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_modal` to `joystick_modal`.
 */
#define joystick_modal conf_controllers_modal

/*
 * This variable name will be redirected to conf_controllers_option_index for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_option_index` to `joystick_option_index`.
 */
#define joystick_option_index conf_controllers_option_index

/*
 * Xbox, Playstation and Nintendo contains
 * fully controller support, so you don't need to worry them.
 */
static int *conf_controllers_options[] = {
    &CONFIG_JOYSTICK_BUTTON_A,
    &CONFIG_JOYSTICK_BUTTON_B,
    &CONFIG_JOYSTICK_BUTTON_X,
    &CONFIG_JOYSTICK_BUTTON_Y,
    &CONFIG_JOYSTICK_BUTTON_L1,
    &CONFIG_JOYSTICK_BUTTON_R1,
    &CONFIG_JOYSTICK_BUTTON_L2,
    &CONFIG_JOYSTICK_BUTTON_R2,
    &CONFIG_JOYSTICK_BUTTON_SELECT,
    &CONFIG_JOYSTICK_BUTTON_START,
    NULL,

    &CONFIG_JOYSTICK_AXIS_X0,
    &CONFIG_JOYSTICK_AXIS_Y0,
    NULL,
    &CONFIG_JOYSTICK_AXIS_X1,
    &CONFIG_JOYSTICK_AXIS_Y1,
    NULL,
};

/*
 * This variable name will be redirected to conf_controllers_options for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_options` to `joystick_options`.
 */
#define joystick_options conf_controllers_options

static const char *conf_controllers_option_names[] = {
    N_("Button A"),
    N_("Button B"),
    N_("Button X"),
    N_("Button Y"),
    N_("Button LB"),
    N_("Button RB"),
    N_("Button LT"),
    N_("Button RT"),
    N_("Select"),
    N_("Start"),

    "",

    N_("X Axis 1"),
    N_("Y Axis 1"),
    N_("Left Stick"),
    N_("X Axis 2"),
    N_("Y Axis 2"),
    N_("Right Stick"),
};

/*
 * This variable name will be redirected to conf_controllers_option_names for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_option_names` to `joystick_option_names`.
 */
#define joystick_option_names conf_controllers_option_names

static const char *conf_controllers_option_values_energizelab[] = {
    "a",
    "b",
    "e",
    "i",
    "LB",
    "RB",
    "LT",
    "RT",
    "",
    "",

    "",

    "X (LS)",
    "Y (LS)",
    "",
    "X (RS)",
    "Y (RS)",
    "",
};

/*
 * This variable name will be redirected to conf_controllers_option_values_energizelab for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_option_values_energizelab` to `joystick_option_values_energizelab`.
 */
#define joystick_option_values_energizelab conf_controllers_option_values_energizelab

static const char *conf_controllers_option_values_xbox[] = {
    "A",
    "B",
    "X",
    "Y",
    "LB",
    "RB",
    "LT",
    "RT",
    GUI_TRIANGLE_LEFT,
    GUI_TRIANGLE_RIGHT,

    "",

    "X (LS)",
    "Y (LS)",
    "LS",
    "X (RS)",
    "Y (RS)",
    "RS",
};

/*
 * This variable name will be redirected to conf_controllers_option_values_xbox for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_option_values_xbox` to `joystick_option_values_xbox`.
 */
#define joystick_option_values_xbox conf_controllers_option_values_xbox

static const char *conf_controllers_option_values_ps[] = {
    "X",
    "△",
    "◻",
    "○",
    "L1",
    "R1",
    "L2",
    "R2",
    GUI_TRIANGLE_LEFT,
    GUI_TRIANGLE_RIGHT,

    "",

    "X (L3)",
    "Y (L3)",
    "L3",
    "X (R3)",
    "Y (R3)",
    "R3",
};

/*
 * This variable name will be redirected to conf_controllers_option_values_ps for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_option_values_ps` to `joystick_option_values_ps`.
 */
#define joystick_option_values_ps conf_controllers_option_values_ps

static const char *conf_controllers_option_values_steamdeck[] = {
    "A",
    "B",
    "X",
    "Y",
    "L1",
    "R1",
    "L2",
    "R2",
    "-",
    "+",

    "",

    "X (L3)",
    "Y (L3)",
    "L3",
    "X (R3)",
    "Y (R3)",
    "R3",
};

/*
 * This variable name will be redirected to conf_controllers_option_values_steamdeck for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_option_values_steamdeck` to `joystick_option_values_steamdeck`.
 */
#define joystick_option_values_steamdeck conf_controllers_option_values_steamdeck

static const char *conf_controllers_option_values_switch[] = {
    "A",
    "B",
    "X",
    "Y",
    "L",
    "R",
    "ZL",
    "ZR",
    "-",
    "+",

    "",

    "X (LS)",
    "Y (LS)",
    "LS",
    "X (RS)",
    "Y (RS)",
    "RS",
};

/*
 * This variable name will be redirected to conf_controllers_option_values_switch for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_option_values_switch` to `joystick_option_values_switch`.
 */
#define joystick_option_values_switch conf_controllers_option_values_switch

static const char *conf_controllers_option_values_handset[] = {
    "A",
    "B",
    "X",
    "Y",
    "L",
    "R",
    "",
    "",
    GUI_TRIANGLE_LEFT,
    GUI_TRIANGLE_RIGHT,

    "",

    "X (LS)",
    "Y (LS)",
    "LS",
    "",
    "",
    "",
};

/*
 * This variable name will be redirected to conf_controllers_option_values_handset for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_option_values_handset` to `joystick_option_values_handset`.
 */
#define joystick_option_values_handset conf_controllers_option_values_handset

static const char *conf_controllers_option_values_wii[] = {
    "A",
    "B",
    "C",
    "Z",
    "",
    "",
    "",
    "",
    "-",
    "+",

    "",

    "X",
    "Y",
    "",
    "",
    "",
    "",
};

/*
 * This variable name will be redirected to conf_controllers_option_values_wii for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_option_values_wii` to `joystick_option_values_wii`.
 */
#define joystick_option_values_wii conf_controllers_option_values_wii

static int conf_controllers_option_ids[ARRAYSIZE(joystick_options)];

/*
 * This variable name will be redirected to conf_controllers_option_ids for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_option_ids` to `joystick_option_ids`.
 */
#define joystick_option_ids conf_controllers_option_ids

static void conf_controllers_set_label(int id, int value)
{
    char str[20];

    if (value == -1)
    {
        gui_set_label(id, _("Unassigned"));
        return;
    }

#if NEVERBALL_FAMILY_API == NEVERBALL_XBOX_FAMILY_API || \
    NEVERBALL_FAMILY_API == NEVERBALL_XBOX_360_FAMILY_API
    if (joystick_option_values_xbox[value % 100000])
    {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(str, 20,
#else
        sprintf(str,
#endif
                "%s", joystick_option_values_xbox[value % 100000]);
    }
#elif NEVERBALL_FAMILY_API == NEVERBALL_PS_FAMILY_API
    if (joystick_option_values_ps[value % 100000])
    {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(str, 20,
#else
        sprintf(str,
#endif
                "%s", joystick_option_values_ps[value % 100000]);
    }
#elif NEVERBALL_FAMILY_API == NEVERBALL_STEAMDECK_FAMILY_API
    if (joystick_option_values_steamdeck[value % 100000])
    {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(str, 20,
#else
        sprintf(str,
#endif
                "%s", joystick_option_values_steamdeck[value % 100000]);
    }
#elif NEVERBALL_FAMILY_API == NEVERBALL_SWITCH_FAMILY_API
    if (joystick_option_values_switch[value % 100000])
    {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(str, 20,
#else
        sprintf(str,
#endif
                "%s", joystick_option_values_switch[value % 100000]);
    }
#elif NEVERBALL_FAMILY_API == NEVERBALL_HANDSET_FAMILY_API
    if (joystick_option_values_switch[value % 100000])
    {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(str, 20,
#else
        sprintf(str,
#endif
                "%s", joystick_option_values_switch[value % 100000]);
    }
#elif NEVERBALL_FAMILY_API == NEVERBALL_WII_FAMILY_API || \
      NEVERBALL_FAMILY_API == NEVERBALL_WIIU_FAMILY_API
    if (joystick_option_values_wii[value % 100000])
    {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(str, 20,
#else
        sprintf(str,
#endif
                "%s", joystick_option_values_wii[value % 100000]);
    }
#elif NEVERBALL_FAMILY_API == NEVERBALL_ENERGIZELAB_FAMILY_API
    if (joystick_option_values_energizelab[value % 100000])
    {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(str, 20,
#else
        sprintf(str,
#endif
            "%s", joystick_option_values_energizelab[value % 100000]);
    }
#else
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(str, 20,
#else
    sprintf(str,
#endif
            "%d", value % 100000);
#endif

#if NEVERBALL_FAMILY_API != NEVERBALL_PC_FAMILY_API
    else SAFECPY(str, "");
#endif

    gui_set_label(id, str);
    gui_set_font(id, "ttf/DejaVuSans-Bold.ttf");
}

/*
 * This function name will be redirected to conf_controllers_set_label() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_set_label()` to `joystick_set_label()`.
 */
#define joystick_set_label conf_controllers_set_label

static void conf_controllers_set_option(int index, int value)
{
    for (int i = 0; i < ARRAYSIZE(joystick_options); i++)
    {
        int option_id = *joystick_options[index];

        if (value == config_get_d(option_id))
        {
            config_set_d(option_id, -1);
            joystick_set_label(option_id, -1);
        }
    }

    if (index < ARRAYSIZE(joystick_options))
    {
        int option = *joystick_options[index];

        config_set_d(option, value);

        joystick_set_label(joystick_option_ids[index], value + (joystick_modal == JOYSTICK_ASSIGN_AXIS ? 11 : 0));

        /* Focus the next button. */

        if (index < ARRAYSIZE(joystick_options) - 1)
        {
            /* Skip over marker, if any. */

            if (index < ARRAYSIZE(joystick_options) - 2 &&
                joystick_options[index + 1] == NULL)
                gui_focus(joystick_option_ids[index + 2]);
            else
                gui_focus(joystick_option_ids[index + 1]);
        }
    }
}

/*
 * This function name will be redirected to conf_controllers_set_option() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_set_option()` to `joystick_set_option()`.
 */
#define joystick_set_option conf_controllers_set_option

static int conf_controllers_action(int tok, int val)
{
    int r = 1;

    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
            if (joystick_modal)
                joystick_modal = 0;
            else
            {
                r = exit_state(joystick_back);
                joystick_back = NULL;
                return r;
            }
            break;

        case JOYSTICK_ASSIGN_BUTTON:
        case JOYSTICK_ASSIGN_AXIS:
            joystick_modal        = tok;
            joystick_option_index = val;
            break;
    }

    return r;
}

/*
 * This function name will be redirected to conf_controllers_action() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_action()` to `joystick_action()`.
 */
#define joystick_action conf_controllers_action

static int conf_controllers_gui(void)
{
    int id, jd, l_pane = 0, r_pane = 0;

    /* Initialize the configuration GUI. */

    if ((id = gui_vstack(0)))
    {
        conf_header(id, _("Gamepad"), GUI_BACK);

        if ((jd = gui_hstack(id)))
        {
            r_pane = gui_vstack(jd);
            gui_space(jd);
            l_pane = gui_vstack(jd);
        }

        int token = JOYSTICK_ASSIGN_BUTTON;

        for (int i = 0; i < ARRAYSIZE(joystick_options); ++i)
        {
            int btn_id;
            int value;

            /* Check for marker. */

            if (i == 10)
            {
                /* Switch the GUI token / assignment type. */
                token = JOYSTICK_ASSIGN_AXIS;

                gui_filler(id);

                continue;
            }

            if (joystick_options[i] == 0)
                continue;

            value = config_get_d(*joystick_options[i]);

            if (l_pane == 0 || r_pane == 0)
                continue;

            if ((btn_id = conf_state(token == JOYSTICK_ASSIGN_AXIS ?
                                     r_pane : l_pane,
                                     _(joystick_option_names[i]), "99", 0)))
            {
                joystick_option_ids[i] = btn_id;

                gui_set_state(btn_id, token, i);

                joystick_set_label(btn_id,
                                   token == JOYSTICK_ASSIGN_AXIS ?
                                   value + 11 : value);
            }
        }

        gui_filler(token == JOYSTICK_ASSIGN_AXIS ? r_pane : l_pane);

        gui_layout(id, 0, 0);
    }

    return id;
}

/*
 * This function name will be redirected to conf_controllers_gui() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_gui()` to `joystick_gui()`.
 */
#define joystick_gui conf_controllers_gui

static int conf_controllers_modal_button_gui(void)
{
    int id;

#if NB_HAVE_PB_BOTH==1
    if ((id = gui_multi(0,
                        _("Press any button on the gamepad\n"
                          "to assign. Press B to cancel."),
                        GUI_SML, GUI_COLOR_WHT)))
#else
    if ((id = gui_title_header(0, _("Press a button..."), GUI_MED, GUI_COLOR_WHT)))
#endif
        gui_layout(id, 0, 0);

    return id;
}

/*
 * This function name will be redirected to conf_controllers_modal_button_gui() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_modal_button_gui()` to `joystick_modal_button_gui()`.
 */
#define joystick_modal_button_gui conf_controllers_modal_button_gui

static int conf_controllers_modal_axis_gui(void)
{
    int id;

#if NB_HAVE_PB_BOTH==1
    if ((id = gui_multi(0,
                        _("Move any stick on the gamepad\n"
                          "to assign. Press B to cancel."),
                        GUI_SML, GUI_COLOR_WHT)))
#else
    if ((id = gui_title_header(0, _("Move a stick..."), GUI_MED, GUI_COLOR_WHT)))
#endif
        gui_layout(id, 0, 0);

    return id;
}

/*
 * This function name will be redirected to conf_controllers_modal_axis_gui() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_modal_axis_gui()` to `joystick_modal_axis_gui()`.
 */
#define joystick_modal_axis_gui conf_controllers_modal_axis_gui

static int conf_controllers_enter(struct state *st, struct state *prev, int intent)
{
    if (!joystick_back)
        joystick_back = prev;

    if (mainmenu_conf && !game_server_state() && !demo_state())
        game_client_free(NULL);

    conf_common_init(joystick_action, mainmenu_conf);

    conf_controllers_modal = 0;

    joystick_modal_button_id = joystick_modal_button_gui();
    joystick_modal_axis_id   = joystick_modal_axis_gui();

    return transition_slide(joystick_gui(), 1, intent);
}

/*
 * This function name will be redirected to conf_controllers_enter() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_enter()` to `joystick_enter()`.
 */
#define joystick_enter conf_controllers_enter

static int conf_controllers_leave(struct state *st, struct state *next, int id, int intent)
{
    conf_common_leave(st, next, id, intent);

    gui_delete(joystick_modal_button_id);
    gui_delete(joystick_modal_axis_id);

    joystick_modal_button_id = 0;
    joystick_modal_axis_id   = 0;

    controllers_modal_alpha = 0.0f;

    return transition_slide(id, 0, intent);
}

/*
 * This function name will be redirected to conf_controllers_leave() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_leave()` to `joystick_leave()`.
 */
#define joystick_leave conf_controllers_leave

static void conf_controllers_paint(int id, float t)
{
    if (mainmenu_conf && !game_server_state() && !demo_state())
    {
        video_set_perspective((float) config_get_d(CONFIG_VIEW_FOV), 0.1f, FAR_DIST);
        back_draw_easy();
    }
    else game_client_draw(0, t);

    gui_paint(id);

    if (joystick_modal == JOYSTICK_ASSIGN_BUTTON)
        gui_paint(joystick_modal_button_id);

    if (joystick_modal == JOYSTICK_ASSIGN_AXIS)
        gui_paint(joystick_modal_axis_id);

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    if (current_platform != PLATFORM_PC || console_gui_shown())
        console_gui_list_paint();
#endif
}

/*
 * This function name will be redirected to conf_controllers_paint() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_paint()` to `joystick_paint()`.
 */
#define joystick_paint conf_controllers_paint

static int conf_controllers_buttn(int b, int d)
{
    if (d)
    {
        if (joystick_modal == JOYSTICK_ASSIGN_BUTTON)
        {
            joystick_set_option(joystick_option_index, b);
            joystick_modal = 0;
            return 1;
        }
        else if (joystick_modal)
        {
            /* Allow backing out of other modal types with B. */

            if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
                joystick_modal = 0;

            return 1;
        }
    }

    return common_buttn(b, d);
}

/*
 * This function name will be redirected to conf_controllers_buttn() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_buttn()` to `joystick_buttn()`.
 */
#define joystick_buttn conf_controllers_buttn

static void conf_controllers_stick(int id, int a, float v, int bump)
{
    if (joystick_modal == JOYSTICK_ASSIGN_AXIS)
    {
        if (bump)
        {
            joystick_set_option(joystick_option_index, a);
            joystick_modal = 0;
        }

        return;
    }
    else if (joystick_modal)
    {
        /* Ignore stick motion if another type of modal is active. */
        return;
    }

    gui_pulse(gui_stick(id, a, v, bump), 1.2f);
}

/*
 * This function name will be redirected to conf_controllers_stick() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_stick()` to `joystick_stick()`.
 */
#define joystick_stick conf_controllers_stick

static void conf_controllers_timer(int id, float dt)
{
    gui_timer(id, dt);
    gui_timer(joystick_modal_button_id, dt);
    gui_timer(joystick_modal_axis_id,   dt);
    gui_alpha(id, 1 - controllers_modal_alpha);

    if (joystick_modal == JOYSTICK_ASSIGN_BUTTON)
    {
        controllers_modal_alpha = controllers_modal_alpha + (dt * 4);
        controllers_modal_alpha = CLAMP(0.0f, controllers_modal_alpha, 1.0f);
        gui_alpha(joystick_modal_button_id, controllers_modal_alpha);
    }
    else if (joystick_modal == JOYSTICK_ASSIGN_AXIS)
    {
        controllers_modal_alpha = controllers_modal_alpha + (dt * 4);
        controllers_modal_alpha = CLAMP(0.0f, controllers_modal_alpha, 1.0f);
        gui_alpha(joystick_modal_axis_id, controllers_modal_alpha);
    }
    else
    {
        controllers_modal_alpha = controllers_modal_alpha - (dt * 4);
        controllers_modal_alpha = CLAMP(0.0f, controllers_modal_alpha, 1.0f);
        gui_alpha(joystick_modal_button_id, 0);
        gui_alpha(joystick_modal_axis_id,  0);
    }
}

/*
 * This function name will be redirected to conf_controllers_timer() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_timer()` to `joystick_timer()`.
 */
#define joystick_timer conf_controllers_timer

/*---------------------------------------------------------------------------*/

static int axis_display_id;

static int conf_calibrate_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
            return exit_state(&st_conf_controls);

        case CONF_CONTROLS_CONTROLLERS_CALIBRATE:
            axis_offset_target[0] = -axis_offset_current[0];
            axis_offset_target[1] = -axis_offset_current[1];
            axis_offset_target[2] = -axis_offset_current[2];
            axis_offset_target[3] = -axis_offset_current[3];
            break;
    }

    return 1;
}

static int conf_calibrate_gui(void)
{
    int id;

    if ((id = gui_vstack(0)))
    {
        axis_display_id = gui_multi(id, "X0: -0.00; Y0: -0.00\nX1: -0.00; Y1: -0.00",
                                        GUI_SML, gui_wht, gui_yel);

        gui_space(id);
        gui_start(id, _("Calibrate"), GUI_SML, CONF_CONTROLS_CONTROLLERS_CALIBRATE, 0);

        gui_layout(id, 0, 0);
    }

    return id;
}

static int conf_calibrate_enter(struct state *st, struct state *prev, int intent)
{
    if (mainmenu_conf && !game_server_state() && !demo_state())
        game_client_free(NULL);

    conf_common_init(conf_calibrate_action, mainmenu_conf);
    return transition_slide(conf_calibrate_gui(), 1, intent);
}

static void conf_calibrate_stick(int id, int a, float v, int bump)
{
    char axisattr[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(axisattr, MAXSTR,
#else
    sprintf(axisattr,
#endif
            "X0: %.2f; Y0: %.2f\nX1: %.2f; Y1: %.2f",
            (axis_offset_current[0] + axis_offset_target[0]), (axis_offset_current[1] + axis_offset_target[1]),
            (axis_offset_current[2] + axis_offset_target[2]), (axis_offset_current[3] + axis_offset_target[3]));

    gui_set_multi(axis_display_id, axisattr);
}

/*---------------------------------------------------------------------------*/

enum
{
    CONF_NOTIFICATION_CHKP = GUI_LAST,
    CONF_NOTIFICATION_REWARD,
    CONF_NOTIFICATION_SHOP
};

static int conf_notification_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
            exit_state(&st_conf);
            while (curr_state() != &st_conf)
                exit_state(&st_conf);
            break;

        case CONF_NOTIFICATION_CHKP:
            audio_play(val != 0 ? "snd/2.2/game_button_down.ogg" : "snd/2.2/game_button_up.ogg", 1.0f);
            config_set_d(CONFIG_NOTIFICATION_CHKP, val);
            goto_state(curr_state());
            config_save();
            break;

        case CONF_NOTIFICATION_REWARD:
            audio_play(val != 0 ? "snd/2.2/game_button_down.ogg" : "snd/2.2/game_button_up.ogg", 1.0f);
            config_set_d(CONFIG_NOTIFICATION_REWARD, val);
            goto_state(curr_state());
            config_save();
            break;

        case CONF_NOTIFICATION_SHOP:
            audio_play(val != 0 ? "snd/2.2/game_button_down.ogg" : "snd/2.2/game_button_up.ogg", 1.0f);
            config_set_d(CONFIG_NOTIFICATION_SHOP, val);
            goto_state(curr_state());
            config_save();
            break;
    }

    return 1;
}

/*
 * This function name will be redirected to conf_notification_action() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_notification_action()` to `notification_action()`.
 */
#define notification_action conf_notification_action

static int conf_notification_gui(void)
{
    int id;

    /* Initialize the configuration GUI. */

    if ((id = gui_vstack(0)))
    {
        conf_header(id, _("Notifications"), GUI_BACK);

#if NB_HAVE_PB_BOTH==1
        conf_toggle_simple(id, _("Checkpoints"), CONF_NOTIFICATION_CHKP,
                               config_get_d(CONFIG_NOTIFICATION_CHKP),
                               1, 0);
        conf_toggle_simple(id, _("Extra balls"), CONF_NOTIFICATION_REWARD,
                               config_get_d(CONFIG_NOTIFICATION_REWARD),
                               1, 0);
        conf_toggle_simple(id, _("Shop"), CONF_NOTIFICATION_SHOP,
                               config_get_d(CONFIG_NOTIFICATION_SHOP),
                               1, 0);
#else
        conf_toggle(id, _("Checkpoints"), CONF_NOTIFICATION_CHKP,
                        config_get_d(CONFIG_NOTIFICATION_CHKP),
                        _("On"), 1, _("Off"), 0);
        conf_toggle(id, _("Extra balls"), CONF_NOTIFICATION_REWARD,
                        config_get_d(CONFIG_NOTIFICATION_REWARD),
                        _("On"), 1, _("Off"), 0);
        conf_toggle(id, _("Shop"), CONF_NOTIFICATION_SHOP,
                        config_get_d(CONFIG_NOTIFICATION_SHOP),
                        _("On"), 1, _("Off"), 0);
#endif

        gui_layout(id, 0, 0);
    }

    return id;
}

/*
 * This function name will be redirected to conf_notification_gui() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_notification_gui()` to `notification_gui()`.
 */
#define notification_gui conf_notification_gui

static int conf_notification_enter(struct state *st, struct state *prev, int intent)
{
    if (mainmenu_conf && !game_server_state() && !demo_state())
        game_client_free(NULL);

    conf_common_init(notification_action, mainmenu_conf);
    return transition_slide(notification_gui(), 1, intent);
}

/*
 * This function name will be redirected to conf_notification_enter() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_notification_enter()` to `notification_enter()`.
 */
#define notification_enter conf_notification_enter

/*---------------------------------------------------------------------------*/

#ifdef SWITCHBALL_GUI
static int master_id;
static int music_id;
static int sound_id;
static int narrator_id;
#else
static int master_id[11];
static int music_id[11];
static int sound_id[11];
static int narrator_id[11];
#endif

#if NB_HAVE_PB_BOTH==1
enum
{
    CONF_AUDIO_CHANGEDEVICE = GUI_LAST,
    CONF_AUDIO_MASTER_VOLUME,
    CONF_AUDIO_MUSIC_VOLUME,
    CONF_AUDIO_SOUND_VOLUME,
    CONF_AUDIO_NARRATOR_VOLUME
};

/*
 * This enum name will be redirected to CONF_AUDIO_MASTER_VOLUME for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `CONF_AUDIO_MASTER_VOLUME` to `AUDIO_MASTER_VOLUME`.
 */
#define AUDIO_MASTER_VOLUME CONF_AUDIO_MASTER_VOLUME

/*
 * This enum name will be redirected to CONF_AUDIO_MUSIC_VOLUME for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `CONF_AUDIO_MUSIC_VOLUME` to `AUDIO_MUSIC_VOLUME`.
 */
#define AUDIO_MUSIC_VOLUME CONF_AUDIO_MUSIC_VOLUME

/*
 * This enum name will be redirected to CONF_AUDIO_SOUND_VOLUME for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `CONF_AUDIO_SOUND_VOLUME` to `AUDIO_SOUND_VOLUME`.
 */
#define AUDIO_SOUND_VOLUME CONF_AUDIO_SOUND_VOLUME

/*
 * This enum name will be redirected to CONF_AUDIO_NARRATOR_VOLUME for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `CONF_AUDIO_NARRATOR_VOLUME` to `AUDIO_NARRATOR_VOLUME`.
 */
#define AUDIO_NARRATOR_VOLUME CONF_AUDIO_NARRATOR_VOLUME

#endif

static int conf_audio_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    int master   = config_get_d(CONFIG_MASTER_VOLUME);
    int sound    = config_get_d(CONFIG_SOUND_VOLUME);
    int music    = config_get_d(CONFIG_MUSIC_VOLUME);
    int narrator = config_get_d(CONFIG_NARRATOR_VOLUME);

    switch (tok)
    {
        case GUI_BACK:
            return exit_state(&st_conf);

#if NB_HAVE_PB_BOTH==1
        case AUDIO_MASTER_VOLUME:
            config_set_d(CONFIG_MASTER_VOLUME, val);
            audio_volume(val, sound, music, narrator);

#ifdef SWITCHBALL_GUI
            conf_set_slider_v2(master_id, val);
            goto_state(curr_state());
#else
            gui_toggle(master_id[val]);
            gui_toggle(master_id[master]);
#endif
            config_save();

            break;

        case AUDIO_MUSIC_VOLUME:
            config_set_d(CONFIG_MUSIC_VOLUME, val);
            audio_volume(master, sound, val, narrator);

#ifdef SWITCHBALL_GUI
            conf_set_slider_v2(music_id, val);
            goto_state(curr_state());
#else
            gui_toggle(music_id[val]);
            gui_toggle(music_id[master]);
#endif
            config_save();

            break;

        case AUDIO_SOUND_VOLUME:
            config_set_d(CONFIG_SOUND_VOLUME, val);
            audio_volume(master, val, music, narrator);
            audio_play(AUD_BUMPM, 1.0f);

#ifdef SWITCHBALL_GUI
            conf_set_slider_v2(sound_id, val);
            goto_state(curr_state());
#else
            gui_toggle(sound_id[val]);
            gui_toggle(sound_id[master]);
#endif
            config_save();

            break;

        case AUDIO_NARRATOR_VOLUME:
            config_set_d(CONFIG_NARRATOR_VOLUME, val);
            audio_volume(master, sound, music, val);

#ifdef SWITCHBALL_GUI
            conf_set_slider_v2(narrator_id, val);
            goto_state(curr_state());
#else
            gui_toggle(narrator_id[val]);
            gui_toggle(narrator_id[master]);
#endif
            config_save();

            break;
#endif
    }

    return 1;
}

/*
 * This function name will be redirected to conf_audio_action() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_audio_action()` to `audio_action()`.
 */
#define audio_action conf_audio_action

static int conf_audio_gui(void)
{
    int id;

#ifdef SWITCHBALL_GUI
    master_id   = 0;
    music_id    = 0;
    sound_id    = 0;
    narrator_id = 0;
#else
    for (int i = 0; i < 11; i++)
    {
        master_id[i]   = 0;
        music_id[i]    = 0;
        sound_id[i]    = 0;
        narrator_id[i] = 0;
    }
#endif

    if ((id = gui_vstack(0)))
    {
        conf_header(id, _("Audio"), GUI_BACK);

        int master   = config_get_d(CONFIG_MASTER_VOLUME);
        int sound    = config_get_d(CONFIG_SOUND_VOLUME);
        int music    = config_get_d(CONFIG_MUSIC_VOLUME);
        int narrator = config_get_d(CONFIG_NARRATOR_VOLUME);

#if NB_HAVE_PB_BOTH==1
#ifdef SWITCHBALL_GUI
        master_id = conf_slider_v2(id, _("Master Volume"), AUDIO_MASTER_VOLUME,
                                       master);
#else
        conf_slider(id, _("Master Volume"), AUDIO_MASTER_VOLUME, master,
                        master_id, ARRAYSIZE(master_id));
#endif

        gui_space(id);

#ifdef SWITCHBALL_GUI
        music_id = conf_slider_v2(id, _("Music Volume"), AUDIO_MUSIC_VOLUME,
                                       music);
        sound_id = conf_slider_v2(id, _("Sound Volume"), AUDIO_SOUND_VOLUME,
                                       sound);
        narrator_id = conf_slider_v2(id, _("Narrator Volume"), AUDIO_NARRATOR_VOLUME,
                                         narrator);
#else
        conf_slider(id, _("Music Volume"), AUDIO_MUSIC_VOLUME, music,
                    music_id, ARRAYSIZE(music_id));
        conf_slider(id, _("Sound Volume"), AUDIO_SOUND_VOLUME, sound,
                    sound_id, ARRAYSIZE(sound_id));
        conf_slider(id, _("Narrator Volume"), AUDIO_NARRATOR_VOLUME, narrator,
                    narrator_id, ARRAYSIZE(narrator_id));
#endif
#else
        gui_multi(id, _("Switchball's configurations\n"
                        "requires NB_HAVE_PB_BOTH=1\n"
                        "preprocessor definitions"),
                      GUI_SML, GUI_COLOR_RED);
#endif
    }
    gui_layout(id, 0, 0);

    return id;
}

/*
 * This function name will be redirected to conf_audio_gui() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_audio_gui()` to `conf_audio_gui()`.
 */
#define audio_gui conf_audio_gui

static int conf_audio_enter(struct state *st, struct state *prev, int intent)
{
    if (mainmenu_conf && !game_server_state() && !demo_state())
        game_client_free(NULL);

    conf_common_init(audio_action, mainmenu_conf);
    return transition_slide(audio_gui(), 1, intent);
}

/*
 * This function name will be redirected to conf_audio_enter() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_audio_enter()` to `audio_enter()`.
 */
#define audio_enter conf_audio_enter

/*---------------------------------------------------------------------------*/

enum
{
    CONF_SYSTEMTRANSFER_TARGET = GUI_LAST,
    CONF_SYSTEMTRANSFER_SOURCE,
    CONF_SOCIAL,
    CONF_ACCOUNT,
    CONF_GAMEPLAY,
#if NB_HAVE_PB_BOTH==1
    CONF_NOTIFICATIONS,
#else
    CONF_BALL,
#endif
    CONF_CONTROLS,
    CONF_VIDEO,
#if NB_HAVE_PB_BOTH==1
    CONF_AUDIO,
#else
    CONF_AUDIO_MASTER_VOLUME,
    CONF_AUDIO_MUSIC_VOLUME,
    CONF_AUDIO_SOUND_VOLUME,
    CONF_AUDIO_NARRATOR_VOLUME,
#endif
    CONF_LANGUAGE,
};

#if !defined(GAME_TRANSFER_TARGET) && ENABLE_GAME_TRANSFER==1
static void demo_transfer_request_addreplay_dispatch_event(int status_limit)
{
    Array items = demo_dir_scan();
    int total = array_len(items);
    if (total != 0)
    {
        demo_dir_load(items, 0, total - 1);

        for (int i = 0; i < total; i++)
        {
            struct demo *demo_data = ((struct demo *) ((struct dir_item *) array_get(items, i))->data);
            struct demo *df;

            if (!demo_data)
                continue;

            int limit = config_get_d(CONFIG_ACCOUNT_LOAD);
            int max = 0;

            if (demo_data->status == 3)
                max = 3;
            else if (demo_data->status == 1 || demo_data->status == 0)
                max = 2;
            else if (demo_data->status == 2)
                max = 1;

            if (max <= limit)
            {
                if (demo_load(df, demo_data->path))
                    transfer_addreplay(demo_data->path);
                else
                    transfer_addreplay_unsupported();
            }
            else
                transfer_addreplay_exceeded();
        }
    }
}
#endif

static int conf_action(int tok, int val)
{
    int r = 1;

#if NB_HAVE_PB_BOTH!=1
    int master   = config_get_d(CONFIG_MASTER_VOLUME);
    int sound    = config_get_d(CONFIG_SOUND_VOLUME);
    int music    = config_get_d(CONFIG_MUSIC_VOLUME);
    int narrator = config_get_d(CONFIG_NARRATOR_VOLUME);
#endif

    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
            if (mainmenu_conf && !game_server_state() && !demo_state())
                game_fade(+6.0f);

            mainmenu_conf = 1;
            return exit_state(conf_back ? conf_back : &st_title);

#if ENABLE_GAME_TRANSFER==1 && \
    !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
#ifdef GAME_TRANSFER_TARGET
        case CONF_SYSTEMTRANSFER_TARGET:
#else
        case CONF_SYSTEMTRANSFER_SOURCE:
            transfer_add_dispatch_event(demo_transfer_request_addreplay_dispatch_event);
#endif
            goto_game_transfer(curr_state());
            break;
#endif

        case CONF_SOCIAL:
            conf_goto_social(curr_state());
            break;

        case CONF_ACCOUNT:
#if NB_HAVE_PB_BOTH==1
            if (!conf_check_playername(config_get_s(CONFIG_PLAYER)))
                goto_name(&st_conf_account, &st_conf, 0, 0, 1);
            else
                goto_state(&st_conf_account);
#else
            goto_name(&st_conf_account, &st_conf, 0, 0, 1);
#endif
            break;

        case CONF_GAMEPLAY:
            goto_state(&st_conf_gameplay);
            break;

#if NB_HAVE_PB_BOTH==1
        case CONF_NOTIFICATIONS:
            goto_state(&st_conf_notification);
            break;
#endif

#if NB_HAVE_PB_BOTH!=1
        case CONF_BALL:
            /* HACK: This avoids spamming stuff */
            if (fs_exists("gui/ball.sol") &&
                (fs_exists("gui/ball.nbr") ||
                 fs_exists("gui/ball.nbrx")))
            {
                game_fade(+6.0);
                goto_state(&st_ball);
            }
            break;
#endif

        case CONF_CONTROLS:
            goto_state(&st_conf_controls);
            break;

        case CONF_VIDEO:
            goto_video(&st_conf);
            break;

#if NB_HAVE_PB_BOTH==1
        case CONF_AUDIO:
            goto_state(&st_conf_audio);
            break;
#else
        case CONF_AUDIO_MASTER_VOLUME:
            config_set_d(CONFIG_MASTER_VOLUME, val);
            audio_volume(val, sound, music, narrator);

#ifdef SWITCHBALL_GUI
            conf_set_slider_v2(master_id, val);
            goto_state(curr_state());
#else
            gui_toggle(master_id[val]);
            gui_toggle(master_id[master]);
#endif
            config_save();

            break;

        case CONF_AUDIO_MUSIC_VOLUME:
            config_set_d(CONFIG_MUSIC_VOLUME, val);
            audio_volume(master, sound, val, narrator);

#ifdef SWITCHBALL_GUI
            conf_set_slider_v2(music_id, val);
            goto_state(curr_state());
#else
            gui_toggle(music_id[val]);
            gui_toggle(music_id[master]);
#endif
            config_save();

            break;

        case CONF_AUDIO_SOUND_VOLUME:
            config_set_d(CONFIG_SOUND_VOLUME, val);
            audio_volume(master, val, music, narrator);
            audio_play(AUD_BUMPM, 1.0f);

#ifdef SWITCHBALL_GUI
            conf_set_slider_v2(sound_id, val);
            goto_state(curr_state());
#else
            gui_toggle(sound_id[val]);
            gui_toggle(sound_id[master]);
#endif
            config_save();

            break;

        case CONF_AUDIO_NARRATOR_VOLUME:
            config_set_d(CONFIG_NARRATOR_VOLUME, val);
            audio_volume(master, sound, music, val);

#ifdef SWITCHBALL_GUI
            conf_set_slider_v2(narrator_id, val);
            goto_state(curr_state());
#else
            gui_toggle(narrator_id[val]);
            gui_toggle(narrator_id[master]);
#endif
            config_save();

            break;
#endif

        case CONF_LANGUAGE:
            goto_state(&st_lang);
            break;
    }

    return r;
}

static int conf_gui(void)
{
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    int root_id = (current_platform == PLATFORM_PC && !console_gui_shown()) ?
                  gui_root() : 0;
#else
    int root_id = gui_root();
#endif

    int id, rd;

    /*
     * Initialize the configuration GUI.
     *
     * In order: Game settings, video settings, audio settings, controls settings
     */

    //if (root_id)
    {
        if ((id = gui_vstack(root_id)))
        {
            if (root_id) gui_space(id);

            conf_header(id, _("Options"), GUI_BACK);

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
#if ENABLE_GAME_TRANSFER==1
            if (mainmenu_conf && !game_server_state() && !demo_state())
            {
#ifdef GAME_TRANSFER_TARGET
                rd = conf_state(id, _("Neverball Game Transfer"), _("Start"),
                                    CONF_SYSTEMTRANSFER_TARGET);
                gui_set_color(rd, gui_wht, gui_yel);
#else
                rd = conf_state(id, _("Pennyball Transfer Tool"), _("Start"),
                                    CONF_SYSTEMTRANSFER_SOURCE);
                gui_set_color(rd, gui_wht, gui_cya);
#endif
            }
#endif

            rd = conf_state(id, _("Community (Discord)"), _("Join"), CONF_SOCIAL);
            gui_set_color(rd, gui_wht, gui_cya);

            gui_space(id);
#endif

#if NB_HAVE_PB_BOTH==1
#ifndef __EMSCRIPTEN__
            const char *conf_account_btn_txt = !conf_check_playername(config_get_s(CONFIG_PLAYER)) ?
                                               N_("Register") : N_("Manage");

            conf_state(id, _("Account"), _(conf_account_btn_txt), CONF_ACCOUNT);
#else
            if (conf_check_playername(config_get_s(CONFIG_PLAYER)))
                conf_state(id, _("Account"), _("Manage"), CONF_ACCOUNT);
#endif

            conf_state(id, _("Notifications"), _("Manage"), CONF_NOTIFICATIONS);
#endif

            gui_space(id);
            conf_state(id, _("Gameplay"), _("Configure"), CONF_GAMEPLAY);
            gui_space(id);

            if (mainmenu_conf && !game_server_state() && !demo_state()) {
                conf_state(id, _("Controls"), _("Configure"), CONF_CONTROLS);
                gui_space(id);
                conf_state(id, _("Graphics"), _("Configure"), CONF_VIDEO);
            }

            if (audio_available()) {
#if NB_HAVE_PB_BOTH==1
                conf_state(id, _("Audio"), _("Configure"), CONF_AUDIO);
#else
                int master   = config_get_d(CONFIG_MASTER_VOLUME);
                int sound    = config_get_d(CONFIG_SOUND_VOLUME);
                int music    = config_get_d(CONFIG_MUSIC_VOLUME);
                int narrator = config_get_d(CONFIG_NARRATOR_VOLUME);

#ifdef SWITCHBALL_GUI
                master_id = conf_slider_v2(id, _("Master Volume"), CONF_AUDIO_MASTER_VOLUME,
                                               master);
                music_id = conf_slider_v2(id, _("Music Volume"), CONF_AUDIO_MUSIC_VOLUME,
                                               music);
                sound_id = conf_slider_v2(id, _("Sound Volume"), CONF_AUDIO_SOUND_VOLUME,
                                              sound);
                narrator_id = conf_slider_v2(id, _("Narrator Volume"), CONF_AUDIO_NARRATOR_VOLUME,
                                                 narrator);
#else
                conf_slider(id, _("Master Volume"), CONF_AUDIO_MUSIC_VOLUME, music,
                            music_id, ARRAYSIZE(music_id));
                conf_slider(id, _("Music Volume"), CONF_AUDIO_MUSIC_VOLUME, music,
                            music_id, ARRAYSIZE(music_id));
                conf_slider(id, _("Sound Volume"), CONF_AUDIO_SOUND_VOLUME, sound,
                            sound_id, ARRAYSIZE(sound_id));
                conf_slider(id, _("Narrator Volume"), CONF_AUDIO_NARRATOR_VOLUME, narrator,
                            narrator_id, ARRAYSIZE(narrator_id));
#endif
#endif
            }

#if NB_HAVE_PB_BOTH!=1
            const char *player = config_get_s(CONFIG_PLAYER);
            const char *ball   = config_get_s(CONFIG_BALL_FILE);

            int name_id, ball_id;
            gui_space(id);
            name_id = conf_state(id, _("Player Name"), "XXXXXXXXXXXXXX",
                                     CONF_ACCOUNT);
            gui_set_trunc(name_id, TRUNC_TAIL);
            ball_id = conf_state(id, _("Ball Model"), "XXXXXXXXXXXXXX",
                                     CONF_BALL);
            gui_set_trunc(ball_id, TRUNC_TAIL);

            gui_set_label(name_id, player);
            gui_set_label(ball_id, base_name(ball));
#endif

#if NB_HAVE_PB_BOTH!=1
#if NB_EOS_SDK==0 || NB_STEAM_API==0
            if (game_server_state() || demo_state() || account_wgcl_name_read_only() ||
#ifndef __EMSCRIPTEN__
                config_playername_locked() ||
#endif
                online_mode)
#else
            if (game_server_state() || demo_state())
#endif
            {
                /*
                 * If the account is signed in e.g. PB+NB WGCL or Steam,
                 * you cannot change the player name.
                 */

                gui_set_state(name_id, GUI_NONE, 0);
                gui_set_color(name_id, GUI_COLOR_GRY);
            }
#endif

            /*if (mainmenu_conf && !game_server_state() && !demo_state()) {
#if ENABLE_NLS==1 || _WIN32
                gui_space(id);

                int lang_id;
                lang_id = conf_state(id, _("Language"), "                            ",
                                         CONF_LANGUAGE);

                gui_set_trunc(lang_id, TRUNC_TAIL);

                if (*config_get_s(CONFIG_LANGUAGE))
                    gui_set_label(lang_id, lang_name(&curr_lang));
                else
                    gui_set_label(lang_id, _("Default"));
#endif
            }*/

            gui_layout(id, 0, root_id ? +1 : 0);
        }

        if (root_id) {
            if ((id = gui_vstack(root_id))) {
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
                gui_label(id, "Neverball " VERSION " (High)", GUI_TNY, GUI_COLOR_WHT);
#endif
#if NB_HAVE_PB_BOTH==1
                gui_multi(id, _("Copyright © 2008, 2026 PennyGames\n"
                                "Neverball is free software available under the terms of GPL v2 or later."),
                              GUI_TNY, GUI_COLOR_WHT);
#else
                gui_multi(id, _("Copyright © 2026 Neverball authors\n"
                                "Neverball is free software available under the terms of GPL v2 or later."),
                              GUI_TNY, GUI_COLOR_WHT);
#endif
                gui_clr_rect(id);
                gui_layout(id, 0, -1);
            }
        }
    }

    if (root_id) gui_layout(root_id, 0, 0);
    return root_id ? root_id : id;
}

static void conf_bg_paint(float t)
{
    if (game_server_state() || demo_state())
        game_client_draw(0, t);
    else
    {
        video_set_perspective((float) config_get_d(CONFIG_VIEW_FOV), 0.1f, FAR_DIST);
        back_draw_easy();
    }
}

static int conf_enter(struct state *st, struct state *prev, int intent)
{
    if (!conf_back)
        conf_back = prev;

    if (!game_server_state() && !demo_state())
    {
        if (mainmenu_conf && prev == &st_title)
            game_fade(-6.0f);

        back_push("back/gui.png");
    }

    conf_common_bg_paint(conf_bg_paint);
    common_init(conf_action);

    return transition_slide(conf_gui(), 1, intent);
}

static int conf_leave(struct state *st, struct state *next, int id, int intent)
{
    config_save();

    if (next == conf_back)
    {
        if (!game_server_state() && !demo_state())
            back_pop();

        conf_common_bg_paint(NULL);
    }

    return transition_slide(id, 0, intent);
}

/*---------------------------------------------------------------------------*/

static void conf_shared_timer(int id, float dt)
{
    game_step_fade(dt);
    gui_timer(id, dt);
}

/*---------------------------------------------------------------------------*/

static int null_enter(struct state *st, struct state *prev, int intent)
{
    if (prev == &st_null) return 0;

#if NB_HAVE_PB_BOTH==1 && _WIN32 && _MSC_VER
    mapmarkers_quit();
#endif

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    game_transitions_quit();
#endif

    package_superwaifu_quit();

#if ENABLE_MOTIONBLUR!=0
    video_motionblur_quit();
#endif

    game_client_free_objects();
    back_free_objects();

#if ENABLE_DUALDISPLAY==1
    game_dualdisplay_gui_free();
#endif
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    console_gui_free();
#endif
    hud_free();
    transition_quit();
    gui_free();

    if (mainmenu_conf)
    {
        online_mode = 0;
        geom_free();
#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT) && defined(CONFIG_INCLUDES_MULTIBALLS)
        ball_multi_free();
#else
        ball_free();
#endif
        shad_free();
        part_free();
        mtrl_free_objects();
    }

    return 0;
}

static int null_leave(struct state *st, struct state *next, int id, int intent)
{
    if (next == &st_null) return 0;

    online_mode = 0;

    if (mainmenu_conf)
    {
        mtrl_load_objects();
        part_init();
        shad_init();
#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT) && defined(CONFIG_INCLUDES_MULTIBALLS)
        ball_multi_init();
#else
        ball_init();
#endif
        geom_init();
    }

    gui_init();
    transition_init();
    hud_init();
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    console_gui_init();
#endif
#if ENABLE_DUALDISPLAY==1
    game_dualdisplay_gui_init();
#endif

    back_load_objects();
    game_client_load_objects();

#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT) && defined(CONFIG_INCLUDES_MULTIBALLS)
    const char *ball;

    switch (ball_multi_curr()) {
        case 0:  ball = account_get_s(ACCOUNT_BALL_FILE_LL); break;
        case 1:  ball = account_get_s(ACCOUNT_BALL_FILE_L);  break;
        case 2:  ball = account_get_s(ACCOUNT_BALL_FILE_C);  break;
        case 3:  ball = account_get_s(ACCOUNT_BALL_FILE_R);  break;
        case 4:  ball = account_get_s(ACCOUNT_BALL_FILE_RR); break;
        default: ball = account_get_s(ACCOUNT_BALL_FILE_C);
    }

    account_set_s(ACCOUNT_BALL_FILE, ball);
#endif

#if ENABLE_MOTIONBLUR!=0
    video_motionblur_init();
#endif

    package_superwaifu_init();

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    game_transitions_init();
#endif

#if NB_HAVE_PB_BOTH==1 && _WIN32 && _MSC_VER
    mapmarkers_init();
#endif

    return 0;
}

/*---------------------------------------------------------------------------*/

static void conf_paint(int id, float t)
{
    if (mainmenu_conf)
    {
        video_set_perspective((float) config_get_d(CONFIG_VIEW_FOV), 0.1f, FAR_DIST);
        back_draw_easy();
    }
    else game_client_draw(0, t);

    gui_paint(id);

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    if (current_platform != PLATFORM_PC || console_gui_shown())
        console_gui_list_paint();
#endif
}

/*---------------------------------------------------------------------------*/

struct state st_conf_social = {
    social_enter,
    conf_common_leave,
    conf_common_paint,
    common_timer,
    common_point,
    common_stick,
    NULL,
    common_click,
    common_keybd,
    common_buttn
};

struct state st_conf_account = {
    account_enter,
    conf_common_leave,
    conf_common_paint,
    account_timer,
    common_point,
    common_stick,
    NULL,
    common_click,
    common_keybd,
    common_buttn
};

struct state st_conf_gameplay = {
    gameplay_enter,
    conf_common_leave,
    conf_common_paint,
    common_timer,
    common_point,
    common_stick,
    NULL,
    common_click,
    common_keybd,
    common_buttn
};

struct state st_conf_notification = {
    notification_enter,
    conf_common_leave,
    conf_common_paint,
    common_timer,
    common_point,
    common_stick,
    NULL,
    common_click,
    common_keybd,
    common_buttn
};

struct state st_conf_controls = {
    controls_enter,
    conf_common_leave,
    conf_common_paint,
    common_timer,
    common_point,
    common_stick,
    NULL,
    common_click,
    common_keybd,
    common_buttn
};

struct state st_conf_touch = {
    touch_enter,
    conf_common_leave,
    conf_common_paint,
    common_timer,
    common_point,
    common_stick,
    NULL,
    common_click,
    common_keybd,
    common_buttn
};

struct state st_conf_keybd = {
    keybd_enter,
    keybd_leave,
    keybd_paint,
    keybd_timer,
    common_point,
    common_stick,
    NULL,
    common_click,
    conf_keybd_keybd
};

struct state st_conf_controllers = {
    joystick_enter,
    conf_common_leave,
    joystick_paint,
    joystick_timer,
    common_point,
    joystick_stick,
    NULL,
    common_click,
    common_keybd,
    joystick_buttn
};

struct state st_conf_calibrate = {
    conf_calibrate_enter,
    conf_common_leave,
    conf_common_paint,
    common_timer,
    common_point,
    conf_calibrate_stick,
    NULL,
    common_click,
    common_keybd,
    common_buttn
};

struct state st_conf_audio = {
    audio_enter,
    conf_common_leave,
    conf_common_paint,
    common_timer,
    common_point,
    common_stick,
    NULL,
    common_click,
    common_keybd,
    common_buttn
};

struct state st_conf = {
    conf_enter,
    conf_leave,
    conf_paint,
    conf_shared_timer,
    common_point,
    common_stick,
    NULL,
    common_click,
    common_keybd,
    common_buttn
};

struct state st_null = {
    null_enter,
    null_leave
};
