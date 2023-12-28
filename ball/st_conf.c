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

#if NB_HAVE_PB_BOTH==1
#include "account.h"
#include "st_intro_covid.h"
#include "st_beam_style.h"
#include "networking.h"
#endif

#include "log.h"
#include "demo.h"
#include "demo_dir.h"
#include "gui.h"
#include "hud.h"
#include "geom.h"
#include "ball.h"
#include "part.h"
#include "audio.h"
#include "config.h"
#include "text.h"
#include "video.h"
#include "common.h"
#include "progress.h"
#ifndef VERSION
#include "version.h"
#endif

#include "game_common.h"
#include "game_client.h"
#include "game_server.h"

#include "st_conf.h"
#include "st_title.h"
#include "st_common.h"
#include "st_name.h"
#include "st_ball.h"
#include "st_shared.h"
#include "st_shop.h"
#if ENABLE_FETCH
#include "st_package.h"
#endif

#ifdef ENABLE_GAME_TRANSFER
#include "st_transfer.h"
#endif

#if NB_HAVE_PB_BOTH!=1 && \
    (defined(ENABLE_GAME_TRANSFER) || defined(GAME_TRANSFER_TARGET))
#error Security compilation error: Preprocessor definitions can be used it, \
       once you've transferred or joined into the target Discord Server, \
       and verified and promoted as Developer Role. \
       This invite link can be found under https://discord.gg/qnJR263Hm2/.
#endif

extern const char TITLE[];
extern const char ICON[];

/*---------------------------------------------------------------------------*/

struct state st_conf_covid_extend;
struct state st_conf_social;
struct state st_conf_notification;
struct state st_conf_control;
struct state st_conf_calibrate;
struct state st_conf_controllers;
struct state st_conf_audio;

/*---------------------------------------------------------------------------*/

static int ingame_demo = 0;
static int mainmenu_conf = 1;
static struct state *conf_back_state;

int goto_conf(struct state *back_state, int using_game, int demo)
{
    conf_back_state = back_state;

    ingame_demo = demo;
    mainmenu_conf = !using_game;

    return goto_state(&st_conf);
}

static int conf_check_playername(const char *regname)
{
    for (int i = 0; i < text_length(regname); i++)
    {
        if (regname[i] == '\\' ||
            regname[i] == '/'  ||
            regname[i] == ':'  ||
            regname[i] == '*'  ||
            regname[i] == '?'  ||
            regname[i] == '"'  ||
            regname[i] == '<'  ||
            regname[i] == '>'  ||
            regname[i] == '|')
        {
            log_errorf("Can't accept other charsets!\n", regname[i]);
            return 0;
        }
    }

    return text_length(config_get_s(CONFIG_PLAYER)) >= 3;
}

/*---------------------------------------------------------------------------*/

static int conf_covid_extend_method;

/**
 * There was an replay duplication bug in version 1.8 that gave users extra
 * replays during COVID 19. This has been fixed in 1.8.6 and those replays
 * will removed. So, if it falls before saving your replays again its
 * because they were bugged. Sorry :/
 */
int conf_covid_extended = 0;

/**
 * de_DE:
 * In Version 1.8 gab es einen Fehler beim Duplizieren von Video,
 * durch den Benutzer während COVID 19 zusätzliche Video erhielten.
 * Dieser Fehler wurde in 1.8.6 behoben und diese Video werden entfernt.
 * Wenn dein Kugel also fällt, bevor Sie Ihre Video erneut speichern,
 * liegt es daran dass sie fehlerhaft waren. Es tut uns leid :/
 *
 * fr_FR:
 * Il y avait un bogue de duplication de film dans la version 1.8 qui
 * donnait aux utilisateurs des films supplémentaires pendant COVID 19.
 * Cela a été corrigé dans la 1.8.6 et ces films seront supprimées. Donc,
 * si votre balle tombe avant de sauvegarder vos films, c'est parce
 * qu'elles ont été mises sur écoute. Pardon :/
 * 
 * es_ES:
 * Hubo un error de duplicación de repetición en la versión 1.8 que dio a
 * los usuarios repeticiónes adicionales durante COVID 19. Esto se ha corregido
 * en 1.8.6 y esas repeticiónes se eliminarán. Entonces, si tu bola cae antes
 * de guardar tus repeticiónes nuevamente, es porque tenían micrófonos.
 * Lo siento :/
 */

static struct state *returnstate;

int goto_conf_covid_extend(struct state *returnable)
{
    conf_covid_extend_method = 1;

    conf_covid_extended = 1;
    return goto_state_full(returnable, 0, 0, 0);
}

void conf_covid_retract(void)
{
    conf_covid_extended = 0;

    if (config_get_d(CONFIG_ACCOUNT_SAVE) > 2)
        config_set_d(CONFIG_ACCOUNT_SAVE, 2);
    if (config_get_d(CONFIG_ACCOUNT_LOAD) > 2)
        config_set_d(CONFIG_ACCOUNT_LOAD, 2);

    config_save();
}

static int conf_covid_extend_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
            conf_covid_extended = 0;

            if (config_get_d(CONFIG_ACCOUNT_SAVE) > 2)
                config_set_d(CONFIG_ACCOUNT_SAVE, 2);
            if (config_get_d(CONFIG_ACCOUNT_LOAD) > 2)
                config_set_d(CONFIG_ACCOUNT_LOAD, 2);

            config_save();

            goto_state(returnstate);
            break;
        case GUI_NEXT:
            if (conf_covid_extend_method == 2)
            {
                conf_covid_extended = 1;
                goto_state(returnstate);
            }
            else
            {
                conf_covid_extend_method++;
                goto_state(&st_conf_covid_extend);
            }
            break;
    }

    return 1;
}

static int conf_covid_extend_gui(void)
{
    int id, jd;
    if ((id = gui_vstack(0)))
    {
        if (conf_covid_extend_method == 2)
        {
            gui_label(id, _("Do you have your real vaccine certificates?"),
                          GUI_SML, 0, 0);
            gui_space(id);
            gui_multi(id, _("To use campaign, check your real vaccine\n"
                            "certificates with valid date\n"
                            "to switch off the replay filters!"),
                          GUI_SML, GUI_COLOR_WHT);
            gui_space(id);
        }
        else
        {
            gui_label(id, _("Do you have your FFP-2 masks?"), GUI_SML, 0, 0);
            gui_space(id);
            gui_multi(id, _("To use campaign, FFP-2 masks are required\n"
                            "to switch off the replay filters!"),
                          GUI_SML, GUI_COLOR_WHT);
            gui_space(id);
        }

        if ((jd = gui_harray(id)))
        {
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            if (current_platform == PLATFORM_PC)
#endif
            {
                gui_start(jd, _("No"), GUI_SML, GUI_BACK, 0);
                gui_state(jd, _("Yes"), GUI_SML, GUI_NEXT, 0);
            }
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            else
                gui_start(jd, _("Yes"), GUI_SML, GUI_NEXT, 0);
#endif
        }
    }
    gui_layout(id, 0, 0);
    return id;
}

