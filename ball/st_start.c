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

#if NB_HAVE_PB_BOTH==1
#ifndef __EMSCRIPTEN__
#include "console_control_gui.h"
#endif

#include "networking.h"
#include "accessibility.h"
#include "account.h"
#include "boost_rush.h"
#include "st_intro_covid.h"
#endif

#include "gui.h"
#include "transition.h"
#include "set.h"
#include "util.h"
#include "progress.h"
#include "audio.h"
#include "config.h"
#include "common.h"
#include "key.h"
#include "text.h"

#include "activity_services.h"

#include "game_common.h"
#include "game_client.h"

#include "st_malfunction.h"
#include "st_set.h"
#include "st_level.h"
#include "st_start.h"
#include "st_title.h"
#include "st_shared.h"

#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT)
#include "st_shop.h"
#endif

/*---------------------------------------------------------------------------*/

struct state st_start_unavailable;
struct state st_start_joinrequired;
struct state st_start_upgraderequired;

/*---------------------------------------------------------------------------*/

#define LEVEL_STEP 25

static int total = 0;
static int first = 0;

enum
{
    START_CHALLENGE = GUI_LAST,
    START_BOOST_RUSH,
    START_LOCK_GOALS,
    START_LEVEL,
    START_CHECKSTARS,
    START_STARVIEWER_SHOP,
    START_OPTIONS
};

static int shot_id;
static int file_id;
static int challenge_id;

static struct state *start_back;

/*---------------------------------------------------------------------------*/

/* Create a level selector button based upon its existence and status. */

static void gui_level(int id, int i)
{
    struct level *l = get_level(i);

    const GLubyte *fore = gui_gry;
    const GLubyte *back = gui_gry;

    int jd = gui_label(id, "XXX", GUI_SML, back, fore);

    if (!l)
    {
        gui_set_label(jd, " ");
        return;
    }

    if (!str_ends_with(l->file, ".csol")  &&
        !str_ends_with(l->file, ".csolx") &&
        !str_ends_with(l->file, ".sol")   &&
        !str_ends_with(l->file, ".solx"))
    {
        gui_set_label(jd, " ");
        return;
    }

    if (level_opened(l))
    {
        if (level_master(l))
        {
            /* Master levels in this set. */

            fore = level_completed(l) ? gui_grn : gui_red;
            back = level_completed(l) ? fore    : gui_blu;
        }
        else if (str_starts_with(set_id(curr_set()), "anime"))
        {
            /* ANA levels in this set. */

            fore = level_bonus(l)     ? gui_grn :
                                        (level_completed(l) ? gui_yel :
                                                              gui_blu);
            back = level_completed(l) ? gui_grn : gui_cya;
        }
        else
        {
            fore = level_bonus(l)     ? gui_grn :
                                        (level_completed(l) ? gui_yel :
                                                              gui_red);
            back = level_completed(l) ? gui_grn : gui_yel;
        }
    }

    gui_set_trunc(jd, TRUNC_TAIL);
    gui_set_label(jd, level_name(l));

    if (level_opened(l)
#if NB_STEAM_API==0 && NB_EOS_SDK==0
     || config_cheat()
#endif
        )
    {
        gui_set_color(jd, back, fore);
        gui_set_state(jd, level_master(l) ? GUI_NONE : START_LEVEL, i);

        if (i == 0)
            gui_focus(jd);
    }
}

static void start_over_level(int i)
{
    struct level *l = get_level(i);

    if (level_opened(l)
#if NB_STEAM_API==0 && NB_EOS_SDK==0
     || config_cheat()
#endif
        )
    {
        if (shot_id)
            gui_set_image(shot_id, level_shot(l));

        gui_set_stats(l);

        set_score_board(level_score(l, SCORE_COIN), -1,
                        level_score(l, SCORE_TIME), -1,
                        level_score(l, SCORE_GOAL), -1);

#if NB_STEAM_API==0 && NB_EOS_SDK==0
        if (config_cheat() && file_id)
#endif
            gui_set_label(file_id, level_file(l));
    }
}

static void start_over(int id, int pulse)
{
    if (id)
    {
        if (pulse)
            gui_pulse(id, 1.2f);

        if (gui_token(id) == START_LEVEL)
            start_over_level(gui_value(id));
        else
        {
            if (shot_id)
                gui_set_image(shot_id, set_shot(curr_set()));

            set_score_board(set_score(curr_set(), SCORE_COIN), -1,
                            set_score(curr_set(), SCORE_TIME), -1,
                            NULL, -1);
        }
    }
}

