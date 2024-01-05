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

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "campaign.h"
#include "progress.h"
#include "demo.h"
#include "audio.h"
#include "config.h"
#include "binary.h"
#include "common.h"
#include "level.h"
#include "array.h"
#include "dir.h"

#include "game_server.h"
#include "game_client.h"
#include "game_proxy.h"
#include "game_common.h"

#define DATELEN sizeof ("DD.MM.YYYYTHH:MM:SS")

fs_file demo_fp;

/*---------------------------------------------------------------------------*/

static const char *demo_path(const char *name)
{
    static char path[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(path, MAXSTR,
#else
    sprintf(path,
#endif
            "Replays/%s.nbr", name);

    return path;
}

static const char *demo_name(const char *path)
{
    static char name[MAXSTR];
    SAFECPY(name, base_name_sans(path, ".nbr"));
    return name;
}

/*---------------------------------------------------------------------------*/

int demo_requires_update;
int demo_old_detected;

static int demo_header_read(fs_file fp, struct demo *d)
{
    int magic;
    int version;
    int t;

    struct tm date;
    char datestr[DATELEN];

    magic   = get_index(fp);
    version = get_index(fp);

    t = get_index(fp);

    if (magic == DEMO_MAGIC && t
     && (version >= DEMO_VERSION_MIN && version <= DEMO_VERSION))
    {
        d->timer = t;

        d->coins  = get_index(fp);
        d->status = get_index(fp);
        d->mode   = get_index(fp);

        get_string(fp, d->player, sizeof (d->player));
        get_string(fp, datestr, sizeof (datestr));

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sscanf_s(datestr,
#else
        sscanf(datestr,
#endif
               "%d-%d-%dT%d:%d:%d",
               &date.tm_year,
               &date.tm_mon,
               &date.tm_mday,
               &date.tm_hour,
               &date.tm_min,
               &date.tm_sec);

        date.tm_year -= 1900;
        date.tm_mon  -= 1;
        date.tm_isdst = -1;

        d->date = make_time_from_utc(&date);

        get_string(fp, d->shot, PATHMAX);
        get_string(fp, d->file, PATHMAX);

        d->time  = get_index(fp);
        d->goal  = get_index(fp);
        (void)     get_index(fp);
        d->score = get_index(fp);
        d->balls = get_index(fp);
        d->times = get_index(fp);

        return 1;
    }

    if (version < DEMO_VERSION_MIN)
        demo_old_detected = 1;

    if (version < DEMO_VERSION)
        demo_requires_update = 1;

    return 0;
}

static void demo_header_write(fs_file fp, struct demo *d)
{
    char datestr[DATELEN];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    struct tm outputGmTime; gmtime_s(&outputGmTime, &d->date);
    strftime(datestr, sizeof (datestr), "%Y-%m-%dT%H:%M:%S", &outputGmTime);
#else
    strftime(datestr, sizeof (datestr), "%Y-%m-%dT%H:%M:%S", gmtime(&d->date));
#endif
    
    put_index(fp, DEMO_MAGIC);
    put_index(fp, DEMO_VERSION);
    put_index(fp, 0);
    put_index(fp, 0);
    put_index(fp, 0);
    put_index(fp, d->mode);

    put_string(fp, d->player);
    put_string(fp, datestr);

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    put_string(fp, d->mode == MODE_CAMPAIGN || campaign_used() ?
                   "gui/levels/campaign_replay.jpg" : d->shot);
#else
    put_string(fp, d->shot);
#endif
    put_string(fp, d->file);

    put_index(fp, d->time);
    put_index(fp, d->goal);
    put_index(fp, 0);                   /* Unused (was goal enabled flag).   */
    put_index(fp, d->score);
    put_index(fp, d->balls);
    put_index(fp, d->times);
}

/*---------------------------------------------------------------------------*/

int demo_load(struct demo *d, const char *path)
{
    int rc = 0;

    if (d)
    {
        fs_file fp;

        memset(d, 0, sizeof (*d));

        if ((fp = fs_open_read(path)))
        {
            SAFECPY(d->path, path);
            SAFECPY(d->name, demo_name(path));

            if (demo_header_read(fp, d))
                rc = 1;

            fs_close(fp);
        }
    }

    return rc;
}

void demo_free(struct demo *d)
{
}

/*---------------------------------------------------------------------------*/

//#define DEMO_FORMAT_DATETIME

int demo_exists(const char *name)
{
    return fs_exists(demo_path(name));
}

const char *demo_format_name(const char *fmt,
                             const char *set,
                             const char *level,
                             int status)
{
    const char *d_status;
    static char name[MAXSTR];
    int         space_left;
    char       *numpart;
    int         i;

    if (!fmt)
        return NULL;

    if (!set)
        set = "none";

    if (!level)
        level = "00";

    switch (status)
    {
        case GAME_GOAL: d_status = "g"; break;
        case GAME_FALL: d_status = "xf"; break;
        case GAME_TIME: d_status = "xt"; break;
        default: d_status = "x"; break;
    }

    memset(name, 0, sizeof (name));
    space_left = MAXSTRLEN(name);

    /* Construct name, replacing each format sequence as appropriate. */

    while (*fmt && space_left > 0)
    {
        if (*fmt == '%')
        {
            fmt++;

            switch (*fmt)
            {
                case 's':
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                    strncat_s(name, MAXSTR, set, space_left);
#else
                    strncat(name, set, space_left);
#endif
                    space_left -= strlen(set);
                    break;

                case 'l':
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                    strncat_s(name, MAXSTR, level, space_left);
#else
                    strncat(name, level, space_left);
#endif
                    space_left -= strlen(level);
                    break;

                case 'r':
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                    strncat_s(name, MAXSTR, d_status, space_left);
#else
                    strncat(name, d_status, space_left);
#endif
                    space_left -= strlen(d_status);
                    break;

                case '%':
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                    strncat_s(name, MAXSTR, "%", space_left);
#else
                    strncat(name, "%", space_left);
#endif
                    space_left--;
                    break;

                case '\0':
                    /* Missing format. */
                    fmt--;
                    break;

                default:
                    /* Invalid format. */
                    break;
            }
        }
        else
        {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            strncat_s(name, MAXSTR, fmt, 1);
#else
            strncat(name, fmt, 1);
#endif
            space_left--;
        }

        fmt++;
    }

#ifdef DEMO_FORMAT_DATETIME
    struct tm date_src; time_t date_dst;

    /*
     * Append a unique datetime format preceded by an underscore to the
     * file name, discarding characters if there's not enough space
     * left in the buffer.
     */

    if (space_left < strlen("_YYYY-MM-DD_HH-MM-SS"))
        numpart = name + MAXSTRLEN(name) - strlen("_YYYY-MM-DD_HH-MM-SS");
    else
        numpart = name + MAXSTRLEN(name) - space_left;

    make_time_from_utc(&date_src);

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(numpart, MAXSTR,
#else
    sprintf(numpart,
#endif
            "_%04d-%02d-%02d_%02d-%02d-%02d",
            date_src.tm_year + 1900,
            date_src.tm_mon + 1,
            date_src.tm_mday,
            date_src.tm_hour,
            date_src.tm_min,
            date_src.tm_sec);
#else
    /*
     * Append a unique 4-digit number preceded by an underscore to the
     * file name, discarding characters if there's not enough space
     * left in the buffer.
     */

    if (space_left < strlen("_23"))
        numpart = name + MAXSTRLEN(name) - strlen("_23");
    else
        numpart = name + MAXSTRLEN(name) - space_left;

    for (i = 1; i < 100; i++)
    {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(numpart, MAXSTR,
#else
        sprintf(numpart,
#endif
                "_%02d", i);

        if (!demo_exists(name))
            break;
    }
#endif

    return name;
}

/*---------------------------------------------------------------------------*/

//#define DISABLE_RECORDINGS 1

static struct demo demo_play;

int demo_play_init(const char *name, const struct level *level,
                   int mode, int scores, int balls, int times, float speedpercent)
{
#if !DISABLE_RECORDINGS
    struct demo *d = &demo_play;

    memset(d, 0, sizeof (*d));

    SAFECPY(d->path,   demo_path(name));
    SAFECPY(d->name,   name);
    SAFECPY(d->player, config_get_s(CONFIG_PLAYER));
    SAFECPY(d->shot,   level_shot(level));
    SAFECPY(d->file,   level_file(level));

    d->mode  = mode;
    d->date  = time(NULL);
    d->time  = level_time(level);
    d->goal  = level_goal(level);
    d->score = scores;
    d->balls = balls;
    d->times = times;

    d->speedpercent = speedpercent;
    if ((demo_fp = fs_open_write(d->path)))
    {
        demo_header_write(demo_fp, d);
        return 1;
    }
#endif
    return 0;
}

void demo_play_stat(int status, int coins, int timer)
{
    if (demo_fp)
    {
        long pos = fs_tell(demo_fp);

        fs_seek(demo_fp, 8, SEEK_SET);

        put_index(demo_fp, timer);
        put_index(demo_fp, coins);
        put_index(demo_fp, status);

        fs_seek(demo_fp, pos, SEEK_SET);
    }
}

static void demo_refresh(void)
{
#ifdef __EMSCRIPTEN__
    EM_ASM({
        Neverball.refreshReplays();
    });
#endif
    return;
}

void demo_play_stop(int d)
{
    if (demo_fp)
    {
        fs_close(demo_fp);
        demo_fp = NULL;

        if (d)
            fs_remove(demo_play.path);

        fs_persistent_sync();
        demo_refresh();
    }
}

int demo_saved(void)
{
    return fs_exists(demo_play.path);
}

int demo_rename(const char *name)
{
    int r = 0;
    char path[MAXSTR];

    if (name && *name)
    {
        SAFECPY(path, demo_path(name));

        if (strcmp(demo_play.name, name) != 0 && fs_exists(demo_play.path))
        {
            r = fs_rename(demo_play.path, path);
            fs_persistent_sync();
            demo_refresh();
        }
    }

    return r;
}

void demo_rename_player(const char *name, const char *player)
{
    /* TODO */
    return;
}

/*---------------------------------------------------------------------------*/

static struct lockstep update_step;

static void demo_update_read(float dt)
{
    if (demo_fp)
    {
        union cmd cmd;

        while (cmd_get(demo_fp, &cmd))
        {
            game_proxy_enq(&cmd);

            if (cmd.type == CMD_UPDATES_PER_SECOND)
                update_step.dt = 1.0f / cmd.ups.n;

            if (cmd.type == CMD_END_OF_UPDATE)
            {
                game_client_sync(NULL);
                break;
            }
        }
    }
}

static struct lockstep update_step = { demo_update_read, DT };

float demo_replay_blend(void)
{
    return lockstep_blend(&update_step);
}

/*---------------------------------------------------------------------------*/

static struct demo demo_replay;

const char *curr_demo(void)
{
    return demo_replay.path;
}

int demo_replay_init(const char *path, int *g, int *m, int *b, int *s, int *tt, float *spp)
{
    lockstep_clr(&update_step);

    if ((demo_fp = fs_open_read(path)))
    {
        if (demo_header_read(demo_fp, &demo_replay))
        {
            struct level level;

            SAFECPY(demo_replay.path, path);
            SAFECPY(demo_replay.name, demo_name(path));

            if (level_load(demo_replay.file, &level))
            {
                if (g)  *g  = demo_replay.goal;
                if (m)  *m  = demo_replay.mode;
                if (b)  *b  = demo_replay.balls;
                if (s)  *s  = demo_replay.score;
                if (tt) *tt = demo_replay.times;

                if (spp) *spp = demo_replay.speedpercent;

                /*
                 * Init client and then read and process the first batch of
                 * commands from the replay file.
                 */

                if (game_client_init(demo_replay.file))
                {
                    game_client_toggle_show_balls(1);

                    /*
                    if (g)
                        audio_music_fade_to(0.5f, BGM_TITLE_MAP(level.song));
                    else
                    {
                        union cmd cmd = { CMD_GOAL_OPEN };
                        game_proxy_enq(&cmd);
                    }
                    */

                    if (g && s && tt)
                        audio_music_fade_to(0.5f, BGM_TITLE_MAP(level.song));

                    demo_update_read(0);

                    if (!fs_eof(demo_fp))
                        return 1;
                }
            }
            else
                log_errorf("Could not find level map!: %s / %s\n",
                           demo_replay.file, fs_error());
        }
        else
            log_errorf("Could not find the replay header file!\n");

        fs_close(demo_fp);
        demo_fp = NULL;
    }
    else
        log_errorf("Could not find the replay file!: %s / %s\n",
                   path, fs_error());

    return 0;
}

int demo_replay_step(float dt)
{
    if (demo_fp)
    {
        lockstep_run(&update_step, dt);
        return !fs_eof(demo_fp);
    }
    return 0;
}

void demo_replay_stop(int d)
{
    if (demo_fp)
    {
        fs_close(demo_fp);
        demo_fp = NULL;

        if (d) fs_remove(demo_replay.path);
        
        fs_persistent_sync();
        demo_refresh();
    }
}

void demo_replay_speed(int speed)
{
    if (SPEED_NONE <= speed && speed < SPEED_MAX)
        lockstep_scl(&update_step, SPEED_FACTORS[speed]);
}

void demo_replay_manual_speed(float speeddir)
{
    lockstep_scl(&update_step, speeddir);
}

/*---------------------------------------------------------------------------*/