static int conf_covid_extend_enter(struct state *st, struct state *prev)
{
    if (mainmenu_conf)
        game_client_free(NULL);

    conf_common_init(conf_covid_extend_action, mainmenu_conf);
    return conf_covid_extend_gui();
}

/*---------------------------------------------------------------------------*/

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

int save_id, load_id;
int online_mode;

enum
{
    CONF_ACCOUNT_SIGNIN = GUI_LAST,
    CONF_ACCOUNT_SIGNOUT,
    CONF_ACCOUNT_COVID_EXTEND,
    CONF_ACCOUNT_AUTOUPDATE,
    CONF_ACCOUNT_MAYHEM,
    CONF_ACCOUNT_TUTORIAL,
    CONF_ACCOUNT_HINT,
    CONF_ACCOUNT_PLAYER,
#if NB_HAVE_PB_BOTH==1
    CONF_ACCOUNT_BALL,
    CONF_ACCOUNT_BEAM,
#endif
    CONF_ACCOUNT_SAVE,
    CONF_ACCOUNT_LOAD
};

static int conf_account_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
            goto_state(&st_conf);
            break;

#ifdef ENABLE_SQL
        case CONF_ACCOUNT_SIGNIN:
            goto_signin(&st_conf_account, &st_conf_account, 1);
            break;
#endif

        case CONF_ACCOUNT_COVID_EXTEND:
            goto_conf_covid_extend(&st_conf_account);
            break;

        case CONF_ACCOUNT_SIGNOUT:
            break;

        case CONF_ACCOUNT_AUTOUPDATE:
            break;

        /*case CONF_ACCOUNT_MAYHEM:
            config_set_d(CONFIG_ACCOUNT_MAYHEM, val);
            goto_state(&st_conf_account);
            config_save();
            break;*/

        case CONF_ACCOUNT_TUTORIAL:
            config_set_d(CONFIG_ACCOUNT_TUTORIAL, val);
            goto_state_full(&st_conf_account, 0, 0, 1);
            config_save();
            break;

        case CONF_ACCOUNT_HINT:
            config_set_d(CONFIG_ACCOUNT_HINT, val);
            goto_state_full(&st_conf_account, 0, 0, 1);
            config_save();
            break;

    case CONF_ACCOUNT_PLAYER:
#ifdef CONFIG_INCLUDES_ACCOUNT
            goto_shop_rename(&st_conf_account, &st_conf_account, 1);
#else
            goto_name(&st_conf_account, &st_conf_account, 0, 0, 1);
#endif
        break;

#if NB_HAVE_PB_BOTH==1
        case CONF_ACCOUNT_BALL:
            if (fs_exists("gui/ball.sol") && fs_exists("gui/ball.nbr"))
            {
                game_fade(+6.0f);
                goto_state(&st_ball);
            }
            break;

        case CONF_ACCOUNT_BEAM:
            if (fs_exists("gui/beam-style.sol"))
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
                if (conf_covid_extended)
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
                if (conf_covid_extended)
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
            config_save();
            break;
    }

    return 1;
}

int time_remain_lbl_id;

