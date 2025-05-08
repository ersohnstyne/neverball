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

#include <assert.h>

#if NB_HAVE_PB_BOTH==1
#include "solid_chkp.h"
#include "campaign.h" /* New: Campaign levels */
#include "boost_rush.h"
#include "mediation.h"
#include "networking.h"
#include "powerup.h"
#include "account.h"
#include "account_wgcl.h"
#include "accessibility.h"
#if ENABLE_RFD==1
#include "rfd.h"
#endif
#if NB_STEAM_API==1
#include "score_online.h"
#endif
#endif

#if ENABLE_MOON_TASKLOADER!=0
#define SKIP_MOON_TASKLOADER
#include "moon_taskloader.h"
#endif

#ifdef MAPC_INCLUDES_CHKP
#include "checkpoints.h" /* New: Checkpoints */
#endif

#include "activity_services.h"

#include "progress.h"
#include "config.h"
#include "demo.h"
#include "level.h"
#include "set.h"
#include "lang.h"
#include "score.h"
#include "audio.h"
#include "video.h"
#include "util.h"

#include "state.h"

#if ENABLE_DUALDISPLAY==1
#include "game_dualdisplay.h"
#endif
#include "game_common.h"
#include "game_client.h"
#include "game_server.h"

#include "st_level.h"

/*---------------------------------------------------------------------------*/

#ifdef CONFIG_INCLUDES_ACCOUNT
#ifdef MAPC_INCLUDES_CHKP
#if ENABLE_RFD==1
/**
 * Pennyball + Neverball - Recipes for Disaster
 *
 * Paid debts after timer expires.
 *
 * If they owe gems and can't pay, but net-worth is greater than debt,
 * they need to raise gems by selling some products and powerups.
 *
 * Try to not overuse the gems as it causes bankrupt.
 */
#define PROGRESS_PLAYER_PAYDEBT_BALLS                                  \
    do {                                                               \
        if (chkp.rfd_balls > 0)                                        \
            chkp.rfd_balls -= 1;                                       \
        else if (account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES) > 0) {} \
        else chkp.balls -= 1;                                          \
                                                                       \
        if (curr.rfd_balls > 0)                                        \
            curr.rfd_balls -= 1;                                       \
        else if (curr.balls > 0)                                       \
            curr.balls -= 1;                                           \
        else if (account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES) > 0)    \
        {                                                              \
            account_wgcl_do_add(0, 0, -1, 0, 0, 0);                    \
            account_wgcl_save();                                       \
        }                                                              \
        else                                                           \
            curr.balls -= 1;                                           \
    } while (0)
#else
/**
 * Paid debts after timer expires.
 *
 * If they owe gems and can't pay, but net-worth is greater than debt,
 * they need to raise gems by selling some products and powerups.
 *
 * Try to not overuse the gems as it causes bankrupt.
 */
#define PROGRESS_PLAYER_PAYDEBT_BALLS                               \
    do {                                                            \
        if (account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES) > 0) {}   \
        else chkp.balls -= 1;                                       \
                                                                    \
        if (curr.balls > 0)                                         \
            curr.balls -= 1;                                        \
        else if (account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES) > 0) \
        {                                                           \
            account_wgcl_do_add(0, 0, -1, 0, 0, 0);                 \
            account_wgcl_save();                                    \
        }                                                           \
        else                                                        \
            curr.balls -= 1;                                        \
    } while (0)
#endif
#else
#if ENABLE_RFD==1
/**
 * Pennyball + Neverball - Recipes for Disaster
 *
 * Paid debts after timer expires.
 *
 * If they owe gems and can't pay, but net-worth is greater than debt,
 * they need to raise gems by selling some products and powerups.
 *
 * Try to not overuse the gems as it causes bankrupt.
 */
#define PROGRESS_PLAYER_PAYDEBT_BALLS                               \
    do {                                                            \
        if (curr.rfd_balls > 0)                                     \
            curr.rfd_balls -= 1;                                    \
        else if (curr.balls > 0)                                    \
            curr.balls -= 1;                                        \
        else if (account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES) > 0) \
        {                                                           \
            account_wgcl_do_add(0, 0, -1, 0, 0, 0);                 \
            account_wgcl_save();                                    \
        }                                                           \
        else curr.balls -= 1;                                       \
    } while (0)
#else
/**
 * Paid debts after timer expires.
 *
 * If they owe gems and can't pay, but net-worth is greater than debt,
 * they need to raise gems by selling some products and powerups.
 *
 * Try to not overuse the gems as it causes bankrupt.
 */
#define PROGRESS_PLAYER_PAYDEBT_BALLS                               \
    do {                                                            \
        if (curr.balls > 0)                                         \
            curr.balls -= 1;                                        \
        else if (account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES) > 0) \
        {                                                           \
            account_wgcl_do_add(0, 0, -1, 0, 0, 0);                 \
            account_wgcl_save();                                    \
        }                                                           \
        else curr.balls -= 1;                                       \
    } while (0)
#endif
#endif
#else
#ifdef MAPC_INCLUDES_CHKP
/**
 * Paid debts after timer expires.
 *
 * You need to join Pennyball Discord Server in order
 * to activate the net-worth for player account:
 *
 * https://discord.gg/qnJR263Hm2/
 */
#define PROGRESS_PLAYER_PAYDEBT_BALLS \
    do {                              \
         curr.balls -= 1;             \
         if (chkp.balls > -1)         \
             chkp.balls -= 1;         \
    } while (0)
#else
/**
 * Paid debts after timer expires.
 *
 * You need to join Pennyball Discord Server in order
 * to activate the net-worth for player account:
 *
 * https://discord.gg/qnJR263Hm2/
 */
#define PROGRESS_PLAYER_PAYDEBT_BALLS \
    curr.balls -= 1
#endif
#endif

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
/**
 * Their debt exceeds above the limit from the net-worth.
 *
 * If debt is greater than net-worth, it will gone bankrupt.  The net-worth is
 * the total value of the coins,  gems and the resale value  of their powerups
 * that have not yet been used.
 *
 * If they bankrupt, all products will be sold, then all the wallet
 * will be transferred to the player to whom they owe the debt.
 */
