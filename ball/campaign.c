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
#include "solid_chkp.h"
#include "campaign.h"
#include "solid_vary.h"

#if NB_STEAM_API==1
#include "score_online.h"
#endif

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "config.h"
#include "fs.h"
#include "level.h"
#include "log.h"
#include "score.h"
#include "vec3.h"
#include "package.h"
#include "lang.h"

#include "game_client.h"

#if _DEBUG && _MSC_VER
#ifndef _CRTDBG_MAP_ALLOC
#pragma message(__FILE__": Missing _CRT_MAP_ALLOC, recreate: _CRTDBG_MAP_ALLOC + crtdbg.h")
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

#if NB_HAVE_PB_BOTH!=1
#error Security compilation error: Preprocessor definitions can be used it, \
       once you have transferred or joined into the target Discord Server, \
       and verified and promoted as Developer Role. \
       This invite link can be found under https://discord.gg/qnJR263Hm2/.
#else
#ifndef MAPC_INCLUDES_CHKP
#if NB_HAVE_PB_BOTH==1
#error Security compilation error: Campaign is added, but requires Checkpoints!
#endif
#endif
#endif

#if DEVEL_BUILD
#define CAREER_PERMA_UNLOCKED
//#define HARDCORE_PERMA_UNLOCKED
#endif

#define cam_box_trigger_test_master(pos, camtrigger, idx) \
    (pos[idx] > camtrigger->positions[idx] - camtrigger->triggerSize[idx] && \
     pos[idx] < camtrigger->positions[idx] + camtrigger->triggerSize[idx])

/*---------------------------------------------------------------------------*/

#define TIME_TRIAL_VERSION  2
#define MAX_CAM_BOX_TRIGGER 512

static char *campaign_name;

/*
 * Switchball levels can be found the levels by Badinfos.
 *
 * Sky world: https://www.youtube.com/watch?v=jLEQvMCTz2g
 * Ice world: https://www.youtube.com/watch?v=KHGqhk-jx6A
 * Cave world: https://www.youtube.com/watch?v=7Bz774GS-HE
 * Cloud world: https://www.youtube.com/watch?v=ORcGb10z5Bo
 * Lava world: https://www.youtube.com/watch?v=4PgaQFv2aQk
 *
 * NOTE: That ball and goal in level map must be containing Gyrocopter.
 * The view direction begins looking down Y-axis.
 *
 * Please review the GitHub repository template before downloading it:
 * https://github.com/ersohnstyne/neverball-switchball-bundle-pack
 */
static char *campaign_levelpath[30];
static int   campaign_count = 30;

static int level_difficulty = -1;

static int exists     = 0;
static int used       = 0;
static int theme_used = 0;

static struct score coin_trials;
static struct score time_trials;
static char        *time_trial_leaderboard;
static int          time_trial_version;

static struct campaign_hardcore_mode   hardcores;
static struct campaign_medal_data      medal_datas;
static struct campaign_cam_box_trigger cam_box_triggers[MAX_CAM_BOX_TRIGGER];

/* How many autocam box triggers have we got? */
static int autocam_count = 0;

/*
 * PennySchloss dynamically adds build-in electricity like
 * airport to your entities. Simple player start are no longer allowed.
 */
static struct level campaign_lvl_v[MAXLVL];

#define put_score  staticlocal_campaign_put_times
#define get_score  staticlocal_campaign_get_times
#define find_level staticlocal_campaign_find_level
#define get_stats  staticlocal_campaign_get_stats

static void put_score(fs_file fp, const struct score *s)
{
    for (int i = RANK_HARD; i <= RANK_EASY; i++)
        fs_printf(fp, "%d %d %s\n", s->timer [i],
                                    s->coins [i],
                                    s->player[i]);
}

