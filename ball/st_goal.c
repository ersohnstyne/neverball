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

#include <stdio.h>

#if NB_HAVE_PB_BOTH==1
#include "solid_chkp.h"

#include "account.h"
#include "account_wgcl.h"
#include "campaign.h" /* New: Campaign */
#include "networking.h"
#endif

#ifdef MAPC_INCLUDES_CHKP
#include "checkpoints.h" /* New: Checkpoints */
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
#include "transition.h"
#include "util.h"
#include "progress.h"
#include "audio.h"
#include "config.h"
#include "video.h"
#include "demo.h"
#include "hud.h"
#include "key.h"
#include "geom.h"

#include "game_common.h"
#include "game_server.h"
#include "game_client.h"

#include "st_common.h"
#include "st_play.h"
#include "st_goal.h"
#include "st_save.h"
#include "st_level.h"
#include "st_name.h"
#include "st_shared.h"

/*---------------------------------------------------------------------------*/

struct state st_goal_extraballs;
struct state st_goal_shop;

/*---------------------------------------------------------------------------*/

enum
{
    GOAL_NEXT = GUI_LAST,
    GOAL_SAME,
    GOAL_SAVE,
    GOAL_DONE,
    GOAL_LAST,

    GOAL_IAP = GOAL_LAST + 1
};

static float score_count_anim_time;
static int   score_count_anim_index;
static int   score_count_anim_locked;

static int shop_product_available;

static int challenge_caught_extra;
static int challenge_disable_all_buttons;

static int balls_id;
static int coins_id;
static int score_id;

static int wallet_id;

static int resume;
static int resume_hold;

static int goal_action(int tok, int val)
{
    /* Waiting for extra balls by collecting 100 coins */
    if (challenge_disable_all_buttons)
    {
        audio_play(AUD_DISABLED, 1.0f);
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
            return goto_state(&st_goal);

        case GOAL_NEXT:
            if (progress_next())
            {
                powerup_stop();
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                if (campaign_used() && campaign_hardcore())
                    campaign_hardcore_nextlevel();
#endif
                return goto_play_level();
            }
            break;

        case GOAL_SAME:
            if (progress_same())
            {
                powerup_stop();
                return goto_play_level();
            }
            break;

#ifdef CONFIG_INCLUDES_ACCOUNT
        case GOAL_IAP:
#if (NB_STEAM_API==1 || NB_EOS_SDK==1) || ENABLE_IAP==1
            return goto_shop_iap(&st_goal, &st_goal, 0, 0, 0, 0, 1);
#else
            return goto_shop_iap(&st_goal, &st_goal, 0, 0, 0, 0, 0);
#endif
            break;
#endif
    }

    return 1;
}

