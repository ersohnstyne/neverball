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

#ifndef LEVEL_H
#define LEVEL_H

#include "base_config.h"
#include "score.h"

/*---------------------------------------------------------------------------*/

enum
{
    SCORE_TIME = 0,
    SCORE_GOAL,
    SCORE_COIN
};

struct stats
{
    int completed;
    int timeout;
    int fallout;
};

#define LEVEL_LOCKED          0x01
#define LEVEL_COMPLETED       0x02

struct level
{
    char file[PATHMAX];
    char shot[PATHMAX];
    char song[PATHMAX];

    char message[MAXSTR];

    char version_str[32];
    int  version_num;
    char author[MAXSTR];

    int time; /* Time limit   */
    int goal; /* Coins needed */

    struct score scores[3];
    struct stats stats;

    /* Set information. */

    int  number;
    int  num_indiv_theme;

    /* String representation of the number (eg. "IV") */
    char name[MAXSTR];

    int is_locked;
    int is_bonus;
    int is_master;
    int is_completed;

    struct level *next;
};

int  level_load(const char *, struct level *);

/*---------------------------------------------------------------------------*/

int level_exists(int);

void level_open(struct level *);
int level_opened(const struct level *);

void level_complete(struct level *);
int level_completed(const struct level *);

int level_time(const struct level *);
int level_goal(const struct level *);
int level_bonus(const struct level *);
int level_master(const struct level *);

const char *level_shot(const struct level *);
const char *level_file(const struct level *);
const char *level_song(const struct level *);
const char *level_name(const struct level *);
const char *level_msg(const struct level *);

const struct score *level_score(struct level *, int);

/*---------------------------------------------------------------------------*/

int  level_score_update (struct level *, int, int, int *, int *, int *);
void level_rename_player(struct level *, int, int, int, const char *);

/*---------------------------------------------------------------------------*/

#endif
