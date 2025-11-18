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

#if NB_HAVE_PB_BOTH==1
#include "networking.h"
#include "campaign.h"
#include "account.h"
#include "account_wgcl.h"
#endif

#include "gui.h"
#include "transition.h"
#include "set.h"
#include "progress.h"
#include "audio.h"
#include "config.h"
#include "video.h"
#include "demo.h"
#include "key.h"

#include "game_common.h"
#include "game_server.h"
#include "game_client.h"

#include "st_common.h"
#include "st_done.h"
#include "st_over.h"
#include "st_start.h"
#include "st_shared.h"
#include "st_name.h"
#include "st_set.h"
#include "st_shop.h"
#include "st_fail.h"

/*---------------------------------------------------------------------------*/

#ifndef LEADERBOARD_ALLOWANCE
#error Security compilation error: Always use preprocessors "LEADERBOARD_ALLOWANCE", \
       or it will not show up to be synced belonging st_done.h.
#elif NB_HAVE_PB_BOTH!=1
#error Security compilation error: Preprocessor definitions can be used it, \
       once you have transferred or joined into the target Discord Server, \
       and verified and promoted as Developer Role. \
       This invite link can be found under https://discord.gg/qnJR263Hm2/.
#endif

/*
 * Player wants to save and open the Leaderboard.
 */

enum {
    OVER_TO_GROUP = GUI_LAST,
    OVER_SHOP
};

static int resume;

static int over_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            campaign_hardcore_quit();
            campaign_theme_quit();
            campaign_quit();
#endif
            return exit_state(&st_start);

        case GUI_NAME:
            return goto_name(&st_over, &st_over, 0, 0, 0);

        case GUI_SCORE:
            gui_score_set(val);
            return goto_state(&st_over);

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        case OVER_TO_GROUP:
            if (campaign_used())
            {
                campaign_hardcore_quit();
                campaign_theme_quit();
                campaign_quit();
            }
#endif
            return goto_playmenu(curr_mode());

#if NB_HAVE_PB_BOTH==1
        case OVER_SHOP:
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            if (campaign_used())
            {
                campaign_hardcore_quit();
                campaign_theme_quit();
                campaign_quit();
            }
#endif
#ifdef CONFIG_INCLUDES_ACCOUNT
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            if (!campaign_used())
#endif
            {
                const char *curr_setid = set_id(curr_set());
                char curr_setid_final[MAXSTR];

                if (!curr_setid)
                {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                    sprintf_s(curr_setid_final, MAXSTR,
#else
                    sprintf(curr_setid_final,
#endif
                            _("none_%d"), curr_set());
                }
                else
                    SAFECPY(curr_setid_final, curr_setid);

                if (str_starts_with(curr_setid_final, "anime"))
                    audio_music_fade_to(0.5f, "bgm/jp/title.ogg", 1);
                else
                    audio_music_fade_to(0.5f, is_boost_on() ? "bgm/boostrush.ogg" :
                                                              "bgm/inter_world.ogg", 1);
            }

            return goto_shop(
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                             campaign_used() ? &st_campaign :
#endif
                             &st_start, 0);
#endif
#endif
    }
    return 1;
}

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
static int over_gui_hardcore(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        int gid;

        if ((gid = gui_title_header(id, _("GAME OVER"), GUI_MED, gui_blk, gui_red)))
            gui_pulse(gid, 1.2f);

        gui_space(id);

        char hardcore_report[MAXSTR];

        char report_themename[32];
        char report_lastline[32];

        switch (campaign_get_hardcore_data().level_theme)
        {
            case 1:
                SAFECPY(report_themename, _("Sky"));
                SAFECPY(report_lastline, _("Keep trying, you will get there!"));
                break;
            case 2:
                SAFECPY(report_themename, _("Ice"));
                SAFECPY(report_lastline,  _("Nice one!"));
                break;
            case 3:
                SAFECPY(report_themename, _("Cave"));
                SAFECPY(report_lastline,  _("Incredible!"));
                break;
            case 4:
                SAFECPY(report_themename, _("Cloud"));
                SAFECPY(report_lastline,  _("Unbelievable! Well done!"));
                break;
            case 5:
                SAFECPY(report_themename, _("Lava"));
                SAFECPY(report_lastline, _("Er, how did you do that?"));
                break;
        }

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(hardcore_report, MAXSTR,
#else
        sprintf(hardcore_report,
#endif
               _("You completed %d levels\n"
                 "and collected %d coins.\n \n"
                 "You managed to reach:\n"
                 "%s (X: %f; Y: %f)\n\n%s"),
               (campaign_get_hardcore_data().level_number + ((campaign_get_hardcore_data().level_theme - 1) * 6)) - 1,
               curr_score(),
               report_themename, campaign_get_hardcore_data().coordinates[0], campaign_get_hardcore_data().coordinates[1],
               report_lastline);

        gui_multi(id, _(hardcore_report), GUI_SML, GUI_COLOR_WHT);

        gui_space(id);

        if ((jd = gui_harray(id)))
        {
            gui_start(jd, _("Return to group"), GUI_SML, OVER_TO_GROUP, 0);
#if NB_HAVE_PB_BOTH==1
            if (server_policy_get_d(SERVER_POLICY_EDITION) > -1 &&
                server_policy_get_d(SERVER_POLICY_SHOP_ENABLED))
                gui_state(jd, _("Shop"), GUI_SML, OVER_SHOP, 0);
#endif
        }

        gui_layout(id, 0, 0);
    }

    return id;
}
#endif