static int goal_gui(void)
{
    const char *s1 = _("New Record");
    const char *s2 = _("GOAL");
    const char *s3 = _("Master complete");

    int id, jd, kd, ld, md;
    int root_id;

    int high   = progress_lvl_high();
    int master = curr_level()->is_master;

    shop_product_available = 0;

    challenge_caught_extra = 0;
    challenge_disable_all_buttons = 0;

#if NB_HAVE_PB_BOTH==1
    if (!resume && config_get_d(CONFIG_NOTIFICATION_SHOP) &&
        server_policy_get_d(SERVER_POLICY_EDITION) > -1)
#else
    if (!resume)
#endif
    {
        if (curr_mode() == MODE_NORMAL
#ifdef LEVELGROUPS_INCLUDES_ZEN
         || curr_mode() == MODE_ZEN
#endif
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
         || curr_mode() == MODE_CAMPAIGN
#endif
            )
        {
#ifdef CONFIG_INCLUDES_ACCOUNT
            if (
#ifdef LEVELGROUPS_INCLUDES_ZEN
                (!account_get_d(ACCOUNT_PRODUCT_MEDIATION) &&
                 (account_get_d(ACCOUNT_DATA_WALLET_COINS) + curr_score()) >= 120) ||
#endif
                (!account_get_d(ACCOUNT_PRODUCT_BONUS) &&
                 (account_get_d(ACCOUNT_DATA_WALLET_COINS) + curr_score()) >= 180) ||
                (!account_get_d(ACCOUNT_PRODUCT_BALLS) &&
                 (account_get_d(ACCOUNT_DATA_WALLET_COINS) + curr_score()) >= 250) ||
                (!account_get_d(ACCOUNT_PRODUCT_LEVELS) &&
                 (account_get_d(ACCOUNT_DATA_WALLET_COINS) + curr_score()) >= 310)
                )
                shop_product_available = 1;
#endif
        }
    }

    if ((root_id = gui_root()))
    {
        if ((id = gui_vstack(root_id)))
        {
            int gid;
#ifdef CONFIG_INCLUDES_ACCOUNT
            int save = config_get_d(CONFIG_ACCOUNT_SAVE);
#else
            int save = 2;
#endif

            if ((jd = gui_vstack(id)))
            {
                if (high &&
                    (
#ifdef CONFIG_INCLUDES_ACCOUNT
                        !campaign_used() ||
#endif
                        (accessibility_get_d(ACCESSIBILITY_SLOWDOWN) >= 100 &&
#if NB_STEAM_API==0 && NB_EOS_SDK==0
                            !config_cheat() &&
#endif
                            (!config_get_d(CONFIG_SMOOTH_FIX) || video_perf() >= NB_FRAMERATE_MIN))))
                    gid = gui_title_header(jd, s1, GUI_MED, GUI_COLOR_GRN);
                else
                    gid = gui_title_header(jd, master ? s3 : s2,
                                               GUI_MED,
                                               gui_blu, gui_grn);

                if (!resume)
                    gui_pulse(gid, 1.2f);

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
                int coins = !resume ? curr_coins() : 0,
                    score = !resume ? curr_score() - coins : curr_score(),
                    balls = curr_balls();

                /* Reverse-engineer initial score and balls. */

                if (!resume
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                 && curr_mode() != MODE_HARDCORE
#endif
                    )
                    for (int i = curr_score(); i > score; i--)
                        if (progress_reward_ball(i))
                        {
                            /* Disable all buttons after collecting 100 coins for each. */

                            challenge_disable_all_buttons = config_get_d(CONFIG_NOTIFICATION_REWARD) ? 1 : 0;

                            balls--;
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
                                        GUI_COLOR_WHT);
                                }
                            }
                            if ((md = gui_harray(ld)))
                            {
                                score_id = gui_count(md, 1000, GUI_MED);
                                gui_label(md, _("Score"), GUI_SML,
                                    GUI_COLOR_WHT);
                            }
                            if ((md = gui_harray(ld)))
                            {
                                coins_id = gui_count(md, 1000, GUI_MED);
                                gui_label(md, _("Coins"), GUI_SML,
                                    GUI_COLOR_WHT);
                            }

                            gui_set_count(balls_id, balls);
                            gui_set_count(score_id, score);
                            gui_set_count(coins_id, coins);
                        }

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                        if (!campaign_used())
#endif

#ifdef CONFIG_INCLUDES_ACCOUNT
                            if (account_get_d(ACCOUNT_PRODUCT_BONUS) == 0)
#endif
                                if ((ld = gui_harray(kd)))
                                {
                                    const struct level *l;

                                    gui_label(ld, "", GUI_SML, 0, 0);

                                    for (int j = MAXLVL - 1; j >= 0; j--)
                                        if ((l = get_level(j)) && level_bonus(l))
                                        {
                                            const GLubyte *c = (level_opened(l) ?
                                                                gui_grn : gui_gry);

                                            gui_label(ld, level_name(l), GUI_SML, c, c);
                                        }

                                    gui_label(ld, "", GUI_SML, 0, 0);
                                }

                        gui_set_rect(kd, GUI_ALL);
                    }

                    gui_filler(jd);
                }

                gui_space(id);
            }
