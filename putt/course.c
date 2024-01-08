/*
 * Copyright (C) 2023 Microsoft / Neverball authors
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

#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "config.h"
#include "course.h"
#include "hole.h"
#include "fs.h"

#include "log.h"

#if _DEBUG && _MSC_VER
#ifndef _CRTDBG_MAP_ALLOC
#pragma message(__FILE__": Missing CRT-Debugger include header, recreate: crtdbg.h")
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

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

/*---------------------------------------------------------------------------*/

/* Scorecard in a single course. */

static int course_score_comp(struct course_score *S, int i, int j)
{
    return  S->score[i] <  S->score[j] ||
           (S->score[i] == S->score[j] && S->score[i] > S->score[j]);
}

static void course_score_swap(struct course_score *S, int i, int j)
{
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

        course_score_insert(s, i, i == 0 ? MAXHOL * -30 : MAXHOL * 30);
    }
}

void course_score_update_name(struct course_score *s,
                              int id, const char *name)
{
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

static int course_is_loaded(const char *path)
{
    int i;

    for (i = 0; i < count; i++)
        if (strcmp(course_v[i].holes, path) == 0)
            return 1;

    return 0;
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

    count = 0;

    if ((fin = fs_open_read(COURSE_FILE)))
    {
        while (count < MAXCRS && read_line(&line, fin))
        {
            if (course_load(&course_v[count], line))
                count++;

            free(line);
            line = NULL;
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
    hole_init(course_v[i].holes);
    course = i;
}

int course_curr(void)
{
    return course;
}

void course_free(void)
{
    hole_free();
    course_state = 0;
}

void course_rand(void)
{
    if (count == 0) return;

    course_goto(rand() % count);
    hole_goto  (rand() % curr_count(), 4);
}

/*---------------------------------------------------------------------------*/

const char *course_name(int i)
{
    return "";
}

const char *course_desc(int i)
{
    return course_exists(i) ? course_v[i].desc : "";
}

const char *course_shot(int i)
{
    return course_exists(i) ? course_v[i].shot : course_v[0].shot;
}

/*---------------------------------------------------------------------------*/