#define PROGRESS_PLAYER_BANKRUPT \
    do { if (progress_dead()) {  \
        next = 0; done = 0;      \
    } } while (0)
#else
/**
 * Their debt exceeds above the limit from the net-worth.
 *
 * You need to join Pennyball Discord Server in order
 * to activate the net-worth for player account:
 *
 * https://discord.gg/qnJR263Hm2/
 */
#define PROGRESS_PLAYER_BANKRUPT \
    do { if (progress_dead()) {  \
        next = 0; done = 0;      \
    } } while (0)
#endif

#define PROGRESS_DEBUG_CHECK_IS_INIT_FUNC_BOOL \
    do {                                       \
        assert(is_init);                       \
        if (!is_init) return 0;                \
    } while (0)

#define PROGRESS_DEBUG_CHECK_IS_INIT_FUNC_VOID \
    do {                                       \
        assert(is_init);                       \
        if (!is_init) return;                  \
    } while (0)

/*---------------------------------------------------------------------------*/

#if ENABLE_LIVESPLIT!=0
struct progress_livesplit
{
    int started; /* S = 83; 0 = Disabled */
    int paused;  /* P = 80; 0 = Disabled */
    int reseted; /* R = 82; 0 = Disabled */

    int level;   /* 1 - infinite; -1 = Disabled */

    float game_time;
};

static struct progress_livesplit curr_livesplit;
#endif

struct progress
{
#if ENABLE_RFD==1
    /* Pennyball + Neverball - Recipes for Disaster */

    int   rfd_balls;
    int   rfd_earninator;
    int   rfd_floatifier;
    int   rfd_speedifier;
#endif

    int   balls;
    int   score;
    int   times;
    float speedpercent;
};

static int is_init = 0;

static int max_speed = 0;
static int exceed_speed = 0;

static int replay = 0;

static int mode = MODE_NORMAL;

static struct level *level, *next;

static int loading = 0; /* L = 76; 0 = Disabled */
static int done    = 0;

#ifdef MAPC_INCLUDES_CHKP
static struct progress curr, prev, chkp;
#else
static struct progress curr, prev;
#endif

/* Set stats. */

static int score_rank = RANK_LAST;
static int times_rank = RANK_LAST;

/* Level stats. */

static int lvl_warn_timer;

static int status       = GAME_NONE;
static int coins        = 0;
static int timer_offset = 0;
#ifdef MAPC_INCLUDES_CHKP
/*
 * Precalculated total timer for each levels with checkpoints.
 */
#define    timer timer_total

static int timer_total  = 0;
#else
static int timer        = 0;
#endif

static int goal   = 0; /* Current goal value */
static int goal_i = 0; /* Initial goal value */

static int goal_e = 0; /* Goal enabled flag  */

static int time_rank = RANK_LAST;
static int goal_rank = RANK_LAST;
static int coin_rank = RANK_LAST;

/* Extension time */

static int extended       = 0;
static int extended_timer = 0;

/*---------------------------------------------------------------------------*/

#if ENABLE_LIVESPLIT!=0
void progress_livesplit_init(int m)
{
    curr_livesplit.reseted = 'R';
    curr_livesplit.started = 0;
    curr_livesplit.paused  = 0;

    if (mode == MODE_CHALLENGE
     || mode == MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
     || mode == MODE_HARDCORE
#endif
        )
        curr_livesplit.level = 1;
    else
        curr_livesplit.level = -1;

    curr_livesplit.game_time = 0;
}

void progress_livesplit_exit(void)
{
    curr_livesplit.reseted = 'R';
    curr_livesplit.started = 0;
    curr_livesplit.paused  = 0;
    curr_livesplit.level   = -1;

    curr_livesplit.game_time = 0;
}

void progress_livesplit_step(float dt)
{
    curr_livesplit.game_time += dt;
}

void progress_livesplit_stat(int s)
{
    if (curr_livesplit.level > 0 &&
        (mode == MODE_CHALLENGE
      || mode == MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
      || mode == MODE_HARDCORE
#endif
        ))
    {
        curr_livesplit.reseted = 0;

        if (s == GAME_GOAL)
            curr_livesplit.level++;

        curr_livesplit.paused  = 'P';
        curr_livesplit.started = 0;
    }
}

void progress_livesplit_pause(int paused)
{
    if (paused != curr_livesplit.paused &&
        (mode == MODE_CHALLENGE
      || mode == MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
      || mode == MODE_HARDCORE
#endif
        ))
    {
        curr_livesplit.reseted = 0;

        curr_livesplit.paused  = paused ? 'P' :  0;
        curr_livesplit.started = paused ?  0  : 'S';
    }
}

void progress_livesplit_next(void)
{
    if (curr_livesplit.level > 0 &&
        (mode == MODE_CHALLENGE
      || mode == MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
      || mode == MODE_HARDCORE
#endif
        ))
        curr_livesplit.reseted = 0;
}

int progress_livesplit_started(void)
{
    return curr_livesplit.started;
}

int progress_livesplit_paused(void)
{
    return curr_livesplit.paused;
}

int progress_livesplit_reseted(void)
{
    return curr_livesplit.reseted;
}

int progress_livesplit_level(void)
{
    return curr_livesplit.level;
}

float progress_livesplit_game_time(void)
{
    return curr_livesplit.game_time;
}
#endif

/*---------------------------------------------------------------------------*/

static int need_coin_val = 0;

void progress_rush_collect_coin_value(int coin_val)
{
    if (curr_mode() == MODE_BOOST_RUSH)
    {
        collect_coin_value(coin_val);

        need_coin_val += coin_val;
        while (need_coin_val > 9)
        {
            curr.speedpercent += 14.28571429f;

            if (curr.speedpercent >= 100.0f)
                curr.speedpercent = 100.0f;

            need_coin_val -= 10;
        }
    }
}

/*---------------------------------------------------------------------------*/