#ifdef CONFIG_INCLUDES_ACCOUNT
            else if (server_policy_get_d(SERVER_POLICY_EDITION) > -1 &&
                     server_policy_get_d(SERVER_POLICY_SHOP_ENABLED) &&
                     !CHECK_ACCOUNT_BANKRUPT)
            {
                balls_id = score_id = 0;

                /* Reverse-engineer initial wallet. */

                int coins = !resume ? curr_coins() : 0,
                    wallet = !resume ? account_get_d(ACCOUNT_DATA_WALLET_COINS) - coins :
                    account_get_d(ACCOUNT_DATA_WALLET_COINS);

                if ((jd = gui_hstack(id)))
                {
                    gui_filler(jd);

                    if ((kd = gui_harray(jd)))
                    {
                        wallet_id = gui_count(kd, ACCOUNT_WALLET_MAX_COINS, GUI_MED);
                        gui_label(kd, _("Wallet"), GUI_SML, GUI_COLOR_WHT);
                    }
                    if ((kd = gui_harray(jd)))
                    {
                        coins_id = gui_count(kd, 1000, GUI_MED);
                        gui_label(kd, _("Coins"), GUI_SML, GUI_COLOR_WHT);
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
#if NB_HAVE_PB_BOTH==1
                gui_score_board(id, (GUI_SCORE_COIN |
                                     GUI_SCORE_TIME |
                                     GUI_SCORE_GOAL), 1,
                                    !resume && !account_wgcl_name_read_only() &&
                                    (shop_product_available || challenge_disable_all_buttons) ? 0 : high);
#else
                gui_score_board(id, (GUI_SCORE_COIN |
                                     GUI_SCORE_TIME |
                                     GUI_SCORE_GOAL), 1,
                                    !resume &&
                                    (shop_product_available || challenge_disable_all_buttons) ? 0 : high);
#endif

                gui_campaign_stats(curr_level());

                gui_space(id);
            }
            else if (campaign_used() && (accessibility_get_d(ACCESSIBILITY_SLOWDOWN) >= 100 &&
#if NB_STEAM_API==0 && NB_EOS_SDK==0
                     !config_cheat() &&
#endif
                     (!config_get_d(CONFIG_SMOOTH_FIX) || video_perf() >= NB_FRAMERATE_MIN)))
            {
#if NB_HAVE_PB_BOTH==1
                gui_score_board(id, ((campaign_career_unlocked() ? GUI_SCORE_COIN : 0) |
                                     GUI_SCORE_TIME |
                                     (campaign_career_unlocked() ? GUI_SCORE_GOAL : 0)), 1,
                                    !resume && !account_wgcl_name_read_only() &&
                                    (shop_product_available || challenge_disable_all_buttons) ? 0 : high);
#else
                gui_score_board(id, ((campaign_career_unlocked() ? GUI_SCORE_COIN : 0) |
                                     GUI_SCORE_TIME |
                                     (campaign_career_unlocked() ? GUI_SCORE_GOAL : 0)), 1,
                                    !resume &&
                                    (shop_product_available || challenge_disable_all_buttons) ? 0 : high);
#endif

                gui_set_stats(curr_level());

                gui_space(id);
            }
#else
#if NB_HAVE_PB_BOTH==1
            gui_score_board(id, (GUI_SCORE_COIN |
                                 GUI_SCORE_TIME |
                                 GUI_SCORE_GOAL), 1,
                                !resume && !account_wgcl_name_read_only() &&
                                (shop_product_available || challenge_disable_all_buttons) ? 0 : high);
#else
            gui_score_board(id, (GUI_SCORE_COIN |
                                 GUI_SCORE_TIME |
                                 GUI_SCORE_GOAL), 1,
                                !resume &&
                                (shop_product_available || challenge_disable_all_buttons) ? 0 : high);
#endif

            gui_set_stats(curr_level());

            gui_space(id);
#endif

            if ((jd = gui_harray(id)))
            {
                /*
                 * Waiting for extra balls by collecting 100 coins
                 * --- OR ---
                 * check, if products is still available
                 */
                int btns_disabled = !resume &&
                                    ((config_get_d(CONFIG_NOTIFICATION_REWARD) && challenge_disable_all_buttons) ||
                                     (config_get_d(CONFIG_NOTIFICATION_SHOP) &&
                                      shop_product_available));

                int btn_ids[3] = { 0, 0, 0 };

                if (progress_done())
                    btn_ids[2] = gui_start(jd, _("Finish"), GUI_SML, GOAL_DONE, 0);
                else if (progress_last())
                    btn_ids[2] = gui_start(jd, _("Finish"), GUI_SML, GOAL_LAST, 0);
                else if (progress_next_avail())
                    btn_ids[2] = gui_start(jd, _("Next Level"), GUI_SML, GOAL_NEXT, 0);
                else
                    btn_ids[2] = gui_start(jd, _("Finish"), GUI_SML, GOAL_LAST, 0);

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                if (progress_same_avail() && !campaign_hardcore())
#else
                if (progress_same_avail())
#endif
                    btn_ids[1] = gui_start(jd, _("Retry Level"), GUI_SML, GOAL_SAME, 0);

                if (demo_saved() && save >= 1)
                    btn_ids[0] = gui_state(jd, _("Save Replay"), GUI_SML, GOAL_SAVE, 0);

                if (btns_disabled)
                    for (int i = 0; i < 3; i++)
                    {
                        if (!btn_ids[i]) continue;

                        gui_set_color(btn_ids[i], GUI_COLOR_GRY);
                        gui_set_state(btn_ids[i], GUI_NONE, 0);
                    }
            }

            gui_layout(id, 0, 0);
        }

        if ((id = gui_vstack(root_id)))
        {
            gui_space(id);

            if ((jd = gui_hstack(id)))
            {
                int back_btn_id = gui_back_button(jd);
                gui_space(jd);
            }

            gui_layout(id, -1, +1);
        }

#ifdef CONFIG_INCLUDES_ACCOUNT
        char account_coinsattr[MAXSTR], account_gemsattr[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(account_gemsattr, MAXSTR, "%s %d", GUI_DIAMOND,
                  account_get_d(ACCOUNT_DATA_WALLET_GEMS));
        sprintf_s(account_coinsattr, MAXSTR, "%s %d", GUI_COIN,
                  account_get_d(ACCOUNT_DATA_WALLET_COINS));
#else
        sprintf(account_gemsattr, "%s %d", GUI_DIAMOND,
                account_get_d(ACCOUNT_DATA_WALLET_GEMS));
        sprintf(account_coinsattr, "%s %d", GUI_COIN,
                account_get_d(ACCOUNT_DATA_WALLET_COINS));
#endif

        if (server_policy_get_d(SERVER_POLICY_SHOP_ENABLED))
        {
            if ((id = gui_vstack(root_id)))
            {
                gui_space(id);

                if ((jd = gui_hstack(id)))
                {
                    gui_space(jd);

                    if (!CHECK_ACCOUNT_BANKRUPT)
                    {
                        gui_state(jd, "+", GUI_XS, GOAL_IAP, 1);
                        gui_label(jd, account_gemsattr, GUI_XS, gui_wht, gui_cya);
                    }

                    gui_label(jd, account_coinsattr, GUI_XS, gui_wht, gui_yel);
                }

                gui_layout(id, +1, +1);
            }
        }
#endif
    }

    set_score_board(level_score(curr_level(), SCORE_COIN), progress_coin_rank(),
                    level_score(curr_level(), SCORE_TIME), progress_time_rank(),
                    level_score(curr_level(), SCORE_GOAL), progress_goal_rank());

    return root_id;
}

