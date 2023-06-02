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

#include <stdio.h>

#if NB_HAVE_PB_BOTH==1
#include "solid_chkp.h"

#include "account.h"
#include "campaign.h" // New: Campaign
#include "networking.h"
#endif
#ifdef MAPC_INCLUDES_CHKP
#include "checkpoints.h" // New: Checkpoints
#endif
#include "accessibility.h"

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
#include "console_control_gui.h"
#endif

#ifdef CONFIG_INCLUDES_ACCOUNT
#include "powerup.h"
#include "mediation.h"
#include "st_shop.h"
#endif

#include "gui.h"
#include "util.h"
#include "progress.h"
#include "audio.h"
#include "config.h"
#include "video.h"
#include "demo.h"
#include "geom.h"

#include "game_common.h"
#include "game_server.h"
#include "game_client.h"

#include "st_play.h"
#include "st_goal.h"
#include "st_save.h"
#include "st_level.h"
#include "st_name.h"
#include "st_shared.h"

/*---------------------------------------------------------------------------*/

/* TODO: Place the following screenstate variables into the header file. */

struct state st_goal_extraballs;
struct state st_goal_shop;

/*---------------------------------------------------------------------------*/

enum
{
    GOAL_NEXT = GUI_LAST,
    GOAL_SAME,
    GOAL_SAVE,
    GOAL_DONE,
    GOAL_LAST
};

static int shop_product_available;

static int challenge_boom;
static int challenge_caught_extra;
static int challenge_disable_all_buttons;

static int balls_id;
static int coins_id;
static int score_id;

int wallet_id;

static int resume;
static int resume_hold;

static void goal_shared_exit(int id)
{
    progress_stop();
    progress_exit();
}

static int goal_action(int tok, int val)
{
    /* Waiting for extra balls by collecting 100 coins */
    if (challenge_disable_all_buttons)
    {
        audio_play(AUD_DISABLED, 1.f);
        return 1;
    }

    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
    case GUI_BACK:
    case GOAL_LAST:
        return goto_exit();

    case GOAL_SAVE:
        progress_stop();
        return goto_save(&st_goal, &st_goal);

    case GUI_NAME:
        progress_stop();
#ifdef CONFIG_INCLUDES_ACCOUNT
        return goto_shop_rename(&st_goal, &st_goal, 0);
#else
        return goto_name(&st_goal, &st_goal, 0, 0, 0);
#endif

    case GOAL_DONE:
        return goto_exit();

    case GUI_SCORE:
        gui_score_set(val);
        return goto_state_full(&st_goal, 0, 0, 1);

    case GOAL_NEXT:
        if (progress_next())
        {
            powerup_stop();
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            if (campaign_used() && campaign_hardcore())
                campaign_hardcore_nextlevel();

            return goto_state(campaign_used() ? &st_play_ready : &st_level);
#else
            return goto_state(&st_level);
#endif
        }
        break;

    case GOAL_SAME:
        if (progress_same())
        {
            powerup_stop();
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            return goto_state(campaign_used() ? &st_play_ready : &st_level);
#else
            return goto_state(&st_level);
#endif
        }
        break;
    }

    return 1;
}