/*---------------------------------------------------------------------------*/

static int set_star_view = 0;

static int set_level_options = 0;

#if ENABLE_MOON_TASKLOADER!=0
static int start_is_scanning_with_moon_taskloader = 0;

static int start_scan_moon_taskloader(void* data, void* execute_data)
{
    while (st_global_animating());

    set_scan_level_files();

    return 1;
}

static void start_scan_done_moon_taskloader(void* data, void* done_data)
{
    start_is_scanning_with_moon_taskloader = 0;

    goto_state(curr_state());
}
#endif

static int start_action(int tok, int val)
{
    GAMEPAD_GAMEMENU_ACTION_SCROLL(GUI_PREV, GUI_NEXT, LEVEL_STEP);

    switch (tok)
    {
        case GUI_BACK:
            if (set_star_view || set_level_options)
            {
                set_level_options = 0;
                set_star_view = 0;
                return exit_state(&st_start);
            }
            else return exit_state(start_back ? start_back : &st_set);

        case GUI_PREV:
            if (first > 1) {
                first -= LEVEL_STEP;
                return exit_state(&st_start);
            }
            break;

        case GUI_NEXT:
            if (first + LEVEL_STEP < total)
            {
                first += LEVEL_STEP;
                return goto_state(&st_start);
            }
            break;

        case START_CHALLENGE:
#if NB_STEAM_API==0 && NB_EOS_SDK==0
#if NB_HAVE_PB_BOTH==1
            if (config_cheat() ||
                !server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_CHALLENGE))
#else
            if (config_cheat())
#endif
            {
#if NB_HAVE_PB_BOTH==1
                if (server_policy_get_d(SERVER_POLICY_EDITION) < 0)
                    return goto_state(&st_start_upgraderequired);
                else if (check_handsoff())
                    return goto_handsoff(&st_start);
                else
#endif
                {
#if DEVEL_BUILD
                    progress_exit();
                    progress_init(curr_mode() == MODE_CHALLENGE ? MODE_NORMAL :
                                                                  MODE_CHALLENGE);
                    gui_toggle(challenge_id);
                    return 1;
#else
                    return goto_state(&st_start_unavailable);
#endif
                }
            }
            else
#endif
            {
#if NB_HAVE_PB_BOTH==1
                if (CHECK_ACCOUNT_ENABLED)
                {
                    if (set_balls_needed(curr_set()) > account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES))
                    {
                    }
                    else if (server_policy_get_d(SERVER_POLICY_EDITION) < 0)
                        return goto_state(&st_start_upgraderequired);
                    else if (check_handsoff())
                        return goto_handsoff(&st_start);
                    else
                    {
                        progress_exit();
                        progress_init(MODE_CHALLENGE);

                        audio_play(AUD_STARTGAME, 1.0f);

                        if (progress_play(get_level(0)))
                        {
                            activity_services_gamemode(curr_mode() == MODE_BOOST_RUSH ? AS_MODE_BOOST_RUSH :
                                                       (curr_mode() == MODE_CHALLENGE ? AS_MODE_CHALLENGE : 
                                                                                        AS_MODE_NORMAL));

                            return goto_play_level();
                        }
                    }
                }
                else return goto_state(&st_start_unavailable);
#else
                return goto_state(&st_start_joinrequired);
#endif 
            }
            break;

        case START_BOOST_RUSH:
            if (check_handsoff())
                return goto_handsoff(&st_set);
            else
            {
                boost_rush_init();

                audio_play(AUD_STARTGAME, 1.0f);

                if (progress_play(get_level(0)))
                {
                    activity_services_gamemode(curr_mode() == MODE_BOOST_RUSH ? AS_MODE_BOOST_RUSH :
                                               (curr_mode() == MODE_CHALLENGE ? AS_MODE_CHALLENGE : 
                                                                                AS_MODE_NORMAL));

                    return goto_play_level();
                }
            }
            break;

        case GUI_SCORE:
            gui_score_set(val);
            if (!is_boost_on()) start_over(gui_active(), 0);
            return 1;

        case START_LOCK_GOALS:
            config_set_d(CONFIG_LOCK_GOALS, val);
            config_save();
            return goto_state(&st_start);

        case START_LEVEL:
            if (check_handsoff())
                return goto_handsoff(&st_start);

            audio_play(AUD_STARTGAME, 1.0f);
            game_fade(+4.0);
            if (progress_play(get_level(val)))
            {
                activity_services_gamemode(curr_mode() == MODE_BOOST_RUSH ? AS_MODE_BOOST_RUSH :
                                           (curr_mode() == MODE_CHALLENGE ? AS_MODE_CHALLENGE : 
                                                                            AS_MODE_NORMAL));

                return goto_play_level();
            }

        break;

        case START_CHECKSTARS:
            set_star_view = 1;
            return goto_state(&st_start);

