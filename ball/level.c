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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if NB_HAVE_PB_BOTH==1
#include "campaign.h"
#endif

#include "solid_base.h"

#include "common.h"
#include "config.h"
#include "level.h"
#include "set.h"
#include "log.h"
#include "text.h"
#include "lang.h"

/*---------------------------------------------------------------------------*/

static int scan_level_attribs(struct level *l,
                              const struct s_base *base,
                              int campaign,
                              int pre_campaign,
                              int werror_campaign)
{
    int i;

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    int difficulty_offered = campaign == 0;
#else
    int difficulty_offered = 1;
#endif

    int have_goal = 0, have_time = 0;

    int mingoal = 0;
    int maxtime = 360000;

    int need_time_medm = 0, need_time_easy = 0;
    int need_goal_medm = 0, need_goal_easy = 0;
    int need_coin_medm = 0, need_coin_easy = 0;

    for (i = 0; i < base->dc; i++)
    {
        char *k = base->av + base->dv[i].ai;
        char *v = base->av + base->dv[i].aj;
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        /* CAMPAIGN */
        if (strcmp(k, "campaign_level_difficulty") == 0)
        {
            if (campaign && !pre_campaign)
            {
                int diff = atoi(v);
                if (campaign_level_difficulty() <= diff)
                {
                    campaign_upgrade_difficulty(diff);
                    difficulty_offered = 1;
                }
                else if(werror_campaign)
                {
                    log_errorf("Switchball does not offer descent or random difficulty!\n");
                    return 0;
                }
            }

        }
        else
#endif
        /* Player levels */
        if (strcmp(k, "message") == 0)
            SAFECPY(l->message, v);
        else if (strcmp(k, "song") == 0)
            SAFECPY(l->song, v);
        else if (strcmp(k, "shot") == 0)
            SAFECPY(l->shot, v);
        else if (strcmp(k, "goal") == 0)
        {
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            if (!campaign || config_get_d(CONFIG_LOCK_GOALS))
#else
            if (config_get_d(CONFIG_LOCK_GOALS))
#endif
            {
                l->goal   = atoi(v);
                mingoal   = atoi(v);
                have_goal = 1;
            }
        }
        else if (strcmp(k, "time") == 0)
        {
            maxtime = atoi(v);
            if (campaign || pre_campaign)
            {
                if (maxtime > 0 && werror_campaign)
                {
                    log_errorf("Switchball does not offer limited time!\n");
                    return 0;
                }
            }
            else if (maxtime > 0)
            {
                l->time   = maxtime;
                have_time = 1;
            }
        }
        else if (strcmp(k, "time_hs") == 0)
        {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            switch (sscanf_s(v,
#else
            switch (sscanf(v,
#endif
                           "%d %d %d",
                           &l->scores[SCORE_TIME].timer[RANK_HARD],
                           &l->scores[SCORE_TIME].timer[RANK_MEDM],
                           &l->scores[SCORE_TIME].timer[RANK_EASY]))
            {
                case 1:
                {
                    if ((campaign || pre_campaign) && werror_campaign)
                    {
                        log_errorf("Switchball requires three premaded best time!\n");
                        return 0;
                    }
                    else need_time_medm = 2;
                }
                case 2:
                {
                    if ((campaign || pre_campaign) && werror_campaign)
                    {
                        log_errorf("Switchball requires three premaded best time!\n");
                        return 0;
                    }
                    else need_time_easy = 1;
                }
                    break;
                case 3: break;
            }
        }
        else if (strcmp(k, "goal_hs") == 0)
        {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            switch (sscanf_s(v,
#else
            switch (sscanf(v,
#endif
                           "%d %d %d",
                           &l->scores[SCORE_GOAL].timer[RANK_HARD],
                           &l->scores[SCORE_GOAL].timer[RANK_MEDM],
                           &l->scores[SCORE_GOAL].timer[RANK_EASY]))
            {
                case 1:
                {
                    if ((campaign || pre_campaign) && werror_campaign)
                    {
                        log_errorf("Switchball requires three premaded best time!\n");
                        return 0;
                    }
                    else need_goal_medm = 2;
                }
                case 2:
                {
                    if ((campaign || pre_campaign) && werror_campaign)
                    {
                        log_errorf("Switchball requires three premaded best time!\n");
                        return 0;
                    }
                    else need_goal_easy = 1;
                }
                    break;
                case 3: break;
            }
        }
        else if (strcmp(k, "coin_hs") == 0)
        {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            switch (sscanf_s(v,
#else
            switch (sscanf(v,
#endif
                           "%d %d %d",
                           &l->scores[SCORE_COIN].coins[RANK_HARD],
                           &l->scores[SCORE_COIN].coins[RANK_MEDM],
                           &l->scores[SCORE_COIN].coins[RANK_EASY]))
            {
                case 1: need_coin_medm = 2; break;
                case 2: need_coin_easy = 1; break;
                case 3: break;
            }
        }
        else if (strcmp(k, "version") == 0)
        {
            SAFECPY(l->version_str, v);

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            int r = sscanf_s(l->version_str,
#else
            int r = sscanf(l->version_str,
#endif
                           "%d", &l->version_num);

            if (!r)
            {
                log_errorf("SOL/SOLX key parameter \"version\" contains unknown version!\n");
                return 0;
            }

            int version_major, version_minor;

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            if (sscanf_s(l->version_str,
#else
            if (sscanf(l->version_str,
#endif
                       "%d.%d", &version_major, &version_minor) != 2)
            {
                /*
                 * Was:
                 *     log_errorf("SOL/SOLX key parameter \"version\" (%s) is not an valid version format!\n", v ? v : "unknown");
                 *     return 0;
                 */
            }
        }
        else if (strcmp(k, "author") == 0)
            SAFECPY(l->author, v);
        else if (strcmp(k, "master") == 0)
        {
            int is_master = atoi(v) ? 1 : 0;

            if ((campaign || pre_campaign) && is_master && werror_campaign)
            {
                log_errorf("Switchball does not offer master levels!\n");
                return 0;
            }
            else
                l->is_master = is_master;
        }
        else if (strcmp(k, "bonus") == 0)
        {
            int is_bonus = atoi(v) ? 1 : 0;

            if ((campaign || pre_campaign) && is_bonus && werror_campaign)
            {
                log_errorf("Switchball does not offer bonus levels!\n");
                return 0;
            }
            else
                l->is_bonus = is_bonus;
        }
    }

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    if (difficulty_offered == 0 && campaign)
    {
        log_errorf("Switchball requires campaign level difficulty!\n");
        return 0;
    }
#endif

    /*
     * No goals associated:
     * - Set to unlimited time
     * - Set to zeroed required coins
     */

    if (base->zc == 0)
    {
        l->time = 0;
        l->goal = 0;

        maxtime = 360000;
        mingoal = 0;
    }

    if (have_time)
    {
        /* Best Time built-in limitations. */

        for (i = 0; i < 3; i++)
            for (int r = RANK_HARD; r < RANK_LAST; r++)
                while (l->scores[i].timer[r] > maxtime)
                {
                    log_errorf("Hard limits detected!: Limit: %d ms; Current: %d ms\n", maxtime * 10, l->scores[i].timer[r] * 10);
                    l->scores[i].timer[r] = maxtime;
                }
    }

    if (have_goal)
    {
        /* Most coins and Fast Unlock built-in limitations. */

        for (i = 0; i < 3; i++)
            for (int r = RANK_HARD; r < RANK_LAST; r++)
                while (l->scores[i].coins[r] < mingoal)
                {
                    log_errorf("Hard limits detected!: Required: %d; Current: %d\n", mingoal, l->scores[i].coins[r]);
                    l->scores[i].coins[r] = mingoal;
                }
    }

    for (i = 0; i < 3; i++)
        for (int r = RANK_HARD; r < RANK_LAST; r++)
        {
            if (l->scores[i].coins[r + 1] > l->scores[i].coins[r])
                l->scores[i].coins[r + 1] = l->scores[i].coins[r];

            if (l->scores[i].timer[r + 1] < l->scores[i].timer[r])
                l->scores[i].timer[r + 1] = l->scores[i].timer[r];
        }

    return 1;
}

static int scan_campaign_level(const struct s_base *base,
                               const char *filename,
                               struct level *l,
                               int campaign,
                               int pre_campaign,
                               int werror_campaign)
{
    int  accept_back        = 0, accept_grad        = 0;
    int  levelset_have_back = 0, levelset_have_grad = 0, levelset_have_song = 0;
    char target_back[MAXSTR]; memset(target_back, 0, MAXSTR);
    char target_grad[MAXSTR]; memset(target_grad, 0, MAXSTR);
    char target_song[MAXSTR]; memset(target_song, 0, MAXSTR);

    const char sbtheme_limitation_grad[][MAXSTR] =
    {
        "back/sky-SB.png",
        "back/ice-SB.png",
        "back/cave-SB.png",
        "back/clouds-SB.png",
        "back/lava-SB.png"
    };
    const char sbtheme_limitation_back[][MAXSTR] =
    {
        "map-back/sky-SB.sol",
        "map-back/ice-SB.sol",
        "map-back/cave-SB.sol",
        "map-back/clouds-SB.sol",
        "map-back/lava-SB.sol"
    };
    const char sbtheme_limitation_song[][MAXSTR] =
    {
        "bgm/track-SB1.ogg", /* alien = sky world    */
        "bgm/track-SB2.ogg", /* city = ice world     */
        "bgm/track-SB3.ogg", /* space = cave world   */
        "bgm/track-SB4.ogg", /* clouds = cloud world */
        "bgm/track-SB5.ogg"  /* volcano = lava world */
    };

    for (int i = 0; i < base->dc; i++)
    {
        char *k = base->av + base->dv[i].ai;
        char *v = base->av + base->dv[i].aj;

        if (strcmp(k, "back") == 0)
        {
            if (campaign)
            {
                if (str_starts_with(filename, "buildin-map-campaign/sky"))
                    SAFECPY(target_back, sbtheme_limitation_back[0]);
                else if (str_starts_with(filename, "buildin-map-campaign/ice"))
                    SAFECPY(target_back, sbtheme_limitation_back[1]);
                else if (str_starts_with(filename, "buildin-map-campaign/cave"))
                    SAFECPY(target_back, sbtheme_limitation_back[2]);
                else if (str_starts_with(filename, "buildin-map-campaign/cloud"))
                    SAFECPY(target_back, sbtheme_limitation_back[3]);
                else if (str_starts_with(filename, "buildin-map-campaign/lava"))
                    SAFECPY(target_back, sbtheme_limitation_back[4]);
                else
                {
                    log_errorf("%s:\n    Switchball does not offer different backgrounds!\n",
                               filename);
                    return 0;
                }

                accept_back = strcmp(v, target_back) == 0;

                if (!accept_back)
                {
                    log_errorf("%s:\n    Switchball requires background environment with matched filename!\n",
                               filename);
                    return 0;
                }
            }
            else if (pre_campaign)
            {
                if (!levelset_have_back && !levelset_have_grad && !levelset_have_song)
                {
                    for (int i = 0; i < 5 && !levelset_have_back; i++)
                        if (strcmp(v, sbtheme_limitation_back[i]) == 0)
                        {
                            SAFECPY(target_grad, sbtheme_limitation_grad[i]);
                            SAFECPY(target_song, sbtheme_limitation_song[i]);

                            levelset_have_back = 1;
                            break;
                        }
                }
                else if ((levelset_have_grad || levelset_have_song) && !levelset_have_back)
                    levelset_have_back = strcmp(v, target_back) == 0;

                if (!levelset_have_back)
                {
                    log_errorf("%s:\n    Switchball does not offer different backgrounds!\n",
                               filename);
                    if (werror_campaign) return 0;
                }
                else if (levelset_have_back || levelset_have_grad || levelset_have_song)
                    accept_back = 1;
            }
        }
        else if (strcmp(k, "grad") == 0)
        {
            if (campaign)
            {
                if (str_starts_with(filename, "buildin-map-campaign/sky"))
                    SAFECPY(target_grad, sbtheme_limitation_grad[0]);
                else if (str_starts_with(filename, "buildin-map-campaign/ice"))
                    SAFECPY(target_grad, sbtheme_limitation_grad[1]);
                else if (str_starts_with(filename, "buildin-map-campaign/cave"))
                    SAFECPY(target_grad, sbtheme_limitation_grad[2]);
                else if (str_starts_with(filename, "buildin-map-campaign/cloud"))
                    SAFECPY(target_grad, sbtheme_limitation_grad[3]);
                else if (str_starts_with(filename, "buildin-map-campaign/lava"))
                    SAFECPY(target_grad, sbtheme_limitation_grad[4]);
                else
                {
                    log_errorf("%s:\n    Switchball does not offer different gradients!\n",
                               filename);
                    return 0;
                }

                accept_grad = strcmp(v, target_grad) == 0;

                if (!accept_grad)
                {
                    log_errorf("%s:\n    Switchball requires gradient with matched filename!\n",
                               filename);
                    return 0;
                }
            }
            else if (pre_campaign)
            {
                if (!levelset_have_back && !levelset_have_grad && !levelset_have_song)
                {
                    for (int i = 0; i < 5 && !levelset_have_grad; i++)
                        if (strcmp(v, sbtheme_limitation_grad[i]) == 0)
                        {
                            SAFECPY(target_back, sbtheme_limitation_back[i]);
                            SAFECPY(target_song, sbtheme_limitation_song[i]);

                            levelset_have_grad = 1;
                            break;
                        }
                }
                else if ((levelset_have_back || levelset_have_song) && !levelset_have_grad)
                    levelset_have_grad = strcmp(v, target_grad) == 0;

                if (!levelset_have_grad)
                {
                    log_errorf("%s:\n    Switchball does not offer different gradients!\n",
                               filename);
                    if (werror_campaign) return 0;
                }
                else if (levelset_have_back || levelset_have_grad || levelset_have_song)
                    accept_grad = 1;
            }
        }
    }

    if (accept_back && accept_grad)
    {
        if (text_length(target_song) > 3 && strcmp(l->song, target_song))
        {
            log_errorf("%s:\n    Campaign music replaced as Switchball: %s -> %s\n",
                       filename, l->song, target_song);
            memset(l->song, 0, 260);
            SAFECPY(l->song, target_song);
        }

        if (base->uc)
        {
            if (base->uc != 1 && werror_campaign)
            {
                log_errorf("%s:\n    Switchball does not offer multiple balls!\n",
                           filename);
                return 0;
            }
        }
        else
        {
            log_errorf("%s:\n    Switchball requires start position!\n",
                       filename);
            return 0;
        }

        if (base->zc)
        {
            if (base->zc != 1 && werror_campaign)
            {
                log_errorf("%s:\n    Switchball does not offer multiple goals!\n",
                           filename);
                return 0;
            }
        }
        else
        {
            log_errorf("%s:\n    Switchball requires goal!\n",
                       filename);
            return 0;
        }

        /* Campaign level accepted! */

        if (pre_campaign)
            log_printf("%s:\n    A new Switchball level creation has been qualified"
                       "\n    and placed into the level set to acknowledge"
                       "\n    your custom campaign!\n",
                       filename);

        return 1;
    }

    if (campaign || pre_campaign)
    {
        log_errorf("%s:\n    Switchball requires background environment and gradient"
                   "\n    with matched filename!\n",
                   filename);
        return 0;
    }

    return 1;
}

int level_load(const char *filename, struct level *level)
{
    struct s_base base;

    memset(level, 0, sizeof (struct level));
    memset(&base, 0, sizeof (base));

    if (!str_ends_with(filename, ".csol")  &&
        !str_ends_with(filename, ".csolx") &&
        !str_ends_with(filename, ".sol")   &&
        !str_ends_with(filename, ".solx"))
    {
        log_errorf("That's not the classic or campaign SOL or SOLX extension!: %s\n",
                   filename);
        return 0;
    }

    if (!sol_load_meta(&base, filename))
    {
        log_errorf("Failure to load level file: %s / %s\n",
                   filename, fs_error());
        return 0;
    }

    SAFECPY(level->file, filename);
    SAFECPY(level->name, "0");

    score_init_hs(&level->scores[SCORE_TIME], 59999, 0);
    score_init_hs(&level->scores[SCORE_GOAL], 59999, 0);
    score_init_hs(&level->scores[SCORE_COIN], 59999, 0);

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

    int level_offered = 0;

    int campaign_werror = !config_cheat();

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    if (campaign_used())
    {
        level_offered = scan_level_attribs(level, &base, 1, 0, campaign_werror);

        if (level_offered)
            level_offered = scan_campaign_level(&base, filename, level, 1, 0, campaign_werror);
    }
    else
#endif
    {
        level_offered = scan_level_attribs(level, &base, 0,
            str_starts_with(curr_setid_final, "SB") ||
            str_starts_with(curr_setid_final, "sb") ||
            str_starts_with(curr_setid_final, "Sb") ||
            str_starts_with(curr_setid_final, "sB"),
            campaign_werror
        );

        if (level_offered)
            level_offered = scan_campaign_level(&base, filename, level, 0,
                str_starts_with(curr_setid_final, "SB") ||
                str_starts_with(curr_setid_final, "sb") ||
                str_starts_with(curr_setid_final, "Sb") ||
                str_starts_with(curr_setid_final, "sB"),
                campaign_werror
            );

        if ((str_starts_with(curr_setid_final, "SB") ||
             str_starts_with(curr_setid_final, "sb") ||
             str_starts_with(curr_setid_final, "Sb") ||
             str_starts_with(curr_setid_final, "sB")) &&
            !level_offered)
            log_errorf("%s:\n    Switchball level creation was disqualified!\n", filename);
    }

    sol_free_base(&base);

    if (!level_offered)
        memset(level, 0, sizeof (struct level));

    return level_offered;
}

/*---------------------------------------------------------------------------*/

int level_exists(int i)
{
    return !!get_level(i);
}

void level_open(struct level *level)
{
    if (level) level->is_locked = 0;
}

int level_opened(const struct level *level)
{
    return level ? level->is_locked == 0 : 1;
}

void level_complete(struct level *level)
{
    if (level) level->is_completed = 1;
}

int level_completed(const struct level *level)
{
    return level ? level->is_completed : 0;
}

int level_time(const struct level *level)
{
    return level ? level->time : 0;
}

int level_goal(const struct level *level)
{
    return level ? level->goal : 0;
}

int  level_bonus(const struct level *level)
{
    return level ? level->is_bonus : 0;
}

int  level_master(const struct level *level)
{
    return level ? level->is_master : 0;
}

const char *level_shot(const struct level *level)
{
    return level ? level->shot : NULL;
}

const char *level_file(const struct level *level)
{
    return level ? level->file : NULL;
}

const char *level_song(const struct level *level)
{
    return level ? level->song : NULL;
}

const char *level_name(const struct level *level)
{
    return level ? level->name : NULL;
}

const char *level_msg(const struct level *level)
{
    if (level && text_length(level->message) > 0)
        return _(level->message);

    return "";
}

const struct score *level_score(struct level *level, int s)
{
    return level ? &level->scores[s] : NULL;
}

/*---------------------------------------------------------------------------*/

int level_score_update(struct level *l,
                       int timer,
                       int coins,
                       int *time_rank,
                       int *goal_rank,
                       int *coin_rank)
{
    const char *player =  config_get_s(CONFIG_PLAYER);

    score_time_insert(&l->scores[SCORE_TIME], time_rank, player, timer, coins);
    score_time_insert(&l->scores[SCORE_GOAL], goal_rank, player, timer, coins);
    score_coin_insert(&l->scores[SCORE_COIN], coin_rank, player, timer, coins);

    if ((time_rank && *time_rank < RANK_LAST) ||
        (goal_rank && *goal_rank < RANK_LAST) ||
        (coin_rank && *coin_rank < RANK_LAST))
        return 1;
    else
        return 0;
}

void level_rename_player(struct level *l,
                         int time_rank,
                         int goal_rank,
                         int coin_rank,
                         const char *player)
{
    SAFECPY(l->scores[SCORE_TIME].player[time_rank], player);
    SAFECPY(l->scores[SCORE_GOAL].player[goal_rank], player);
    SAFECPY(l->scores[SCORE_COIN].player[coin_rank], player);
}

/*---------------------------------------------------------------------------*/