static int conf_account_gui(void)
{
    online_mode = 0;
    int id;

    /* Initialize the configuration GUI. */

    if ((id = gui_vstack(0)))
    {
        const char *player = config_get_s(CONFIG_PLAYER);
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

#ifdef ENABLE_SQL
        if (online_mode)
        {
            config_set_s(CONFIG_PLAYER, config_get_s(CONFIG_ACCOUNT_ONLINE_USERNAME));
            player = config_get_s(CONFIG_PLAYER);
        }
#endif

        int save = config_get_d(CONFIG_ACCOUNT_SAVE);
        int load = config_get_d(CONFIG_ACCOUNT_LOAD);

        int name_id, ball_id, beam_id;

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
            if (nolockdown
                && CHECK_ACCOUNT_ENABLED)
            {
                char filter_introductive_attr[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(filter_introductive_attr, MAXSTR,
#else
                sprintf(filter_introductive_attr,
#endif
                        CONF_ACCOUNT_DEMO_LOCKED_DESC_INTRODUCTIVE,
                        _(status_to_str(3)));

                time_remain_lbl_id = gui_multi(id,
                                               "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n"
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

#ifdef ENABLE_SQL
        int signin_id = gui_state(id, _("Sign in as Neverball"),
                                      GUI_SML, CONF_ACCOUNT_SIGNIN, 0);

        if (online_mode)
        {
            gui_set_state(signin_id, CONF_ACCOUNT_SIGNOUT, 0);
            gui_set_label(signin_id, _("Sign out from Neverball"));
        }
        gui_space(id);
#endif

        //conf_toggle(id, _("Mayhem"), CONF_ACCOUNT_MAYHEM,
            //config_get_d(CONFIG_ACCOUNT_MAYHEM), _("On"), 1, _("Off"), 0);

        //gui_space(id);

        if (mainmenu_conf)
        {
#if ENABLE_FETCH
#if NB_HAVE_PB_BOTH==1
            conf_toggle_simple(id, _("Auto-Update"), CONF_ACCOUNT_AUTOUPDATE,
                                   config_get_d(CONFIG_ACCOUNT_AUTOUPDATE), 1, 0);
#else
            conf_toggle(id, _("Auto-Update"), CONF_ACCOUNT_AUTOUPDATE,
                            config_get_d(CONFIG_ACCOUNT_AUTOUPDATE), _("On"), 1, _("Off"), 0);
#endif
            gui_space(id);
#endif
        }

#if NB_HAVE_PB_BOTH==1
        conf_toggle_simple(id, _("Show Tutorial"), CONF_ACCOUNT_TUTORIAL,
                               config_get_d(CONFIG_ACCOUNT_TUTORIAL),
                               1, 0);
        conf_toggle_simple(id, _("Show Hint"), CONF_ACCOUNT_HINT,
                               config_get_d(CONFIG_ACCOUNT_HINT), 
                               1, 0);
#else
        conf_toggle(id, _("Show Tutorial"), CONF_ACCOUNT_TUTORIAL,
                        config_get_d(CONFIG_ACCOUNT_TUTORIAL),
                        _("On"), 1, _("Off"), 0);
        conf_toggle(id, _("Show Hint"), CONF_ACCOUNT_HINT,
                        config_get_d(CONFIG_ACCOUNT_HINT), 
                        _("On"), 1, _("Off"), 0);
#endif

        gui_space(id);
        if (mainmenu_conf)
        {
            name_id = conf_state(id, _("Player Name"), "XXXXXXXXXXXXXX",
                                     CONF_ACCOUNT_PLAYER);
#if NB_HAVE_PB_BOTH==1
            ball_id = conf_state(id, _("Ball Model"), "XXXXXXXXXXXXXX",
                                     CONF_ACCOUNT_BALL);
#ifdef CONFIG_INCLUDES_ACCOUNT
            beam_id = conf_state(id, _("Beam Style"), "XXXXXXXXXXXXXX",
                                     CONF_ACCOUNT_BEAM);
#endif
#endif

            gui_set_trunc(name_id, TRUNC_TAIL);
            gui_set_trunc(ball_id, TRUNC_TAIL);
#ifdef CONFIG_INCLUDES_ACCOUNT
            gui_set_trunc(beam_id, TRUNC_TAIL);
#endif

#if NB_HAVE_PB_BOTH==1
#if NB_EOS_SDK==0 || NB_STEAM_API==0
            if (online_mode || !account_changeable)
#endif
            {
                /*
                 * If the account is signed in e.g. Steam,
                 * you cannot change the player name.
                 */

                gui_set_state(name_id, GUI_NONE, 0);
                gui_set_color(name_id, GUI_COLOR_GRY);
            }
#endif

            gui_space(id);

            gui_set_label(name_id, player);
#if defined(CONFIG_INCLUDES_ACCOUNT) && defined(CONFIG_INCLUDES_MULTIBALLS)
            gui_set_label(ball_id, _("Manage"));
#else
            gui_set_label(ball_id, base_name(ball));
#endif

            const char *beam_version_name = "";

#ifdef CONFIG_INCLUDES_ACCOUNT
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
#endif

            gui_set_label(beam_id, beam_version_name);
        }

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
                             !ingame_demo && !mainmenu_conf ?
                             GUI_NONE : CONF_ACCOUNT_SAVE);
        load_id = conf_state(id, _("Replay Filter"), "XXXXXXXXXXXXXX",
                             CONF_ACCOUNT_LOAD);

        if (!ingame_demo && !mainmenu_conf)
            gui_set_color(save_id, GUI_COLOR_GRY);

        gui_set_trunc(save_id, TRUNC_TAIL);
        gui_set_trunc(load_id, TRUNC_TAIL);

        gui_set_label(save_id, savefilter);
        gui_set_label(load_id, loadfilter);

        gui_layout(id, 0, 0);
    }

    return id;
}

static int conf_account_enter(struct state *st, struct state *prev)
{
    if (prev == &st_ball) game_fade(-6.0f);

    if (mainmenu_conf)
        game_client_free(NULL);

    conf_common_init(conf_account_action, mainmenu_conf);
    return conf_account_gui();
}

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
        if (config_get_d(CONFIG_ACCOUNT_SAVE) > 2 && save_id && load_id)
        {
            gui_set_label(save_id, CONF_ACCOUNT_DEMO_FILTER_CURR_OPTTION_2);
            gui_set_label(load_id, CONF_ACCOUNT_DEMO_FILTER_CURR_OPTTION_2);
        }

        conf_covid_retract();
        config_save();

        if (time_remain_lbl_id != 0)
            gui_set_multi(time_remain_lbl_id,
                          CONF_ACCOUNT_DEMO_LOCKED_DESC_NIGHT);
    }
}

/*---------------------------------------------------------------------------*/

static int conf_join_confirm = 0;

enum
{
    CONF_SOCIAL_DISCORD = GUI_LAST,
};

static int conf_social_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    char linkstr_cmd[MAXSTR], linkstr_code[64];

    switch (tok)
    {
        case GUI_BACK:
            conf_join_confirm = 0;
            return goto_state(&st_conf);
            break;
        case CONF_SOCIAL_DISCORD:
#if NB_HAVE_PB_BOTH==1
            if (conf_join_confirm)
#endif
            {
#if defined(__EMSCRIPTEN__)
#if NB_HAVE_PB_BOTH==1
                EM_ASM({ window.open("https://discord.gg/qnJR263Hm2");  }, 0);
#else
                EM_ASM({ window.open("https://discord.gg/HhMfr4N6H6"); }, 0);
#endif
#else
#if NB_HAVE_PB_BOTH==1
                SAFECPY(linkstr_code, "qnJR263Hm2");
#else
                SAFECPY(linkstr_code, "HhMfr4N6H6");
#endif

#if _WIN32
                SAFECPY(linkstr_cmd, "start msedge https://discord.gg/");
#elif defined(__APPLE__)
                SAFECPY(linkstr_cmd, "open https://discord.gg/");
#elif defined(__linux__)
                SAFECPY(linkstr_cmd, "x-www-browser https://discord.gg/");
#endif

                SAFECAT(linkstr_cmd, linkstr_code);

                system(linkstr_cmd);
#endif

                if (mainmenu_conf)
                    return 0; /* bye! */
            }
#if NB_HAVE_PB_BOTH==1
            else
            {
                conf_join_confirm = 1;
                goto_state(curr_state());
            }
#endif
            break;
    }
    return 1;
}

static int conf_social_gui(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("Join Discord?"), GUI_MED, 0, 0);

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
                                "new members after joined, before send community messages,\n"
                                "connect voice chats and watch streaming."),
                              GUI_SML, GUI_COLOR_WHT);

#if NB_HAVE_PB_BOTH==1
            if (mainmenu_conf && conf_join_confirm)
#endif
                gui_label(jd, _("This may take a while, and the game will then exit."),
                              GUI_SML, gui_twi, gui_vio);

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

static int conf_social_enter(struct state *st, struct state *prev)
{
    if (mainmenu_conf)
        game_client_free(NULL);

    conf_common_init(conf_social_action, mainmenu_conf);
    return conf_social_gui();
}

/*---------------------------------------------------------------------------*/

/*
 * Should be set the preset keys as well?
 */
#define CONF_CONTROL_SET_PRESET_KEYS(cam_tgl, cam1, cam2, \
                                     cam3, camL, camR, axYP, \
                                     axXN, axYN, axXP)   \
    do {                                                 \
        config_set_d(CONFIG_KEY_CAMERA_TOGGLE, cam_tgl); \
        config_set_d(CONFIG_KEY_CAMERA_1, cam1);         \
        config_set_d(CONFIG_KEY_CAMERA_2, cam2);         \
        config_set_d(CONFIG_KEY_CAMERA_3, cam3);         \
        config_set_d(CONFIG_KEY_CAMERA_L, camL);         \
        config_set_d(CONFIG_KEY_CAMERA_R, camR);         \
        config_set_d(CONFIG_KEY_FORWARD, axYP);          \
        config_set_d(CONFIG_KEY_LEFT, axXN);             \
        config_set_d(CONFIG_KEY_BACKWARD, axYN);         \
        config_set_d(CONFIG_KEY_RIGHT, axXP);            \
    } while (0)

