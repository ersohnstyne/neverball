/*
 * Copyright (C) 2026 Microsoft / Neverball authors / Jānis Rūcis
 *
 * NEVERPUTT is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */

#if NB_HAVE_PB_BOTH==1 && NB_PB_SDL3==1
#define SDL_ENABLE_OLD_NAMES
#include <SDL3/SDL.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_oldnames.h>
#elif _WIN32 && __MINGW32__
#include <SDL2/SDL.h>
#include <SDL2/SDL_stdinc.h>
#elif _WIN32 && _MSC_VER
#include <SDL.h>
#include <SDL_stdinc.h>
#elif _WIN32
#error Security compilation error: No target include file in path for Windows specified!
#else
#include <SDL.h>
#include <SDL_stdinc.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strbuf/joinstr.h"

#include "common.h"
#include "config.h"
#include "course.h"
#include "hole.h"
#include "fs.h"
#include "lang.h"
#include "../ball/set.h"

#include "package.h"

#include "log.h"

/*---------------------------------------------------------------------------*/

struct course
{
    char holes[MAXSTR];

    char shot[MAXSTR];
    char desc[MAXSTR];
};

static int course_state = 0;

static int course;
static int count;

static struct course course_v[MAXCRS];

static char *bundled_filenames[MAXCRS];
static int bundled_count;

/*---------------------------------------------------------------------------*/

/* Scorecard in a single course. */

static int course_score_comp(struct course_score *S, int i, int j)
{
    if (i < 0 || j < 0 || i >= MAXPLY || j >= MAXPLY) return 0;

    return  S->score[i] <  S->score[j] ||
           (S->score[i] == S->score[j] && S->score[i] > S->score[j]);
}

static void course_score_swap(struct course_score *S, int i, int j)
{
    if (i < 0 || j < 0 || i >= MAXPLY || j >= MAXPLY) return;

    char player[MAXSTR];
    int  tmp;

    SAFECPY(player,       S->player[i]);
    SAFECPY(S->player[i], S->player[j]);
    SAFECPY(S->player[j], player);

    tmp             = S->player_id[i];
    S->player_id[i] = S->player_id[j];
    S->player_id[j] = tmp;

    tmp         = S->score[i];
    S->score[i] = S->score[j];
    S->score[j] = tmp;

    tmp            = S->score_hs[i];
    S->score_hs[i] = S->score_hs[j];
    S->score_hs[j] = tmp;

    tmp              = S->score_high[i];
    S->score_high[i] = S->score_high[j];
    S->score_high[j] = tmp;
}