static int get_score(fs_file fp, struct score *s)
{
    char line[MAXSTR];

    for (int i = RANK_HARD; i <= RANK_EASY; i++)
    {
        int n = -1;

        if (!fs_gets(line, sizeof (line), fp))
            return 0;

        strip_newline(line);

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        if (sscanf_s(line,
#else
        if (sscanf(line,
#endif
                   "%d %d %n", &s->timer[i], &s->coins[i], &n) < 2)
            return 0;

        if (n < 0)
            return 0;

        SAFECPY(s->player[i], line + n);
    }

    return 1;
}

static struct level *find_level(const char *file)
{
    for (int i = 0; i < campaign_count; i++)
        if (strcmp(campaign_lvl_v[i].file, file) == 0)
            return &campaign_lvl_v[i];

    return NULL;
}

static int get_stats(fs_file fp, struct level *l)
{
    char line[MAXSTR];

    if (!fs_gets(line, sizeof (line), fp))
        return 0;

    strip_newline(line);

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    if (sscanf_s(line, "stats %d %d %d", &l->stats.completed,
                                         &l->stats.timeout,
                                         &l->stats.fallout) < 3)
#else
    if (sscanf(line, "stats %d %d %d", &l->stats.completed,
                                       &l->stats.timeout,
                                       &l->stats.fallout) < 3)
#endif
    {
        /* compatible with save files without stats info */
        l->stats.completed = 0;
        l->stats.timeout   = 0;
        l->stats.fallout   = 0;

        /* stats not available, rewind file pointer */
        fs_seek(fp, -strlen(line) - 1, SEEK_CUR);
    }

    return 1;
}

void campaign_store_hs(void)
{
    fs_file fp;

    if ((fp = fs_open_write(time_trial_leaderboard)))
    {
        fs_printf(fp, "version %d\ncampaign\n", TIME_TRIAL_VERSION);

        put_score(fp, &time_trials);
        put_score(fp, &coin_trials);

        for (int i = 0; i < campaign_count; i++)
        {
            const struct level *l = &campaign_lvl_v[i];

            int flags = 0;

            if (l->is_locked)    flags |= LEVEL_LOCKED;
            if (l->is_completed) flags |= LEVEL_COMPLETED;

            fs_printf(fp, "level %d %d %s\n", flags, l->version_num, l->file);

            fs_printf(fp, "stats %d %d %d\n", l->stats.completed,
                                              l->stats.timeout,
                                              l->stats.fallout);

            put_score(fp, &l->scores[SCORE_TIME]);
            put_score(fp, &l->scores[SCORE_GOAL]);
            put_score(fp, &l->scores[SCORE_COIN]);
        }

        fs_close(fp);
    }
    else log_errorf("Save campaign highscores failed!: %s / %s\n", time_trial_leaderboard, fs_error());
}

static void campaign_load_hs_v2(fs_file fp, char *buf, int size)
{
    struct score time_trial;
    struct score coin_trial;

    int campaign_score = 0;
    int campaign_match = 1;

    while (fs_gets(buf, size, fp))
    {
        int campaign_version = 0;
        int flags            = 0;
        int n                = 0;

        strip_newline(buf);

        if (strncmp(buf, "campaign", 9) == 0)
        {
            get_score(fp, &time_trial);
            get_score(fp, &coin_trial);

            campaign_score = 1;
        }
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        else if (sscanf_s(buf,
#else
        else if (sscanf(buf,
#endif
                        "level %d %d %n", &flags, &campaign_version, &n) >= 2)
        {
            struct level *l;

            if ((l = find_level(buf + n)))
            {
                get_stats(fp, l);

                /* Always prefer "locked" flag from the score file. */

                l->is_locked = !!(flags & LEVEL_LOCKED);

                /* Only use "completed" flag and scores on version match. */

                if (campaign_version == l->version_num)
                {
                    l->is_completed = !!(flags & LEVEL_COMPLETED);

                    get_score(fp, &l->scores[SCORE_TIME]);
                    get_score(fp, &l->scores[SCORE_GOAL]);
                    get_score(fp, &l->scores[SCORE_COIN]);
                }
                else campaign_match = 0;
            }
            else campaign_match = 0;
        }
    }

    if (campaign_match && campaign_score)
    {
        time_trials = time_trial;
        coin_trials = coin_trial;
    }
}

static void campaign_load_hs_v1(fs_file fp, char *buf, int size)
{
    struct level *l;
    int i, n;

    n = MIN(strlen(buf), campaign_count);

    for (i = 0; i < n; i++)
    {
        l = &campaign_lvl_v[i];

        l->is_locked    = (buf[i] == 'L');
        l->is_completed = (buf[i] == 'C');
    }

    get_score(fp, &time_trials);
    get_score(fp, &coin_trials);

    for (i = 0; i < n; i++)
    {
        l = &campaign_lvl_v[i];

        get_score(fp, &l->scores[SCORE_TIME]);
        get_score(fp, &l->scores[SCORE_GOAL]);
        get_score(fp, &l->scores[SCORE_COIN]);
    }
}

static void campaign_load_hs(void)
{
    fs_file fp;

    if ((fp = fs_open_read(time_trial_leaderboard)))
    {
        char buf[MAXSTR];

        if (fs_gets(buf, sizeof (buf), fp))
        {
            strip_newline(buf);

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            if (sscanf_s(buf,
#else
            if (sscanf(buf,
#endif
                       "version %d", &time_trial_version) == 1)
            {
                switch (time_trial_version)
                {
                    case 2: campaign_load_hs_v2(fp, buf, sizeof (buf)); break;
                    /* case 3: campaign_load_hs_v3(fp, buf, sizeof (buf)); break; */
                }
            }
            else
                campaign_load_hs_v1(fp, buf, sizeof (buf));
        }

        fs_close(fp);
    }
}

/*---------------------------------------------------------------------------*/

int campaign_score_update(int timer, int coins, int *score_rank, int *times_rank)
{
    if (!campaign_used()) return 0;

    const char *player = config_get_s(CONFIG_PLAYER);

    int career_hs_unlocked = (campaign_career_unlocked() &&
                              config_get_d(CONFIG_LOCK_GOALS));

    score_coin_insert(&coin_trials, score_rank, player,
                      career_hs_unlocked ? timer : 360000, career_hs_unlocked ? coins : 0);
    score_time_insert(&time_trials, times_rank, player,
                      timer, coins);

    if ((score_rank && *score_rank < RANK_LAST) ||
        (times_rank && *times_rank < RANK_LAST))
        return 1;
    else
        return 0;
}

void campaign_rename_player(int times_rank, const char *player)
{
    SAFECPY(coin_trials.player[times_rank], player);
    SAFECPY(time_trials.player[times_rank], player);
}

int campaign_rank(void)
{
    /* Was: config_cheat() ? 4 : medal_datas.curr_rank; */
    return medal_datas.curr_rank;
}

/*---------------------------------------------------------------------------*/

int campaign_load(const char *filename)
{
    fs_file fin;
    char *scores, *level_name;

    if (!(fin = fs_open_read(filename)))
    {
        log_errorf("Failure to load campaign file: %s / %s\n",
                   filename, fs_error());
        return 0;
    }

    score_init_hs(&time_trials, 359999, 0);

#if NB_STEAM_API==1
    /* HACK: Only hardcore campaigns, that will work. */
    score_steam_init_hs(0, 0);
#endif

    read_line(&campaign_name, fin);

    if (strcmp(campaign_name, "Campaign") != 0)
    {
        free(campaign_name);
        campaign_name = NULL;

        return 0;
    }

    if (read_line(&scores, fin))
    {
        /* HACK: Using temporary HS values may especially efficient version. */

        int tmp_hs_timer[RANK_LAST], tmp_hs_coins[RANK_LAST];

        for (int i = 0; i < 3; i++)
        {
            tmp_hs_timer[i] = 359999;
            tmp_hs_coins[i] = 0;
        }

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        int val_currents = sscanf_s(scores,
#else
        int val_currents = sscanf(scores,
#endif
              "%d %d %d %d %d %d",
              &tmp_hs_timer[RANK_HARD],
              &tmp_hs_timer[RANK_MEDM],
              &tmp_hs_timer[RANK_EASY],
              &tmp_hs_coins[RANK_HARD],
              &tmp_hs_coins[RANK_MEDM],
              &tmp_hs_coins[RANK_EASY]);
        
        for (int i = 0; i < 3; i++)
        {
            if (tmp_hs_timer[i] != 0) time_trials.timer[i] = tmp_hs_timer[i];
            if (tmp_hs_coins[i] != 0) time_trials.coins[i] = tmp_hs_coins[i];

            if (val_currents != 6)
            {
                time_trials.timer[i] = 359999;
                time_trials.coins[i] = 0;
            }
        }

        free(scores);
        scores = NULL;

        time_trial_leaderboard = strdup("Campaign/time-trial.txt");

        campaign_count = 0;

        while (campaign_count < MAXLVL && read_line(&level_name, fin))
        {
            campaign_levelpath[campaign_count] = level_name;
            campaign_count++;
        }

        fs_close(fin);
        return 1;
    }

    free(campaign_name);
    campaign_name = NULL;

    fs_close(fin);

    return 0;
}

/* Initialize the campaign level */
static void campaign_load_levels(void)
{
    level_difficulty = -1;

    /* Bonus levels won't be shown as of the level sets */
    int i, regular = 1;

    int  i_num_indiv_theme = 1;
    char tmp_bgm_path[MAXSTR];
    SAFECPY(tmp_bgm_path, "bgm/");

    /* Atomic Elbow tried to retreat! */
    int i_retreat = 0;

    for (i = 0; i < 30; i++)
        memset(&campaign_lvl_v[i], 0, sizeof (struct level));

    for (i = 0; i < 30; ++i)
    {
        struct level *l = &campaign_lvl_v[i - i_retreat];

        int lvl_was_offered = level_load(campaign_levelpath[i], l);

        l->number       = i - i_retreat;
        l->is_locked    = (i - i_retreat) > 0 && lvl_was_offered;
        l->is_completed = 0;

        if (lvl_was_offered)
        {
            if (strcmp(tmp_bgm_path, l->song) == 0)
                i_num_indiv_theme++;
            else
            {
                i_num_indiv_theme = 1;
                SAFECPY(tmp_bgm_path, l->song);
            }

            l->num_indiv_theme = i_num_indiv_theme;

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(l->name, MAXSTR,
#else
            sprintf(l->name,
#endif
                    "%d", regular++);

            if ((i - i_retreat) > 0)
                campaign_lvl_v[(i - i_retreat) - 1].next = l;
        }
        else
        {
            SAFECPY(l->file, ""); l->file[0] = 0;
            i_retreat++;
            log_errorf("Could not load level file: %s / Retreated levels: %d\n", campaign_levelpath[i], i_retreat);
        }
    }
}

int campaign_find(const char *file)
{
    return (strcmp(CAMPAIGN_FILE, file) == 0);
}

int campaign_level_difficulty(void)
{
    return level_difficulty;
}

void campaign_upgrade_difficulty(int new_difficulty)
{
    if (level_difficulty < new_difficulty)
        level_difficulty = new_difficulty;
}

int campaign_used(void)
{
    return used;
}

int campaign_theme_used(void)
{
    return theme_used;
}

/* Initialize the campaign */
int campaign_init(void)
{
    level_difficulty = -1;

    /* Load the campaign file. */

    if (!campaign_load(CAMPAIGN_FILE))
    {
        log_errorf("Failure to load campaign! Is the campaign levels installed? / %s\n",
                   fs_error());
        return 0;
    }
    else
    {
        memset(&medal_datas, 0, sizeof (medal_datas));
        campaign_load_levels();
        campaign_load_hs();

        memset(&hardcores, '\0', sizeof (hardcores));

        /* As this should, it will be calculate the ranks. */
        for (int i = 0; i < 30; i++)
        {
            struct level *l = campaign_get_level(i);

            if (l && level_opened(l))
            {
                medal_datas.unlocks++;

                if (strcmp(l->scores->player[0], N_("Hard")) != 0 &&
                    strcmp(l->scores->player[0], "") != 0)
                    medal_datas.gold++;

                else if (strcmp(l->scores->player[1], N_("Medium")) != 0 &&
                         strcmp(l->scores->player[1], "") != 0)
                    medal_datas.silver++;

                else if (strcmp(l->scores->player[2], N_("Easy")) == 0 &&
                         strcmp(l->scores->player[2], "") == 0)
                    medal_datas.bronze++;
            }
        }

        /*
         * To get wing medals: Complete Level 3 in Campaign.
         * To get silver badge: Obtain most silver awards.
         * To get gold badge: Obtain most gold awards.
         * To get the gold wings: Achieve every gold awards on all levels.
         */

        if ((medal_datas.gold == medal_datas.unlocks) &&
             medal_datas.silver == 0 && medal_datas.bronze == 0 &&
            level_completed(campaign_get_level(3)))
            medal_datas.curr_rank = 4;
        else if (medal_datas.gold > medal_datas.silver &&
                 medal_datas.gold > medal_datas.bronze &&
                 level_completed(campaign_get_level(3)))
            medal_datas.curr_rank = 3;
        else if (medal_datas.gold < medal_datas.silver &&
                 medal_datas.silver > medal_datas.bronze &&
                 level_completed(campaign_get_level(3)))
            medal_datas.curr_rank = 2;
        else if (level_completed(campaign_get_level(3)))
            medal_datas.curr_rank = 1;
        else
            medal_datas.curr_rank = 0;

        campaign_reset_camera_box_trigger();

        used = 1;
    }

    return used;
}

/* Quit campaign */
void campaign_quit(void)
{
    used = 0;
    exists = 0;

    /* Once the campaign is left off, remove this. */

    if (time_trial_leaderboard)
    {
        free(time_trial_leaderboard);
        time_trial_leaderboard = NULL;
    }

    for (int i = 0; i < campaign_count; i++)
    {
        if (campaign_levelpath[i])
        {
            free(campaign_levelpath[i]);
            campaign_levelpath[i] = NULL;
        }
    }
}

/* Initialize campaign theme */
void campaign_theme_init(void)
{
    theme_used = 1;

    campaign_load_levels();
    campaign_load_hs();
}

void campaign_theme_quit(void)
{
    theme_used = 0;
}

/*---------------------------------------------------------------------------*/

struct level *campaign_get_level(int i)
{
    return (i >= 0 && i < campaign_count) ?
            (campaign_lvl_v[i].file[0] ? &campaign_lvl_v[i] : NULL) : NULL;
}

int campaign_career_unlocked(void)
{
#if !defined(CAREER_PERMA_UNLOCKED)
    for (int i = 0; i < 30; i++)
    {
        if (!level_completed(campaign_get_level(i)))
            return 0;
    }
#endif
    return 1;
}

int campaign_hardcore(void)
{
    return hardcores.used;
}

void campaign_hardcore_nextlevel(void)
{
    hardcores.level_number++;

    if (hardcores.level_number > 6)
    {
        hardcores.level_number = 1;
        hardcores.level_theme++;
    }
}

int campaign_hardcore_norecordings(void)
{
    return hardcores.norecordings;
}

int campaign_hardcore_unlocked(void)
{
#if !defined(HARDCORE_PERMA_UNLOCKED)
    for (int i = 0; i < 30; i++)
    {
        struct level *l = campaign_get_level(i);
        if (!l || (strcmp(l->scores->player[1], N_("Medium")) == 0 ||
                   strcmp(l->scores->player[1], "") == 0))
            return 0;
    }
#endif
    return 1;
}

void campaign_hardcore_set_coordinates(float x_position, float y_position)
{
    hardcores.coordinates[0] = x_position;
    hardcores.coordinates[1] = y_position;
}

void campaign_hardcore_play(int no_rec)
{
    hardcores.used         = 1;
    hardcores.norecordings = no_rec;
    hardcores.level_theme  = 1;
    hardcores.level_number = 1;
}

void campaign_hardcore_quit(void)
{
    game_fade_color(0.0f, 0.0f, 0.0f);
    hardcores.used         = 0;
    hardcores.norecordings = 0;
    hardcores.level_theme  = 0;
    hardcores.level_number = 0;
}

struct campaign_hardcore_mode campaign_get_hardcore_data(void)
{
    return hardcores;
}

/*---------------------------------------------------------------------------*/

int campaign_load_camera_box_trigger(const char *levelname)
{
    fs_file fh;
    char camFilename[MAXSTR];
    char camLinePrefix[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(camFilename, MAXSTR,
#else
    sprintf(camFilename,
#endif
            "buildin-map-campaign/autocam-level-%s.txt", levelname);

    campaign_reset_camera_box_trigger();

    if ((fh = fs_open_read(camFilename)))
    {
        int camBoxIdx = 0;
        while (fs_gets(camLinePrefix, MAXSTR, fh) &&
               camBoxIdx < MAX_CAM_BOX_TRIGGER)
        {
            cam_box_triggers[camBoxIdx].positions[0] = 0.0f;
            cam_box_triggers[camBoxIdx].positions[1] = 0.0f;
            cam_box_triggers[camBoxIdx].positions[2] = 0.0f;

            cam_box_triggers[camBoxIdx].triggerSize[0] = 1.0f;
            cam_box_triggers[camBoxIdx].triggerSize[1] = 1.0f;
            cam_box_triggers[camBoxIdx].triggerSize[2] = 1.0f;

            /* This level uses automatic camera for campaign
             * and is written the filename as follows:
             *
             *     autocam-level-5.txt
             *     autocam-level-12.txt
             *
             * Valid values:
             *
             *     pos: -65536 - 65536 (require 3 float values)
             *     size: -65536 - 65536 (require 3 float values)
             *     mode: 0 - 2
             *     campos: -65536 - 65536 (require 3 float values)
             *     dir: -180 - 180 (0 = North; 90 = East; 180/-180 = South; -90 = West)
             *
             * Auto-Camera values per line must be programmed
             * with corrected order:
             *
             *     pos (float vector)
             *     size (float vector)
             *     mode (integer)
             *     campos (float vector)
             *     dir (float)
             *
             * Especially, 64 units called 1 metres.
             *
             * Autocam files must be placed in the directory
             * "buildin-map-campaign" in the data folder, so it can be
             * test your camera modes in a single level.
             */
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            if (sscanf_s(camLinePrefix,
#else
            if (sscanf(camLinePrefix,
#endif
                       "pos:%f %f %f size:%f %f %f mode:%i campos:%f %f %f dir:%f",
                       &cam_box_triggers[camBoxIdx].positions[0], &cam_box_triggers[camBoxIdx].positions[2], &cam_box_triggers[camBoxIdx].positions[1],
                       &cam_box_triggers[camBoxIdx].triggerSize[0], &cam_box_triggers[camBoxIdx].triggerSize[2], &cam_box_triggers[camBoxIdx].triggerSize[1],
                       &cam_box_triggers[camBoxIdx].cammode,
                       &cam_box_triggers[camBoxIdx].campositions[0], &cam_box_triggers[camBoxIdx].campositions[2], &cam_box_triggers[camBoxIdx].campositions[1],
                       &cam_box_triggers[camBoxIdx].camdirection) == 11)
            {
                cam_box_triggers[camBoxIdx].positions[0] /= 64;
                cam_box_triggers[camBoxIdx].positions[1] /= 64;
                cam_box_triggers[camBoxIdx].positions[2] /= -64;

                cam_box_triggers[camBoxIdx].triggerSize[0] /= 64;
                cam_box_triggers[camBoxIdx].triggerSize[1] /= 64;
                cam_box_triggers[camBoxIdx].triggerSize[2] /= 64;

                cam_box_triggers[camBoxIdx].campositions[0] /= 64;
                cam_box_triggers[camBoxIdx].campositions[1] /= 64;
                cam_box_triggers[camBoxIdx].campositions[2] /= -64;

                cam_box_triggers[camBoxIdx].activated = 1;
                cam_box_triggers[camBoxIdx].inside    = 0;

                autocam_count++;
                ++camBoxIdx;
            }
            else
                log_errorf("%s: Parse line error on auto-camera trigger: %s\n", camFilename, camLinePrefix);

            if (camBoxIdx >= MAX_CAM_BOX_TRIGGER)
                break;
        }
        fs_close(fh);
        return 1;
    }

    log_errorf("Failure to load autocam level file: %s / %s\n",
               camFilename, fs_error());

    return 0;
}

void campaign_reset_camera_box_trigger(void)
{
    autocam_count = 0;

    for (int i = 0; i < MAX_CAM_BOX_TRIGGER; ++i)
    {
        memset(&cam_box_triggers[i], 0, sizeof (struct campaign_cam_box_trigger));

        /* Default should be permanently disabled */

        cam_box_triggers[i].activated = 0;
        cam_box_triggers[i].inside    = 0;
    }
}

struct campaign_cam_box_trigger *campaign_get_camera_box_trigger(int index)
{
    return &cam_box_triggers[index];
}

int campaign_camera_box_trigger_count(void)
{
    return autocam_count;
}

int campaign_camera_box_trigger_test(struct s_vary *vary, int ui)
{
    const float *ball_p = vary->uv[ui].p;

    for (int cam_id = 0; cam_id < autocam_count; cam_id++)
    {
        struct campaign_cam_box_trigger *localcamboxtrigger = cam_box_triggers + cam_id;

        if (cam_box_trigger_test_master(ball_p, localcamboxtrigger, 0) &&
            cam_box_trigger_test_master(ball_p, localcamboxtrigger, 1) &&
            cam_box_trigger_test_master(ball_p, localcamboxtrigger, 2))
        {
            cam_box_triggers[cam_id].activated = -1;
            cam_box_triggers[cam_id].inside    =  1;
            return cam_id;
        }

        if (cam_box_triggers[cam_id].inside    != 0 &&
            cam_box_triggers[cam_id].activated == -1)
        {
            cam_box_triggers[cam_id].activated = 1;
            cam_box_triggers[cam_id].inside    = 0;
        }
    }

    return -1;
}

/*---------------------------------------------------------------------------*/

#endif