#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT)
        case START_STARVIEWER_SHOP:
            return goto_shop(&st_start, 0);
#endif

#if NB_HAVE_PB_BOTH==1
        case START_OPTIONS:
            set_level_options = 1;
            return goto_state(&st_start);
            break;
#endif
    }

    return 1;
}

static int start_star_view_gui(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
#if NB_HAVE_PB_BOTH==1
        if (set_star(curr_set()) > 0)
        {
            char set_star_attr[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(set_star_attr, MAXSTR,
#else
            sprintf(set_star_attr,
#endif
                    GUI_STAR " %d", set_star(curr_set()));

            const char *s0 = _("Complete the level set and earn stars\n"
                               "during playing Challenge Mode.");

            if ((jd = gui_vstack(id)))
            {
                char s_needed[MAXSTR];

                gui_label(jd, set_star_attr,
                              GUI_LRG, gui_wht, gui_yel);

#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT)
                if (set_balls_needed(curr_set()) > account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES) + 3)
                {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                    sprintf_s(s_needed, MAXSTR,
#else
                    sprintf(s_needed,
#endif
                            _("Balls needed: %d"), set_balls_needed(curr_set()));

                    gui_multi(jd, s_needed, GUI_SML, GUI_COLOR_RED);
                }
                else if (set_balls_needed(curr_set()) > account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES))
                {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                    sprintf_s(s_recommend, MAXSTR,
#else
                    sprintf(s_needed,
#endif
                            _("Balls recommended: %d"), set_balls_needed(curr_set()));

                    gui_multi(jd, s_needed, GUI_SML, GUI_COLOR_YEL);
                }
#endif

                gui_set_rect(jd, GUI_ALL);
            }

            gui_space(id);

            gui_multi(id, s0, GUI_SML, GUI_COLOR_WHT);
        }
        else
            gui_multi(id,
                      _("This set difficulty is unrated until completes\n"
                        "Challenge Mode by the developer or moderator."),
                      GUI_SML, GUI_COLOR_WHT);
#else
            gui_multi(id,
                      _("Set stars with Player level sets\n"
                        "requires Premium version."),
                      GUI_SML, GUI_COLOR_RED);
#endif

        gui_space(id);
#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT)
        if ((jd = gui_harray(id)))
        {
            gui_start(jd, _("OK"),   GUI_SML, GUI_BACK, 0);
            gui_state(jd, _("Shop"), GUI_SML, START_STARVIEWER_SHOP, 0);
        }
#else
        gui_start(id, _("OK"), GUI_SML, GUI_BACK, 0);
#endif
        gui_layout(id, 0, 0);
    }

    return id;
}

static int start_gui(void)
{
    int w = video.device_w;
    int h = video.device_h;
    int i, j;

    int id, jd, kd, ld, md;

#if ENABLE_MOON_TASKLOADER!=0
    if (start_is_scanning_with_moon_taskloader)
    {
        if ((id = gui_vstack(0)))
        {
            gui_title_header(id, _("Loading..."), GUI_MED, GUI_COLOR_DEFAULT);
            gui_space(id);
            gui_multi(id, _("Scanning Levels from Level Set..."), GUI_SML, GUI_COLOR_WHT);

            gui_layout(id, 0, 0);

            return id;
        }
        else
            return 0;
    }
#endif

    if (total == 0)
    {
        if ((id = gui_vstack(0)))
        {
            gui_title_header(id, _("No Levels"), GUI_MED, GUI_COLOR_DEFAULT);
            gui_space(id);
            gui_back_button(id);

            gui_layout(id, 0, 0);

            return id;
        }
        else
            return 0;
    }

    if ((id = gui_vstack(0)))
    {
        if ((jd = gui_hstack(id)))
        {
            char set_name_final[MAXSTR];
#if NB_HAVE_PB_BOTH==1
            if (set_star(curr_set()) > 0)
            {
                char set_star_attr[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(set_star_attr, MAXSTR,
#else
                sprintf(set_star_attr,
#endif
                        GUI_STAR " %d", set_star(curr_set()));
                
                int star_completed = 
                    set_star(curr_set()) == set_star_curr(curr_set());

                int star_btn_id = gui_label(jd,
                                            set_star_attr, GUI_SML,
                                            star_completed ? gui_yel : gui_wht,
                                            star_completed ? gui_grn : gui_yel);

                if (!star_completed)
                {
#ifdef CONFIG_INCLUDES_ACCOUNT
                    if (CHECK_ACCOUNT_BANKRUPT)
                        gui_set_color(star_btn_id, gui_red, gui_blk);
                    else
#endif
                        gui_set_state(star_btn_id, START_CHECKSTARS, 0);
                }

                gui_space(jd);
            }
#endif

            const char *curr_setname = set_name(curr_set());
            char curr_setname_final[MAXSTR];

            if (!curr_setname)
            {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(curr_setname_final, MAXSTR,
#else
                sprintf(curr_setname_final,
#endif
                        _("Untitled set name (%d)"), curr_set());
            }
            else
                SAFECPY(curr_setname_final, curr_setname);

            int set_title_id = gui_label(jd, "XXXXXXXXXXXXXXXXXX", GUI_SML, GUI_COLOR_DEFAULT);

            if (str_starts_with(set_id(curr_set()), "anime") ||
                str_starts_with(set_id(curr_set()), "SB") ||
                str_starts_with(set_id(curr_set()), "sb") ||
                str_starts_with(set_id(curr_set()), "Sb") ||
                str_starts_with(set_id(curr_set()), "sB"))
            {
                SAFECPY(set_name_final, GUI_AIRPLANE " ");
                SAFECAT(set_name_final, curr_setname_final);
            }
            else
                SAFECPY(set_name_final, curr_setname_final);

            gui_set_trunc(set_title_id, TRUNC_TAIL);
            gui_set_label(set_title_id, set_name_final);

            gui_filler(jd);
            gui_space(jd);
            gui_navig(jd, total, first, LEVEL_STEP);
        }

        gui_space(id);

        if ((jd = gui_hstack(id)))
        {
            gui_filler(jd);

            if (video.aspect_ratio >= 1.0f)
            {
#if NB_STEAM_API==0 && NB_EOS_SDK==0
                if (config_cheat())
                {
                    if ((kd = gui_vstack(jd)))
                    {
                        const int ww = MIN(w, h) / 2;
                        const int hh = ww / 4 * 3;

                        shot_id = gui_image(kd, set_shot(curr_set()),
                                                ww, hh);
                        file_id = gui_label(kd, " ", GUI_TNY, GUI_COLOR_DEFAULT);
                    }
                }
                else
#endif
                {
                    const int ww = MIN(w, h) * 7 / 12;
                    const int hh = ww / 4 * 3;

                    shot_id = gui_image(jd, set_shot(curr_set()), ww, hh);
                }
            }

            if ((kd = gui_varray(jd)))
            {
                for (i = 0; i < 5; i++)
                    if ((ld = gui_harray(kd)))
                        for (j = 4; j >= 0; j--)
                            gui_level(ld, ((i * 5) + j) + first);

#if NB_HAVE_PB_BOTH==1 && !defined(COVID_HIGH_RISK)
                if (server_policy_get_d(SERVER_POLICY_EDITION) != 0)
#endif
                {
                    if ((md = gui_harray(kd)))
                    {
                        challenge_id = gui_state(md, _("Challenge"),
                                                     GUI_SML, START_CHALLENGE, 0);

#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT)
                        if (set_balls_needed(curr_set()) > account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES) + 3)
                        {
                            gui_set_color(challenge_id, GUI_COLOR_RED);
                            gui_set_state(challenge_id, GUI_NONE, 0);
                        }
                        else if (set_balls_needed(curr_set()) > account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES))
                            gui_set_color(challenge_id, GUI_COLOR_YEL);
                        else
                            gui_set_color(challenge_id, GUI_COLOR_GRN);
#endif
                    }

#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT)
                    if (CHECK_ACCOUNT_BANKRUPT)
                    {
                        gui_set_state(challenge_id, GUI_NONE, 0);
                        gui_set_color(challenge_id, GUI_COLOR_GRY);
                    }
                    else
#endif
                        gui_set_hilite(challenge_id,
                                       curr_mode() == MODE_CHALLENGE);
                }
            }

            gui_filler(jd);
        }

        gui_space(id);
        gui_score_board(id, (GUI_SCORE_COIN |
                             GUI_SCORE_TIME |
                             GUI_SCORE_GOAL), 0, 0);
        gui_space(id);

#if NB_HAVE_PB_BOTH==1
        gui_state(id, _("Level Options"), GUI_SML, START_OPTIONS, 0);
#else
        if (video.aspect_ratio >= 1.0f)
        {
            if ((jd = gui_hstack(id)))
            {
                if ((kd = gui_harray(jd)))
                {
                    int btn0, btn1;

                    btn0 = gui_state(kd, _("Unlocked"),
                                         GUI_SML, START_LOCK_GOALS, 0);
                    btn1 = gui_state(kd, _("Locked"),
                                         GUI_SML, START_LOCK_GOALS, 1);

                    if (config_get_d(CONFIG_LOCK_GOALS))
                        gui_set_hilite(btn1, 1);
                    else
                        gui_set_hilite(btn0, 1);
                }

                gui_space(jd);

                kd = gui_label(jd, _("Goal State in Completed Levels"),
                                   GUI_SML, 0, 0);

                gui_set_trunc(kd, TRUNC_TAIL);
                gui_set_fill(kd);
            }
        }
#endif

        gui_layout(id, 0, 0);

#if NB_STEAM_API==0 && NB_EOS_SDK==0
        if (config_cheat() && file_id)
            gui_set_trunc(file_id, TRUNC_HEAD);
#endif

        set_score_board(NULL, -1, NULL, -1, NULL, -1);
    }

    return id;
}

static int start_gui_options(void)
{
    int root_id, id, jd, kd;

    if ((root_id = gui_root()))
    {
        if ((id = gui_vstack(root_id)))
        {
            if ((jd = gui_hstack(id)))
            {
                gui_label(jd, _("Level Options"), GUI_SML, GUI_COLOR_DEFAULT);
                gui_filler(jd);

#if NB_HAVE_PB_BOTH==1
#ifndef __EMSCRIPTEN__
                if (current_platform == PLATFORM_PC)
#endif
#endif
                    gui_back_button(jd);
            }

            gui_space(id);

            if ((jd = gui_hstack(id)))
            {
                if ((kd = gui_harray(jd)))
                {
                    int btn0, btn1;

#if NB_HAVE_PB_BOTH==1
                    btn1 = gui_state(kd, GUI_BALLOT_X,
                                         GUI_SML, START_LOCK_GOALS, 1);
                    btn0 = gui_state(kd, GUI_CHECKMARK,
                                         GUI_SML, START_LOCK_GOALS, 0);

                    gui_set_color(btn1, GUI_COLOR_RED);
                    gui_set_color(btn0, GUI_COLOR_GRN);
#else
                    btn0 = gui_state(kd, _("Unlocked"),
                                         GUI_SML, START_LOCK_GOALS, 0);
                    btn1 = gui_state(kd, _("Locked"),
                                         GUI_SML, START_LOCK_GOALS, 1);
#endif

                    if (config_get_d(CONFIG_LOCK_GOALS))
                        gui_set_hilite(btn1, 1);
                    else
                        gui_set_hilite(btn0, 1);
                }

                gui_space(jd);

                kd = gui_label(jd, _("Goal State in Completed Levels"),
                                   GUI_SML, 0, 0);

                gui_set_trunc(kd, TRUNC_TAIL);
                gui_set_fill(kd);
            }

            gui_layout(id, 0, 0);
        }
    }

    return root_id;
}

/*---------------------------------------------------------------------------*/

#if NB_HAVE_PB_BOTH==1

static int start_unavailable_enter(struct state *st, struct state *prev, int intent)
{
    audio_play("snd/uierror.ogg", 1.0f);

    int id;

    if ((id = gui_vstack(0)))
    {
        gui_label(id, _("Not Available"), GUI_MED, gui_gry, gui_red);
        gui_space(id);

        if (CHECK_ACCOUNT_ENABLED)
        {
            if (!server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_CHALLENGE))
                gui_multi(id, _("Challenge Mode is not available\n"
                                "with server group policy."),
                              GUI_SML, GUI_COLOR_WHT);
            else
                gui_multi(id, _("Challenge Mode is not available\n"
                                "with slowdown or cheat."),
                              GUI_SML, GUI_COLOR_WHT);
        }
        else
            gui_multi(id, _("Challenge Mode is not available.\n"
                            "Please check your account settings!"),
                          GUI_SML, GUI_COLOR_WHT);

        gui_layout(id, 0, 0);
    }

    return transition_slide(id, 1, intent);
}

static int start_unavailable_click(int b, int d)
{
    if (b == SDL_BUTTON_LEFT && d == 1)
        return goto_state(&st_start);

    return 1;
}

static int start_unavailable_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT
#ifndef __EMSCRIPTEN__
         && current_platform == PLATFORM_PC
#endif
            )
            return goto_state(&st_start);
    }
    return 1;
}