void course_score_init_hs(struct course_score *s)
{
    memset(s, 0, sizeof (struct course_score));

    for (int i = 0; i < 5; i++)
    {
        if (i == 5) break;

        char name[MAXSTR];

        if (i < 2)
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(name, MAXSTR,
#else
            sprintf(name,
#endif
                          _("%s (%d)"),
                          config_get_s(CONFIG_PLAYER), i - 1);
        else
            SAFECPY(name, config_get_s(CONFIG_PLAYER));

        s->player_id[i] = i;

        course_score_update_name(s, i, name);

        /* MAXHOL * 30 = 840 */

        course_score_insert(s, i /*, i == 0 ? MAXHOL * -30 : MAXHOL * 30*/);
    }
}

void course_score_update_name(struct course_score *s,
                              int id, const char *name)
{
    if (id < 0 || id >= MAXPLY) return;

    memset(s->player[id], 0, sizeof (s->player[id]));
    SAFECPY(s->player[id], name);
}

void course_score_insert(struct course_score *s, int score)
{
    for (int i = 1, done = 0; i < 5 && !done; i++)
    {
        if (i == 5) done = 1;

        if (done) break;

        if (s->player_id[i] == curr_player())
        {
            s->score[i] = score;

            for (int j = i; j > 0 && course_score_comp(s, j + 1, j); j--)
                course_score_swap(s, j + 1, j);

            done = 1;
            break;
        }
    }
}

void course_score_finish(struct course_score *s)
{
    for (int i = 1, done = 0; i < 5 && !done; i++)
    {
        if (s->score_hs[i] > s->score[i])
        {
#ifndef COURSE_SCORE_NOSAVEGAME
            s->score_high[i] = 1;
#endif
            s->score_hs  [i] = s->score[i];
        }
    }
}

/*---------------------------------------------------------------------------*/

static int course_load(struct course *crs, const char *filename)
{
    fs_file fin;
    int rc = 0;

    if (!crs || !filename || !*filename)
        return 0;

    memset(crs, 0, sizeof (*crs));

    strncpy(crs->holes, filename, MAXSTR - 1);

    if ((fin = fs_open_read(filename)))
    {
        if (fs_gets(crs->shot, sizeof (crs->shot), fin) &&
            fs_gets(crs->desc, sizeof (crs->desc), fin))
        {
            strip_newline(crs->shot);
            strip_newline(crs->desc);

            rc = 1;
        }

        fs_close(fin);
    }

    return rc;
}

static int cmp_dir_items(const void *A, const void *B)
{
    const struct dir_item *a = A, *b = B;
    return strcmp(a->path, b->path);
}

int course_find(const char *path)
{
    int i;

    for (i = 0; i < count; i++)
        if (strcmp(course_v[i].holes, path) == 0)
            return i;

    return -1;
}

static int course_is_loaded(const char *path)
{
    return course_find(path) >= 0;
}

static int is_unseen_course(struct dir_item *item)
{
    return (str_starts_with(base_name(item->path), "holes-") &&
            str_ends_with(item->path, ".txt") &&
            !course_is_loaded(item->path));
}

void course_init(void)
{
    fs_file fin;
    char *line;

    Array items;
    int i;

    if (course_state)
        course_free();

    for (i = 0; i < bundled_count; ++i)
    {
        if (bundled_filenames[i])
        {
            free(bundled_filenames[i]);
            bundled_filenames[i] = NULL;
        }
    }
    bundled_count = 0;

    count = 0;

    if ((fin = fs_open_read(COURSE_FILE)))
    {
        while (read_line(&line, fin))
        {
            if (count < MAXCRS && course_load(&course_v[count], line))
                count++;

            if (bundled_count < MAXCRS)
                bundled_filenames[bundled_count++] = line;
            else
            {
                free(line);
                line = NULL;
            }
        }

        fs_close(fin);

        course_state = 1;
    }

    if ((items = fs_dir_scan("", is_unseen_course)))
    {
        array_sort(items, cmp_dir_items);

        for (i = 0; i < array_len(items) && count < MAXCRS; i++)
            if (course_load(&course_v[count], DIR_ITEM_GET(items, i)->path))
                count++;

        fs_dir_free(items);

        course_state = 1;
    }
}

int course_exists(int i)
{
    return (0 <= i && i < count);
}

int course_count(void)
{
    return count;
}

void course_goto(int i)
{
    if (course_exists(i))
    {
        hole_init(course_v[i].holes);
        course = i;
    }
}

int course_curr(void)
{
    return course;
}

void course_free(void)
{
    if (!course_state) return;

    for (int i = 0; i < bundled_count; ++i)
    {
        free(bundled_filenames[i]);
        bundled_filenames[i] = NULL;
    }
    bundled_count = 0;

    hole_free();
    course_state = 0;
}

int course_bundle_index(const char *filename)
{
    int i;

    if (!(filename && *filename))
        return -1;

    for (i = 0; i < bundled_count; ++i)
    {
        if (bundled_filenames[i] && strcmp(bundled_filenames[i], filename) == 0)
            return i;
    }

    return -1;
}

void course_rand(void)
{
    if (count > 0)
    {
        course_goto(rand() % count);
        if (curr_count() > 0)
            hole_goto(rand() % curr_count(), 4);
    }
}

int course_cmp_available_packages(const void *A, const void *B)
{
    const struct package *a = (const struct package *) A;
    const struct package *b = (const struct package *) B;

    const int a_is_set = strcmp(a->type, "set") == 0;
    const int b_is_set = strcmp(b->type, "set") == 0;

    // Level sets come first.

    if (a_is_set && !b_is_set)
        return -1;

    if (b_is_set && !a_is_set)
        return +1;

    if (a_is_set && b_is_set)
    {
        // Level sets in sets.txt take precedence.

        STRBUF a_file = joinstr(a->id, ".txt");
        STRBUF b_file = joinstr(b->id, ".txt");

        int a_index = set_bundle_index(CSTR(a_file));
        int b_index = set_bundle_index(CSTR(a_file));

        if (a_index >= 0 && b_index < 0)
            return -1;

        if (b_index >= 0 && a_index < 0)
            return +1;

        if (a_index >= 0 && b_index >= 0)
            return a_index - b_index;
    }

    const int a_is_course = strcmp(a->type, "course") == 0;
    const int b_is_course = strcmp(b->type, "course") == 0;

    // Courses come after level sets.

    if (a_is_course && !b_is_course)
        return -1;

    if (b_is_course && !a_is_course)
        return +1;

    if (a_is_course && b_is_course)
    {
        const char *a_sub = strchr(a->id, '-');
        const char *b_sub = strchr(b->id, '-');

        if (a_sub && b_sub)
        {
            STRBUF a_name = joinstr("holes-", a_sub + 1);
            STRBUF b_name = joinstr("holes-", b_sub + 1);
            STRBUF a_file = joinstr(CSTR(a_name), ".txt");
            STRBUF b_file = joinstr(CSTR(b_name), ".txt");

            int a_index = course_bundle_index(CSTR(a_file));
            int b_index = course_bundle_index(CSTR(b_file));

            if (a_index >= 0 && b_index < 0)
                return -1;

            if (b_index >= 0 && a_index < 0)
                return +1;

            if (a_index >= 0 && b_index >= 0)
                return a_index - b_index;
        }
    }

    // Rest are ordered by id (e.g., "set-mym").

    return SDL_strcasecmp(a->id, b->id);
}

/*---------------------------------------------------------------------------*/

const char *course_name(int i)
{
    static char name[MAXSTR];

    if (course_exists(i))
    {
        const char *p = strchr(course_v[i].desc, '\\');

        if (p)
        {
            size_t len = p - course_v[i].desc;

            if (len >= sizeof (name))
                len = sizeof (name) - 1;

            strncpy(name, course_v[i].desc, len);
            name[len] = '\0';

            return name;
        }
        return course_v[i].desc;
    }
    return "";
}

const char *course_desc(int i)
{
    return course_exists(i) ? course_v[i].desc : "";
}

const char *course_shot(int i)
{
    if (course_exists(i))
        return course_v[i].shot;
    if (count > 0)
        return course_v[0].shot;
    return "";
}

/*---------------------------------------------------------------------------*/
