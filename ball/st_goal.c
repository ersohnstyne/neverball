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

#ifndef NDEBUG
#include <assert.h>
#elif defined(_MSC_VER) && defined(_AFXDLL)
#include <afx.h>
/**
 * HACK: assert() for Microsoft Windows Apps in Release builds
 * will be replaced to VERIFY() - Ersohn Styne
 */
#define assert VERIFY
#else
#define assert(_x) (_x)
#endif

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

/*
 * HACK: Used with console version
 */
#include "console_control_gui.h"

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
    GOAL_LAST
};

static int goal_intro_animation_phase;

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
static int resume_locked;

static int goal_action(int tok, int val)
{
    /* Waiting for extra balls by collecting 100 coins */
    if (challenge_disable_all_buttons)
    {
        audio_play(AUD_DISABLED, 1.0f);
        return 1;
    }

    GENERIC_GAMEMENU_ACTION;

    goal_intro_animation_phase = 0;

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
            resume_locked = 1;
            gui_score_set(val);
            return goto_state(&st_goal);

        case GOAL_NEXT:
            if (progress_next())
            {
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                if (campaign_used() && campaign_hardcore())
                    campaign_hardcore_nextlevel();
#endif
                return goto_play_level();
            }
            break;

        case GOAL_SAME:
            if (progress_same())
                return goto_play_level();
            break;
    }

    return 1;
}

static int goal_gui(void)
{
    const char *s1 = _("New Record");
    const char *s2 = _("GOAL");
    const char *s3 = _("Master complete");

    struct level *l = curr_level();

    int id, jd, kd, ld, md;
    int root_id;

    int high   = progress_lvl_high();
    int master = l ? l->is_master : 0;

    shop_product_available = 0;

    challenge_caught_extra = 0;
    challenge_disable_all_buttons = 0;

    if (goal_intro_animation_phase == 1)
    {
        if ((id = gui_title_header(0, s2, GUI_LRG, gui_blu, gui_grn)))
        {
            gui_set_slide(id, GUI_E | GUI_FLING | GUI_EASE_BACK, 0, 0.5f, 0);
            gui_layout(id, 0, 0);
            gui_pulse(id, 1.2f);
            return id;
        }
        else return 0;
    }

#if NB_HAVE_PB_BOTH==1
    if ((!resume || (!resume_locked && goal_intro_animation_phase == 2)) &&
        config_get_d(CONFIG_NOTIFICATION_SHOP) &&
        server_policy_get_d(SERVER_POLICY_EDITION) > -1)
#else
    if (!resume || (!resume_locked && goal_intro_animation_phase == 2))
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

    root_id = !console_gui_shown() && progress_next_avail() && !progress_done() ? gui_root() : 0;

    //if ((root_id = gui_root()))
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
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
                      !config_cheat() &&
#endif
                      (!config_get_d(CONFIG_SMOOTH_FIX) || video_perf() >= NB_FRAMERATE_MIN))))
                    gid = gui_title_header(jd, s1, GUI_MED, GUI_COLOR_GRN);
                else
                    gid = gui_title_header(jd, master ? s3 : s2, GUI_MED,
                                               gui_blu, gui_grn);

                if (!resume)
                    gui_pulse(gid, 1.2f);

                gui_set_rect(jd, GUI_ALL);

                if (!resume_locked && goal_intro_animation_phase == 2)
                    gui_set_slide(jd, GUI_N | GUI_FLING | GUI_EASE_ELASTIC, 0, 0.8f, 0);
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
                int coins = (!resume || (!resume_locked && goal_intro_animation_phase == 2)) ? curr_coins() : 0,
                    score = (!resume || (!resume_locked && goal_intro_animation_phase == 2)) ? curr_score() - coins : curr_score(),
                    balls = curr_balls();

                /* Reverse-engineer initial score and balls. */

                if ((!resume || (!resume_locked && goal_intro_animation_phase == 2))
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

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                            if (curr_mode() != MODE_HARDCORE)
#endif
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

                    if (!resume_locked && goal_intro_animation_phase == 2)
                        gui_set_slide(jd, GUI_S | GUI_FLING | GUI_EASE_ELASTIC, 0.2f, 0.8f, 0);
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

                int coins  = (!resume || (!resume_locked && goal_intro_animation_phase == 2)) ? curr_coins() : 0,
                    wallet = (!resume || (!resume_locked && goal_intro_animation_phase == 2)) ? account_get_d(ACCOUNT_DATA_WALLET_COINS) - coins :
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

                    if ((!resume_locked && goal_intro_animation_phase == 2))
                        gui_set_slide(jd, GUI_S | GUI_FLING | GUI_EASE_ELASTIC, 0.2f, 0.8f, 0);
                }

                gui_space(id);
            }
