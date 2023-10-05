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
#include "networking.h"
#include "campaign.h"
#include "account.h"
#endif

#include "gui.h"
#include "set.h"
#include "progress.h"
#include "audio.h"
#include "config.h"
#include "video.h"
#include "demo.h"

#include "game_common.h"
#include "game_client.h"

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
#error Always use preprocessors LEADERBOARD_ALLOWANCE, \
       or it will not show up to be synced belonging st_done.h.
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
        return goto_state(&st_start);

    case GUI_NAME:
        return goto_name(&st_over, &st_over, 0, 0, 0);

    case GUI_SCORE:
        gui_score_set(val);
        return goto_state(&st_over);

    case OVER_TO_GROUP:
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        campaign_hardcore_quit();
        campaign_theme_quit();
        campaign_quit();
        return goto_playmenu(curr_mode());
#endif

    case OVER_SHOP:
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        campaign_hardcore_quit();
        campaign_theme_quit();
        campaign_quit();
#endif
        return goto_state(&st_shop);
    }
    return 1;
}

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
static int over_gui_hardcore(void)
{
    int id, jd;

    int high = progress_set_high();

    if ((id = gui_vstack(0)))
    {
        int gid;

        gid = gui_title_header(id, _("GAME OVER"), GUI_MED, gui_blk, gui_red);

        gui_space(id);

        char hardcore_report[MAXSTR];

        char *report_themename = _("Sky");
        char *report_lastline = _("Keep trying, you will get there!");

        switch (campaign_get_hardcore_data().level_theme)
        {
        case 2:
            report_themename = _("Ice");
            report_lastline = _("Nice one!");
            break;
        case 3:
            report_themename = _("Cave");
            report_lastline = _("Incredible!");
            break;
        case 4:
            report_themename = _("Cloud");
            report_lastline = _("Unbelievable! Well done!");
            break;
        case 5:
            report_themename = _("Lava");
            report_lastline = _("Er, how did you do that?");
            break;
        }

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(hardcore_report, MAXSTR,
#else
        sprintf(hardcore_report,
#endif
               _("You completed %d levels\\"
                 "and collected %d coins.\\ \\"
                 "You managed to reach:\\"
                 "%s (X: %f; Y: %f)\\ \\%s"),
               (campaign_get_hardcore_data().level_number + ((campaign_get_hardcore_data().level_theme - 1) * 6)) - 1,
               curr_score(),
               report_themename, campaign_get_hardcore_data().coordinates[0], campaign_get_hardcore_data().coordinates[1],
               report_lastline);

        gui_multi(id, _(hardcore_report), GUI_SML, gui_wht, gui_wht);

        gui_space(id);

        if ((jd = gui_harray(id)))
        {
            gui_start(jd, _("Return to group"), GUI_SML, OVER_TO_GROUP, 0);
            if (server_policy_get_d(SERVER_POLICY_EDITION) > -1 &&
                server_policy_get_d(SERVER_POLICY_SHOP_ENABLED))
                gui_state(jd, _("Shop"), GUI_SML, OVER_SHOP, 0);
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
    int id, jd, kd;

    int high = progress_set_high();

    if ((id = gui_vstack(0)))
    {
        int gid;

#if NB_HAVE_PB_BOTH==1
        const char *s0 = CHECK_ACCOUNT_BANKRUPT ? _("Bankrupt") : _("GAME OVER");
#else
        const char *s0 = _("GAME OVER");
#endif

        gid = gui_title_header(id, s0, GUI_MED, gui_blk, gui_red);

#ifdef CONFIG_INCLUDES_ACCOUNT
        gui_space(id);
        
        if ((jd = gui_hstack(id)))
        {
            int calc_new_wallet_id;

            gui_filler(jd);

            if ((kd = gui_harray(jd)))
            {
                calc_new_wallet_id = gui_count(kd, 1000, GUI_MED);
                gui_label(kd, _("Coins"), GUI_SML,
                              gui_wht, gui_wht);

                gui_set_count(calc_new_wallet_id, curr_score());
            }

            gui_filler(jd);

            gui_set_rect(jd, GUI_ALL);
        }
#endif

        gui_space(id);
        gui_score_board(id, GUI_SCORE_COIN | GUI_SCORE_TIME, 1, high);
        gui_space(id);

        if ((jd = gui_harray(id)))
            gui_start(jd, _("Select Level"), GUI_SML, GUI_BACK, 0);

        if (!resume
#if NB_HAVE_PB_BOTH==1
            && server_policy_get_d(SERVER_POLICY_EDITION) > -1
#endif
            )
        {
            gui_pulse(gid, 1.2f);
#ifdef CONFIG_INCLUDES_ACCOUNT
            int curr_wallet = account_get_d(ACCOUNT_DATA_WALLET_COINS) + ((curr_balls() * 100) + curr_score());
            account_set_d(ACCOUNT_DATA_WALLET_COINS, curr_wallet);
            account_save();
#endif
        }

        gui_layout(id, 0, 0);
    }

    set_score_board(set_score(curr_set(), SCORE_COIN), progress_score_rank(),
                    set_score(curr_set(), SCORE_TIME), progress_times_rank(),
                    NULL, -1);
#endif
    return id;
}

static int over_enter(struct state *st, struct state *prev)
{
#ifdef LEADERBOARD_ALLOWANCE
    if (prev == &st_name)
        progress_rename(1);
#endif

    resume = prev != &st_fail;

    if (!resume)
    {
        audio_music_fade_out(0.0f);
        audio_narrator_play(AUD_OVER);
        audio_play(AUD_INTRO_SHATTER, 1.0f);

#if NB_HAVE_PB_BOTH==1
        if (CHECK_ACCOUNT_BANKRUPT)
            audio_play("snd/bankrupt.ogg", 1.f);
#endif
    }

    video_clr_grab();

    return
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        (campaign_used() && campaign_hardcore()) ? over_gui_hardcore() :
#endif
        over_gui();
}

static void over_timer(int id, float dt)
{
#ifndef LEADERBOARD_ALLOWANCE
    if (time_state() > 3.f)
    {
        goto_state(&st_start);
        return;
    }
#endif

    gui_timer(id, dt);
    game_step_fade(dt);
}

#ifndef LEADERBOARD_ALLOWANCE
static int over_click(int b, int d)
{
    return (b == SDL_BUTTON_LEFT && d == 1) ? goto_state(&st_exit) : 1;
}
#endif

static int over_keybd(int c, int d)
{
    if (d)
    {
#ifndef LEADERBOARD_ALLOWANCE
        if (c == KEY_EXIT)
            return goto_state(&st_start);
#else
        if (c == KEY_EXIT)
            return over_action(campaign_hardcore() ? OVER_TO_GROUP : GUI_BACK, 0);

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
#ifndef LEADERBOARD_ALLOWANCE
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b) ||
            config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return goto_state(&st_start);
#else
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return over_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return over_action(campaign_hardcore() ? OVER_TO_GROUP : GUI_BACK, 0);
#endif
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

#ifndef LEADERBOARD_ALLOWANCE
struct state st_over = {
    over_enter,
    shared_leave,
    shared_paint,
    over_timer,
    NULL,
    NULL,
    NULL,
    over_click,
    over_keybd,
    over_buttn
};
#else
struct state st_over = {
    over_enter,
    shared_leave,
    shared_paint,
    over_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    over_keybd,
    over_buttn
};
#endif