void progress_init_home(void)
{
    if (is_init) return;

    mode   = MODE_NONE;
    replay = 0;

    curr.balls        = 0;
    curr.score        = 0;
    curr.times        = 0;
    curr.speedpercent = 0;

    done = 0;

#if ENABLE_LIVESPLIT!=0
    progress_livesplit_init(mode);
#endif

    is_init = 1;
}

void progress_init(int m)
{
    if (is_init) return;

    need_coin_val = 0;
    mode          = m;
    replay        = 0;

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    curr.balls    = (m == MODE_HARDCORE ? 0 : 2);
#else
    curr.balls    = 2;
#endif

#if ENABLE_RFD==1
    /* Pennyball + Neverball - Recipes for Disaster */

    curr.rfd_balls      = rfd_get_d(RFD_CHALLENGE_BALLS);
    curr.rfd_earninator = rfd_get_d(RFD_CHALLENGE_EARNINATOR);
    curr.rfd_floatifier = rfd_get_d(RFD_CHALLENGE_FLOATIFIER);
    curr.rfd_speedifier = rfd_get_d(RFD_CHALLENGE_SPEEDIFIER);
#endif

#ifdef CONFIG_INCLUDES_ACCOUNT
    /* HACK: Those purchased balls are associated from the shop. */

    if (account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES) > 0 &&
        server_policy_get_d(SERVER_POLICY_EDITION) > 0)
    {
        curr.balls -= account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES);
        curr.balls = MAX(curr.balls, 0);
    }
#endif

    curr.score        = 0;
    curr.times        = 0;
    curr.speedpercent = 0;

    prev = curr;
#ifdef MAPC_INCLUDES_CHKP
    chkp = prev;
#endif

    score_rank = RANK_LAST;
    times_rank = RANK_LAST;

    done = 0;

#if ENABLE_LIVESPLIT!=0
    progress_livesplit_init(mode);
#endif

#if ENABLE_DUALDISPLAY==1
    game_dualdisplay_set_mode(m);
    game_dualdisplay_set_heart(curr.balls);
#endif

    is_init = 1;
}

void progress_extend(void)
{
    PROGRESS_DEBUG_CHECK_IS_INIT_FUNC_VOID;

    extended    = 1;
    status      = GAME_NONE;
    curr.times -= extended_timer;
    curr.balls += 1;

#if ENABLE_DUALDISPLAY==1
    game_dualdisplay_set_heart(curr.balls);
#endif
}

int progress_extended(void)
{
    return extended;
}

/*---------------------------------------------------------------------------*/

#if ENABLE_MOON_TASKLOADER!=0 && !defined(SKIP_MOON_TASKLOADER)
static int moon_taskloader_phase = 0;

static void init_level_moon_taskloader_done(void *data, void *done_data)
{
    struct game_moon_taskloader_info *mtli = (struct moon_taskloader_info *) data;
    struct moon_taskloader_done      *dn   = (struct moon_taskloader_done *) done_data;

    if (mtli && dn)
    {
        if (dn->success)
        {
            if (moon_taskloader_phase != 2)
            {
                moon_taskloader_phase = 2;

                struct moon_taskloader_callback callback = { 0 };
                callback.execute = game_server_load_moon_taskloader;
                callback.done    = init_level_moon_taskloader_done;

                game_server_init_moon_taskloader(level_file(level), level_time(level), goal_e,
                                                 callback);
                return;
            }
            else
            {
                game_client_toggle_show_balls(1);

                game_client_sync(
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                                 !campaign_hardcore_norecordings() &&
#endif
                                 curr_mode() != MODE_NONE ? demo_fp : NULL);

                lvl_warn_timer = curr_clock() < 1000 && curr_time_limit() > 0;

                loading = 0;
            }
        }

        if (mtli->callback.done)
            mtli->callback.done(mtli, dn);
        else if (dn->success)
            goto_play_level();
        else
            goto_exit();
    }
}

static int init_level_moon_taskloader(void)
{
    int curr_rfd_balls = 0;
#if ENABLE_RFD==1
    curr_rfd_balls = curr.rfd_balls;
#endif

    if (curr_mode() != MODE_NONE &&
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        !campaign_hardcore_norecordings() &&
#endif
        config_get_d(CONFIG_ACCOUNT_SAVE) > 0)
        demo_play_init(USER_REPLAY_FILE,
                       level, mode,
                       curr.score,
#ifdef CONFIG_INCLUDES_ACCOUNT
                       curr.balls +
                       account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES) +
                       curr_rfd_balls,
#else
                       curr.balls + curr_rfd_balls,
#endif
                       curr.times,
                       curr.speedpercent);

    struct moon_taskloader_callback callback = {0};
    callback.execute = game_client_load_moon_taskloader;
    callback.done    = init_level_moon_taskloader_done;

    moon_taskloader_phase = 1;
    game_client_init_moon_taskloader(level_file(level), callback);

    return 1;
}
#endif

static int init_level(void)
{
    PROGRESS_DEBUG_CHECK_IS_INIT_FUNC_BOOL;

    loading = 'L';

    int curr_rfd_balls = 0;
#if ENABLE_RFD==1
    curr_rfd_balls = curr.rfd_balls;
#endif

    if (curr_mode() != MODE_NONE &&
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        !campaign_hardcore_norecordings() &&
#endif
        config_get_d(CONFIG_ACCOUNT_SAVE) > 0)
        demo_play_init(USER_REPLAY_FILE,
                       level, mode,
                       curr.score,
#ifdef CONFIG_INCLUDES_ACCOUNT
                       curr.balls +
                       account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES) +
                       curr_rfd_balls,
#else
                       curr.balls + curr_rfd_balls,
#endif
                       curr.times,
                       curr.speedpercent);

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    /* Init campaign camera triggers for auto cameras. */

    if (campaign_used())
        campaign_load_camera_box_trigger(level_name(curr_level()));
#endif

    /*
     * Init both client and server, then process the first batch
     * of commands generated by the server to sync client to
     * server.
     */

#if ENABLE_MOON_TASKLOADER!=0 && !defined(SKIP_MOON_TASKLOADER)
    if (init_level_moon_taskloader())
        return 1;