enum
{
    CONF_CONTROL_INPUT_PRESET = GUI_LAST,
    CONF_CONTROL_TILTING_FLOOR,
    CONF_CONTROL_CAMERA_ROTATE_MODE,
    CONF_CONTROL_MOUSE_SENSE,
    CONF_CONTROL_INVERT_MOUSE_Y,
    CONF_CONTROL_CHANGECONTROLLERS,
    CONF_CONTROL_CALIBRATE
};

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
}

static int conf_control_action(int tok, int val)
{
    int mouse = MOUSE_RANGE_MAP(config_get_d(CONFIG_MOUSE_SENSE));

    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
            goto_state(&st_null);
            goto_state(&st_conf);
            break;

        case CONF_CONTROL_INPUT_PRESET:
            control_set_input();
            config_save();
            break;

        case CONF_CONTROL_TILTING_FLOOR:
            config_set_d(CONFIG_TILTING_FLOOR, val);
            config_save();
            goto_state_full(&st_conf_control, 0, 0, 1);
            break;

        case CONF_CONTROL_CAMERA_ROTATE_MODE:
#ifdef SWITCHBALL_GUI
            config_tgl_d(CONFIG_CAMERA_ROTATE_MODE);
            gui_set_label(camrot_mode_id,
                          config_get_d(CONFIG_CAMERA_ROTATE_MODE) == 1 ?
                          _("Inverted") : _("Normal"));
#endif
            break;

        case CONF_CONTROL_MOUSE_SENSE:
            config_set_d(CONFIG_MOUSE_SENSE, MOUSE_RANGE_UNMAP(val));

#ifdef SWITCHBALL_GUI
            conf_set_slider_v2(mouse_id, val);
            goto_state_full(curr_state(), 0, 0, 1);
#else
            gui_toggle(mouse_id[val]);
            gui_toggle(mouse_id[mouse]);
#endif
            config_save();
            break;

        case CONF_CONTROL_INVERT_MOUSE_Y:
            config_set_d(CONFIG_MOUSE_INVERT, val);
            config_save();
            goto_state_full(&st_conf_control, 0, 0, 1);
        break;

        case CONF_CONTROL_CHANGECONTROLLERS:
            goto_state(&st_conf_controllers);
            break;

        case CONF_CONTROL_CALIBRATE:
            goto_state(&st_conf_calibrate);
            break;
    }

    return 1;
}

int conf_control_gui(void)
{
    int id;

    /* Initialize the configuration GUI. */

    if ((id = gui_vstack(0)))
    {
        int mouse = MOUSE_RANGE_MAP(config_get_d(CONFIG_MOUSE_SENSE));

        conf_header(id, _("Controls"), GUI_BACK);

        preset_id = conf_state(id, _("Preset"), "XXXXXXXXXXXXX",
                                   CONF_CONTROL_INPUT_PRESET);

        const char *presetname = _("Custom");

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

        gui_set_label(preset_id, presetname);

#if NB_HAVE_PB_BOTH==1
        conf_toggle_simple(id, _("Tilting Floor"), CONF_CONTROL_TILTING_FLOOR,
                               config_get_d(CONFIG_TILTING_FLOOR),
                               1, 0);
#else
        conf_toggle(id, _("Tilting Floor"), CONF_CONTROL_TILTING_FLOOR,
                        config_get_d(CONFIG_TILTING_FLOOR),
                        _("On"), 1, _("Off"), 0);
#endif

#ifdef SWITCHBALL_GUI
        camrot_mode_id = conf_state(id, _("Camera rotate"),
                                    config_get_d(CONFIG_CAMERA_ROTATE_MODE) == 1 ?
                                    _("Inverted") : _("Normal"),
                                    CONF_CONTROL_CAMERA_ROTATE_MODE);
#endif

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        if (current_platform == PLATFORM_PC)
#endif
        {
            gui_space(id);

#ifdef SWITCHBALL_GUI
            mouse_id = conf_slider_v2(id, _("Mouse Sensitivity"), CONF_CONTROL_MOUSE_SENSE,
                                      mouse);
#else
            conf_slider(id, _("Mouse Sensitivity"), CONF_CONTROL_MOUSE_SENSE,
                            mouse, mouse_id, ARRAYSIZE(mouse_id));
#endif

#if NB_HAVE_PB_BOTH==1
            conf_toggle_simple(id, _("Invert Y Axis"), CONF_CONTROL_INVERT_MOUSE_Y,
                                   config_get_d(CONFIG_MOUSE_INVERT),
                                   1, 0);
#else
            conf_toggle(id, _("Invert Y Axis"), CONF_CONTROL_INVERT_MOUSE_Y,
                            config_get_d(CONFIG_MOUSE_INVERT),
                            _("On"), 1, _("Off"), 0);
#endif
        }

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        if (current_platform != PLATFORM_PC)
#endif
        {
            gui_space(id);

            conf_state(id, _("Gamepad"), _("Change"), CONF_CONTROL_CHANGECONTROLLERS);
            conf_state(id, _("Axis"), _("Calibrate"), CONF_CONTROL_CALIBRATE);
        }

        gui_layout(id, 0, 0);
    }

    return id;
}

static int conf_control_enter(struct state *st, struct state *prev)
{
    if (mainmenu_conf)
        game_client_free(NULL);

    conf_common_init(conf_control_action, mainmenu_conf);
    return conf_control_gui();
}

/*---------------------------------------------------------------------------*/

enum
{
    CONF_CONTROLLERS_ASSIGN_BUTTON = GUI_LAST,
    CONF_CONTROLLERS_ASSIGN_AXIS,
};

static struct state *conf_controllers_back;

static int conf_controllers_modal_button_id;
static int conf_controllers_modal_axis_id;

static int conf_controllers_modal;

static int conf_controllers_option_index;

static float controllers_modal_alpha = 0.0f;

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

static int conf_controllers_option_ids[ARRAYSIZE(conf_controllers_options)];

