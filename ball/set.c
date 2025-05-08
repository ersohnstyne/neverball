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
#include <string.h>

#if NB_HAVE_PB_BOTH==1
#include "campaign.h"
#include "networking.h"

#if NB_STEAM_API==1
#include "score_online.h"
#endif
#endif

#include "glext.h"
#include "account.h"
#include "config.h"
#include "video.h"
#include "image.h"
#include "set.h"
#include "common.h"
#include "fs.h"
#include "log.h"
#include "lang.h"

#include "game_server.h"
#include "game_client.h"
#include "game_proxy.h"

/*---------------------------------------------------------------------------*/

struct set
{
    /* Levelset info                                                         */

    char file[PATHMAX];

    char *id;                           /* Internal set identifier           */
    char *name;                         /* Set name                          */
    char *desc;                         /* Set description                   */
    char *shot;                         /* Set screen-shot                   */

#if NB_HAVE_PB_BOTH==1
    int   star;                         /* Set completition stars (set)      */
    int   star_prev;                    /* Set completition stars (current)  */
    int   star_obtained;                /* Set completition stars (current)  */

    int   balls_needed;                 /* Balls needed to start challenge   */
#endif

    /* HS Info                                                               */

    char *user_scores;                  /* User high-score file              */
    char *cheat_scores;                 /* Cheat mode score file             */

    struct score coin_score;            /* Challenge score                   */
    struct score time_score;            /* Challenge score                   */

    /* Level info                                                            */

    int   count;                        /* Number of levels                  */
    char *level_name_v[MAXLVL_SET];     /* List of level file names          */
};

#define SET_GET(a, i) ((struct set *) array_get((a), (i)))

static Array sets;
static int   curr;

static struct level level_v[MAXLVL_SET];

/*---------------------------------------------------------------------------*/

#define SCORE_VERSION 3

static int score_version;

#define put_score  staticlocal_set_put_score
#define get_score  staticlocal_set_get_score
#define find_level staticlocal_set_find_level
#define get_stats  staticlocal_set_get_stats

static void put_score(fs_file fp, const struct score *s)
{
    for (int i = RANK_HARD; i <= RANK_EASY; i++)
        fs_printf(fp, "%d %d %s\n", s->timer[i], s->coins[i], s->player[i]);
}