#else
    if (game_client_init(level_file(level)) &&
        game_server_init(level_file(level), level_time(level), goal_e))
    {
        game_client_toggle_show_balls(1);

#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
        EM_ASM({ Neverball.gamecore_levelmap_loading_finished(true, false); });
#endif

        game_client_sync(
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                         !campaign_hardcore_norecordings() &&
#endif
                         curr_mode() != MODE_NONE ? demo_fp : NULL);

        lvl_warn_timer = curr_clock() < 1000 && curr_time_limit() > 0;

        audio_music_fade_to(1.0f, mode == MODE_CHALLENGE || mode == MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                               || mode == MODE_HARDCORE
#endif
                                  ? "bgm/challenge_mbu.ogg" :
                                  lvl_warn_timer ? "bgm/time-warning.ogg" :
                                                   BGM_TITLE_MAP(level_song(level)), 1);
        loading = 0;

        return 1;
    }
#endif

    demo_play_stop(1);
    loading = 0;

    return 0;
}

int  progress_loading(void)
{
    return loading;
}

int  progress_play(struct level *l)
{
    PROGRESS_DEBUG_CHECK_IS_INIT_FUNC_BOOL;

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    game_fade_color(mode == MODE_HARDCORE ? 0.25f : 0.0f, 0.0f, 0.0f);
#endif

    lvl_warn_timer = 0;
    done           = 0;

    if (l)
    {
        level = l;

        next   = NULL;
        status = GAME_NONE;

#ifdef MAPC_INCLUDES_CHKP
        /* HACK: Must be recalculate after respawn! */

        coins  = last_active ? checkpoints_respawn_coins()              : 0;
        timer  = last_active ? checkpoints_respawn_time_elapsed() * 100 : 0;
#else
        coins  = 0;
        timer  = 0;
#endif
        goal_i = level_goal(level);
        goal   = goal_i;

        extended = 0;

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        if (!campaign_used())
        {
            /* When they using level set, it will be added in the required coins. */

            goal_e = (((mode != MODE_CHALLENGE &&
                        mode != MODE_HARDCORE  &&
                        mode != MODE_BOOST_RUSH) &&
                       level_completed(level) &&
                       config_get_d(CONFIG_LOCK_GOALS) == 0) ||
                      goal == 0) ||
                     mode == MODE_ZEN;
        }
        else if (campaign_used() &&
                 (server_policy_get_d(SERVER_POLICY_PLAYMODES_UNLOCKED_MODE_CAREER) ||
                  campaign_career_unlocked()))
        {
            goal_e = !config_get_d(CONFIG_LOCK_GOALS) || goal == 0;

            /* Seriously, this will never use the required coins without career mode. */

            if (goal_e)
                goal = 0;
        }
        else if (campaign_used())
        {
            /*
             * Seriously, this will never lock the goal state
             * and never use the required coins without career mode.
             */

            goal   = 0;
            goal_e = 1;
        }
#else
        goal_e = (((mode != MODE_CHALLENGE && mode != MODE_BOOST_RUSH) &&
                   level_completed(level) && config_get_d(CONFIG_LOCK_GOALS) == 0) ||
                  goal == 0)
#ifdef LEVELGROUPS_INCLUDES_ZEN
              || mode == MODE_ZEN
#endif
            ;
#endif

#ifdef CONFIG_INCLUDES_ACCOUNT
        const int account_curr_balls =
            server_policy_get_d(SERVER_POLICY_EDITION) > 0 ?
            account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES) : 0;
#else
        const int account_curr_balls = 0;
#endif

#ifdef MAPC_INCLUDES_CHKP
        /*
         * HACK: Only lmited amount of where was bought from the shop
         * and respawn onto it.
         */

        if (last_active)
        {
#if ENABLE_RFD==1
            /* Pennyball + Neverball - Recipes for Disaster */

            prev.rfd_balls = curr.rfd_balls;
#endif
            prev.balls = curr.balls;
        }
        else
#endif
            prev = curr;

        time_rank = RANK_LAST;
        goal_rank = RANK_LAST;
        coin_rank = RANK_LAST;

#if ENABLE_DUALDISPLAY==1
        game_dualdisplay_set_heart(curr.balls + account_curr_balls);
#endif

        activity_services_level_update(curr.balls + account_curr_balls);

        return init_level();
    }
    return 0;
}

void progress_buy_balls(int amount)
{
    PROGRESS_DEBUG_CHECK_IS_INIT_FUNC_VOID;

    curr.balls += amount;
}

void progress_step(void)
{
    if (mode != MODE_CHALLENGE && mode != MODE_BOOST_RUSH &&
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        mode != MODE_HARDCORE &&
#endif
        level && !replay && level_time(level) != 0
#ifdef CONFIG_INCLUDES_ACCOUNT
     && !mediation_enabled()
#endif
        )
    {
        if (curr_clock() >= 1000 && curr_time_limit() > 0 && lvl_warn_timer)
        {
            lvl_warn_timer = 0;
            audio_music_fade_to(.5f, BGM_TITLE_MAP(level_song(level)), 1);
        }
        else if (curr_clock() < 1000 && curr_time_limit() > 0 && !lvl_warn_timer)
        {
            lvl_warn_timer = 1;
            audio_music_fade_to(.1f, "bgm/time-warning.ogg", 1);
        }
    }

    if (goal <= 0)
    {
        goal = 0;
        return;
    }

    goal = goal_i - curr_coins();

    if (goal <= 0 && !replay) game_set_goal();
}