static void conf_controllers_set_label(int id, int value)
{
    char str[20];

#if NEVERBALL_FAMILY_API == NEVERBALL_XBOX_FAMILY_API || \
    NEVERBALL_FAMILY_API == NEVERBALL_XBOX_360_FAMILY_API
    if (conf_controllers_option_values_xbox[value % 100000])
    {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(str, 256,
#else
        sprintf(str,
#endif
                "%s", conf_controllers_option_values_xbox[value % 100000]);
    }
#elif NEVERBALL_FAMILY_API == NEVERBALL_PS_FAMILY_API
    if (conf_controllers_option_values_ps[value % 100000])
    {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(str, 256,
#else
        sprintf(str,
#endif
                "%s", conf_controllers_option_values_ps[value % 100000]);
    }
#elif NEVERBALL_FAMILY_API == NEVERBALL_STEAMDECK_FAMILY_API
    if (conf_controllers_option_values_steamdeck[value % 100000])
    {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(str, 256,
#else
        sprintf(str,
#endif
                "%s", conf_controllers_option_values_steamdeck[value % 100000]);
    }
#elif NEVERBALL_FAMILY_API == NEVERBALL_SWITCH_FAMILY_API
    if (conf_controllers_option_values_switch[value % 100000])
    {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(str, 256,
#else
        sprintf(str,
#endif
                "%s", conf_controllers_option_values_switch[value % 100000]);
    }
#elif NEVERBALL_FAMILY_API == NEVERBALL_HANDSET_FAMILY_API
    if (conf_controllers_option_values_switch[value % 100000])
    {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(str, 256,
#else
        sprintf(str,
#endif
                "%s", conf_controllers_option_values_switch[value % 100000]);
    }
#else
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(str, 256,
#else
    sprintf(str,
#endif
            "%d", value % 100000);
#endif

#if NEVERBALL_FAMILY_API != NEVERBALL_PC_FAMILY_API
    else SAFECPY(str, "");
#endif

    gui_set_label(id, str);
}

static void conf_controllers_set_option(int index, int value)
{
    if (index < ARRAYSIZE(conf_controllers_options))
    {
        int option = *conf_controllers_options[index];

        config_set_d(option, value);

        conf_controllers_set_label(conf_controllers_option_ids[index], value);

        /* Focus the next button. */

        if (index < ARRAYSIZE(conf_controllers_options) - 1)
        {
            /* Skip over marker, if any. */

            if (index < ARRAYSIZE(conf_controllers_options) - 2 &&
                conf_controllers_options[index + 1] == NULL)
                gui_focus(conf_controllers_option_ids[index + 2]);
            else
                gui_focus(conf_controllers_option_ids[index + 1]);
        }
    }
}

static int conf_controllers_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
            if (conf_controllers_modal)
            conf_controllers_modal = 0;
            else
            {
                goto_state(conf_controllers_back);
                while (curr_state() != conf_controllers_back)
                {
                    goto_state(conf_controllers_back);
                    conf_controllers_back = NULL;
                }
            }
            break;

        case CONF_CONTROLLERS_ASSIGN_BUTTON:
        case CONF_CONTROLLERS_ASSIGN_AXIS:
            conf_controllers_modal = tok;
            conf_controllers_option_index = val;
            break;
    }

    return 1;
}

static int conf_controllers_gui(void)
{
    int id, jd, l_pane, r_pane;

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

        int token = CONF_CONTROLLERS_ASSIGN_BUTTON;
        int i;

        for (i = 0; i < ARRAYSIZE(conf_controllers_options); ++i)
        {
            int btn_id;
            int value;

            /* Check for marker. */

            if (i == 10)
            {
                /* Switch the GUI token / assignment type. */
                token = CONF_CONTROLLERS_ASSIGN_AXIS;

                gui_filler(id);

                continue;
            }

            if (conf_controllers_options[i] == 0)
                continue;

            value = config_get_d(*conf_controllers_options[i]);

            if ((btn_id = conf_state(token == CONF_CONTROLLERS_ASSIGN_AXIS ?
                                     r_pane : l_pane,
                _(conf_controllers_option_names[i]), "99", 0)))
            {
                conf_controllers_option_ids[i] = btn_id;

                gui_set_state(btn_id, token, i);

                conf_controllers_set_label(btn_id,
                                           token == CONF_CONTROLLERS_ASSIGN_AXIS ?
                                           value + 11 : value);
            }
        }

        gui_filler(token == CONF_CONTROLLERS_ASSIGN_AXIS ? r_pane : l_pane);

        gui_layout(id, 0, 0);
    }

    return id;
}

static int conf_controllers_modal_button_gui(void)
{
    int id;

    if ((id = gui_label(0, _("Press a button..."), GUI_MED, GUI_COLOR_WHT)))
        gui_layout(id, 0, 0);

    return id;
}

static int conf_controllers_modal_axis_gui(void)
{
    int id;

    if ((id = gui_label(0, _("Move a stick..."), GUI_MED, GUI_COLOR_WHT)))
        gui_layout(id, 0, 0);

    return id;
}

static int conf_controllers_enter(struct state *st, struct state *prev)
{
    if (!conf_controllers_back)
        conf_controllers_back = prev;

    if (mainmenu_conf)
        game_client_free(NULL);

    conf_common_init(conf_controllers_action, mainmenu_conf);

    conf_controllers_modal = 0;

    conf_controllers_modal_button_id = conf_controllers_modal_button_gui();
    conf_controllers_modal_axis_id   = conf_controllers_modal_axis_gui();

    return conf_controllers_gui();
}

static void conf_controllers_leave(struct state *st, struct state *next, int id)
{
    conf_common_leave(st, next, id);

    gui_delete(conf_controllers_modal_button_id);
    gui_delete(conf_controllers_modal_axis_id);

    controllers_modal_alpha = 0.0f;
}

static void conf_controllers_paint(int id, float t)
{
    if (mainmenu_conf)
    {
        video_push_persp((float) config_get_d(CONFIG_VIEW_FOV), 0.1f, FAR_DIST);
        {
            back_draw_easy();
        }
        video_pop_matrix();
    }
    else
        game_client_draw(0, t);

    gui_paint(id);

    if (conf_controllers_modal == CONF_CONTROLLERS_ASSIGN_BUTTON)
        gui_paint(conf_controllers_modal_button_id);

    if (conf_controllers_modal == CONF_CONTROLLERS_ASSIGN_AXIS)
        gui_paint(conf_controllers_modal_axis_id);
}

static int conf_controllers_buttn(int b, int d)
{
    if (d)
    {
        if (conf_controllers_modal == CONF_CONTROLLERS_ASSIGN_BUTTON)
        {
            conf_controllers_set_option(conf_controllers_option_index, b);
            conf_controllers_modal = 0;
            return 1;
        }
        else if (conf_controllers_modal)
        {
            /* Allow backing out of other modal types with B. */

            if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
                conf_controllers_modal = 0;

            return 1;
        }
    }

    return common_buttn(b, d);
}

static void conf_controllers_stick(int id, int a, float v, int bump)
{
    if (conf_controllers_modal == CONF_CONTROLLERS_ASSIGN_AXIS)
    {
        if (bump)
        {
            conf_controllers_set_option(conf_controllers_option_index, a);
            conf_controllers_modal = 0;
        }

        return;
    }
    else if (conf_controllers_modal)
    {
        /* Ignore stick motion if another type of modal is active. */
        return;
    }

    gui_pulse(gui_stick(id, a, v, bump), 1.2f);
}

static void conf_controllers_timer(int id, float dt)
{
    gui_timer(id, dt);
    gui_alpha(id, 1 - controllers_modal_alpha);

    if (conf_controllers_modal == CONF_CONTROLLERS_ASSIGN_BUTTON)
    {
        controllers_modal_alpha = controllers_modal_alpha + dt;
        controllers_modal_alpha = CLAMP(0.0f, controllers_modal_alpha, 1.0f);
        gui_alpha(conf_controllers_modal_button_id, controllers_modal_alpha);
    }
    else if (conf_controllers_modal == CONF_CONTROLLERS_ASSIGN_AXIS)
    {
        controllers_modal_alpha = controllers_modal_alpha + dt;
        controllers_modal_alpha = CLAMP(0.0f, controllers_modal_alpha, 1.0f);
        gui_alpha(conf_controllers_modal_axis_id, controllers_modal_alpha);
    }
    else
    {
        controllers_modal_alpha = controllers_modal_alpha - dt;
        gui_alpha(conf_controllers_modal_button_id, 0);
        gui_alpha(conf_controllers_modal_axis_id, 0);
    }
}

/*---------------------------------------------------------------------------*/

static int calibrate_method;
static int axis_title_id;
static int axis_display_id;
static float calib_x0, calib_y0, calib_x1, calib_y1;

static void change_method(void)
{
    char titleattr[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(titleattr, MAXSTR,
#else
    sprintf(titleattr,
#endif
            _("Method %i"), calibrate_method);

    gui_set_label(axis_title_id, titleattr);
}

static int conf_calibrate_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
            return goto_state(&st_conf_control);
        case CONF_CONTROL_CALIBRATE:
            if (calibrate_method == 2)
            {
                axis_offset[0]   = -calib_x0;
                axis_offset[1]   = -calib_y0;
                calibrate_method = 1;
            }
            else
            {
                axis_offset[2]   = -calib_x1;
                axis_offset[3]   = -calib_y1;
                calibrate_method = 2;
            }

            change_method();
            break;
    }

    return 1;
}

static int conf_calibrate_gui(void)
{
    int id;

    if ((id = gui_vstack(0)))
    {
        axis_title_id = gui_label(id, _("Method -"), GUI_SML, 0, 0);
        gui_space(id);
        axis_display_id = gui_label(id, "super-long-control-axis-display",
                                        GUI_SML, gui_wht, gui_yel);
        gui_set_label(axis_display_id ,"X: -; Y: -");

        char titleattr[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(titleattr, MAXSTR,
#else
        sprintf(titleattr,
#endif
                _("Method %i"), calibrate_method);

        gui_set_label(axis_title_id, titleattr);

        gui_space(id);

        gui_start(id, _("Calibrate"), GUI_SML, CONF_CONTROL_CALIBRATE, 0);

        gui_layout(id, 0, 0);
    }

    return id;
}

static int conf_calibrate_enter(struct state *st, struct state *prev)
{
    calibrate_method = 1;
    calib_x0 = 0; calib_x1 = 0; calib_y0 = 0; calib_y1 = 0;

    if (mainmenu_conf)
        game_client_free(NULL);

    conf_common_init(conf_calibrate_action, mainmenu_conf);
    return conf_calibrate_gui();
}

void conf_calibrate_stick(int id, int a, float v, int bump)
{
    char axisattr[MAXSTR];

    if (calibrate_method == 1)
    {
        if (config_tst_d(CONFIG_JOYSTICK_AXIS_X0, a))
            calib_x0 = v;
        else if (config_tst_d(CONFIG_JOYSTICK_AXIS_Y0, a))
            calib_y0 = v;

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(axisattr, MAXSTR,
#else
        sprintf(axisattr,
#endif
                "X: %f; Y: %f",
                (calib_x0 + axis_offset[0]), (calib_y0 + axis_offset[1]));
    }
    else if (calibrate_method == 2)
    {
        if (config_tst_d(CONFIG_JOYSTICK_AXIS_X1, a))
            calib_x1 = v;
        else if (config_tst_d(CONFIG_JOYSTICK_AXIS_Y1, a))
            calib_y1 = v;
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(axisattr, MAXSTR,
#else
        sprintf(axisattr,
#endif
                "X: %f; Y: %f",
                (calib_x1 + axis_offset[2]), (calib_y1 + axis_offset[3]));
    }
    else
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(axisattr, MAXSTR,
#else
        sprintf(axisattr,
#endif
                "X: -; Y: -");

    gui_set_label(axis_display_id, axisattr);
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
            goto_state(&st_conf);
            while (curr_state() != &st_conf)
                goto_state(&st_conf);
        break;

        case CONF_NOTIFICATION_CHKP:
            config_set_d(CONFIG_NOTIFICATION_CHKP, val);
            goto_state_full(&st_conf_notification, 0, 0, 1);
            config_save();
            break;

        case CONF_NOTIFICATION_REWARD:
            config_set_d(CONFIG_NOTIFICATION_REWARD, val);
            goto_state_full(&st_conf_notification, 0, 0, 1);
            config_save();
            break;

        case CONF_NOTIFICATION_SHOP:
            config_set_d(CONFIG_NOTIFICATION_SHOP, val);
            goto_state_full(&st_conf_notification, 0, 0, 1);
            config_save();
            break;
    }

    return 1;
}

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

static int conf_notification_enter(struct state *st, struct state *prev)
{
    if (mainmenu_conf)
        game_client_free(NULL);

    conf_common_init(conf_notification_action, mainmenu_conf);
    return conf_notification_gui();
}

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
#if NB_HAVE_PB_BOTH==1
        case CONF_AUDIO_MASTER_VOLUME:
            config_set_d(CONFIG_MASTER_VOLUME, val);
            audio_volume(val, sound, music, narrator);

#ifdef SWITCHBALL_GUI
            conf_set_slider_v2(master_id, val);
            goto_state_full(curr_state(), 0, 0, 1);
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
            goto_state_full(curr_state(), 0, 0, 1);
#else
            gui_toggle(music_id[val]);
            gui_toggle(music_id[master]);
#endif
            config_save();

            break;

        case CONF_AUDIO_SOUND_VOLUME:
            config_set_d(CONFIG_SOUND_VOLUME, val);
            audio_volume(master, val, music, narrator);
            audio_play(AUD_BUMPM, 1.f);

#ifdef SWITCHBALL_GUI
            conf_set_slider_v2(sound_id, val);
            goto_state_full(curr_state(), 0, 0, 1);
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
            conf_set_slider_v2(narrator, val);
            goto_state_full(curr_state(), 0, 0, 1);
#else
            gui_toggle(narrator_id[val]);
            gui_toggle(narrator_id[master]);
#endif
            config_save();

            break;
#endif
        case GUI_BACK:
            return goto_state(&st_conf);
            break;
    }

    return 1;
}

static int conf_audio_gui(void)
{
    int id;

    if ((id = gui_vstack(0)))
    {
        conf_header(id, _("Audio"), GUI_BACK);

        int master   = config_get_d(CONFIG_MASTER_VOLUME);
        int sound    = config_get_d(CONFIG_SOUND_VOLUME);
        int music    = config_get_d(CONFIG_MUSIC_VOLUME);
        int narrator = config_get_d(CONFIG_NARRATOR_VOLUME);

#if NB_HAVE_PB_BOTH==1
#ifdef SWITCHBALL_GUI
        master_id = conf_slider_v2(id, _("Master Volume"), CONF_AUDIO_MASTER_VOLUME,
                                       master);
#else
        conf_slider(id, _("Master Volume"), CONF_AUDIO_MASTER_VOLUME, master,
                        master_id, ARRAYSIZE(master_id));
#endif

        gui_space(id);

#ifdef SWITCHBALL_GUI
        music_id = conf_slider_v2(id, _("Music Volume"), CONF_AUDIO_MUSIC_VOLUME,
                                       music);
        sound_id = conf_slider_v2(id, _("Sound Volume"), CONF_AUDIO_SOUND_VOLUME,
                                       sound);
        narrator_id = conf_slider_v2(id, _("Narrator Volume"), CONF_AUDIO_NARRATOR_VOLUME,
                                         narrator);
#else
        conf_slider(id, _("Music Volume"), CONF_AUDIO_MUSIC_VOLUME, music,
                    music_id, ARRAYSIZE(music_id));
        conf_slider(id, _("Sound Volume"), CONF_AUDIO_SOUND_VOLUME, sound,
                    sound_id, ARRAYSIZE(sound_id));
        conf_slider(id, _("Narrator Volume"), CONF_AUDIO_NARRATOR_VOLUME, narrator,
                    narrator_id, ARRAYSIZE(narrator_id));
#endif
#else
        gui_multi(id, _("Switchball configurations\n"
                        "requires NB_HAVE_PB_BOTH\n"
                        "preprocessor definitions"),
                      GUI_SML, GUI_COLOR_RED);
#endif
    }
    gui_layout(id, 0, 0);

    return id;
}

static int conf_audio_enter(struct state *st, struct state *prev)
{
    if (mainmenu_conf)
        game_client_free(NULL);

    conf_common_init(conf_audio_action, mainmenu_conf);
    return conf_audio_gui();
}

/*---------------------------------------------------------------------------*/

enum
{
    CONF_SYSTEMTRANSFER_TARGET = GUI_LAST,
    CONF_SYSTEMTRANSFER_SOURCE,
    CONF_SOCIAL,
    CONF_MANAGE_ACCOUNT,
    CONF_PACKAGES,
#if NB_HAVE_PB_BOTH==1
    CONF_MANAGE_NOTIFICATIONS,
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

#if GAME_TRANSFER_TARGET==0 && ENABLE_GAME_TRANSFER==1
void demo_transfer_request_addreplay_dispatch_event(int status_limit)
{
    Array items = demo_dir_scan();
    int total = array_len(items);
    if (total != 0)
    {
        demo_dir_load(items, 0, total - 1);

        for (int i = 0; i < total; i++)
        {
            struct demo *demo_data = ((struct demo *) ((struct dir_item *) array_get(items, i))->data);

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
                transfer_addreplay(demo_data->path);
            else
                transfer_addreplay_exceeded();
        }
    }
}
#endif

static int conf_action(int tok, int val)
{
    int r = 1;

    int master   = config_get_d(CONFIG_MASTER_VOLUME);
    int sound    = config_get_d(CONFIG_SOUND_VOLUME);
    int music    = config_get_d(CONFIG_MUSIC_VOLUME);
    int narrator = config_get_d(CONFIG_NARRATOR_VOLUME);

    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
            if (mainmenu_conf)
                game_fade(+6.0f);

            goto_state(conf_back_state);
            break;

#if ENABLE_GAME_TRANSFER==1
#if GAME_TRANSFER_TARGET==1
        case CONF_SYSTEMTRANSFER_TARGET:
#else
        case CONF_SYSTEMTRANSFER_SOURCE:
            transfer_add_dispatch_event(demo_transfer_request_addreplay_dispatch_event);
#endif
            goto_state(&st_transfer);
            break;
#endif

        case CONF_SOCIAL:
            goto_state(&st_conf_social);
            break;

        case CONF_MANAGE_ACCOUNT:
#if NB_HAVE_PB_BOTH==1
            if (!conf_check_playername(config_get_s(CONFIG_PLAYER)))
                goto_name(&st_conf_account, &st_conf, 0, 0, 1);
            else
                goto_state(&st_conf_account);
#else
            goto_name(&st_conf_account, &st_conf, 0, 0, 1);
#endif
            break;

#if NB_HAVE_PB_BOTH==1
        case CONF_MANAGE_NOTIFICATIONS:
            goto_state(&st_conf_notification);
            break;
#endif

#if NB_HAVE_PB_BOTH!=1
        case CONF_PACKAGES:
#if ENABLE_FETCH
            return goto_package(curr_state());
#endif
            break;

        case CONF_BALL:
            // HACK: This avoids spamming stuff
            if (fs_exists("gui/ball.sol") && fs_exists("gui/ball.nbr"))
            {
                game_fade(+6.0);
                goto_state(&st_ball);
            }
            break;
#endif

        case CONF_CONTROLS:
            goto_state(&st_conf_control);
            break;

        case CONF_VIDEO:
            goto_state(&st_video);
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
            goto_state_full(curr_state(), 0, 0, 1);
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
            goto_state_full(curr_state(), 0, 0, 1);
#else
            gui_toggle(music_id[val]);
            gui_toggle(music_id[master]);
#endif
            config_save();

            break;

        case CONF_AUDIO_SOUND_VOLUME:
            config_set_d(CONFIG_SOUND_VOLUME, val);
            audio_volume(master, val, music, narrator);
            audio_play(AUD_BUMPM, 1.f);

#ifdef SWITCHBALL_GUI
            conf_set_slider_v2(sound_id, val);
            goto_state_full(curr_state(), 0, 0, 1);
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
            goto_state_full(curr_state(), 0, 0, 1);
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
    int root_id;

    /*
     * Initialize the configuration GUI.
     *
     * In order: Game settings, video settings, audio settings, controls settings
     */

    if ((root_id = gui_root()))
    {
        int id;

        if ((id = gui_vstack(root_id)))
        {
            conf_header(id, _("Options"), GUI_BACK);

#if ENABLE_GAME_TRANSFER==1
            if (mainmenu_conf)
            {
#if GAME_TRANSFER_TARGET==1
                conf_state(id, _("Neverball Game Transfer"), _("Start"),
                               CONF_SYSTEMTRANSFER_TARGET);
#else
                conf_state(id, _("Pennyball Transfer Tool"), _("Start"),
                               CONF_SYSTEMTRANSFER_SOURCE);
#endif
                gui_space(id);
            }
#endif

            int rd = conf_state(id, _("Community (Discord)"), _("Join"), CONF_SOCIAL);
            gui_set_color(rd, gui_wht, gui_cya);

            gui_space(id);

#if NB_HAVE_PB_BOTH==1
            conf_state(id, _("Account"), _(!conf_check_playername(config_get_s(CONFIG_PLAYER)) ?
                                           "Register" : "Manage"), CONF_MANAGE_ACCOUNT);

            conf_state(id, _("Notifications"), _("Manage"), CONF_MANAGE_NOTIFICATIONS);

#if ENABLE_FETCH
            if (CHECK_ACCOUNT_ENABLED)
                conf_state(id, _("Addons"), _("Manage"), CONF_PACKAGES);
#endif
            gui_space(id);
#endif

            conf_state(id, _("Controls"), _("Configure"), CONF_CONTROLS);

            if (mainmenu_conf)
            {
                gui_space(id);
                conf_state(id, _("Graphics"), _("Configure"), CONF_VIDEO);
            }

            if (audio_available())
            {
                gui_space(id);
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
                                     CONF_MANAGE_ACCOUNT);
            gui_set_trunc(name_id, TRUNC_TAIL);
            ball_id = conf_state(id, _("Ball Model"), "XXXXXXXXXXXXXX",
                                     CONF_BALL);
            gui_set_trunc(ball_id, TRUNC_TAIL);

            gui_set_label(name_id, player);
            gui_set_label(ball_id, base_name(ball));
#endif

#if NB_HAVE_PB_BOTH!=1
#if NB_EOS_SDK==0 || NB_STEAM_API==0
            if (online_mode)
#endif
            {
                /*
                 * If the account is signed in e.g. Steam,
                 * you cannot change the player name.
                 */

                gui_set_state(name_id, GUI_NONE, 0);
                gui_set_color(name_id, GUI_COLOR_GRY);
            }
#endif

            if (mainmenu_conf)
            {
#if ENABLE_NLS==1 || _WIN32
                int lang_id;
#if NB_HAVE_PB_BOTH==1
                gui_space(id);
#endif
                lang_id = conf_state(id, _("Language"), "                            ",
                                         CONF_LANGUAGE);

                gui_set_trunc(lang_id, TRUNC_TAIL);

                if (*config_get_s(CONFIG_LANGUAGE))
                    gui_set_label(lang_id, lang_name(&curr_lang));
                else
                    gui_set_label(lang_id, _("Default"));
#endif
            }

            gui_layout(id, 0, 0);
        }

        if ((id = gui_vstack(root_id)))
        {
            gui_label(id, "Neverball " VERSION, GUI_TNY, GUI_COLOR_WHT);
            gui_multi(id, "Copyright © 2023 Neverball authors\n"
                          "Neverball is free software available under the terms of GPL v2 or later.",
                          GUI_TNY, GUI_COLOR_WHT);
            gui_clr_rect(id);
            gui_layout(id, 0, -1);
        }
    }

    gui_layout(root_id, 0, 0);

    return root_id;
}

static int conf_enter(struct state *st, struct state *prev)
{
    if (mainmenu_conf && prev == &st_title)
        game_fade(-6.0f);

    conf_common_init(conf_action, mainmenu_conf);
    return conf_gui();
}

static void conf_leave(struct state *st, struct state *next, int id)
{
    conf_common_leave(st, next, id);
}

/*---------------------------------------------------------------------------*/

static void conf_shared_timer(int id, float dt)
{
    game_step_fade(dt);
    gui_timer(id, dt);
}

/*---------------------------------------------------------------------------*/

static int null_enter(struct state *st, struct state *prev)
{
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
    xbox_control_gui_free();
#endif
    hud_free();
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

static void null_leave(struct state *st, struct state *next, int id)
{
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
    hud_init();
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
    xbox_control_gui_init();
#endif

    if (mainmenu_conf)
    {
#ifdef ENABLE_SQL
        if (database_init(config_get_s(CONFIG_ACCOUNT_ONLINE_USERNAME),
                          config_get_s(CONFIG_ACCOUNT_ONLINE_PASSWORD)))
            if (database_signin() > 0) online_mode = 1;
#endif
    }

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
}

/*---------------------------------------------------------------------------*/

void conf_paint(int id, float t)
{
    if (mainmenu_conf)
    {
        video_push_persp((float) config_get_d(CONFIG_VIEW_FOV), 0.1f, FAR_DIST);
        {
            back_draw_easy();
        }
        video_pop_matrix();
    }
    else
        game_client_draw(0, t);

    gui_paint(id);
}

/*---------------------------------------------------------------------------*/

struct state st_conf_covid_extend = {
    conf_covid_extend_enter,
    conf_leave,
    conf_paint,
    conf_shared_timer,
    common_point,
    common_stick,
    NULL,
    common_click,
    common_keybd,
    common_buttn,
    NULL,
    NULL,
    NULL,
};

struct state st_conf_account = {
    conf_account_enter,
    conf_leave,
    conf_paint,
    conf_account_timer,
    common_point,
    common_stick,
    NULL,
    common_click,
    common_keybd,
    common_buttn,
    NULL,
    NULL,
    NULL,
};

struct state st_conf_social = {
    conf_social_enter,
    conf_leave,
    conf_paint,
    common_timer,
    common_point,
    common_stick,
    NULL,
    common_click,
    common_keybd,
    common_buttn,
    NULL,
    NULL,
    NULL,
};

struct state st_conf_notification = {
    conf_notification_enter,
    conf_leave,
    conf_paint,
    common_timer,
    common_point,
    common_stick,
    NULL,
    common_click,
    common_keybd,
    common_buttn,
    NULL,
    NULL,
    NULL,
};

struct state st_conf_control = {
    conf_control_enter,
    conf_leave,
    conf_paint,
    common_timer,
    common_point,
    common_stick,
    NULL,
    common_click,
    common_keybd,
    common_buttn,
    NULL,
    NULL,
    NULL,
};

struct state st_conf_calibrate = {
    conf_calibrate_enter,
    conf_leave,
    conf_paint,
    common_timer,
    common_point,
    conf_calibrate_stick,
    NULL,
    common_click,
    common_keybd,
    common_buttn,
    NULL,
    NULL,
    NULL,
};

struct state st_conf_controllers = {
    conf_controllers_enter,
    conf_leave,
    conf_paint,
    conf_controllers_timer,
    common_point,
    conf_controllers_stick,
    NULL,
    common_click,
    common_keybd,
    conf_controllers_buttn,
    NULL,
    NULL,
    NULL,
};

struct state st_conf_audio = {
    conf_audio_enter,
    conf_leave,
    conf_paint,
    common_timer,
    common_point,
    common_stick,
    NULL,
    common_click,
    common_keybd,
    common_buttn,
    NULL,
    NULL,
    NULL,
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
    common_buttn,
    NULL,
    NULL,
    NULL,
};

struct state st_null = {
    null_enter,
    null_leave,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};