static int goal_gui(void)
{
    const char *s1 = _("New Record");
    const char *s2 = _("GOAL");

    int id, jd, kd, ld, md;

    int high = progress_lvl_high();

    shop_product_available = 0;

    challenge_boom = 0;
    challenge_caught_extra = 0;
    challenge_disable_all_buttons = 0;

#if NB_HAVE_PB_BOTH==1
    if (!resume && config_get_d(CONFIG_NOTIFICATION_SHOP) && server_policy_get_d(SERVER_POLICY_EDITION) > -1)
#else
    if (!resume)
#endif
    {
        if (curr_mode() == MODE_NORMAL || curr_mode() == MODE_ZEN
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            || curr_mode() == MODE_CAMPAIGN
#endif
            )
        {
#ifdef CONFIG_INCLUDES_ACCOUNT
#ifdef LEVELGROUPS_INCLUDES_ZEN
            if (!account_get_d(ACCOUNT_PRODUCT_MEDIATION) && (account_get_d(ACCOUNT_DATA_WALLET_COINS) + curr_score()) >= 120)
                shop_product_available = 1;
            else if (!account_get_d(ACCOUNT_PRODUCT_BONUS) && (account_get_d(ACCOUNT_DATA_WALLET_COINS) + curr_score()) >= 180)
                shop_product_available = 1;
            else if (!account_get_d(ACCOUNT_PRODUCT_BALLS) && (account_get_d(ACCOUNT_DATA_WALLET_COINS) + curr_score()) >= 250)
                shop_product_available = 1;
            else if (!account_get_d(ACCOUNT_PRODUCT_LEVELS) && (account_get_d(ACCOUNT_DATA_WALLET_COINS) + curr_score()) >= 310)
                shop_product_available = 1;
#else
            if (!account_get_d(ACCOUNT_PRODUCT_BONUS) && (account_get_d(ACCOUNT_DATA_WALLET_COINS) + curr_score()) >= 180)
                shop_product_available = 1;
            else if (!account_get_d(ACCOUNT_PRODUCT_BALLS) && (account_get_d(ACCOUNT_DATA_WALLET_COINS) + curr_score()) >= 250)
                shop_product_available = 1;
            else if (!account_get_d(ACCOUNT_PRODUCT_LEVELS) && (account_get_d(ACCOUNT_DATA_WALLET_COINS) + curr_score()) >= 310)
                shop_product_available = 1;
#endif
#endif
        }
    }

    if ((id = gui_vstack(0)))
    {
        int gid;
#ifdef CONFIG_INCLUDES_ACCOUNT
        int save = config_get_d(CONFIG_ACCOUNT_SAVE);
#else
        int save = 2;
#endif

        if ((jd = gui_vstack(id)))
        {
            if (high && (!campaign_used() || (accessibility_get_d(ACCESSIBILITY_SLOWDOWN) >= 100
#if NB_STEAM_API==0 && NB_EOS_SDK==0
                && !config_cheat()
#endif
                && (!config_get_d(CONFIG_SMOOTH_FIX) || video_perf() >= 25))))
                gid = gui_title_header(jd, s1, GUI_MED, gui_grn, gui_grn);
            else
                gid = gui_title_header(jd, s2, GUI_LRG, gui_blu, gui_grn);

            if (!resume)
                gui_pulse(gid, 1.2f);

            if (curr_coins() == curr_max_coins() * get_coin_multiply())
                gui_label(jd, _("Perfect!"), GUI_SML, gui_grn, gui_grn);

            gui_set_rect(jd, GUI_ALL);
        }

        if (save == 0)
            demo_play_stop(1);

        gui_space(id);

        if (curr_mode() == MODE_CHALLENGE || curr_mode() == MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            || curr_mode() == MODE_HARDCORE
#endif
            )
        {
            int coins, score, balls;
            int i;

            /* Reverse-engineer initial score and balls. */

            if (resume)
            {
                coins = 0;
                score = curr_score();
                balls = curr_balls();
            }
            else
            {
                coins = curr_coins();
                score = curr_score() - coins;
                balls = curr_balls();
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                if (curr_mode() != MODE_HARDCORE)
#endif
                {
                    for (i = curr_score(); i > score; i--)
                        if (progress_reward_ball(i))
                        {
                            /* Waiting for extra balls by collecting 100 coins */
                            challenge_disable_all_buttons = config_get_d(CONFIG_NOTIFICATION_REWARD) ? 1 : 0;
                            balls--;
                        }
                }
            }

            if ((jd = gui_hstack(id)))
            {
                gui_filler(jd);

                if ((kd = gui_vstack(jd)))
                {
                    if ((ld = gui_hstack(kd)))
                    {
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                        if (curr_mode() != MODE_HARDCORE)
#endif
                        {
                            if ((md = gui_harray(ld)))
                            {
                                balls_id = gui_count(md, 1000, GUI_MED);
                                gui_label(md, _("Balls"), GUI_SML,
                                    gui_wht, gui_wht);
                            }
                        }
                        if ((md = gui_harray(ld)))
                        {
                            score_id = gui_count(md, 1000, GUI_MED);
                            gui_label(md, _("Score"), GUI_SML,
                                      gui_wht, gui_wht);
                        }
                        if ((md = gui_harray(ld)))
                        {
                            coins_id = gui_count(md, 1000, GUI_MED);
                            gui_label(md, _("Coins"), GUI_SML,
                                      gui_wht, gui_wht);
                        }

                        gui_set_count(balls_id, balls);
                        gui_set_count(score_id, score);
                        gui_set_count(coins_id, coins);
                    }

#ifdef CONFIG_INCLUDES_ACCOUNT
                    if (account_get_d(ACCOUNT_PRODUCT_BONUS) == 0)
#endif
                    {
                        if ((ld = gui_harray(kd)))
                        {
                            const struct level *l;

                            gui_label(ld, "", GUI_SML, 0, 0);

                            for (i = MAXLVL - 1; i >= 0; i--)
                                if ((l = get_level(i)) && level_bonus(l))
                                {
                                    const GLubyte *c = (level_opened(l) ?
                                        gui_grn : gui_gry);

                                    gui_label(ld, level_name(l), GUI_SML, c, c);
                                }

                            gui_label(ld, "", GUI_SML, 0, 0);
                        }
                    }

                    gui_set_rect(kd, GUI_ALL);
                }

                gui_filler(jd);
            }

            gui_space(id);
        }
#ifdef CONFIG_INCLUDES_ACCOUNT
        else if (server_policy_get_d(SERVER_POLICY_EDITION) > -1 && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED)
              && !CHECK_ACCOUNT_BANKRUPT)
        {
            balls_id = score_id = 0;

            int wallet = account_get_d(ACCOUNT_DATA_WALLET_COINS);
            int coins = resume ? curr_coins() : MAX((wallet + curr_coins()) - ACCOUNT_WALLET_MAX_COINS, 0);

            if ((jd = gui_hstack(id)))
            {
                gui_filler(jd);

                if ((kd = gui_harray(jd)))
                {
                    wallet_id = gui_count(kd, ACCOUNT_WALLET_MAX_COINS, GUI_MED);
                    gui_label(kd, _("Wallet"), GUI_SML,
                              gui_wht, gui_wht);
                }
                if ((kd = gui_harray(jd)))
                {
                    coins_id = gui_count(kd, 1000, GUI_MED);
                    gui_label(kd, _("Coins"), GUI_SML,
                              gui_wht, gui_wht);
                }

                gui_set_count(coins_id, coins);
                gui_set_count(wallet_id, wallet);

                gui_filler(jd);

                gui_set_rect(jd, GUI_ALL);
            }

            gui_space(id);
        }
#endif

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        if (!campaign_used())
        {
            /* Waiting for extra balls by collecting 100 coins */
            gui_score_board(id, (GUI_SCORE_COIN |
                                 GUI_SCORE_TIME |
                                 GUI_SCORE_GOAL), 1,
                                !resume && (shop_product_available
                                         || challenge_disable_all_buttons) ? 0 : high);
            gui_space(id);
        }
        else if (campaign_used() && (accessibility_get_d(ACCESSIBILITY_SLOWDOWN) >= 100
#if NB_STEAM_API==0 && NB_EOS_SDK==0
            && !config_cheat()
#endif
            && (!config_get_d(CONFIG_SMOOTH_FIX) || video_perf() >= 25)))
        {
            gui_score_board(id, ((campaign_career_unlocked() ? GUI_SCORE_COIN : 0) |
                                 GUI_SCORE_TIME |
                                 (campaign_career_unlocked() ? GUI_SCORE_GOAL : 0)), 1,
                                !resume && (shop_product_available
                                         || challenge_disable_all_buttons) ? 0 : high);
            gui_space(id);
        }
#else
        /* Waiting for extra balls by collecting 100 coins */
        gui_score_board(id, (GUI_SCORE_COIN |
            GUI_SCORE_TIME |
            GUI_SCORE_GOAL), 1,
            !resume && (shop_product_available || challenge_disable_all_buttons) ? 0 : high);
        gui_space(id);
#endif

        if ((jd = gui_harray(id)))
        {
            int m1 = 0, m2 = 0, m3 = 0;
            /*
             * Waiting for extra balls by collecting 100 coins
             * --- OR ---
             * check, if products is still available
             */
            if (!resume && ((config_get_d(CONFIG_NOTIFICATION_REWARD) && challenge_disable_all_buttons) || (config_get_d(CONFIG_NOTIFICATION_SHOP) && shop_product_available)))
            {
                if (progress_done())
                    m1 = gui_label(jd, _("Finish"), GUI_SML, gui_gry, gui_gry);
                else if (progress_last())
                    m1 = gui_label(jd, _("Finish"), GUI_SML, gui_gry, gui_gry);
                else if (progress_next_avail())
                    m1 = gui_label(jd, _("Next Level"), GUI_SML, gui_gry, gui_gry);
                else
                    m1 = gui_label(jd, _("Finish"), GUI_SML, gui_gry, gui_gry);

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                if (progress_same_avail() && !campaign_hardcore())
#else
                if (progress_same_avail())
#endif
                    m2 = gui_label(jd, _("Retry Level"), GUI_SML, gui_gry, gui_gry);

                if (demo_saved() && save >= 1)
                    m3 = gui_label(jd, _("Save Replay"), GUI_SML, gui_gry, gui_gry);

                if (m1)
                    gui_set_state(m1, GUI_NONE, 0);

                if (m2 && progress_same_avail() && !campaign_hardcore())
                    gui_set_state(m2, GUI_NONE, 0);
                
                if (m3 && save >= 1)
                    gui_set_state(m3, GUI_NONE, 0);
            }
            else
            {
                if (progress_done())
                    gui_start(jd, _("Finish"), GUI_SML, GOAL_DONE, 0);
                else if (progress_last())
                    gui_start(jd, _("Finish"), GUI_SML, GOAL_LAST, 0);
                else if (progress_next_avail())
                    gui_start(jd, _("Next Level"), GUI_SML, GOAL_NEXT, 0);
                else
                    gui_start(jd, _("Finish"), GUI_SML, GOAL_LAST, 0);

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                if (progress_same_avail() && !campaign_hardcore())
#else
                if (progress_same_avail())
#endif
                    gui_start(jd, _("Retry Level"), GUI_SML, GOAL_SAME, 0);

                if (demo_saved() && save >= 1)
                    gui_state(jd, _("Save Replay"), GUI_SML, GOAL_SAVE, 0);
            }
        }

#ifdef CONFIG_INCLUDES_ACCOUNT
        if (!resume && server_policy_get_d(SERVER_POLICY_EDITION) > -1) {
            if (curr_mode() == MODE_NORMAL || curr_mode() == MODE_ZEN
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                || curr_mode() == MODE_CAMPAIGN
#endif
                )
            {
                int curr_wallet = MIN(ACCOUNT_WALLET_MAX_COINS, account_get_d(ACCOUNT_DATA_WALLET_COINS) + curr_coins());
                account_set_d(ACCOUNT_DATA_WALLET_COINS, curr_wallet);
                account_save();
            }
        }
#endif

        gui_layout(id, 0, 0);
    }

    set_score_board(level_score(curr_level(), SCORE_COIN), progress_coin_rank(),
                    level_score(curr_level(), SCORE_TIME), progress_time_rank(),
                    level_score(curr_level(), SCORE_GOAL), progress_goal_rank());

    return id;
}

