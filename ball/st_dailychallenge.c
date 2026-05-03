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

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "console_control_gui.h"

#if NB_HAVE_PB_BOTH==1
#include "account_wgcl.h"
#include "campaign.h"
#endif

#include "activity_services.h"

#include "common.h"
#include "key.h"
#include "config.h"
#include "progress.h"
#include "state.h"
#include "gui.h"
#include "transition.h"
#include "audio.h"
#include "score.h"

#include "st_level.h"
#include "st_shared.h"
#include "st_title.h"

#include "st_dailychallenge.h"

#include "game_common.h"
#include "game_client.h"

/*---------------------------------------------------------------------------*/

/* === WGCL DAILY CHALLENGE STRUCTS === */

struct dailychallenge {
    char *id;
    char *name;

#if NB_HAVE_PB_BOTH==1
    int   reward_gems;                  /* Challenge Rewards                 */
    int   reward_gems_hardcore;         /* Challenge Rewards (Hardcore)      */

    int   balls_needed;                 /* Balls needed to start challenge   */
#endif

    /* HS Info                                                               */

    char *user_scores;                  /* User high-score file              */

    struct score coin_score;            /* Challenge score (Coins)           */
    struct score time_score;            /* Challenge score (Time)            */

    /* Level info                                                            */

    int   count;                        /* Number of levels                  */
    char *level_name_v[MAXLVL_DAILY];   /* List of level file names          */
};

/* === END WGCL DAILY CHALLENGE STRUCTS === */

/* === WGCL DAILY CHALLENGE VARIABLES === */

static struct dailychallenge challenge_data;

static struct level level_v[MAXLVL_DAILY];

static int dailychallenge_lvlindex;
static int dailychallenge_num_indiv_theme;
static int dailychallenge_regular;
static int dailychallenge_master;

static int dailychallenge_maxtimelimit_hard;
static int dailychallenge_maxtimelimit_medm;
static int dailychallenge_maxtimelimit_easy;
static int dailychallenge_mincoinrequired_hard;
static int dailychallenge_mincoinrequired_medm;
static int dailychallenge_mincoinrequired_easy;

/*
 * 0 = Challenge Set Name
 * 1 = Game Mode
 * 2 = Rewards
 * 3 = Time left
 * 4 = Compete button
 * 5 = Leaderboard button
 */
static int  dailychallenge_gui_ids[6];
static char dailychallenge_gui_texts[6][MAXSTR];

/* === END WGCL DAILY CHALLENGE VARIABLES === */

/*---------------------------------------------------------------------------*/

enum {
    DAILYCHALLENGE_LEADERBOARD = GUI_LAST,
    DAILYCHALLENGE_START,
};

struct state st_dailychallenge;
static int dailychallenge_mode;

static void dailychallenge_update_hs(void)
{
    for (int r = 0; r < 3; r++) {
        const int fixed_timehs[] = { dailychallenge_maxtimelimit_hard, dailychallenge_maxtimelimit_medm, dailychallenge_maxtimelimit_easy };
        const int fixed_coinhs[] = { dailychallenge_mincoinrequired_hard, dailychallenge_mincoinrequired_medm, dailychallenge_mincoinrequired_easy };

        /* Most coins and Best Time built-in limitations. */

        if (challenge_data.coin_score.coins[r] < fixed_coinhs[r])
            challenge_data.coin_score.coins[r] = fixed_coinhs[r];
        if (challenge_data.coin_score.timer[r] > fixed_timehs[r])
            challenge_data.coin_score.timer[r] = fixed_timehs[r];
        if (challenge_data.time_score.coins[r] < fixed_coinhs[r])
            challenge_data.time_score.coins[r] = fixed_coinhs[r];
        if (challenge_data.time_score.timer[r] > fixed_timehs[r])
            challenge_data.time_score.timer[r] = fixed_timehs[r];
    }
}

/* === WGCL DAILY CHALLENGE FUNCTIONS === */

#if NB_HAVE_PB_BOTH==1

void WGCL_Goto_DailyChallenge(void) {
    goto_state(&st_dailychallenge);
}

void WGCL_DailyChallenge_Clear(void) {
    for (int i = 0; i < MAXLVL_DAILY; i++)
        memset(&level_v[i], 0, sizeof (struct level));

    memset(&challenge_data, 0, sizeof (struct dailychallenge));

    dailychallenge_lvlindex = 0;

    dailychallenge_num_indiv_theme = 1;
    dailychallenge_regular         = 1;
    dailychallenge_master          = 1;

    dailychallenge_maxtimelimit_hard    = 0;
    dailychallenge_maxtimelimit_medm    = 0;
    dailychallenge_maxtimelimit_easy    = 0;
    dailychallenge_mincoinrequired_hard = 0;
    dailychallenge_mincoinrequired_medm = 0;
    dailychallenge_mincoinrequired_easy = 0;
}