static int over_gui(void)
{
#ifndef LEADERBOARD_ALLOWANCE
    int id;

    if ((id = gui_title_header(0, _("GAME OVER"), GUI_LRG, gui_blk, gui_red)))
    {
        gui_layout(id, 0, 0);
        gui_pulse(id, 1.2f);
    }
#else
    /* Do this for now! */
    int id, jd;

    int high = progress_set_high();

    if ((id = gui_vstack(0)))
    {
        int gid;

#if NB_HAVE_PB_BOTH==1
        const char *s0 = CHECK_ACCOUNT_BANKRUPT ? N_("Bankrupt") : N_("GAME OVER");
#else
        const char *s0 = N_("GAME OVER");
#endif

        gid = gui_title_header(id, _(s0), GUI_MED, gui_blk, gui_red);

        gui_space(id);
#if NB_HAVE_PB_BOTH==1
        gui_score_board(id, GUI_SCORE_COIN | GUI_SCORE_TIME,
                            1,
                            high && !account_wgcl_name_read_only());
#else
        gui_score_board(id, GUI_SCORE_COIN | GUI_SCORE_TIME,
                            1,
                            high);
#endif
        gui_space(id);

        if ((jd = gui_harray(id)))
            gui_start(jd, _("Select Level"), GUI_SML, GUI_BACK, 0);

        if (!resume)
            gui_pulse(gid, 1.2f);

        gui_layout(id, 0, 0);
    }

    set_score_board(set_score(curr_set(), SCORE_COIN), progress_score_rank(),
                    set_score(curr_set(), SCORE_TIME), progress_times_rank(),
                    NULL, -1);
#endif
    return id;
}

static int over_enter(struct state *st, struct state *prev, int intent)
{
#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT)
    account_wgcl_restart_attempt();
#endif

#ifdef LEADERBOARD_ALLOWANCE
    if (prev == &st_name)
        progress_rename(1);
#endif

    resume = prev != &st_fail;

    if (!resume)
    {
        audio_music_fade_out(0.0f);
#if NB_HAVE_PB_BOTH!=1 || !defined(__EMSCRIPTEN__)
        audio_narrator_play(AUD_OVER);
#endif
        audio_play(AUD_UI_SHATTER, 1.0f);

#if NB_HAVE_PB_BOTH==1
        if (CHECK_ACCOUNT_BANKRUPT)
            audio_play("snd/bankrupt.ogg", 1.0f);
#endif
    }

    video_clr_grab();

    return transition_slide(
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        (campaign_used() && campaign_hardcore()) ? over_gui_hardcore() :
#endif
        over_gui(), 1, intent);
}

static int over_leave(struct state *st, struct state *next, int id, int intent)
{
    if (next == &st_null)
    {
        progress_exit();

        campaign_quit();
        set_quit();

        game_server_free(NULL);
        game_client_free(NULL);

        gui_delete(id);
        return 0;
    }

    if (next == &st_over)
    {
        gui_delete(id);
        return 0;
    }

    return transition_slide(id, 0, intent);
}

static void over_timer(int id, float dt)
{
#ifndef LEADERBOARD_ALLOWANCE
    if (time_state() > 3.0f && !st_global_animating())
    {
        exit_state(&st_start);
        return;
    }
#endif

    gui_timer(id, dt);
    game_step_fade(dt);
}

#ifndef LEADERBOARD_ALLOWANCE
static int over_click(int b, int d)
{
    if (d && config_tst_d(CONFIG_MOUSE_CANCEL_MENU, b))
        return exit_state(&st_start);

    return (b == SDL_BUTTON_LEFT && d == 1) ? exit_state(&st_start) : 1;
}
#endif

static int over_keybd(int c, int d)
{
    if (d)
    {
#ifndef LEADERBOARD_ALLOWANCE
        if (c == KEY_EXIT)
            return exit_state(&st_start);
#else
        if (c == KEY_EXIT)
            return over_action(
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                               campaign_hardcore() ? OVER_TO_GROUP :
#endif
                               GUI_BACK, 0);

            return over_action(GUI_BACK, 0);

        if (config_tst_d(CONFIG_KEY_SCORE_NEXT, c))
            return over_action(GUI_SCORE, GUI_SCORE_NEXT(gui_score_get()));
#endif
    }
    return 1;
}

static int over_buttn(int b, int d)
{
    if (d)
    {
#ifdef LEADERBOARD_ALLOWANCE
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return over_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return over_action(
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                               campaign_hardcore() ? OVER_TO_GROUP :
#endif
                GUI_BACK, 0);
#else
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b) ||
            config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return exit_state(&st_start);
#endif
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

#ifdef LEADERBOARD_ALLOWANCE
struct state st_over = {
    over_enter,
    over_leave,
    shared_paint,
    over_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    over_keybd,
    over_buttn
};
#else
struct state st_over = {
    over_enter,
    over_leave,
    shared_paint,
    over_timer,
    NULL,
    NULL,
    NULL,
    over_click,
    over_keybd,
    over_buttn
};
#endif
