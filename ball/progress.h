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

#ifndef PROGRESS_H
#define PROGRESS_H

#include "util.h"

#include "level.h"
#include "campaign.h"

/*---------------------------------------------------------------------------*/

void progress_rush_collect_coin_value(int);
void progress_enable_max_speed(void);
void progress_sonic_step(float dt);

/*---------------------------------------------------------------------------*/

void progress_init_home(void);
void progress_init(int);
void progress_extend(void);
int  progress_extended(void);

int  progress_play(struct level *);
void progress_buy_balls(int);
void progress_step(void);
void progress_stat(int status);
void progress_stop(void);
void progress_exit(void);

#if NB_HAVE_PB_BOTH==1
int  progress_raise_gems(int, int, int *, int *, int *, int *);
#endif
int  progress_same_avail(void);
int  progress_next_avail(void);

/*
 * Please disable all checkpoints,
 * before you attempt to restart the level.
 */
int  progress_same(void);

/*
 * Please disable all checkpoints,
 * before you attempt to restart the level.
 */
int  progress_next(void);

void progress_rename(int);

/*
 * This replay function init will be replaced into the progress_replay_full.
 * Your functions will be replaced using seven parameters.
 */
_CRT_NB_UTIL_DEPRECATED(int, (const char *), progress_replay, progress_replay_full);

int  progress_replay_full(const char *filename,
                          int *g, int *m,
                          int *b, int *s, int *tt,
                          int mus);

int  progress_dead(void);
int  progress_done(void);
int  progress_last(void);

int  progress_lvl_high(void);
int  progress_set_high(void);

struct level *curr_level(void);

float curr_speed_percent(void);

int  curr_balls(void);
int  curr_score(void);
int  curr_times(void);
int  curr_mode (void);
int  curr_goal (void);

int  progress_time_rank(void);
int  progress_goal_rank(void);
int  progress_coin_rank(void);

int  progress_times_rank(void);
int  progress_score_rank(void);

int  progress_reward_ball(int);

#if ENABLE_RFD==1
int  progress_rfd_take_powerup(int);
int  progress_rfd_get_powerup (int);
#endif

/*---------------------------------------------------------------------------*/

enum
{
    MODE_NONE = 0,

    MODE_CHALLENGE = 1,
    MODE_NORMAL = 2,
    MODE_STANDALONE = 3,
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    MODE_HARDCORE = 4,
#endif
    MODE_ZEN = 5,
    MODE_BOOST_RUSH = 6,
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    MODE_CAMPAIGN = 7,
#endif

    MODE_MAX
};

const char *mode_to_str(int, int);

/*---------------------------------------------------------------------------*/

#endif