static int goal_enter(struct state *st, struct state *prev, int intent)
{
    score_count_anim_time = 0.2f;
    score_count_anim_index = 0;

    if (prev == &st_name)
        progress_rename(0);

    if (curr_mode() != MODE_CHALLENGE &&
        curr_mode() != MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
     && curr_mode() != MODE_HARDCORE
#endif
        )
        audio_music_fade_out(2.0f);

    video_clr_grab();

    resume = (prev != &st_play_loop || (prev == &st_goal && !resume_hold));

    if (prev == &st_play_loop)
    {
#ifdef MAPC_INCLUDES_CHKP
        checkpoints_stop();
#endif
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        if (campaign_used())
            audio_play(AUD_GOAL_CAMPAIGN, 1.0f);
#endif
    }

    return transition_slide(goal_gui(), 1, intent);
}

static int goal_leave(struct state *st, struct state *next, int id, int intent)
{
    if (!resume || !resume_hold)
    {
        resume = (next != &st_goal);

        if (!resume_hold)
            resume_hold = (next == &st_goal);

        resume = !resume_hold;
    }

    if (next == &st_null)
    {
        progress_exit();

        campaign_quit();
        set_quit();

        game_server_free(NULL);
        game_client_free(NULL);
    }

    return transition_slide(id, 0, intent);
}

