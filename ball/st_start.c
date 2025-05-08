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

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#if NB_HAVE_PB_BOTH==1
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
#include "game_server.h"
#include "game_client.h"

#include "st_malfunction.h"
#include "st_set.h"
#include "st_level.h"
#include "st_start.h"
#include "st_title.h"
#include "st_common.h"
#include "st_shared.h"

#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT)
#include "st_shop.h"
#endif

/*---------------------------------------------------------------------------*/

struct state st_start_unavailable;
struct state st_start_upgraderequired;
struct state st_start_signinrequired;
struct state st_start_joinrequired;

/*---------------------------------------------------------------------------*/

#define LEVEL_STEP 25

static int total = 0;
static int first = 0;

enum
{
    START_CHALLENGE = GUI_LAST,
    START_BOOST_RUSH,
    START_LOCK_GOALS,
    START_HARDCORE,
    START_LEVEL,
    START_CHECKSTARS,
    START_STARVIEWER_SHOP,
    START_OPTIONS
};

static int shot_id;
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
static int file_id;
#endif
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

    if (!l ||
        (!str_ends_with(l->file, ".csol")  &&
         !str_ends_with(l->file, ".csolx") &&
         !str_ends_with(l->file, ".sol")   &&
         !str_ends_with(l->file, ".solx")))
    {
        gui_set_label(jd, " ");
        gui_set_hidden(jd, 1);
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
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
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
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
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

#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
        if (config_cheat() && file_id)
            gui_set_label(file_id, level_file(l));
#endif
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
    //while (st_global_animating());

    set_scan_level_files();

    return 1;
}

static void start_scan_done_moon_taskloader(void* data, void* done_data)
{
    start_is_scanning_with_moon_taskloader = 0;

    goto_state(curr_state());
}
#endif

static int   start_play_level_index;
static int   start_play_level_pending;
static float start_play_level_state_time;

static int set_level_play(int i)
{
    const struct level *l = campaign_get_level(i);

    if (!l) return 0;

#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    EM_ASM({
        Neverball.gamecore_levelmap_load($0, UTF8ToString($1), false);
    }, l->num_indiv_theme, l->song);

    start_play_level_state_time = time_state() + 0.2;
#else
    start_play_level_state_time = time_state();
#endif

    start_play_level_index   = i;
    start_play_level_pending = 1;

    audio_play(AUD_STARTGAME, 1.0f);
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    return 0;
#else
    return progress_play(l);
#endif
}

static int set_level_play_timer(float dt)
{
    if (start_play_level_pending &&
        time_state() >= start_play_level_state_time)
    {
        start_play_level_pending = 0;

        if (progress_play(get_level(start_play_level_index)))
        {
            activity_services_mode_update((enum activity_services_mode) curr_mode());

            return goto_play_level();
        }
    }

    return 0;
}

static int start_action(int tok, int val)
{
    GAMEPAD_GAMEMENU_ACTION_SCROLL(GUI_PREV, GUI_NEXT, LEVEL_STEP);

    int have_online_session_data = 0;

#ifdef CONFIG_INCLUDES_ACCOUNT
#ifdef __EMSCRIPTEN__
    have_online_session_data = EM_ASM_INT({ return tmp_online_session_data != undefined &&
                                                   tmp_online_session_data != null ? 1 : 0; });
#endif

    const int curr_balls =
        server_policy_get_d(SERVER_POLICY_EDITION) > 0 ?
        account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES) : 3;
#else
    const int curr_balls = 3;
#endif

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

        case START_HARDCORE:
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            progress_exit();
            progress_init(val ? MODE_HARDCORE : MODE_NORMAL);
            return goto_state(&st_start);
#endif
            break;

        case START_CHALLENGE:
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            if (curr_mode() == MODE_HARDCORE)
            {
                if (check_handsoff())
                    return goto_handsoff(curr_state());
                else if (CHECK_ACCOUNT_ENABLED)
                {
                    if (set_level_play(0))
                    {
                        activity_services_mode_update(AS_MODE_HARDCORE);

                        return goto_play_level();
                    }
                }
                else return goto_state(&st_start_unavailable);
            }
#endif

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
                    return goto_handsoff(curr_state());
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
                    if (set_balls_needed(curr_set()) > curr_balls)
                    {
                    }
#ifdef __EMSCRIPTEN__
                    else if (!have_online_session_data)
                        return goto_state(&st_start_signinrequired);