static int goal_enter(struct state *st, struct state *prev)
{
    if (prev == &st_name)
        progress_rename(0);

    audio_music_fade_out(1.0f);
    video_clr_grab();

    resume = (prev != &st_play_loop || (prev == &st_goal && !resume_hold));

#ifdef MAPC_INCLUDES_CHKP
    if (!resume)
        checkpoints_stop();
#endif

    return goal_gui();
}

void goal_leave(struct state *st, struct state *next, int id)
{
    if (!resume || !resume_hold)
    {
        resume = (next != &st_goal);

        if (!resume_hold)
            resume_hold = (next == &st_goal);

        resume = !resume_hold;
    }

    gui_delete(id);
}

static void goal_paint(int id, float t)
{
    game_client_draw(0, t);

    gui_paint(id);
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
    xbox_control_death_gui_paint();
#endif
}

static void goal_timer(int id, float dt)
{
    if (!resume || resume_hold)
    {
        static float t = 0.0f;

        t += dt;

        geom_step(dt);
        game_server_step(dt);

        int record_screenanimations = time_state() < (config_get_d(CONFIG_SCREEN_ANIMATIONS) ? 1.5f : 1.f);
        int record_modes = curr_mode() != MODE_NONE;
        int record_campaign = !campaign_hardcore_norecordings();

        game_client_blend(game_server_blend());
        game_client_sync(!resume
                      && record_screenanimations
                      && record_modes
                      && record_campaign ? demo_fp : NULL);

        if ((t > 0.05f && coins_id)
            && (!resume && time_state() > (config_get_d(CONFIG_SCREEN_ANIMATIONS) ? 1.5f : 1.f)))
        {
            int coins = 0;
#ifdef CONFIG_INCLUDES_ACCOUNT
            if (!CHECK_ACCOUNT_BANKRUPT)
#endif
                coins = gui_value(coins_id);

#ifdef CONFIG_INCLUDES_ACCOUNT
            int wallet = 0;
            if (!CHECK_ACCOUNT_BANKRUPT)
                wallet = gui_value(wallet_id);

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            if ((curr_mode() == MODE_CAMPAIGN || curr_mode() == MODE_NORMAL || curr_mode() == MODE_ZEN) && server_policy_get_d(SERVER_POLICY_EDITION) > -1 && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED))
#else
            if ((curr_mode() == MODE_NORMAL || curr_mode() == MODE_ZEN) && server_policy_get_d(SERVER_POLICY_EDITION) > -1 && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED))
#endif
                wallet = 0;
#endif

#ifdef CONFIG_INCLUDES_ACCOUNT
            if (coins > 0 && (wallet < ACCOUNT_WALLET_MAX_COINS))
#else
            if (coins > 0)
#endif
            {
                int score = gui_value(score_id);

                int balls = 0;
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                if (curr_mode() != MODE_HARDCORE)
#endif
                    balls = gui_value(balls_id);

#ifdef CONFIG_INCLUDES_ACCOUNT
                wallet = gui_value(wallet_id);
#endif

                gui_set_count(coins_id, coins - 1);
                gui_pulse(coins_id, 1.1f);

                gui_set_count(score_id, score + 1);
                gui_pulse(score_id, 1.1f);

#ifdef CONFIG_INCLUDES_ACCOUNT
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                if ((curr_mode() == MODE_CAMPAIGN || curr_mode() == MODE_NORMAL || curr_mode() == MODE_ZEN) && server_policy_get_d(SERVER_POLICY_EDITION) > -1 && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED)) {
#else
                if ((curr_mode() == MODE_NORMAL || curr_mode() == MODE_ZEN) && server_policy_get_d(SERVER_POLICY_EDITION) > -1 && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED)) {
#endif
                    gui_set_count(wallet_id, wallet + 1);
                    gui_pulse(wallet_id, 1.1f);
                }
#endif

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                if (progress_reward_ball(score + 1) && (curr_mode() != MODE_HARDCORE && curr_mode() != MODE_CAMPAIGN && curr_mode() != MODE_NORMAL && curr_mode() != MODE_ZEN))
#else
                if (progress_reward_ball(score + 1) && (curr_mode() != MODE_NORMAL && curr_mode() != MODE_ZEN))
#endif
                {
                    challenge_boom = 1;
                    challenge_caught_extra = 1;

                    gui_set_count(balls_id, balls + 1);
                    gui_pulse(balls_id, 2.0f);
                    if (config_get_d(CONFIG_NOTIFICATION_REWARD) == 0) audio_play(AUD_BALL, 1.0f);
                }
            }
            t = 0.0f;
        }
    }

    gui_timer(id, dt);

    /* Waiting for extra balls by collecting 100 coins */
    if (challenge_caught_extra && config_get_d(CONFIG_NOTIFICATION_REWARD) && (curr_mode() == MODE_CHALLENGE || curr_mode() == MODE_BOOST_RUSH))
    {
        /* Notify me, if you collected 100 coins */
        goto_state(&st_goal_extraballs);
    }