static void goal_paint(int id, float t)
{
    game_client_draw(0, t);

    gui_paint(id);

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    if (console_gui_show())
        console_gui_death_paint();
#endif
    if (hud_visibility() || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        hud_paint();
}

static void goal_timer(int id, float dt)
{
    if (!resume || resume_hold)
    {
        static float t = 0.0f;

        if (time_state() > 1.0f)
            t += dt;

        geom_step(dt);
        game_server_step(dt);

        int record_screenanimations = resume && time_state() < 1.0f;
        int record_modes            = curr_mode() != MODE_NONE;
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        int record_campaign         = !campaign_hardcore_norecordings();
#else
        int record_campaign         = 1;
#endif

        game_client_blend(game_server_blend());
        game_client_sync(!resume
                      && record_screenanimations
                      && record_modes
                      && record_campaign ? demo_fp : NULL);

        while ((t > 0.05f && coins_id) &&
               (!resume && time_state() > 1.0f))
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
            if ((curr_mode() == MODE_CAMPAIGN ||
                 curr_mode() == MODE_NORMAL
#ifdef LEVELGROUPS_INCLUDES_ZEN
              || curr_mode() == MODE_ZEN
#endif
                ) &&
                server_policy_get_d(SERVER_POLICY_EDITION) > -1 &&
                server_policy_get_d(SERVER_POLICY_SHOP_ENABLED))
#else
            if ((curr_mode() == MODE_NORMAL
#ifdef LEVELGROUPS_INCLUDES_ZEN
                || curr_mode() == MODE_ZEN
#endif
                ) &&
                server_policy_get_d(SERVER_POLICY_EDITION) > -1 &&
                server_policy_get_d(SERVER_POLICY_SHOP_ENABLED))
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

#if NB_HAVE_PB_BOTH==1
                score_count_anim_time += 0.05f;

                if (score_count_anim_locked && score_count_anim_time > 0.2f)
                    score_count_anim_locked = 0;

                while (score_count_anim_time > 0.2f)
                {
                    score_count_anim_time -= 0.2f;

                    if (score_count_anim_index == 0 && !score_count_anim_locked)
                    {
                        audio_play("snd/rank_countup_1.ogg", 1.0f);
                        score_count_anim_locked = 1;
                        score_count_anim_index = 1;
                    }
                    if (score_count_anim_index == 1 && !score_count_anim_locked)
                    {
                        audio_play("snd/rank_countup_2.ogg", 1.0f);
                        score_count_anim_locked = 1;
                        score_count_anim_index = 0;
                    }
                }
#endif

#ifdef CONFIG_INCLUDES_ACCOUNT
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                if ((curr_mode() == MODE_CAMPAIGN ||
                     curr_mode() == MODE_NORMAL
#ifdef LEVELGROUPS_INCLUDES_ZEN
                  || curr_mode() == MODE_ZEN
#endif
                    ) &&
                    server_policy_get_d(SERVER_POLICY_EDITION) > -1 &&
                    server_policy_get_d(SERVER_POLICY_SHOP_ENABLED)) {
#else
                if ((curr_mode() == MODE_NORMAL || curr_mode() == MODE_ZEN) &&
                    server_policy_get_d(SERVER_POLICY_EDITION) > -1 &&
                    server_policy_get_d(SERVER_POLICY_SHOP_ENABLED)) {
#endif
                    gui_set_count(wallet_id, wallet + 1);
                    gui_pulse(wallet_id, 1.1f);
                }
#endif

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                if (progress_reward_ball(score + 1) &&
                    (curr_mode() != MODE_HARDCORE &&
                     curr_mode() != MODE_CAMPAIGN &&
                     curr_mode() != MODE_NORMAL
#ifdef LEVELGROUPS_INCLUDES_ZEN
                  && curr_mode() != MODE_ZEN
#endif
                    ))
#else
                if (progress_reward_ball(score + 1) &&
                    (curr_mode() != MODE_NORMAL
#ifdef LEVELGROUPS_INCLUDES_ZEN
                  && curr_mode() != MODE_ZEN
#endif
                     ))
#endif
                {
                    challenge_caught_extra = 1;

                    gui_set_count(balls_id, balls + 1);
                    gui_pulse(balls_id, 2.0f);

                    if (config_get_d(CONFIG_NOTIFICATION_REWARD) == 0)
                        audio_play(AUD_BALL, 1.0f);
                }
            }

            t -= 0.05f;
        }
    }

    if (challenge_caught_extra && config_get_d(CONFIG_NOTIFICATION_REWARD) &&
        (curr_mode() == MODE_CHALLENGE ||
         curr_mode() == MODE_BOOST_RUSH))
        goto_state(&st_goal_extraballs);

#ifdef CONFIG_INCLUDES_ACCOUNT
    if (!resume && time_state() >= 1.0f &&
        shop_product_available && config_get_d(CONFIG_NOTIFICATION_SHOP) &&
        server_policy_get_d(SERVER_POLICY_EDITION) > -1 &&
        server_policy_get_d(SERVER_POLICY_SHOP_ENABLED))
        goto_state(&st_goal_shop);
#endif

    gui_timer(id, dt);
    hud_timer(dt);
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
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
         && current_platform == PLATFORM_PC
#endif
            )
            return goal_action(GUI_SCORE, GUI_SCORE_NEXT(gui_score_get()));
        if (config_tst_d(CONFIG_KEY_RESTART, c)
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
         && !campaign_hardcore()
#endif
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
         && current_platform == PLATFORM_PC
#endif
            )
            return goal_action(progress_same_avail() ? GOAL_SAME :
                                                       GUI_NONE, 0);
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
        int msgid = gui_title_header(id, _("New balls earned!"),
                                         GUI_MED, GUI_COLOR_GRN);
        gui_space(id);
        gui_multi(id, _("You've earned extra ball by collecting\n"
                        "100 coins in a single set."),
                      GUI_SML, GUI_COLOR_WHT);
        gui_pulse(msgid, 1.2f);
        gui_layout(id, 0, 0);
    }

    return id;
}