static int start_unavailable_buttn(int b, int d)
{
    if (d)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b)
         || config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return goto_state(&st_start);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int start_compat_gui()
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        gui_label(id, _("Play this level pack?"), GUI_MED, GUI_COLOR_DEFAULT);
        gui_space(id);

        const char *curr_setname = set_name(curr_set());
        char curr_setname_final[MAXSTR];

        if (!curr_setname)
        {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(curr_setname_final, MAXSTR,
#else
            sprintf(curr_setname_final,
#endif
                    _("Untitled set name (%d)"), curr_set());
        }
        else
            SAFECPY(curr_setname_final, curr_setname);

        char multiattr[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(multiattr, MAXSTR,
#else
        sprintf(multiattr,
#endif
                "%s (%s)", curr_setname_final, set_id(curr_set()));

        gui_multi(id, multiattr, GUI_SML, GUI_COLOR_WHT);

        gui_space(id);

        if ((jd = gui_harray(id)))
        {
#ifndef __EMSCRIPTEN__
            if (current_platform == PLATFORM_PC)
#endif
            {
                gui_start(jd, _("Cancel"), GUI_SML, GUI_BACK, 0);
                gui_state(jd, _("Play"), GUI_SML, START_LEVEL, 0);
            }
#ifndef __EMSCRIPTEN__
            else
                gui_start(jd, _("Play"), GUI_SML, START_LEVEL, 0);
#endif
        }

        gui_layout(id, 0, 0);
    }

    return id;
}

static int start_compat_enter(struct state *st, struct state *prev, int intent)
{
    progress_exit();
    progress_init(MODE_BOOST_RUSH);

    return transition_slide(start_compat_gui(), 1, intent);
}

#endif

static void start_paint(int id, float t)
{
    game_client_draw(0, t);

    gui_paint(id);
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    if (console_gui_show())
        console_gui_list_paint();
#endif
}

static int start_howmany()
{
    int loctotal = 1;
    int loop = 1;

    while (loop == 1)
    {
        struct level *l = get_level(loctotal);

        /* End of level track */

        if (!l) loop = 0;

        /* ...or continue to the next */

        else loctotal++;
    }

    return loctotal - 1;
}

static int start_enter(struct state *st, struct state *prev, int intent)
{
#if NB_HAVE_PB_BOTH==1
    if (str_starts_with(set_id(curr_set()), "anime"))
        audio_music_fade_to(0.5f, "bgm/jp/title.ogg", 1);
    else
        audio_music_fade_to(0.5f, is_boost_on() ? "bgm/boostrush.ogg" :
                                                  "bgm/inter_world.ogg", 1);
#else
    audio_music_fade_to(0.5f, "gui/bgm/inter.ogg", 1);
#endif

#if ENABLE_MOON_TASKLOADER!=0
    if (prev == &st_set)
    {
        start_is_scanning_with_moon_taskloader = 1;

        struct moon_taskloader_callback callback = {0};
        callback.execute = start_scan_moon_taskloader;
        callback.done    = start_scan_done_moon_taskloader;

        moon_taskloader_load(NULL, callback);

        return transition_slide(start_gui(), 1, intent);
    }
#endif

    if (set_star_view) return start_star_view_gui();

    /* Bonus levels will be unlocked automatically, if you use the bonus pack */
#if NB_HAVE_PB_BOTH==1
    if ((server_policy_get_d(SERVER_POLICY_LEVELSET_UNLOCKED_BONUS) ||
         account_get_d(ACCOUNT_PRODUCT_BONUS) == 1) &&
        server_policy_get_d(SERVER_POLICY_LEVELSET_ENABLED_BONUS))
        set_detect_bonus_product();
#endif

    if (prev == &st_set)
        first = 0;

    /* For Switchball, it uses for 30 levels */

    total = start_howmany();
    first = MIN(first, (total - 1) - ((total - 1) % LEVEL_STEP));

    progress_exit();
    progress_init(MODE_NORMAL);

    return transition_slide(set_level_options ? start_gui_options() : start_gui(), 1, intent);
}

static void start_point(int id, int x, int y, int dx, int dy)
{
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    if (current_platform == PLATFORM_PC)
        console_gui_toggle(0);
#endif

#if ENABLE_MOON_TASKLOADER
    if (start_is_scanning_with_moon_taskloader)
    {
        shared_point(id, x, y, dx, dy);
        return;
    }
#endif

    if (!set_star_view && !set_level_options)
        start_over(gui_point(id, x, y), 1);
    else
        shared_point(id, x, y, dx, dy);
}

static void start_stick(int id, int a, float v, int bump)
{
#ifndef __EMSCRIPTEN__
    console_gui_toggle(1);
#endif

#if ENABLE_MOON_TASKLOADER
    if (start_is_scanning_with_moon_taskloader)
    {
        shared_stick_basic(id, a, v, bump);
        return;
    }
#endif

    if (!set_star_view && !set_level_options)
        start_over(gui_stick(id, a, v, bump), 1);
    else
        shared_stick_basic(id, a, v, bump);
}

static int start_score(int d)
{
#if ENABLE_MOON_TASKLOADER
    if (start_is_scanning_with_moon_taskloader)
        return 1;
#endif

    if (!set_star_view && !set_level_options) return 1;

    int s = (d < 0 ?
             GUI_SCORE_PREV(gui_score_get()) :
             GUI_SCORE_NEXT(gui_score_get()));

    return start_action(GUI_SCORE, s);
}

static void start_wheel(int x, int y)
{
#if ENABLE_MOON_TASKLOADER
    if (start_is_scanning_with_moon_taskloader)
        return;
#endif
    if (!set_star_view && !set_level_options) return;

    if (y > 0) start_score(-1);
    if (y < 0) start_score(+1);
}

static int start_keybd(int c, int d)
{
#if ENABLE_MOON_TASKLOADER
    if (start_is_scanning_with_moon_taskloader)
        return 1;
#endif

    if (d)
    {
        if (c == KEY_EXIT
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
         && current_platform == PLATFORM_PC
#endif
            )
            return start_action(GUI_BACK, 0);

#if NB_STEAM_API==0 && NB_EOS_SDK==0
        if (c == SDLK_c && config_cheat()
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
         && current_platform == PLATFORM_PC
         && !set_star_view
         && !set_level_options
#endif
            )
        {
            set_cheat();
            return goto_state(&st_start);
        }
        else if (c == KEY_LEVELSHOTS && config_cheat()
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
              && current_platform == PLATFORM_PC
              && !set_star_view
              && !set_level_options
#endif
            )
        {
            char *dir = concat_string("Screenshots/shot-",
                                      set_id(curr_set()), NULL);
            int i;

            fs_mkdir(dir);

            /* Iterate over all levels, taking a screenshot of each. */

            for (i = first; i < MAXLVL_SET + first; i++)
                if (level_exists(i))
                    level_snap(i, dir);

            free(dir);
            dir = NULL;
        }
        else
#endif
        if (config_tst_d(CONFIG_KEY_SCORE_NEXT, c)
#if NB_HAVE_PB_BOTH==1
         && !set_star_view
         && !set_level_options
#endif
            )
            return start_score(+1);
    }

    return 1;
}

static int start_compat_keybd(int c, int d)
{
#if ENABLE_MOON_TASKLOADER
    if (start_is_scanning_with_moon_taskloader)
        return 1;
#endif

    if (d && (c == KEY_EXIT
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
           && current_platform == PLATFORM_PC
#endif
        ))
        return start_action(GUI_BACK, 0);

    return 1;
}

static int start_buttn(int b, int d)
{
#if ENABLE_MOON_TASKLOADER
    if (start_is_scanning_with_moon_taskloader)
        return 1;
#endif

    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return start_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return start_action(GUI_BACK, 0);
#if NB_HAVE_PB_BOTH==1
        if (!set_star_view && !set_level_options)
#endif
        {
            if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L1, b))
                return start_action(GUI_PREV, 0);
            if (config_tst_d(CONFIG_JOYSTICK_BUTTON_R1, b))
                return start_action(GUI_NEXT, 0);
        }
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