#ifdef CONFIG_INCLUDES_ACCOUNT
    /* Notify me, if you have enough coins */
    if (!resume && time_state() >= 1.0f && shop_product_available && config_get_d(CONFIG_NOTIFICATION_SHOP) && server_policy_get_d(SERVER_POLICY_EDITION) > -1 && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED))
        goto_state(&st_goal_shop);
#endif
}

static int goal_keybd(int c, int d)
{
    if (d)
    {
        /*
         * This is revealed in death_screen.json.
         * This methods can't use this: fail_action(GUI_BACK, 0);
         */
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        if (c == KEY_EXIT && current_platform == PLATFORM_PC)
#else
        if (c == KEY_EXIT)
#endif
            return 1;
        if (config_tst_d(CONFIG_KEY_SCORE_NEXT, c)
#if NB_HAVE_PB_BOTH==1
            && current_platform == PLATFORM_PC
#endif
            )
            return goal_action(GUI_SCORE, GUI_SCORE_NEXT(gui_score_get()));
        if (config_tst_d(CONFIG_KEY_RESTART, c) && progress_same_avail() && !campaign_hardcore()
#if NB_HAVE_PB_BOTH==1
            && current_platform == PLATFORM_PC
#endif
            )
            return goal_action(GOAL_SAME, 0);
    }

    return 1;
}