void progress_stat(int s)
{
    PROGRESS_DEBUG_CHECK_IS_INIT_FUNC_VOID;

    int i;

    /* Cannot save highscore in home room. */

    if (mode == MODE_NONE) return;

    if (status != GAME_NONE) return;

    status = s;

    coins = curr_coins();

    /* HACK: Each timer must be substracted for each checkpoints! */

#ifdef MAPC_INCLUDES_CHKP
    timer_offset = checkpoints_respawn_time_elapsed() * 100;
    timer = ROUND((curr_time_elapsed() -
                   checkpoints_respawn_time_elapsed()) * 100);
#else
    timer = ROUND(curr_time_elapsed() * 100);
#endif

#if ENABLE_LIVESPLIT!=0
    progress_livesplit_stat(s);
#endif

    switch (status)
    {
        case GAME_GOAL:
            activity_services_status_update(AS_GAME_STATCODE_G);

            if (mode == MODE_CHALLENGE || mode == MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
             || mode == MODE_HARDCORE
#endif
                )
            {
#ifdef MAPC_INCLUDES_CHKP
                if (last_active)
                {
#if ENABLE_RFD==1
                    /* Pennyball + Neverball - Recipes for Disaster */

                    chkp.rfd_balls = curr.rfd_balls;
#endif
                    chkp.balls = curr.balls;
                }
#endif

                for (i = curr.score + 1; i <= curr.score + coins; i++)
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                    if (progress_reward_ball(i) && mode != MODE_HARDCORE)
#else
                    if (progress_reward_ball(i))
#endif
                        curr.balls++;

                curr.score += coins;
                curr.times += timer;

#if ENABLE_DUALDISPLAY==1
                game_dualdisplay_set_heart(curr.balls);
#endif
            }

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            int career_unlocked = server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_CAREER)
                               && (server_policy_get_d(SERVER_POLICY_PLAYMODES_UNLOCKED_MODE_CAREER)
                                || campaign_career_unlocked());

            if (!campaign_used() &&
                (!config_get_d(CONFIG_SMOOTH_FIX) || video_perf() >= NB_FRAMERATE_MIN))
                level_score_update(level,
                                   timer + timer_offset,
#ifdef ENABLE_POWERUP
                                   coins / powerup_get_coin_multiply(),
#else
                                   coins,
#endif
                                   &time_rank,
                                   goal == 0 ? &goal_rank : NULL,
                                   &coin_rank);
            else if (!campaign_hardcore() && campaign_used() &&
                     (accessibility_get_d(ACCESSIBILITY_SLOWDOWN) >= 100 &&
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
                      !config_cheat() &&
#endif
                      (!config_get_d(CONFIG_SMOOTH_FIX) || video_perf() >= NB_FRAMERATE_MIN)))
                level_score_update(level,
                                   timer + timer_offset,
#ifdef ENABLE_POWERUP
                                   coins / powerup_get_coin_multiply(),
#else
                                   coins,
#endif
                                   &time_rank,
                                   career_unlocked && goal == 0 ? &goal_rank : NULL,
                                   career_unlocked ? &coin_rank : NULL);
#else
            level_score_update(level,
                               timer + timer_offset,
#ifdef ENABLE_POWERUP
                               coins / powerup_get_coin_multiply(),
#else
                               coins,
#endif
                               &time_rank,
                               goal == 0 ? &goal_rank : NULL,
                               &coin_rank);
#endif

            level->stats.completed++;

            if (!level_completed(level))
                level_complete(level);

            /* Compute next level. */

            next = level->next;

            if (mode == MODE_CHALLENGE
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
             || mode == MODE_HARDCORE
#endif
             || mode == MODE_BOOST_RUSH
                )
            {
                int next_has_master = 0;

                for (;
                     next && (level_master(next) || level_bonus(next)) &&
                     !next_has_master;
                     next = next->next)
                {
                    if (!level_opened(next))
                        level_open(next);

                    if (level_master(next))
                    {
                        /* Go to the next level as master mode, darn! */

                        next_has_master = 1;
                        break;
                    }
                }
            }
            else if (mode == MODE_NORMAL
                  || mode == MODE_STANDALONE
#ifdef LEVELGROUPS_INCLUDES_ZEN
                  || mode == MODE_ZEN
#endif
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                  || mode == MODE_CAMPAIGN
#endif
                    )
            {
                for (;
                     next && (level_master(next) || (level_bonus(next) && !level_opened(next)));
                     next = next->next);
            }

#ifdef CONFIG_INCLUDES_ACCOUNT
            /* Add coins for each levels to your account */

            int disable_live_earn = mode == MODE_CHALLENGE
                                 || mode == MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                                 || mode == MODE_HARDCORE
#endif
            ;

            if (status == GAME_GOAL && !disable_live_earn && !CHECK_ACCOUNT_BANKRUPT &&
                server_policy_get_d(SERVER_POLICY_EDITION) > -1)
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
                    if (account_get_d(ACCOUNT_DATA_WALLET_COINS) < ACCOUNT_WALLET_MAX_COINS
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
                     && !config_cheat()
#endif
                        )
                        account_wgcl_do_add(curr_coins(), 0, 0, 0, 0, 0);

                    account_wgcl_save();
                }
            }
