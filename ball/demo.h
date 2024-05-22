/*
 * Copyright (C) 2024 Microsoft / Neverball authors
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

#ifndef DEMO_H
#define DEMO_H

#define DEMO_MAGIC (0xAF | 'N' << 8 | 'B' << 16 | 'R' << 24)

enum
{
    DEMO_VERSION_1_6 = 9,
    DEMO_VERSION_DEV
};

#define DEMO_VERSION_MIN  DEMO_VERSION_1_6
#define DEMO_VERSION_CURR DEMO_VERSION_DEV

#include "cmd.h"

#if NB_HAVE_PB_BOTH==1 && defined(CMD_NBRX)
#define DEMO_VERSION      DEMO_VERSION_CURR
#else
#define DEMO_VERSION      DEMO_VERSION_1_6
#endif

#include <time.h>
#include <stdio.h>

#include "st_intro_covid.h"

#include "level.h"
#include "fs.h"

/*---------------------------------------------------------------------------*/

struct demo
{
    char   path[MAXSTR];                /* Demo path                         */
    char   name[PATHMAX];               /* Demo basename                     */

    char   player[MAXSTR];
    time_t date;

    int    timer;
    int    coins;
    int    status;
    int    mode;

    char   shot[PATHMAX];               /* Image filename                    */
    char   file[PATHMAX];               /* Level filename                    */

    int    time;                        /* Time limit                        */
    int    goal;                        /* Coin limit                        */
    int    score;                       /* Total coins                       */
    int    balls;                       /* Number of balls                   */
    int    times;                       /* Total time                        */

    float  speedpercent;                /* Speed in Percent                  */

};

/*---------------------------------------------------------------------------*/

int  demo_load(struct demo *, const char *);
void demo_free(struct demo *);

int demo_exists(const char *);

const char *demo_format_name(const char *fmt,
                             const char *set,
                             const char *level,
                             int status);

/*---------------------------------------------------------------------------*/

int  demo_play_init(const char *, const struct level *, int, int, int, int, float);
void demo_play_stat(int, int, int);
void demo_play_stop(int);

int  demo_saved (void);
int  demo_rename(const char *);

void demo_rename_player(const char *name, const char *player);

/*---------------------------------------------------------------------------*/

int  demo_replay_init(const char *, int *, int *, int *, int *, int *, float *);
int  demo_replay_step(float);
void demo_replay_stop(int);
float demo_replay_blend(void);

const char *curr_demo(void);

void demo_replay_speed(int);

void demo_replay_manual_speed(float);

/*---------------------------------------------------------------------------*/

extern int demo_requires_update;

extern fs_file demo_fp;

/*---------------------------------------------------------------------------*/

#endif