static int goal_extraballs_enter(struct state *st, struct state *prev, int intent)
{
    audio_play("snd/extralives.ogg", 1.0f);
    return transition_slide(goal_extraballs_gui(), 1, intent);
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
            return exit_state(&st_goal);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return exit_state(&st_goal);
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
        int msgid = gui_title_header(id, _("Product available!"),
                                         GUI_MED, GUI_COLOR_GRN);
        gui_space(id);

        const char *prodname = "none";

#ifdef CONFIG_INCLUDES_ACCOUNT
#ifdef LEVELGROUPS_INCLUDES_ZEN
        if (!account_get_d(ACCOUNT_PRODUCT_MEDIATION) &&
            (account_get_d(ACCOUNT_DATA_WALLET_COINS) + curr_score()) >= 120)
            prodname = _("Mediation");
        else if (!account_get_d(ACCOUNT_PRODUCT_BONUS) &&
            (account_get_d(ACCOUNT_DATA_WALLET_COINS) + curr_score()) >= 180)
            prodname = _("Bonus Pack");
        else if (!account_get_d(ACCOUNT_PRODUCT_BALLS) &&
            (account_get_d(ACCOUNT_DATA_WALLET_COINS) + curr_score()) >= 250)
            prodname = _("Online Balls");
        else if(!account_get_d(ACCOUNT_PRODUCT_LEVELS) &&
            (account_get_d(ACCOUNT_DATA_WALLET_COINS) + curr_score()) >= 310)
            prodname = _("Extra Levels");
#else
        if (!account_get_d(ACCOUNT_PRODUCT_BONUS) &&
            (account_get_d(ACCOUNT_DATA_WALLET_COINS) + curr_score()) >= 180)
            prodname = _("Bonus Pack");
        else if (!account_get_d(ACCOUNT_PRODUCT_BALLS) &&
            (account_get_d(ACCOUNT_DATA_WALLET_COINS) + curr_score()) >= 250)
            prodname = _("Online Balls");
        else if (!account_get_d(ACCOUNT_PRODUCT_LEVELS) &&
            (account_get_d(ACCOUNT_DATA_WALLET_COINS) + curr_score()) >= 310)
            prodname = _("Extra Levels");
#endif
#endif

        char productmsg[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(productmsg, MAXSTR,
#else
        sprintf(productmsg,
#endif
                _("You have enough coins to buy\n%s.\nTry it out!"), prodname);

        gui_multi(id, productmsg, GUI_SML, GUI_COLOR_WHT);

        gui_pulse(msgid, 1.2f);
        gui_layout(id, 0, 0);
    }

    return id;
}

static int goal_shop_enter(struct state *st, struct state *prev, int intent)
{
    audio_play("snd/extralives.ogg", 1.0f);
    return transition_slide(goal_shop_gui(), 1, intent);
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
            return exit_state(&st_goal);
    }

    return 1;
}