#endif

            /* Open next level or complete the campaign or set. */

            if (next)
            {
                if (!level_opened(next))
                    level_open(next);

                struct level *tmp_n;

                for (tmp_n = next;
                     tmp_n && (level_bonus(tmp_n) || level_master(tmp_n));
                     tmp_n = tmp_n->next)
                    if (tmp_n                &&
                        !level_bonus (tmp_n) &&
                        !level_master(tmp_n))
                    {
                        if (!level_opened(tmp_n))
                            level_open(tmp_n);

                        break;
                    }
            }
            else
            {
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                done = mode == MODE_CAMPAIGN
                    || mode == MODE_CHALLENGE
                    || mode == MODE_BOOST_RUSH
                    || mode == MODE_HARDCORE;
#else
                done = mode == MODE_CHALLENGE
                    || mode == MODE_BOOST_RUSH;
#endif

#ifdef CONFIG_INCLUDES_ACCOUNT
                /*
                 * Unlock the next set. Campaign will unlock
                 * the first level set after complete the game.
                 */

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                if (done && mode == MODE_CAMPAIGN &&
                    account_get_d(ACCOUNT_SET_UNLOCKS) == 0)
                    account_set_d(ACCOUNT_SET_UNLOCKS, 1);
                else
#endif
                if (server_policy_get_d(SERVER_POLICY_EDITION) == 0 &&
                    account_get_d(ACCOUNT_SET_UNLOCKS) == curr_set() + 1)
                    account_set_d(ACCOUNT_SET_UNLOCKS, curr_set() + 2);
#endif
            }
            break;

        case GAME_FALL:
            /* It should be both below. */
        case GAME_TIME:
            if (status != GAME_GOAL)
            {
                activity_services_status_update(status == GAME_FALL ? AS_GAME_STATCODE_XF :
                                                                      AS_GAME_STATCODE_XT);

                /* Rejects offer after end of the level. */

                done = 0;

                extended_timer = timer;

                if (mode == MODE_CHALLENGE || mode == MODE_BOOST_RUSH)
                    curr.times += timer;

                if (
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                    mode != MODE_CAMPAIGN &&
#endif
                    mode != MODE_NORMAL
#ifdef LEVELGROUPS_INCLUDES_ZEN
                 && mode != MODE_ZEN
#endif
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                 && mode != MODE_HARDCORE
#endif
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
                 && !config_cheat()
#endif
                )
                    PROGRESS_PLAYER_PAYDEBT_BALLS;

#if ENABLE_DUALDISPLAY==1
                game_dualdisplay_set_heart(curr.balls);
#endif

                /* Decreases acceleration at half. */

                if (mode == MODE_BOOST_RUSH && curr.speedpercent > 1)
                {
                    curr.speedpercent /= 2;
                    exceed_speed       = 0;
                    max_speed          = 0;

                    /* Kill speed percent immediately. */

                    if (progress_dead())
                        curr.speedpercent = 0;
                }

                PROGRESS_PLAYER_BANKRUPT;

                status == GAME_FALL ? level->stats.fallout++ : level->stats.timeout++;
            }

        break;
    }

    demo_play_stat(status, coins, timer);

    if (status == GAME_NONE) return;

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    if (campaign_used())
    {
        campaign_store_hs();

        if (progress_done() && campaign_career_unlocked())
        {
            config_set_d(CONFIG_LOCK_GOALS, 1);
            config_save();
        }
    }
    else if (mode != MODE_CAMPAIGN && mode != MODE_STANDALONE)
#else
    if (mode != MODE_STANDALONE)
#endif
        set_store_hs();
}

void progress_stop(void)
{
    PROGRESS_DEBUG_CHECK_IS_INIT_FUNC_VOID;

    int d = 0;

    /* Cannot save replay in home room. */

    if (mode == MODE_NONE || replay) return;

    if (level)
    {
#ifdef MAPC_INCLUDES_CHKP
        d = curr_time_elapsed() - checkpoints_respawn_time_elapsed() <= 0.0f;
#else
        d = curr_time_elapsed() <= 0.0f;
#endif
    }

    demo_play_stop(d);
}

void progress_exit(void)
{
    if (!is_init) return;

    progress_stop();

#if ENABLE_LIVESPLIT!=0
    if (!done)
        progress_livesplit_exit();
#endif

    activity_services_mode_update(AS_MODE_NONE);

    if (done)
    {
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        if (campaign_used() && mode == MODE_HARDCORE &&
            (!config_get_d(CONFIG_SMOOTH_FIX) && video_perf() >= NB_FRAMERATE_MIN))
        {
            campaign_score_update(curr.times,
                                  curr.score,
                                  campaign_career_unlocked() ? &coin_rank : 0,
                                  done ? &time_rank : NULL);
#if NB_STEAM_API==1
            score_steam_hs_save(curr.score, curr.times);
#endif
        }
        else if (!campaign_used())
#endif
        {
#if NB_HAVE_PB_BOTH==1
            set_star_update(done);
#endif

            if (set_score_update(curr.times,
                                 curr.score,
                                 &score_rank,
                                 done ? &times_rank : NULL))
                set_store_hs();
#if NB_STEAM_API==1
            score_steam_hs_save(curr.score, curr.times);
#endif
        }

#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT)
        if (server_policy_get_d(SERVER_POLICY_EDITION) > -1
         && !CHECK_ACCOUNT_BANKRUPT)
        {
            /* This gems will earn only, after competed the challenge mode. */

            if ((mode == MODE_CHALLENGE ||
                 mode == MODE_BOOST_RUSH)
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
             && !config_cheat()
#endif
                )
            {
#if ENABLE_RFD==1
                account_wgcl_do_add(curr_score(), (curr.balls * 5) + (curr.rfd_balls * 5), 0, 0, 0, 0);
#else
                account_wgcl_do_add(curr_score(), (curr.balls * 5), 0, 0, 0, 0);
#endif
            }
            else
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            if (mode == MODE_HARDCORE
             && !CHECK_ACCOUNT_BANKRUPT
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
             && !config_cheat()
#endif
                )
                account_wgcl_do_add(curr_score(), ROUND(curr_score() / 10), 0, 0, 0, 0);
            else
#endif
            if (!CHECK_ACCOUNT_BANKRUPT
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
             && !config_cheat()
#endif
                )
                account_wgcl_do_add(curr_score(), 0, 0, 0, 0, 0);

            account_wgcl_save();
        }
#endif
    }

    replay = 0;

    is_init = 0;
}

#if ENABLE_MOON_TASKLOADER!=0 && !defined(SKIP_MOON_TASKLOADER)
static int progress_load_replay_moon_taskloader(void *data, void *execute_data)
{
    struct game_moon_taskloader_info *mtli = (struct game_moon_taskloader_info *) execute_data;

    if (!mtli) return 0;

    return progress_replay_full(mtli->filename,
                                &goal, &mode,
                                &curr.balls, &curr.score, &curr.times,
                                1);
}

int  progress_replay_moon_taskloader(const char *filename,
                                     struct moon_taskloader_callback callback)
{
    struct game_moon_taskloader_info *mtli = game_create_mtli();

    if (mtli)
    {
        mtli->callback = callback;
        mtli->filename = strdup(filename);

        callback.execute  = game_mtli_execute;
        callback.progress = game_mtli_progress;
        callback.done     = game_mtli_done;
        callback.data     = mtli;

        return moon_taskloader_load(filename, callback);
    }