enum
{
    START_JOINREQUIRED_OPEN = GUI_LAST,
    START_JOINREQUIRED_SKIP
};

static int start_joinrequired_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
            return goto_state(&st_start);

        case START_JOINREQUIRED_OPEN:
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
#if defined(__EMSCRIPTEN__)
            EM_ASM({ window.open("https://discord.gg/qnJR263Hm2/"); }, 0);
#elif _WIN32
            system("explorer https://discord.gg/qnJR263Hm2/");
#elif defined(__APPLE__)
            system("open https://discord.gg/qnJR263Hm2/");
#elif defined(__linux__)
            system("x-www-browser https://discord.gg/qnJR263Hm2/");
#endif
#endif
            break;

        case START_JOINREQUIRED_SKIP:
            progress_exit();
            progress_init(MODE_CHALLENGE);

            audio_play(AUD_STARTGAME, 1.0f);

            if (progress_play(get_level(0)))
                return goto_play_level();

            break;
    }

    return 1;
}

static int start_upgraderequired_enter(struct state *st, struct state *prev, int intent)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("Powerups available"), GUI_MED, GUI_COLOR_DEFAULT);
        gui_space(id);
        gui_multi(id,
                  _("Pennyball offers some of the most creative ways to\n"
                    "compete with powerups! We just need you to upgrade\n"
                    "to Pro edition so that we can make sure you have\n"
                    "permission to use it."),
                  GUI_SML, GUI_COLOR_WHT);
        gui_space(id);

        if ((jd = gui_harray(id)))
        {
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
            gui_start(jd, _("Join/Upgrade"),
                          GUI_SML, START_JOINREQUIRED_OPEN, 0);
            gui_state(jd, _("Skip"), GUI_SML, START_JOINREQUIRED_SKIP, 0);
#else
            gui_start(jd, _("Skip"), GUI_SML, START_JOINREQUIRED_SKIP, 0);
#endif
            gui_state(jd, _("Cancel"), GUI_SML, GUI_BACK, 0);
        }
    }

    gui_layout(id, 0, 0);

    return transition_slide(id, 1, intent);
}