static int goal_shop_buttn(int b, int d)
{
    if (d)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return exit_state(&st_goal);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return exit_state(&st_goal);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int restrict_hardcore_nextstate = 0;

static int goal_hardcore_enter(struct state *st, struct state *prev, int intent)
{
#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT)
    account_wgcl_restart_attempt();
#endif

    restrict_hardcore_nextstate = 0;
    powerup_stop();

    video_clr_grab();

    resume = (prev != &st_play_loop);

    if (prev == &st_play_loop)
    {
#ifdef MAPC_INCLUDES_CHKP
        checkpoints_stop();
#endif
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        if (campaign_used())
            audio_play(AUD_GOAL_CAMPAIGN, 1.0f);
#endif
    }

    game_fade(+1.0f);

    return goal_gui();
}

static void goal_hardcore_timer(int id, float dt)
{
    geom_step(dt);

    if (!restrict_hardcore_nextstate)
    {
        game_server_step(dt);
        game_client_sync(NULL);
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
#endif
                goto_play_level();
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
    goal_leave,
    goal_paint,
    goal_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    goal_keybd,
    goal_buttn
};

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
struct state st_goal_hardcore = {
    goal_hardcore_enter,
    shared_leave,
    goal_hardcore_paint,
    goal_hardcore_timer
};
#endif

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
    goal_extraballs_buttn
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
    goal_shop_buttn
};