    return 0;
}
#endif

int  progress_replay(const char *filename)
{
    return progress_replay_full(filename,
                                &goal, &mode,
                                &curr.balls, &curr.score, &curr.times,
                                1);
}

int  progress_replay_full(const char *filename,
                          int *g, int *m,
                          int *b, int *s, int *tt,
                          int in_game)
{
    if (demo_replay_init(filename,
                         in_game && g  ? g  : (in_game ? &goal       : 0),
                         in_game && m  ? m  : (in_game ? &mode       : 0),
                         in_game && b  ? b  : (in_game ? &curr.balls : 0),
                         in_game && s  ? s  : (in_game ? &curr.score : 0),
                         in_game && tt ? tt : (in_game ? &curr.times : 0),
                         &curr.speedpercent))
    {
        goal_i = g ? (*g) : goal;

        replay = 1;
        return 1;
    }
    else
        return 0;
}

#if NB_HAVE_PB_BOTH==1
int  progress_raise_gems(int action_performed, int needed,
                         int *wgcoins,
                         int *wgearn, int *wgfloat, int *wgspeed)
{
    int final_resale = 0;

#ifdef CONFIG_INCLUDES_ACCOUNT
    const int temp_src_gems    = account_get_d(ACCOUNT_DATA_WALLET_GEMS);

    const int temp_src_coins   = account_get_d(ACCOUNT_DATA_WALLET_COINS);
    const int temp_src_wgearn  = account_get_d(ACCOUNT_CONSUMEABLE_EARNINATOR);
    const int temp_src_wgfloat = account_get_d(ACCOUNT_CONSUMEABLE_FLOATIFIER);
    const int temp_src_wgspeed = account_get_d(ACCOUNT_CONSUMEABLE_SPEEDIFIER);

    int diff_coins   = temp_src_coins;
    int diff_wgearn  = temp_src_wgearn;
    int diff_wgfloat = temp_src_wgfloat;
    int diff_wgspeed = temp_src_wgspeed;

    if (wgcoins) *wgcoins = 0;
    if (wgearn)  *wgearn  = 0;
    if (wgfloat) *wgfloat = 0;
    if (wgspeed) *wgspeed = 0;

    while (temp_src_gems + final_resale < needed)
    {
        if (diff_coins >= 10)
        {
            /* Sell 5 coins at half gem price. */
            diff_coins -= 10;
            if (wgcoins) *wgcoins += 10;

            final_resale += 1;
        }
        else if ((diff_wgspeed >= diff_wgfloat &&
                  diff_wgspeed >= diff_wgearn) &&
                 diff_wgspeed > 0)
        {
            /* Sell speedifier powerups at half coin price. */
            diff_wgspeed--;
            if (wgspeed) *wgspeed += 1;

            diff_coins += 38;
            if (wgcoins) *wgcoins -= 38;
        }
        else if ((diff_wgfloat >= diff_wgspeed &&
                  diff_wgfloat >= diff_wgearn) &&
                 diff_wgfloat > 0)
        {
            /* Sell floatifier powerups at half coin price. */
            diff_wgfloat--;
            if (wgfloat) *wgfloat += 1;

            diff_coins += 38;
            if (wgcoins) *wgcoins -= 38;
        }
        else if ((diff_wgearn >= diff_wgspeed &&
                  diff_wgearn >= diff_wgfloat) &&
                 diff_wgearn > 0)
        {
            /* Sell earninator powerups at half coin price. */
            diff_wgearn--;
            if (wgearn) *wgearn += 1;

            diff_coins += 38;
            if (wgcoins) *wgcoins -= 38;
        }
        /* Debt is greater than net-worth, so go bankrupt. */
        else return MAX(final_resale, 0);

        if (temp_src_gems + final_resale >= needed)
            break;
    }

    if (action_performed && temp_src_gems + final_resale >= needed)
    {
        account_wgcl_do_set(diff_coins, temp_src_gems + final_resale,
                            account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES),
                            diff_wgearn, diff_wgfloat, diff_wgspeed);
    }

#endif
    return MAX(final_resale, 0);
}
#endif

int  progress_next_avail(void)
{
    PROGRESS_DEBUG_CHECK_IS_INIT_FUNC_BOOL;

    if (next &&
        (str_ends_with(next->file, ".csol")  ||
         str_ends_with(next->file, ".csolx") ||
         str_ends_with(next->file, ".sol")   ||
         str_ends_with(next->file, ".solx")))
    {
        if ((mode == MODE_CHALLENGE || mode == MODE_BOOST_RUSH)
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
         && !config_cheat()
#endif
            )
            return status == GAME_GOAL;
        else
            return level_opened(next);
    }

    return 0;
}

int  progress_same_avail(void)
{
    PROGRESS_DEBUG_CHECK_IS_INIT_FUNC_BOOL;

    /* Cannot restart in home room or hardcore mode. */

    if (mode == MODE_NONE
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
     || mode == MODE_HARDCORE
#endif
        ) return 0;

    switch (status)
    {
        case GAME_NONE:
            return (mode != MODE_CHALLENGE &&
                    mode != MODE_BOOST_RUSH) ||
                   config_cheat();

        default:
            if (mode == MODE_CHALLENGE || mode == MODE_BOOST_RUSH)
                return !progress_dead();
            else
                return 1;
    }
}

int  progress_next(void)
{
    PROGRESS_DEBUG_CHECK_IS_INIT_FUNC_BOOL;

    if (next && status == GAME_GOAL && !progress_dead() &&
        (str_ends_with(next->file, ".csol")  ||
         str_ends_with(next->file, ".csolx") ||
         str_ends_with(next->file, ".sol")   ||
         str_ends_with(next->file, ".solx")))
    {
        progress_stop();

#ifdef MAPC_INCLUDES_CHKP
        if (chkp.balls > -1)
        {
#if ENABLE_RFD==1
            curr.rfd_balls = chkp.rfd_balls;
            chkp.rfd_balls = -1;
#endif
            curr.balls = chkp.balls;
            chkp.balls = -1;
        }
#endif

#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
        EM_ASM({
            Neverball.gamecore_levelmap_load($0, UTF8ToString($1), true);
        }, next->num_indiv_theme, next->song);
#endif

#if ENABLE_LIVESPLIT!=0
        int r = progress_play(next);

        if (r) progress_livesplit_next();

        return r;
#else
        return progress_play(next);
#endif
    }

    return 0;
}