void WGCL_DailyChallenge_AddLevelFile(const char *filename) {
    struct level *l = &level_v[dailychallenge_lvlindex];

    if (level_load(filename, l)) {
        if (l->is_bonus) return;

        l->number    = dailychallenge_lvlindex + 1;
        l->is_locked = dailychallenge_lvlindex == 0 ? 0 : 1;

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(l->name, MAXSTR,
#else
        sprintf(l->name,
#endif
                "%d", l->number);

        /* === BEST TIME MERGER === */

        dailychallenge_maxtimelimit_hard += l->scores[SCORE_GOAL].timer[RANK_HARD];
        dailychallenge_maxtimelimit_medm += l->scores[SCORE_GOAL].timer[RANK_MEDM];
        dailychallenge_maxtimelimit_easy += l->scores[SCORE_GOAL].timer[RANK_EASY];

        /* === END BEST TIME MERGER === */

        /* === MOST COINS MERGER === */

        dailychallenge_mincoinrequired_hard += l->scores[SCORE_COIN].coins[RANK_HARD];
        dailychallenge_mincoinrequired_medm += l->scores[SCORE_COIN].coins[RANK_MEDM];
        dailychallenge_mincoinrequired_easy += l->scores[SCORE_COIN].coins[RANK_EASY];

        /* === END MOST COINS MERGER === */

        if (dailychallenge_lvlindex - 1 >= 0)
            level_v[dailychallenge_lvlindex].prev = &level_v[dailychallenge_lvlindex - 1];

        if (dailychallenge_lvlindex > 0)
            level_v[dailychallenge_lvlindex - 1].next = l;

        dailychallenge_lvlindex++;
    }
}

void WGCL_DailyChallenge_SetInfocard(int hardcore, int balls_needed, int reward_gems, const char *timeleft_text, const char *setid, const char *setname) {
    char s_rewards[MAXSTR], s_timeleft[20];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(s_rewards, MAXSTR,
#else
    sprintf(s_rewards,
#endif
            "%s %d", GUI_DIAMOND, reward_gems);
    
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    const int expected_mode = hardcore ? MODE_HARDCORE : MODE_CHALLENGE;
#else
    const int expected_mode = MODE_CHALLENGE;
#endif

    SAFECPY(dailychallenge_gui_texts[0], setname);
    SAFECPY(dailychallenge_gui_texts[1], mode_to_str(expected_mode, 0));
    SAFECPY(dailychallenge_gui_texts[2], s_rewards);
    SAFECPY(dailychallenge_gui_texts[3], timeleft_text);

    dailychallenge_mode = expected_mode;

    gui_set_label(dailychallenge_gui_ids[0], dailychallenge_gui_texts[0]);
    gui_set_label(dailychallenge_gui_ids[1], dailychallenge_gui_texts[1]);
    gui_set_label(dailychallenge_gui_ids[2], dailychallenge_gui_texts[2]);
    gui_set_font (dailychallenge_gui_ids[2], "ttf/DejaVuSans-Bold.ttf");
    gui_set_label(dailychallenge_gui_ids[3], dailychallenge_gui_texts[3]);

    challenge_data.balls_needed = balls_needed;

    if (challenge_data.name) {
        free(challenge_data.name);
        challenge_data.name = NULL;
    }

    if (challenge_data.id) {
        free(challenge_data.id);
        challenge_data.id = NULL;
    }

    challenge_data.id   = strdup(setid);
    challenge_data.name = strdup(setname);
}

void WGCL_DailyChallenge_FinishPreparing(int done, int hardcore, int error_state, int balls_needed,
                                         const char *lang_texts, const char *packages_texts, const char *compete_text) {
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    const int expected_mode = hardcore ? MODE_HARDCORE : MODE_CHALLENGE;
#else
    const int expected_mode = MODE_CHALLENGE;
#endif

    char s_needed[MAXSTR];
    activity_services_mode_update(expected_mode);
    progress_reinit(expected_mode);

    if (!done) {
        switch (error_state) {
            /* 1 = NOT ENOUGH BALLS */
            case 1:
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(s_needed, MAXSTR,
#else
                sprintf(s_needed,
#endif
                        _(compete_text), balls_needed);
                gui_set_label(dailychallenge_gui_ids[4], s_needed);
                break;

            /* 2 = MISMATCH LANGUAGES */
            case 2:
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(s_needed, MAXSTR,
#else
                sprintf(s_needed,
#endif
                        _(compete_text), lang_texts);
                gui_set_label(dailychallenge_gui_ids[4], s_needed);
                break;

            /* 3 = MISSING ADD-ONS */
            case 3:
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(s_needed, MAXSTR,
#else
                sprintf(s_needed,
#endif
                        _(compete_text), packages_texts);
                gui_set_label(dailychallenge_gui_ids[4], s_needed);
                break;

            /* 4 = NOT ENOUGH TIME */
            case 4:
                gui_set_label(dailychallenge_gui_ids[4], _("Not enough time"));
                break;
        }
    } else {
        gui_set_state(dailychallenge_gui_ids[4], DAILYCHALLENGE_START, 0);
        gui_set_label(dailychallenge_gui_ids[4], compete_text);
    }
}

void WGCL_DailyChallenge_StartGame(void) {
    if (dailychallenge_lvlindex == 0) {
        dailychallenge_exit();
        exit_state(&st_title);
        return;
    }

    dailychallenge_update_hs();

    audio_play(AUD_STARTGAME, 1.0f);
    progress_reinit(dailychallenge_mode);

    if (progress_play(&level_v[0])) {
#if NB_HAVE_PB_BOTH==1
        account_wgcl_autokick_func_prepare(goto_exit);
        account_wgcl_autokick_state_prepare(&st_title);
#endif
        goto_play_level();
    } else {
        dailychallenge_exit();
        exit_state(&st_title);
        return;
    }
}

void WGCL_DailyChallenge_LoadSetHS_Coins(int timer_first, const char *name_first, int coins_first,
                                         int timer_sec,   const char *name_sec,   int coins_sec, 
                                         int timer_third, const char *name_third, int coins_third) {
    const int  hs_coins[3]      = { coins_first, coins_sec, coins_third };
    const int  hs_timer[3]      = { timer_first, timer_sec, timer_third };

    for (int i = RANK_HARD; i < RANK_LAST; i++)
        if (dailychallenge_mincoinrequired_easy <= hs_coins[i]) {
            challenge_data.coin_score.coins[i] = hs_coins[i];
            challenge_data.coin_score.timer[i] = hs_timer[i];

            switch (i) {
                case 0: SAFECPY(challenge_data.coin_score.player[i], name_first); break;
                case 1: SAFECPY(challenge_data.coin_score.player[i], name_sec);   break;
                case 2: SAFECPY(challenge_data.coin_score.player[i], name_third); break;
            }
        }
}

void WGCL_DailyChallenge_LoadSetHS_Time(int timer_first, const char *name_first, int coins_first,
                                        int timer_sec,   const char *name_sec,   int coins_sec, 
                                        int timer_third, const char *name_third, int coins_third) {
    const int  hs_coins[3]      = { coins_first, coins_sec, coins_third };
    const int  hs_timer[3]      = { timer_first, timer_sec, timer_third };
    
    for (int i = RANK_HARD; i < RANK_LAST; i++)
        if (dailychallenge_mincoinrequired_easy <= hs_coins[i]) {
            challenge_data.coin_score.coins[i] = hs_coins[i];
            challenge_data.coin_score.timer[i] = hs_timer[i];

            switch (i) {
                case 0: SAFECPY(challenge_data.coin_score.player[i], name_first); break;
                case 1: SAFECPY(challenge_data.coin_score.player[i], name_sec);   break;
                case 2: SAFECPY(challenge_data.coin_score.player[i], name_third); break;
            }
        }
}

#endif

/* === END WGCL DAILY CHALLENGE FUNCTIONS === */

/*---------------------------------------------------------------------------*/

static int dailychallenge_readytofetch;

int dailychallenge_active_mode(void)
{
    return dailychallenge_mode;
}

void dailychallenge_exit(void)
{
    dailychallenge_mode = MODE_NONE;
}

const char *dailychallenge_id(void)
{
#if NB_HAVE_PB_BOTH==1
    return challenge_data.id;
#else
    return 0;
#endif
}

const char *dailychallenge_name(void)
{
#if NB_HAVE_PB_BOTH==1
    return challenge_data.name;
#else
    return 0;
#endif
}

const struct score *dailychallenge_score(int s) {
    switch (s) {
        case SCORE_TIME: return &challenge_data.time_score;
        case SCORE_COIN: return &challenge_data.coin_score;
        default: return NULL;
    }
}

static int dailychallenge_start(void)
{
    int first = 1;
    char s_packages_list_raw[8192];
    fs_file handle;

    if ((handle = fs_open_read("DLC/installed-packages.txt"))) {
        SAFECPY(s_packages_list_raw, "");

        char line[MAXSTR] = "";

        while (fs_gets(line, sizeof (line), handle)) {
            strip_newline(line);

            if (strncmp(line, "package ", 8) == 0) {
                if (!first)
                    SAFECAT(s_packages_list_raw, ",");

                SAFECAT(s_packages_list_raw, line + 8);
                first = 0;
            }
        }

        fs_close(handle);
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
        EM_ASM({ Neverball.gamecore_try_start_daily_challenge(UTF8ToString($0)); }, s_packages_list_raw);
        return 1;
#endif
    }

    return 0;
}

static int dailychallenge_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok) {
        case GUI_BACK:
            if (challenge_data.name) {
                free(challenge_data.name);
                challenge_data.name = NULL;
            }

            if (challenge_data.id) {
                free(challenge_data.id);
                challenge_data.id = NULL;
            }

            dailychallenge_mode         = 0;
            dailychallenge_readytofetch = 0;
            dailychallenge_exit();
            progress_exit();
            exit_state(&st_title);
            break;

        case DAILYCHALLENGE_START:
#ifdef __EMSCRIPTEN__
            dailychallenge_start();
            return 1;
#endif

            break;
    }

    return 1;
}

static int dailychallenge_enter(struct state *st, struct state *prev, int intent)
{
    dailychallenge_readytofetch = prev != &st_dailychallenge;

    int id, jd, kd;

    if ((id = gui_vstack(0))) {
        if ((jd = gui_hstack(id))) {
            if ((kd = gui_vstack(jd)))
                for (int i = 0; i < 4; i++) {
                    dailychallenge_gui_ids[i] = gui_label(kd, GUI_ELLIPSIS, GUI_SML, GUI_COLOR_DEFAULT);
                    gui_set_font(dailychallenge_gui_ids[i], "ttf/DejaVuSans-Bold.ttf");
                }

            if ((kd = gui_vstack(jd))) {
                gui_label(kd, _("Today"),     GUI_SML, GUI_COLOR_DEFAULT);
                gui_label(kd, _("Mode"),      GUI_SML, GUI_COLOR_DEFAULT);
                gui_label(kd, _("Reward"),    GUI_SML, GUI_COLOR_DEFAULT);
                gui_label(kd, _("Time left"), GUI_SML, GUI_COLOR_DEFAULT);
            }

            gui_set_rect(jd, GUI_ALL);
        }

        gui_space(id);

        if ((dailychallenge_gui_ids[4] = gui_multi(id, " \n ", GUI_SML, GUI_COLOR_GRY))) {
            gui_set_font(dailychallenge_gui_ids[4], "ttf/DejaVuSans-Bold.ttf");
            gui_set_state(dailychallenge_gui_ids[4], GUI_NONE, 0);
        }

        gui_layout(id, 0, 0);
    }

    return transition_slide(id, 1, intent);
}

static int dailychallenge_leave(struct state *st, struct state *next, int id, int intent)
{
    return transition_slide(id, 0, intent);
}

static void dailychallenge_paint(int id, float t)
{
    game_client_draw(0, t);
    gui_paint(id);
}

static void dailychallenge_timer(int id, float dt)
{
    if (dailychallenge_readytofetch && time_state() > 0.5f) {
        dailychallenge_readytofetch = 0;
#if NB_HAVE_PB_BOTH==1
        int first = 1;
        char s_packages_list_raw[8192];
        fs_file handle;

        if ((handle = fs_open_read("DLC/installed-packages.txt"))) {
            SAFECPY(s_packages_list_raw, "");

            char line[MAXSTR] = "";

            while (fs_gets(line, sizeof(line), handle)) {
                strip_newline(line);

                if (strncmp(line, "package ", 8) == 0) {
                    if (!first)
                        SAFECAT(s_packages_list_raw, ",");

                    SAFECAT(s_packages_list_raw, line + 8);
                    first = 0;
                }
            }

            fs_close(handle);
        }

#ifdef __EMSCRIPTEN__
        EM_ASM({ Neverball.gamecore_try_fetch_daily_challenge(UTF8ToString($0)); }, s_packages_list_raw);
#endif
#endif
    }

    gui_timer(id, dt);
}

static int dailychallenge_keybd(int c, int d)
{
    if (d)
    {
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        if (c == KEY_EXIT && current_platform == PLATFORM_PC)
#else
        if (c == KEY_EXIT)
#endif
            return dailychallenge_action(GUI_BACK, 0);
    }
    return 1;
}

static int dailychallenge_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return dailychallenge_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return dailychallenge_action(GUI_BACK, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

struct state st_dailychallenge = {
    dailychallenge_enter,
    dailychallenge_leave,
    dailychallenge_paint,
    dailychallenge_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    dailychallenge_keybd,
    dailychallenge_buttn
};