#endif

            int scoreboard_id = 0;

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            if (!campaign_used())
            {
                if ((scoreboard_id = gui_hstack(id)))
                {
                    gui_filler(scoreboard_id);
#if NB_HAVE_PB_BOTH==1
                    gui_score_board(scoreboard_id,
                                    (GUI_SCORE_COIN |
                                     GUI_SCORE_TIME |
                                     GUI_SCORE_GOAL), 1,
                                    (!resume || (!resume_locked && goal_intro_animation_phase == 2)) && !account_wgcl_name_read_only() &&
                                    (shop_product_available || challenge_disable_all_buttons) ? 0 : high);
#else
                    gui_score_board(scoreboard_id,
                                    (GUI_SCORE_COIN |
                                     GUI_SCORE_TIME |
                                     GUI_SCORE_GOAL), 1,
                                    (!resume || (!resume_locked && goal_intro_animation_phase == 2)) &&
                                    (shop_product_available || challenge_disable_all_buttons) ? 0 : high);
#endif
                }

                gui_campaign_stats(curr_level());

                gui_space(id);
            }
            else if (campaign_used() && (accessibility_get_d(ACCESSIBILITY_SLOWDOWN) >= 100 &&
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
                     !config_cheat() &&
#endif
                     (!config_get_d(CONFIG_SMOOTH_FIX) || video_perf() >= NB_FRAMERATE_MIN)))
            {
                if ((scoreboard_id = gui_hstack(id)))
                {
                    gui_filler(scoreboard_id);
#if NB_HAVE_PB_BOTH==1
                    gui_score_board(scoreboard_id,
                                    campaign_career_unlocked() ? (GUI_SCORE_COIN |
                                                                  GUI_SCORE_TIME |
                                                                  GUI_SCORE_GOAL) :
                                                                 GUI_SCORE_TIME,
                                    1,
                                    (!resume || (!resume_locked && goal_intro_animation_phase == 2)) && !account_wgcl_name_read_only() &&
                                    (shop_product_available || challenge_disable_all_buttons) ? 0 : high);
#else
                    gui_score_board(scoreboard_id,
                                    campaign_career_unlocked() ? (GUI_SCORE_COIN |
                                                                  GUI_SCORE_TIME |
                                                                  GUI_SCORE_GOAL) :
                                                                 GUI_SCORE_TIME,
                                    1,
                                    (!resume || (!resume_locked && goal_intro_animation_phase == 2)) &&
                                    (shop_product_available || challenge_disable_all_buttons) ? 0 : high);
#endif
                }

                gui_set_stats(curr_level());

                gui_space(id);
            }