int  progress_same(void)
{
    PROGRESS_DEBUG_CHECK_IS_INIT_FUNC_BOOL;

    if (!progress_dead())
    {
        progress_stop();

        /* Reset progress and goal enabled state. */

        if (status == GAME_GOAL)
        {
            curr = prev;

#ifdef MAPC_INCLUDES_CHKP
            if (chkp.balls > -1)
            {
#if ENABLE_RFD==1
                curr.rfd_balls = chkp.rfd_balls;
                chkp.rfd_balls = -1;
#endif
                curr.balls = chkp.balls;
                chkp.balls = -1;
            }
#endif
        }

        return progress_play(level);
    }

    return 0;
}

int  progress_dead(void)
{
    if (!is_init || mode == MODE_NONE) return 1;

#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
    if (config_cheat()) return 0;
#endif

    switch (mode)
    {
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        case MODE_HARDCORE: return status != GAME_GOAL;
#endif

        case MODE_CHALLENGE:
        case MODE_BOOST_RUSH:
            return curr.balls
#if ENABLE_RFD==1
                 + curr.rfd_balls
#endif
#ifdef CONFIG_INCLUDES_ACCOUNT
                 + account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES)
#endif
                 < 0;

        default: return 0;
    }
}

int  progress_done(void)
{
    return is_init ? done : 0;
}

int  progress_last(void)
{
    PROGRESS_DEBUG_CHECK_IS_INIT_FUNC_BOOL;

    return (mode != MODE_CHALLENGE
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
         && mode != MODE_HARDCORE
#endif
         && mode != MODE_BOOST_RUSH)
        && status == GAME_GOAL && !next;
}

int  progress_lvl_high(void)
{
    return (time_rank < RANK_LAST ||
            goal_rank < RANK_LAST ||
            coin_rank < RANK_LAST);
}

int  progress_set_high(void)
{
    return (score_rank < RANK_LAST ||
            times_rank < RANK_LAST);
}

void progress_rename(int set_only)
{
    const char *player = config_get_s(CONFIG_PLAYER);

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    if (campaign_used())
    {
        level_rename_player(level, time_rank, goal_rank, coin_rank, player);
        demo_rename_player(USER_REPLAY_FILE, player);

        campaign_store_hs();
    }
    else
#endif
    {
        if (set_only)
            set_rename_player(score_rank, times_rank, player);
        else
        {
            level_rename_player(level, time_rank, goal_rank, coin_rank, player);
            demo_rename_player(USER_REPLAY_FILE, player);

            if (progress_done() && curr_mode() != MODE_STANDALONE)
                set_rename_player(score_rank, times_rank, player);
        }

        if (curr_mode() != MODE_STANDALONE)
            set_store_hs();
    }
}

int  progress_reward_ball(int s)
{
    return s > 0 && s % 100 == 0;
}

#if ENABLE_RFD==1
int progress_rfd_take_powerup(int t)
{
    PROGRESS_DEBUG_CHECK_IS_INIT_FUNC_BOOL;

    switch (t)
    {
        case 0:
            if (curr.rfd_earninator <= 0) return 0;
            else curr.rfd_earninator--;
            break;
        case 1:
            if (curr.rfd_floatifier <= 0) return 0;
            else curr.rfd_floatifier--;
            break;
        case 2:
            if (curr.rfd_speedifier <= 0) return 0;
            else curr.rfd_speedifier--;
            break;
    }

    return 1;
}

int progress_rfd_get_powerup(int t)
{
    switch (t)
    {
        case 0:
            return curr.rfd_earninator;
            break;
        case 1:
            return curr.rfd_floatifier;
            break;
        case 2:
            return curr.rfd_speedifier;
            break;
    }

    return 0;
}
#endif

/*---------------------------------------------------------------------------*/

struct level *curr_level(void) { return level; }

float curr_speed_percent(void) { return curr.speedpercent; }

int curr_balls(void)
{
    int curr_rfd_balls = 0;

#if ENABLE_RFD==1
    curr_rfd_balls = curr.rfd_balls;
#endif

#ifdef CONFIG_INCLUDES_ACCOUNT
    if (!replay)
        return curr.balls + account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES) +
               curr_rfd_balls;
#endif

    return curr.balls;
}

int curr_score(void) { return curr.score; }
int curr_times(void) { return curr.times; }
int curr_mode (void) { return mode;       }
int curr_goal (void) { return goal;       }

int progress_time_rank(void) { return time_rank; }
int progress_goal_rank(void) { return goal_rank; }
int progress_coin_rank(void) { return coin_rank; }

int progress_times_rank(void) { return times_rank; }
int progress_score_rank(void) { return score_rank; }

/*---------------------------------------------------------------------------*/

const char *mode_to_str(int m, int l)
{
    switch (m)
    {
        case MODE_CHALLENGE: return l ? _("Challenge Mode")  : _("Challenge");
        case MODE_NORMAL:    return l ? _("Classic Mode")    : _("Classic");
        case MODE_STANDALONE:return l ? _("Standalone Mode") : _("Standalone");
#ifdef LEVELGROUPS_INCLUDES_ZEN
        case MODE_ZEN:       return l ? _("Zen Mode")        : _("Zen");
#endif
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        case MODE_HARDCORE:  return l ? _("Hardcore Mode")   : _("Hardcore");
#endif
        case MODE_BOOST_RUSH:return l ? _("Boost Rush Mode") : _("Boost Rush");
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        case MODE_CAMPAIGN:  return l ? _("Campaign Mode")   : _("Campaign");
#endif
        default:             return l ? _("Unknown Mode")    : _("Unknown");
    }
}

/*---------------------------------------------------------------------------*/