static int get_score(fs_file fp, struct score *s)
{
    char line[MAXSTR];
    int i;

    for (i = RANK_HARD; i <= RANK_EASY; i++)
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

void set_store_hs(void)
{
    if (!sets) return;

    const struct set *s = SET_GET(sets, curr);
    fs_file fp;
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
    if ((fp = fs_open_write(config_cheat() ? s->cheat_scores : s->user_scores)))
#else
    if ((fp = fs_open_write(s->user_scores)))
#endif
    {
        int i;

#if NB_HAVE_PB_BOTH==1
        fs_printf(fp, "version %d\nrewarded %d\nset %s\n", SCORE_VERSION,
                      s->star_obtained, s->id);
#else
        fs_printf(fp, "version %d\nset %s\n", 2, s->id);
#endif

        put_score(fp, &s->time_score);
        put_score(fp, &s->coin_score);

        for (i = 0; i < s->count; i++)
        {
            const struct level *l = &level_v[i];

            int flags = 0;

            if (l->is_locked)    flags |= LEVEL_LOCKED;
            if (l->is_completed) flags |= LEVEL_COMPLETED;

            fs_printf(fp, "level %d %d %s\n", flags, l->version_num, l->file);

            fs_printf(fp, "stats %d %d %d\n", l->stats.completed,
                      l->stats.timeout, l->stats.fallout);

            put_score(fp, &l->scores[SCORE_TIME]);
            put_score(fp, &l->scores[SCORE_GOAL]);
            put_score(fp, &l->scores[SCORE_COIN]);
        }

        fs_close(fp);
    }
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
    else log_errorf("Failure to save set highscores!: %s / %s\n",
                    config_cheat() ? s->cheat_scores : s->user_scores,
                    fs_error());
#else
    else log_errorf("Failure to save set highscores!: %s / %s\n",
                    s->user_scores, fs_error());
#endif
}

static struct level *find_level(const struct set *s, const char *file)
{
    int i;

    for (i = 0; i < s->count; i++)
        if (strcmp(level_v[i].file, file) == 0)
            return &level_v[i];

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

static void set_load_hs_v3(fs_file fp, struct set *s, char *buf, int size)
{
    struct score time_score;
    struct score coin_score;

#if NB_HAVE_PB_BOTH==1
    int rewarded_stars = 0;
    int set_stars = 0;
#endif

    int set_score = 0;
    int set_match = 1;

    while (fs_gets(buf, size, fp))
    {
        int set_version = 0;
        int flags       = 0;
        int n           = 0;

        strip_newline(buf);

#if NB_HAVE_PB_BOTH==1
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        if (sscanf_s(buf,
#else
        if (sscanf(buf,
#endif
                   "rewarded %d", &rewarded_stars) >= 1)
            set_stars = 1;

        else
#endif
        if (strncmp(buf, "set ", 4) == 0)
        {
            get_score(fp, &time_score);
            get_score(fp, &coin_score);

            set_score = 1;
        }
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        else if (sscanf_s(buf,
#else
        else if (sscanf(buf,
#endif
                        "level %d %d %n", &flags, &set_version, &n) >= 2)
        {
            struct level *l;

            if ((l = find_level(s, buf + n)))
            {
                get_stats(fp, l);

                /* Always prefer "locked" flag from the score file. */

                l->is_locked = (flags & LEVEL_LOCKED);

                /* Only use "completed" flag and scores on version match. */

                if (set_version == l->version_num)
                {
                    l->is_completed = (flags & LEVEL_COMPLETED);

                    get_score(fp, &l->scores[SCORE_TIME]);
                    get_score(fp, &l->scores[SCORE_GOAL]);
                    get_score(fp, &l->scores[SCORE_COIN]);
                }
                else set_match = 0;
            }
            else set_match = 0;
        }
    }

#if NB_HAVE_PB_BOTH==1
    if (set_match && set_stars && s->star > 0)
    {
        s->star_obtained = rewarded_stars;
        s->star_prev = s->star_obtained;
    }
#endif

    if (set_match && set_score)
    {
        s->time_score = time_score;
        s->coin_score = coin_score;
    }
}

static void set_load_hs_v2(fs_file fp, struct set *s, char *buf, int size)
{
    struct score time_score;
    struct score coin_score;

    int set_score = 0;
    int set_match = 1;

    while (fs_gets(buf, size, fp))
    {
        int set_version = 0;
        int flags       = 0;
        int n           = 0;

        strip_newline(buf);

        if (strncmp(buf, "set ", 4) == 0)
        {
            get_score(fp, &time_score);
            get_score(fp, &coin_score);

            set_score = 1;
        }
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        else if (sscanf_s(buf,
#else
        else if (sscanf(buf,
#endif
                        "level %d %d %n", &flags, &set_version, &n) >= 2)
        {
            struct level *l;

            if ((l = find_level(s, buf + n)))
            {
                get_stats(fp, l);

                /* Always prefer "locked" flag from the score file. */

                l->is_locked = (flags & LEVEL_LOCKED);

                /* Only use "completed" flag and scores on version match. */

                if (set_version == l->version_num)
                {
                    l->is_completed = (flags & LEVEL_COMPLETED);

                    get_score(fp, &l->scores[SCORE_TIME]);
                    get_score(fp, &l->scores[SCORE_GOAL]);
                    get_score(fp, &l->scores[SCORE_COIN]);
                }
                else set_match = 0;
            }
            else set_match = 0;
        }
    }

    if (set_match && set_score)
    {
        s->time_score = time_score;
        s->coin_score = coin_score;
    }
}

static void set_load_hs_v1(fs_file fp, struct set *s, char *buf, int size)
{
    struct level *l;
    int    i, n;

    /* First line holds level states. */

    n = MIN(strlen(buf), s->count);

    for (i = 0; i < n; i++)
    {
        l = &level_v[i];

        l->is_locked    = (buf[i] == 'L');
        l->is_completed = (buf[i] == 'C');
    }

    get_score(fp, &s->time_score);
    get_score(fp, &s->coin_score);

    for (i = 0; i < n; i++)
    {
        l = &level_v[i];

        get_score(fp, &l->scores[SCORE_TIME]);
        get_score(fp, &l->scores[SCORE_GOAL]);
        get_score(fp, &l->scores[SCORE_COIN]);
    }
}

static void set_load_hs(void)
{
    struct set *s = SET_GET(sets, curr);
    fs_file fp;
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
    if ((fp = fs_open_read(config_cheat() ? s->cheat_scores : s->user_scores)))
#else
    if ((fp = fs_open_read(s->user_scores)))
#endif
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
                       "version %d", &score_version) == 1)
            {
                switch (score_version)
                {
                    case 2: set_load_hs_v2(fp, s, buf, sizeof (buf)); break;
                    case 3: set_load_hs_v3(fp, s, buf, sizeof (buf)); break;
                }
            }
            else
                set_load_hs_v1(fp, s, buf, sizeof (buf));
        }

        fs_close(fp);
    }
}

/*---------------------------------------------------------------------------*/

static int set_load(struct set *s, const char *filename)
{
    fs_file  fin;
    char    *scores, *level_name;
    int      curr_date_month = 0;

    time_t     curr_date = time(NULL);
    struct tm *curr_date_localtime = localtime(&curr_date);

    if (curr_date_localtime)
        curr_date_month = curr_date_localtime->tm_mon + 1;

    if (!filename)
        return 0;

    /* Skip "Misc" when not in dev mode. */

    if ((strcmp(filename, SET_MISC) == 0)
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
        && !config_cheat()
#endif
        ) return 0;

#if NB_HAVE_PB_BOTH==1
#ifdef CONFIG_INCLUDES_ACCOUNT
    /* Add more level sets when products bought. */

    if ((strcmp(filename, "set-easy.txt")   != 0 &&
         strcmp(filename, "set-medium.txt") != 0 &&
         strcmp(filename, "set-hard.txt")   != 0 &&
         strcmp(filename, "set-mym.txt")    != 0 &&
         strcmp(filename, "set-mym2.txt")   != 0 &&
         strcmp(filename, "set-fwp.txt")    != 0 &&
         strcmp(filename, "set-tones.txt")  != 0 &&
         strcmp(filename, SET_MISC)         != 0) &&
        (!account_get_d(ACCOUNT_PRODUCT_LEVELS) &&
         !server_policy_get_d(SERVER_POLICY_LEVELSET_ENABLED_CUSTOMSET)))
        return 0;
#else
    if ((strcmp(filename, "set-easy.txt")   != 0 &&
         strcmp(filename, "set-medium.txt") != 0 &&
         strcmp(filename, "set-hard.txt")   != 0 &&
         strcmp(filename, "set-mym.txt")    != 0 &&
         strcmp(filename, "set-mym2.txt")   != 0 &&
         strcmp(filename, "set-fwp.txt")    != 0 &&
         strcmp(filename, "set-tones.txt")  != 0 &&
         strcmp(filename, SET_MISC)         != 0) &&
        !server_policy_get_d(SERVER_POLICY_LEVELSET_ENABLED_CUSTOMSET))
        return 0;
#endif

    /* Limited offered region only */

    if (str_starts_with(filename, "set-anime") &&
        str_ends_with(filename, ".txt") &&
        !config_cheat())
    {
        if (server_policy_get_d(SERVER_POLICY_EDITION) < 3)
            return 0;

        if (config_get_s(CONFIG_LANGUAGE) &&
            (strcmp(config_get_s(CONFIG_LANGUAGE), "ja") != 0 &&
             strcmp(config_get_s(CONFIG_LANGUAGE), "jp") != 0))
            return 0;
    }

    /* Limited special offers only */

    if ((str_starts_with(filename, "set-valentine") &&
         str_ends_with  (filename, ".txt")) &&
        curr_date_month != 2 &&
        !config_cheat())
        return 0;

    if ((str_starts_with(filename, "set-freeland") &&
        str_ends_with(filename, ".txt")) &&
        curr_date_month != 5 &&
        !config_cheat())
        return 0;

    if ((str_starts_with(filename, "set-halloween") &&
         str_ends_with  (filename, ".txt")) &&
        curr_date_month != 10 &&
        !config_cheat())
        return 0;

    if ((str_starts_with(filename, "set-christmas") &&
         str_ends_with  (filename, ".txt")) &&
        curr_date_month != 12 &&
        !config_cheat())
        return 0;
#endif

    if (!(fin = fs_open_read(filename)))
    {
        log_errorf("Failure to load set file: %s / %s\n",
                   filename, fs_error());
        return 0;
    }

    memset(s, 0, sizeof (struct set));

    /* Set some same values in case the scores are missing. */

    score_init_hs(&s->time_score, 359999, 0);
    score_init_hs(&s->coin_score, 359999, 0);

#if NB_STEAM_API==1
    score_steam_init_hs(s->id, 1);
#endif

    SAFECPY(s->file, filename);

    if (read_line(&s->name, fin) &&
        read_line(&s->desc, fin) &&
        read_line(&s->id,   fin) &&
        read_line(&s->shot, fin) &&
        read_line(&scores,  fin))
    {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sscanf_s(scores,
#else
        sscanf(scores,
#endif
#if NB_HAVE_PB_BOTH==1
               "%d %d %d %d %d %d %d %d",
#else
               "%d %d %d %d %d %d",
#endif
               &s->time_score.timer[RANK_HARD],
               &s->time_score.timer[RANK_MEDM],
               &s->time_score.timer[RANK_EASY],
               &s->coin_score.coins[RANK_HARD],
               &s->coin_score.coins[RANK_MEDM],
               &s->coin_score.coins[RANK_EASY]
#if NB_HAVE_PB_BOTH==1
               , &s->star, &s->balls_needed
#endif
        );

        free(scores);
        scores = NULL;

        s->user_scores  = concat_string("Scores/", s->id, ".txt",       NULL);
        s->cheat_scores = concat_string("Scores/", s->id, "-cheat.txt", NULL);

        s->count = 0;

        while (s->count < MAXLVL_SET && read_line(&level_name, fin))
        {
            strip_spaces(level_name);

            if (*level_name)
            {
                s->level_name_v[s->count] = level_name;
                s->count++;
            }
            else
            {
                free(level_name);
                level_name = NULL;
            }
        }

        fs_close(fin);

        return 1;
    }

    log_errorf("Failure to load set file: %s / %s\n",
               filename, fs_error());

    free(s->name);
    free(s->desc);
    free(s->id);
    free(s->shot);

#if NB_HAVE_PB_BOTH==1
    s->balls_needed = 0;
    s->star = 0;
#endif

    s->name = NULL;
    s->desc = NULL;
    s->id   = NULL;
    s->shot = NULL;

    fs_close(fin);

    return 0;
}

static void set_free(struct set *s)
{
    int i;

    free(s->name);
    free(s->desc);
    free(s->id);
    free(s->shot);

#if NB_HAVE_PB_BOTH==1
    s->balls_needed = 0;
    s->star = 0;
#endif

    s->name = NULL;
    s->desc = NULL;
    s->id   = NULL;
    s->shot = NULL;

    free(s->user_scores);
    free(s->cheat_scores);

    s->user_scores = NULL;
    s->cheat_scores = NULL;

    for (i = 0; i < s->count; i++)
    {
        free(s->level_name_v[i]);
        s->level_name_v[i] = NULL;
    }
}

/*---------------------------------------------------------------------------*/

static int cmp_dir_items(const void *A, const void *B)
{
    const struct dir_item *a = A, *b = B;
    return strcmp(a->path, b->path);
}

static int set_is_loaded(const char *path)
{
    return (set_find(path) >= 0);
}

static int is_unseen_set(struct dir_item *item)
{
    return (str_starts_with(base_name(item->path), "set-") &&
            str_ends_with(item->path, ".txt") &&
            !set_is_loaded(item->path));
}

static int is_unseen_boost(struct dir_item *item)
{
    return (str_starts_with(base_name(item->path), "boost-") &&
            str_ends_with(item->path, ".txt") &&
            !set_is_loaded(item->path));
}

int set_init(int boost_active)
{
    fs_file fin;

    Array items;
    int i;

    if (sets)
        set_quit();

    sets = array_new(sizeof (struct set));
    curr = 0;

    /*
     * First, load the sets listed in the set file, preserving order.
     */

    if ((fin = fs_open_read(boost_active == 1 ? BOOST_FILE : SET_FILE)))
    {
        char *name;

        while (read_line(&name, fin))
        {
            struct set *s = array_add(sets);

            if (!set_load(s, name))
                array_del(sets);

            free(name);
            name = NULL;
        }
        fs_close(fin);
    }

    /*
     * Then, scan for any remaining set description files, and add
     * them after the first group in alphabetic order.
     */

    if ((items = fs_dir_scan("", (boost_active ? is_unseen_boost : is_unseen_set))))
    {
        array_sort(items, cmp_dir_items);

        for (i = 0; i < array_len(items); i++)
        {
            struct set *s = array_add(sets);

            if (!set_load(s, DIR_ITEM_GET(items, i)->path))
                array_del(sets);
        }

        fs_dir_free(items);
    }

    return array_len(sets);
}

void set_quit(void)
{
    if (sets)
    {
        int i, n = array_len(sets);

        for (i = 0; i < n; i++)
            set_free(array_get(sets, i));

        array_free(sets);
        sets = NULL;
    }
}

/*---------------------------------------------------------------------------*/


int set_exists(int i)
{
    return sets ? 0 <= i && i < array_len(sets) : 0;
}

const char *set_file(int i)
{
    return set_exists(i) ? SET_GET(sets, i)->file : NULL;
}

const char *set_id(int i)
{
    return set_exists(i) ? SET_GET(sets, i)->id : NULL;
}

const char *set_name(int i)
{
    return set_exists(i) ? _(SET_GET(sets, i)->name) : NULL;
}

const char *set_name_n(int i)
{
    return set_exists(i) ? N_(SET_GET(sets, i)->name) : NULL;
}

const char *set_desc(int i)
{
    return set_exists(i) ? _(SET_GET(sets, i)->desc) : NULL;
}

const char *set_shot(int i)
{
    return set_exists(i) ? SET_GET(sets, i)->shot : NULL;
}

#if NB_HAVE_PB_BOTH==1
int set_star(int i)
{
    return set_exists(i) ? SET_GET(sets, i)->star : 0;
}

int set_star_curr(int i)
{
    return set_exists(i) ? SET_GET(sets, i)->star_obtained : 0;
}

int set_star_gained(int i)
{
    return set_exists(i) ?
           SET_GET(sets, i)->star_prev < SET_GET(sets, i)->star_obtained : 0;
}

int set_balls_needed(int i)
{
    return set_exists(i) ?
           SET_GET(sets, i)->balls_needed : 0;
}
#endif

const struct score *set_score(int i, int s)
{
    if (set_exists(i))
    {
        if (s == SCORE_TIME) return &SET_GET(sets, i)->time_score;
        if (s == SCORE_COIN) return &SET_GET(sets, i)->coin_score;
    }
    return NULL;
}

/*---------------------------------------------------------------------------*/

#define SET_DEFAULT_MAX_TIME_LIMIT 359999
static int default_set_maxtimelimit;
static int default_set_mincoinrequired;

static void set_load_levels(void)
{
    /*
     * Legacy roman numbers doesn't: I V X C D M
     * New roman numbers should work: Ⅰ Ⅱ Ⅲ Ⅳ Ⅴ Ⅵ Ⅶ Ⅷ Ⅸ Ⅹ Ⅺ Ⅻ Ⅼ Ⅽ Ⅾ Ⅿ
     */
    static const char *roman[] = {
        "",
        "I",   "II",   "III",   "IV",   "V",                /* 1 - 5   */
        "VI",  "VII",  "VIII",  "IX",   "X",                /* 6 - 10  */
        "XI",  "XII",  "XIII",  "XIV",  "XV",               /* 11 - 15 */
        "XVI", "XVII", "XVIII", "XIX",  "XX",               /* 16 - 20 */
        "XXI", "XXII", "XXIII", "XXIV", "XXV",              /* 21 - 25 */

        "XXVI",  "XXVII",  "XXVIII",  "XXIX",  "XXX",       /* 26 - 30 */
        "XXXI",  "XXXII",  "XXXIII",  "XXXIV", "XXXV",      /* 31 - 35 */
        "XXXVI", "XXXVII", "XXXVIII", "XIL",   "XL",        /* 36 - 40 */
        "XLI",   "XLII",   "XLIII",   "XLIV",  "XLV",       /* 41 - 45 */
        "XLVI",  "XLVII",  "XLVIII",  "IL",    "L",         /* 46 - 50 */

        "LI",   "LII",   "LIII",   "LIV",   "LV",           /* 51 - 55 */
        "LVI",  "LVII",  "LVIII",  "LIX",   "LX",           /* 56 - 60 */
        "LXI",  "LXII",  "LXIII",  "LXIV",  "LXV",          /* 61 - 65 */
        "LXVI", "LXVII", "LXVIII", "LXIX",  "LXX",          /* 66 - 70 */
        "LXXI", "LXXII", "LXXIII", "LXXIV", "LXXV",         /* 71 - 75 */

        "LXXVI",  "LXXVII",  "LXXVIII",  "LXXIX",  "LXXX",  /* 76 - 80  */
        "LXXXI",  "LXXXII",  "LXXXIII",  "LXXXIV", "LXXXV", /* 81 - 85  */
        "LXXXVI", "LXXXVII", "LXXXVIII", "LXXXIX", "XC",    /* 86 - 90  */
        "XCI",    "XCII",    "XCIII",    "XCIV",   "XCV",   /* 91 - 95  */
        "XCVI",   "XCVII",   "XCVIII",   "XCIX",   "C",     /* 96 - 100 */

        "CI",   "CII",   "CIII",   "CIV",   "CV",           /* 101 - 105 */
        "CVI",  "CVII",  "CVIII",  "CIX",   "CX",           /* 106 - 110 */
        "CXI",  "CXII",  "CXIII",  "CXIV",  "CXV",          /* 111 - 115 */
        "CXVI", "CXVII", "CXVIII", "CXIX",  "CXX",          /* 116 - 120 */
        "CXXI", "CXXII", "CXXIII", "CXXIV", "CXXV",         /* 121 - 125 */

        "CXXVI",  "CXXVII",  "CXXVIII",  "CXXIX",  "CXXX",  /* 126 - 130 */
        "CXXXI",  "CXXXII",  "CXXXIII",  "CXXXIV", "CXXXV", /* 131 - 135 */
        "CXXXVI", "CXXXVII", "CXXXVIII", "CXIL",   "CXL",   /* 136 - 140 */
        "CXLI",   "CXLII",   "CXLIII",   "CXLIV",  "CXLV",  /* 141 - 145 */
        "CXLVI",  "CXLVII",  "CXLVIII",  "CIL",    "CL",    /* 146 - 150 */
    };

    struct set *s = SET_GET(sets, curr);
    int         regular = 1,
                bonus   = 1,
                master  = 1;

    int i;

    int  i_num_indiv_theme = 1;
    char tmp_bgm_path[MAXSTR];
    SAFECPY(tmp_bgm_path, "bgm/");

    /* Atomic Elbow tried to retreat! */
    int i_retreat = 0;

    default_set_maxtimelimit    = 0;
    default_set_mincoinrequired = 0;

    for (i = 0; i < 150; i++)
        memset(&level_v[i], 0, sizeof (struct level));

    for (i = 0; i < s->count; i++)
    {
        struct level *l = &level_v[i - i_retreat];

        int lvl_was_offered = level_load(s->level_name_v[i], l);

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

            if (l->is_master)
            {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(l->name, MAXSTR,
#else
                sprintf(l->name,
#endif
                        "M%d", master);

                master++;
            }
            else if (l->is_bonus)
            {
                SAFECPY(l->name, roman[bonus]);
                bonus++;
            }
            else
            {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(l->name, MAXSTR,
#else
                sprintf(l->name,
#endif
                       "%d", regular);

                regular++;
            }

            if (l->time > 0 && !l->is_bonus)
                default_set_maxtimelimit += l->time;
            else if (default_set_maxtimelimit < SET_DEFAULT_MAX_TIME_LIMIT && !l->is_bonus)
            {
                log_printf("No time limit on this level, so time limit for level set has been lifted.\n");
                default_set_maxtimelimit = SET_DEFAULT_MAX_TIME_LIMIT;
            }

            if (l->goal && !l->is_bonus)
                default_set_mincoinrequired += l->goal;

            if ((i - i_retreat) > 0)
                level_v[(i - i_retreat) - 1].next = l;
        }
        else
        {
            SAFECPY(l->file, ""); l->file[0] = 0;
            i_retreat++;
            log_errorf("Could not load level file: %s / Retreated levels: %d\n", s->level_name_v[i], i_retreat);
        }
    }

    for (int r = 0; r < 3; r++)
    {
        /* Most coins and Best Time built-in limitations. */

        if (s->coin_score.coins[r] < default_set_mincoinrequired)
            s->coin_score.coins[r] = default_set_mincoinrequired;
        if (s->coin_score.timer[r] > default_set_maxtimelimit)
            s->coin_score.timer[r] = default_set_maxtimelimit;
        if (s->time_score.coins[r] < default_set_mincoinrequired)
            s->time_score.coins[r] = default_set_mincoinrequired;
        if (s->time_score.timer[r] > default_set_maxtimelimit)
            s->time_score.timer[r] = default_set_maxtimelimit;
    }
}

void set_goto(int i)
{
    curr = i;

#if ENABLE_MOON_TASKLOADER==0
    set_load_levels();
    set_load_hs();
#endif
}

void set_scan_level_files(void)
{
#if ENABLE_MOON_TASKLOADER!=0
    set_load_levels();
    set_load_hs();
#endif
}

int set_find(const char *file)
{
    if (sets)
    {
        int i, n;

        for (i = 0, n = array_len(sets); i < n; ++i)
            if (strcmp(SET_GET(sets, i)->file, file) == 0)
                return i;
    }

    return -1;
}

/*
 * Find a level in the set given a SOL/SOLX basename.
 */
struct level *set_find_level(const char *basename)
{
    if (sets && curr_set() < array_len(sets))
    {
        struct set *s = SET_GET(sets, curr_set());

        int i;

        for (i = 0; i < s->count; ++i)
        {
            if (strcmp(basename, base_name(level_v[i].file)) == 0)
                return &level_v[i];
        }
    }

    return NULL;
}

int curr_set(void)
{
    return curr;
}

struct level *get_level(int i)
{
    return (i >= 0 && i < SET_GET(sets, curr)->count) ?
           (level_v[i].file[0] ? &level_v[i] : NULL) : NULL;
}

/*---------------------------------------------------------------------------*/

#if NB_HAVE_PB_BOTH==1
int set_star_update(int completed)
{
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    if (campaign_used()) return 0;
#endif

    struct set *s = SET_GET(sets, curr);

    s->star_prev = s->star_obtained;

    if (s->star == s->star_obtained) return 0;

    if (completed)
        s->star_obtained = s->star;
    else
        s->star_obtained = 0;

    return completed;
}
#endif

int set_score_update(int timer, int coins, int *score_rank, int *times_rank)
{
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    if (campaign_used()) return 0;
#endif

    struct set *s = SET_GET(sets, curr);
    const char *player = config_get_s(CONFIG_PLAYER);

    score_coin_insert(&s->coin_score, score_rank, player, timer, coins);
    score_time_insert(&s->time_score, times_rank, player, timer, coins);

    if ((score_rank && *score_rank < RANK_LAST) ||
        (times_rank && *times_rank < RANK_LAST))
        return 1;
    else
        return 0;
}

void set_rename_player(int score_rank, int times_rank, const char *player)
{
    struct set *s = SET_GET(sets, curr);

    SAFECPY(s->coin_score.player[score_rank], player);
    SAFECPY(s->time_score.player[times_rank], player);
}

/*---------------------------------------------------------------------------*/

void level_snap(int i, const char *path)
{
    char *filename;

    /* Convert the level name to a PNG filename. */

    filename = concat_string(path,
                             "/",
                             base_name_sans(level_v[i].file, ".sol"),
                             ".png",
                             NULL);

    /* Initialize the game for a snapshot. */

    if (game_client_init(level_v[i].file))
    {
        /* HACK: Can avoid placing balls before screenshots. */

        game_client_toggle_show_balls(0);

        union cmd cmd = { CMD_GOAL_OPEN };
        game_proxy_enq(&cmd);
        game_client_sync(NULL);

        /* Render the level and grab the screen. */

        video_clear();

        game_client_fly(1.0f);
        game_kill_fade();
        game_client_draw(POSE_LEVEL, 0);
        video_snap(filename);
        video_swap();
    }

    free(filename);
    filename = NULL;
}

void set_cheat(void)
{
    int i;

    for (i = 0; i < SET_GET(sets, curr)->count; i++)
    {
        level_v[i].is_locked    = 0;
        level_v[i].is_completed = 1;
    }
}

void set_detect_bonus_product(void)
{
    int i;

    for (i = 0; i < SET_GET(sets, curr)->count; i++)
    {
        if (level_v[i].is_bonus) {
            level_v[i].is_locked    = 0;
            level_v[i].is_completed = 0;
        }
    }
}

/*---------------------------------------------------------------------------*/