static int goal_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        /*
         * This is revealed in death_screen.json.
         * This methods can't use this: goal_action(GUI_BACK, 0);
         */

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return goal_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return 1;
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int goal_extraballs_gui(void)
{
    int id;

    if ((id = gui_vstack(0)))
    {
        int msgid = gui_label(id, _("New balls earned!"), GUI_MED, gui_grn, gui_grn);
        gui_space(id);
        gui_multi(id, _("You've earned extra ball by collecting\\100 coins in a single set."),
            GUI_SML, gui_wht, gui_wht);
        gui_pulse(msgid, 1.2f);
        gui_layout(id, 0, 0);
    }

    return id;
}

static int goal_extraballs_enter(struct state *st, struct state *prev)
{
    audio_play("snd/extralives.ogg", 1.0f);
    return goal_extraballs_gui();
}

static int goal_extraballs_keybd(int c, int d)
{
    if (d)
    {
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        if (c == KEY_EXIT && current_platform == PLATFORM_PC)
#else
        if (c == KEY_EXIT)
#endif
            return goto_state(&st_goal);
    }

    return 1;
}

static int goal_extraballs_buttn(int b, int d)
{
    if (d)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return goto_state(&st_goal);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return goto_state(&st_goal);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int product_index = 0;

static int goal_shop_gui(void)
{
    int id;

    if ((id = gui_vstack(0)))
    {
        int msgid = gui_label(id, _("Product available!"), GUI_MED, gui_grn, gui_grn);
        gui_space(id);

        const char *prodname = "none";

#ifdef CONFIG_INCLUDES_ACCOUNT
#ifdef LEVELGROUPS_INCLUDES_ZEN
        if (!account_get_d(ACCOUNT_PRODUCT_MEDIATION) && (account_get_d(ACCOUNT_DATA_WALLET_COINS) + curr_score()) >= 120)
            prodname = _("Mediation");
        else if (!account_get_d(ACCOUNT_PRODUCT_BONUS) && (account_get_d(ACCOUNT_DATA_WALLET_COINS) + curr_score()) >= 180)
            prodname = _("Bonus Pack");
        else if (!account_get_d(ACCOUNT_PRODUCT_BALLS) && (account_get_d(ACCOUNT_DATA_WALLET_COINS) + curr_score()) >= 250)
            prodname = _("Online Balls");
        else if(!account_get_d(ACCOUNT_PRODUCT_LEVELS) && (account_get_d(ACCOUNT_DATA_WALLET_COINS) + curr_score()) >= 310)
            prodname = _("Extra Levels");
#else
        if (!account_get_d(ACCOUNT_PRODUCT_BONUS) && (account_get_d(ACCOUNT_DATA_WALLET_COINS) + curr_score()) >= 180)
            prodname = _("Bonus Pack");
        else if (!account_get_d(ACCOUNT_PRODUCT_BALLS) && (account_get_d(ACCOUNT_DATA_WALLET_COINS) + curr_score()) >= 250)
            prodname = _("Online Balls");
        else if (!account_get_d(ACCOUNT_PRODUCT_LEVELS) && (account_get_d(ACCOUNT_DATA_WALLET_COINS) + curr_score()) >= 310)
            prodname = _("Extra Levels");
#endif
#endif

        char productmsg[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(productmsg, dstSize,
#else
        sprintf(productmsg,
#endif
                _("You have enough coins to buy\\%s.\\Try it out!"), prodname);

        gui_multi(id, productmsg, GUI_SML, gui_wht, gui_wht);

        gui_pulse(msgid, 1.2f);
        gui_layout(id, 0, 0);
    }

    return id;
}

static int goal_shop_enter(struct state *st, struct state *prev)
{
    audio_play("snd/extralives.ogg", 1.0f);
    return goal_shop_gui();
}

static int goal_shop_keybd(int c, int d)
{
    if (d)
    {
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        if (c == KEY_EXIT && current_platform == PLATFORM_PC)
#else
        if (c == KEY_EXIT)
#endif
            return goto_state(&st_goal);
    }

    return 1;
}

static int goal_shop_buttn(int b, int d)
{
    if (d)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return goto_state(&st_goal);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return goto_state(&st_goal);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int restrict_hardcore_nextstate = 0;

static int goal_hardcore_enter(struct state *st, struct state *prev)
{
    restrict_hardcore_nextstate = 0;
    powerup_stop();

    video_clr_grab();

    resume = (prev != &st_play_loop);

#ifdef MAPC_INCLUDES_CHKP
    checkpoints_stop();
#endif

    game_fade(+1.0f);

    return goal_gui();
}

static void goal_hardcore_timer(int id, float dt)
{
    geom_step(dt);
    
    if (!restrict_hardcore_nextstate)
    {
        game_server_step(dt);
        game_client_sync(!campaign_hardcore_norecordings() && curr_mode() != MODE_NONE ? NULL : demo_fp);
        game_client_blend(game_server_blend());

        game_step_fade(dt);
    }

    if (!progress_done() && !progress_last())
    {
        if (time_state() > 1.0f && !restrict_hardcore_nextstate)
        {
            if (progress_next())
            {
                restrict_hardcore_nextstate = 1;
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                if (campaign_used() && campaign_hardcore())
                    campaign_hardcore_nextlevel();

                goto_state(campaign_used() ? &st_play_ready : &st_level);
#else
                goto_state(&st_level);
#endif
            }
        }
    }
}

static void goal_hardcore_paint(int id, float t)
{
    game_client_draw(0, t);
}

/*---------------------------------------------------------------------------*/

struct state st_goal = {
    goal_enter,
    shared_leave,
    goal_paint,
    goal_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    goal_keybd,
    goal_buttn,
    NULL,
    NULL,
    NULL,
    goal_shared_exit
};

struct state st_goal_hardcore = {
    goal_hardcore_enter,
    shared_leave,
    goal_hardcore_paint,
    goal_hardcore_timer,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    goal_shared_exit
};

struct state st_goal_extraballs = {
    goal_extraballs_enter,
    shared_leave,
    shared_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click_basic,
    goal_extraballs_keybd,
    goal_extraballs_buttn,
    NULL,
    NULL,
    NULL,
    goal_shared_exit
};

struct state st_goal_shop = {
    goal_shop_enter,
    shared_leave,
    shared_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click_basic,
    goal_shop_keybd,
    goal_shop_buttn,
    NULL,
    NULL,
    NULL,
    goal_shared_exit
};