#endif
                    else if (server_policy_get_d(SERVER_POLICY_EDITION) < 0)
                        return goto_state(&st_start_upgraderequired);
                    else if (check_handsoff())
                        return goto_handsoff(curr_state());
                    else
                    {
                        progress_exit();
                        progress_init(MODE_CHALLENGE);

                        if (set_level_play(0))
                        {
                            activity_services_mode_update(curr_mode() == MODE_BOOST_RUSH ? AS_MODE_BOOST_RUSH :
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

                if (set_level_play(0))
                {
                    activity_services_mode_update(curr_mode() == MODE_BOOST_RUSH ? AS_MODE_BOOST_RUSH :
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
                return goto_handsoff(curr_state());

            progress_exit();
            progress_init(MODE_NORMAL);
            game_fade(+4.0);

            if (set_level_play(val))
            {
                activity_services_mode_update(curr_mode() == MODE_BOOST_RUSH ? AS_MODE_BOOST_RUSH :
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

#ifdef CONFIG_INCLUDES_ACCOUNT
                const int curr_balls =
                    server_policy_get_d(SERVER_POLICY_EDITION) > 0 ?
                    account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES) : 3;
#else
                const int curr_balls = 0;
#endif

                const int lbl_id = gui_label(jd, set_star_attr,
                                                 GUI_LRG, gui_wht, gui_yel);
                gui_set_font(lbl_id, "ttf/DejaVuSans-Bold.ttf");

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                if (curr_mode() == MODE_HARDCORE);
                else
#endif
#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT)
                if (set_balls_needed(curr_set()) > curr_balls + 3)
                {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                    sprintf_s(s_needed, MAXSTR,
#else
                    sprintf(s_needed,
#endif
                            _("Balls needed: %d"), set_balls_needed(curr_set()));

                    gui_multi(jd, s_needed, GUI_SML, GUI_COLOR_RED);
                }
                else if (set_balls_needed(curr_set()) > curr_balls)
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
                ; /* None what to say */

                gui_set_rect(jd, GUI_ALL);
            }

            gui_space(id);

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            if (curr_mode() == MODE_HARDCORE)
                gui_multi(id, _("Some products are not purchasable\n"
                                "because you selected hardcore mode."),
                              GUI_SML, GUI_COLOR_WHT);
            else
#endif
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

            if (server_policy_get_d(SERVER_POLICY_EDITION) > 0)
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
                gui_set_font(star_btn_id, "ttf/DejaVuSans-Bold.ttf");

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

            SAFECPY(set_name_final, curr_setname_final);

            gui_set_trunc(set_title_id, TRUNC_TAIL);
            gui_set_label(set_title_id, set_name_final);

            gui_filler(jd);
            gui_space(jd);
            gui_navig(jd, total, first, LEVEL_STEP);
        }

        if ((jd = gui_vstack(id)))
        {
            gui_space(jd);

            if ((kd = gui_hstack(jd)))
            {
                gui_filler(kd);

                if (video.aspect_ratio >= 1.0f)
                {
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
                    if (config_cheat())
                    {
                        if ((ld = gui_vstack(kd)))
                        {
                            const int ww = MIN(w, h) / 2;
                            const int hh = ww / 4 * 3;

                            shot_id = gui_image(ld, set_shot(curr_set()),
                                                    ww, hh);
                            file_id = gui_label(ld, " ", GUI_TNY, GUI_COLOR_DEFAULT);
                        }
                    }
                    else
#endif
                    {
                        const int ww = MIN(w, h) * 7 / 12;
                        const int hh = ww / 4 * 3;

                        shot_id = gui_image(kd, set_shot(curr_set()), ww, hh);
                    }
                }

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                if ((ld = curr_mode() == MODE_HARDCORE ? gui_vstack(kd) :
                                                         gui_varray(kd)))
#else
                if ((ld = gui_varray(kd)))
#endif
                {
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                    if (curr_mode() == MODE_HARDCORE)
                    {
                        gui_multi(ld, _("You can't respawn if you die\n"
                                        "or finish levels for each.\n"
                                        "Good luck! You'll need it.\n"),
                                      GUI_SML, GUI_COLOR_RED);
                        gui_filler(ld);

                        challenge_id = gui_state(ld, _("Start Game"),
                                                     GUI_SML, START_CHALLENGE, 0);
                    }
                    else
#endif
                    {
                        for (i = 0; i < 5; i++)
                            if ((md = gui_harray(ld)))
                                for (j = 4; j >= 0; j--)
                                    gui_level(md, ((i * 5) + j) + first);

#if NB_HAVE_PB_BOTH==1 && !defined(COVID_HIGH_RISK)
                        if (server_policy_get_d(SERVER_POLICY_EDITION) != 0)
#endif
                        {
#ifdef CONFIG_INCLUDES_ACCOUNT
                            const int curr_balls =
                                server_policy_get_d(SERVER_POLICY_EDITION) > 0 ?
                                account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES) : 3;
#else
                            const int curr_balls = 0;
#endif

                            if ((md = gui_harray(ld)))
                            {
                                challenge_id = gui_state(md, _("Challenge"),
                                                             GUI_SML, START_CHALLENGE, 0);

#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT)
                                if (set_balls_needed(curr_set()) > curr_balls + 3)
                                {
                                    gui_set_color(challenge_id, GUI_COLOR_RED);
                                    gui_set_state(challenge_id, GUI_NONE, 0);
                                }
                                else if (set_balls_needed(curr_set()) > curr_balls)
                                    gui_set_color(challenge_id, GUI_COLOR_YEL);
                                else
                                    gui_set_color(challenge_id, GUI_COLOR_GRN);

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
                    }
                }

                gui_filler(kd);
            }

            const int scoreboard_flags = (GUI_SCORE_COIN | GUI_SCORE_TIME |
                                          (curr_mode() == MODE_HARDCORE ? 0 : GUI_SCORE_GOAL));

            gui_space(jd);
            gui_score_board(jd, scoreboard_flags, 0, 0);

#if NB_HAVE_PB_BOTH==1
#ifndef __EMSCRIPTEN__
            if (!console_gui_shown())
#endif
            {
                gui_space(jd);
                gui_state(jd, _("Level Options"), GUI_SML, START_OPTIONS, 0);
            }
#else
            gui_space(jd);

            if (video.aspect_ratio >= 1.0f)
                if ((kd = gui_hstack(jd)))
                {
                    if ((ld = gui_harray(kd)))
                    {
                        int btn0, btn1;

                        btn0 = gui_state(ld, _("Unlocked"),
                                             GUI_SML, START_LOCK_GOALS, 0);
                        btn1 = gui_state(ld, _("Locked"),
                                             GUI_SML, START_LOCK_GOALS, 1);

                        if (config_get_d(CONFIG_LOCK_GOALS))
                            gui_set_hilite(btn1, 1);
                        else
                            gui_set_hilite(btn0, 1);
                    }

                    gui_space(kd);

                    ld = gui_label(kd, _("Goal State in Completed Levels"),
                                       GUI_SML, 0, 0);

                    gui_set_trunc(ld, TRUNC_TAIL);
                    gui_set_fill(ld);
                }
#endif
        }

        gui_layout(id, 0, 0);

#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
        if (config_cheat() && file_id)
            gui_set_trunc(file_id, TRUNC_HEAD);
#endif

        set_score_board(NULL, -1, NULL, -1, NULL, -1);
    }

    return id;
}

static int start_gui_options(void)
{
    int id, jd, kd;

    if ((id = gui_vstack(0)))
    {
        if ((jd = gui_hstack(id)))
        {
            gui_label(jd, _("Level Options"), GUI_SML, GUI_COLOR_DEFAULT);
            gui_filler(jd);

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            if (current_platform == PLATFORM_PC && !console_gui_shown())
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
                btn1 = gui_state(kd, GUI_CROSS,
                                     GUI_SML, START_LOCK_GOALS, 1);
                btn0 = gui_state(kd, GUI_CHECKMARK,
                                     GUI_SML, START_LOCK_GOALS, 0);

                gui_set_font(btn1, "ttf/DejaVuSans-Bold.ttf");
                gui_set_font(btn0, "ttf/DejaVuSans-Bold.ttf");

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

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        /* OK, how about hardcore mode? */

        const int hardc_requirement = accessibility_get_d(ACCESSIBILITY_SLOWDOWN) >= 100 &&
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
                                      !config_cheat() &&
#endif
                                      (!config_get_d(CONFIG_SMOOTH_FIX) || video_perf() >= NB_FRAMERATE_MIN) &&
                                      server_policy_get_d(SERVER_POLICY_EDITION) > 0;

#ifdef CONFIG_INCLUDES_ACCOUNT
        const int hardc_available = !CHECK_ACCOUNT_BANKRUPT;
#else
        const int hardc_available = 0;
#endif
        if (hardc_requirement && hardc_available)
        {
            gui_space(id);
#ifdef SWITCHBALL_GUI
            conf_toggle_simple(id, _("Hardcore Mode"), START_HARDCORE,
                                   curr_mode() == MODE_HARDCORE,
                                   1, 0);
#else
            conf_toggle(id, _("Hardcore Mode"), START_HARDCORE,
                            curr_mode() == MODE_HARDCORE,
                            _("On"), 1, _("Off"), 0);
#endif
        }
#endif

        gui_layout(id, 0, 0);
    }

    return id;
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
        return exit_state(&st_start);

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
            return exit_state(&st_start);
    }
    return 1;
}

static int start_unavailable_buttn(int b, int d)
{
    if (d)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b)
         || config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return exit_state(&st_start);
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
    if (console_gui_shown())
    {
        if (!set_star_view && !set_level_options)
            console_gui_levelopt_paint();
        else
            console_gui_list_paint();
    }
#endif
}

static void start_timer(int id, float dt)
{
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    set_level_play_timer(dt);
#endif

    gui_timer(id, dt);
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

    if (set_star_view)
        return transition_slide(start_star_view_gui(), 1, intent);

    /* Bonus levels will be unlocked automatically, if you use the bonus pack */
#if NB_HAVE_PB_BOTH==1
    if ((server_policy_get_d(SERVER_POLICY_LEVELSET_UNLOCKED_BONUS) ||
         account_get_d(ACCOUNT_PRODUCT_BONUS) == 1) &&
        server_policy_get_d(SERVER_POLICY_LEVELSET_ENABLED_BONUS))
        set_detect_bonus_product();
#endif

    if (prev == &st_set)
    {
        first = 0;

        progress_exit();
        progress_init(MODE_NORMAL);
    }

    /* For Switchball, it uses for 30 levels */

    total = start_howmany();
    first = MIN(first, (total - 1) - ((total - 1) % LEVEL_STEP));

    if (prev == &st_start)
        return transition_page(set_level_options ? start_gui_options() : start_gui(), 1, intent);

    return transition_slide(set_level_options ? start_gui_options() : start_gui(), 1, intent);
}

static int start_leave(struct state *st, struct state *next, int id, int intent)
{
    if (next == &st_null)
    {
        progress_exit();

        campaign_quit();
        set_quit();

        game_server_free(NULL);
        game_client_free(NULL);
    }

    if (next == &st_start)
        return transition_page(id, 0, intent);

    return transition_slide(id, 0, intent);
}

static void start_point(int id, int x, int y, int dx, int dy)
{
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

#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
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
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_Y, b))
            return start_action(START_OPTIONS, 0);
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
    START_JOINREQUIRED_SIGNIN,
    START_JOINREQUIRED_SKIP
};

static int start_joinrequired_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
            return exit_state(&st_start);

        case START_JOINREQUIRED_OPEN:
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
#if defined(__EMSCRIPTEN__)
            EM_ASM({ window.open("https://discord.gg/qnJR263Hm2/"); });
#elif _WIN32
            system("explorer https://discord.gg/qnJR263Hm2/");
#elif defined(__APPLE__)
            system("open https://discord.gg/qnJR263Hm2/");
#elif defined(__linux__)
            system("x-www-browser https://discord.gg/qnJR263Hm2/");
#endif
#endif
            break;

        case START_JOINREQUIRED_SIGNIN:
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
            EM_ASM({
                CoreLauncher_ShowLoginModalWindow();
            });
#endif
            break;

        case START_JOINREQUIRED_SKIP:
            progress_exit();
            progress_init(MODE_CHALLENGE);

            if (set_level_play(0))
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

static int start_signinrequired_enter(struct state *st, struct state *prev, int intent)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("Powerups available"), GUI_MED, GUI_COLOR_DEFAULT);
        gui_space(id);
        gui_multi(id,
                  _("Pennyball offers some of the most creative ways to\n"
                    "compete with powerups! We just need you to login\n"
                    "so that we can make sure you have permission to use it."),
                  GUI_SML, GUI_COLOR_WHT);
        gui_space(id);

        if ((jd = gui_harray(id)))
        {
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
            gui_start(jd, _("Login"), GUI_SML, START_JOINREQUIRED_SIGNIN, 0);
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
    start_leave,
    start_paint,
    start_timer,
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
    start_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    start_compat_keybd,
    start_buttn
};

#endif

struct state st_start_upgraderequired = {
    start_upgraderequired_enter,
    shared_leave,
    shared_paint,
    start_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    start_joinrequired_keybd,
    start_joinrequired_buttn
};

struct state st_start_signinrequired = {
    start_signinrequired_enter,
    shared_leave,
    shared_paint,
    start_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    start_joinrequired_keybd,
    start_joinrequired_buttn
};

struct state st_start_joinrequired = {
    start_joinrequired_enter,
    shared_leave,
    shared_paint,
    start_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    start_joinrequired_keybd,
    start_joinrequired_buttn
};