#else
            if ((scoreboard_id = gui_hstack(id)))
            {
                gui_filler(scoreboard_id);
#if NB_HAVE_PB_BOTH==1
                gui_score_board(scoreboard_id,
                                (GUI_SCORE_COIN |
                                 GUI_SCORE_TIME |
                                 GUI_SCORE_COIN), 1,
                                (!resume || (!resume_locked && goal_intro_animation_phase == 2)) && !account_wgcl_name_read_only() &&
                                (shop_product_available || challenge_disable_all_buttons) ? 0 : high);
#else
                gui_score_board(scoreboard_id,
                                (GUI_SCORE_COIN |
                                 GUI_SCORE_TIME |
                                 GUI_SCORE_GOAL), 1,
                                (!resume || (!resume_locked && goal_intro_animation_phase == 2)) &&
                                (shop_product_available || challenge_disable_all_buttons) ? 0 : high);
#endif
            }

            gui_set_stats(curr_level());

            gui_space(id);
#endif

            if (!resume_locked && goal_intro_animation_phase == 2 &&
                scoreboard_id)
                gui_set_slide(scoreboard_id, GUI_S | GUI_FLING | GUI_EASE_ELASTIC, 0.4f, 0.8f, 0);

            if ((jd = gui_harray(id)))
            {
                /*
                 * Waiting for extra balls by collecting 100 coins
                 * --- OR ---
                 * check, if products is still available
                 */
                const int btns_disabled = (!resume || (!resume_locked && goal_intro_animation_phase == 2)) &&
                                          ((config_get_d(CONFIG_NOTIFICATION_REWARD) && challenge_disable_all_buttons) ||
                                           (config_get_d(CONFIG_NOTIFICATION_SHOP) &&
                                            shop_product_available));

                int btn_ids[3] = { 0, 0, 0 };

                const char *next_btn_text = progress_next_avail() ? N_("Next Level") :
                                                                    N_("Finish");

                const int next_btn_tok = progress_done() ? GOAL_DONE :
                                                           progress_next_avail() ? GOAL_NEXT :
                                                                                   GOAL_LAST;

                btn_ids[2] = gui_start(jd, _(next_btn_text), GUI_SML, next_btn_tok, 0);

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                if (progress_same_avail() &&
                    (!campaign_hardcore() && curr_mode() != MODE_HARDCORE))
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

                if (!resume_locked && goal_intro_animation_phase == 2)
                    gui_set_slide(jd, GUI_S | GUI_FLING | GUI_EASE_ELASTIC, 0.6, 0.8f, 0.05f);
            }

            gui_layout(id, 0, 0);
        }

        if (root_id && (id = gui_vstack(root_id)))
        {
            gui_space(id);

            if ((jd = gui_hstack(id)))
            {
                int back_btn_id = gui_back_button(jd);
                gui_space(jd);

                if (!resume_locked && goal_intro_animation_phase == 2)
                    gui_set_slide(back_btn_id, GUI_N | GUI_FLING | GUI_EASE_ELASTIC, 0.0f, 0.8f, 0.05f);
            }

            gui_layout(id, -1, +1);
        }
    }

    set_score_board(level_score(curr_level(), SCORE_COIN), progress_coin_rank(),
                    level_score(curr_level(), SCORE_TIME), progress_time_rank(),
                    level_score(curr_level(), SCORE_GOAL), progress_goal_rank());

    return root_id ? root_id : id;
}

static int goal_enter(struct state *st, struct state *prev, int intent)
{
    score_count_anim_time  = 0.2f;
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

    /* Check if we came from a known previous state. */

    const int stat_allow_intro = prev == &st_play_loop;

    goal_intro_animation_phase =  stat_allow_intro && prev != &st_goal ? 1 :
                                 !stat_allow_intro && prev == &st_goal ? 2 : 0;

    resume        = !stat_allow_intro;

    if (resume_locked)
        resume_locked = !stat_allow_intro;

#ifndef NDEBUG
    if (stat_allow_intro)
        assert(stat_allow_intro && prev != &st_goal &&
               goal_intro_animation_phase == 1 && !resume);
#endif

    /* Note the current status if we got here from elsewhere. */

    if (!resume)
    {
#if NB_HAVE_PB_BOTH==1 && \
    defined(CONFIG_INCLUDES_ACCOUNT) && defined(ENABLE_POWERUP)
        powerup_stop();
#endif
#ifdef MAPC_INCLUDES_CHKP
        checkpoints_stop();
#endif
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        if (campaign_used())
            audio_play(AUD_GOAL_CAMPAIGN, 1.0f);
#endif
    }

    if (goal_intro_animation_phase != 0)
        return goal_gui();

    return transition_slide(goal_gui(), 1, intent);
}