static int start_joinrequired_enter(struct state *st, struct state *prev, int intent)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("Powerups available"), GUI_MED, GUI_COLOR_DEFAULT);
        gui_space(id);
        gui_multi(id,
                  _("Pennyball offers some of the most creative ways to\n"
                    "compete with powerups! We just need you to join\n"
                    "and verify Discord server so that we can make sure\n"
                    "you have permission to use it."),
                  GUI_SML, GUI_COLOR_WHT);
        gui_space(id);

        if ((jd = gui_harray(id)))
        {
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
            gui_start(jd, _("Join"), GUI_SML, START_JOINREQUIRED_OPEN, 0);
            gui_state(jd, _("Skip"), GUI_SML, START_JOINREQUIRED_SKIP, 0);
#else
            gui_start(jd, _("Skip"), GUI_SML, START_JOINREQUIRED_SKIP, 0);
#endif
            gui_state(jd, _("Cancel"), GUI_SML, GUI_BACK, 0);
        }
    }

    gui_layout(id, 0, 0);

    return transition_slide(id, 1, intent);
}

static int start_joinrequired_keybd(int c, int d)
{
    if (d && (c == KEY_EXIT
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
           && current_platform == PLATFORM_PC
#endif
        ))
        return start_joinrequired_action(GUI_BACK, 0);
    return 1;
}

static int start_joinrequired_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return start_joinrequired_action(gui_token(active),
                                             gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return start_joinrequired_action(GUI_BACK, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

int goto_start(int index, struct state *back_state)
{
    set_goto(index);
    return goto_state(&st_start);
}

/*---------------------------------------------------------------------------*/

struct state st_start = {
    start_enter,
    shared_leave,
    start_paint,
    shared_timer,
    start_point,
    start_stick,
    shared_angle,
    shared_click,
    start_keybd,
    start_buttn,
    start_wheel
};

#if NB_HAVE_PB_BOTH==1

struct state st_start_unavailable = {
    start_unavailable_enter,
    shared_leave,
    shared_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    start_unavailable_click,
    start_unavailable_keybd,
    start_unavailable_buttn
};

struct state st_start_compat = {
    start_compat_enter,
    shared_leave,
    shared_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    start_compat_keybd,
    start_buttn
};

#endif

struct state st_start_joinrequired = {
    start_joinrequired_enter,
    shared_leave,
    shared_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    start_joinrequired_keybd,
    start_joinrequired_buttn
};

struct state st_start_upgraderequired = {
    start_upgraderequired_enter,
    shared_leave,
    shared_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    start_joinrequired_keybd,
    start_joinrequired_buttn
};