static int goal_leave(struct state *st, struct state *next, int id, int intent)
{
    if (!resume_locked)
        resume_locked = next != &st_goal;

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

    if (next == &st_goal && resume_locked)
    {
        gui_delete(id);
        return 0;
    }

    return transition_slide(id, 0, intent);
}

static void goal_paint(int id, float t)
{
    game_client_draw(0, t);

    gui_paint(id);
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    if (console_gui_shown())
        console_gui_death_paint();
#endif
    if (hud_visibility() || config_get_d(CONFIG_SCREEN_ANIMATIONS))
    {
        hud_paint();
        hud_lvlname_paint();
    }
}

static void goal_timer(int id, float dt)
{
    const float goal_time_state = time_state();

    if (goal_intro_animation_phase == 1)
    {
        if (goal_time_state >= 2.0f)
        {
            goal_intro_animation_phase = 2;
            goto_state(&st_goal);

            return;
        }
    }

    gui_timer(id, dt);
    hud_timer(dt);

    if (!resume || (!resume_locked && goal_intro_animation_phase == 2))
    {
        static float t = 0.0f;

        if (goal_time_state > 1.0f)
            t += dt;

        geom_step(dt);
        game_server_step(dt);

        int record_modes    = curr_mode() != MODE_NONE;
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        int record_campaign = !campaign_hardcore_norecordings();
#else
        int record_campaign = 1;
#endif

        game_client_sync(!resume
                      && goal_time_state < 1.0f
                      && record_modes
                      && record_campaign
                      && goal_intro_animation_phase == 1 ? demo_fp : NULL);
        game_client_blend(game_server_blend());

        while ((t > 0.05f && coins_id) &&
               goal_time_state > 1.0f)
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
    if ((!resume || (!resume_locked && goal_intro_animation_phase == 2)) &&
        goal_time_state >= 1.0f &&
        shop_product_available && config_get_d(CONFIG_NOTIFICATION_SHOP) &&
        server_policy_get_d(SERVER_POLICY_EDITION) > -1 &&
        server_policy_get_d(SERVER_POLICY_SHOP_ENABLED))
        goto_state(&st_goal_shop);
#endif
}

static int goal_click(int b, int d)
{
    if (goal_intro_animation_phase == 1)
        return (b == SDL_BUTTON_LEFT && d) ?
               st_buttn(config_get_d(CONFIG_JOYSTICK_BUTTON_A), 1) : 1;

    return gui_click(b, d) ?
           st_buttn(config_get_d(CONFIG_JOYSTICK_BUTTON_A), 1) : 1;
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
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
         && current_platform == PLATFORM_PC
#endif
            )
        {
            if (progress_same())
            {
#if NB_HAVE_PB_BOTH==1 && \
    defined(CONFIG_INCLUDES_ACCOUNT) && defined(ENABLE_POWERUP)
                powerup_stop();
#endif
                return goto_play_level();
            }
            else
            {
                /* Can't do yet, play buzzer sound. */

                audio_play(AUD_DISABLED, 1.0f);
            }
        }
    }

    return 1;
}

static int goal_buttn(int b, int d)
{
    if (d)
    {
        if (goal_intro_animation_phase == 1 &&
            config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
        {
            goal_intro_animation_phase = 2;
            return goto_state(&st_goal);
        }

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
    restrict_hardcore_nextstate = 0;
#if NB_HAVE_PB_BOTH==1 && \
    defined(CONFIG_INCLUDES_ACCOUNT) && defined(ENABLE_POWERUP)
    powerup_stop();
#endif

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
    goal_click,
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
