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

#ifndef HOLE_H
#define HOLE_H

/*---------------------------------------------------------------------------*/

#define MAXHOL 28
#define MAXPLY 5

/*---------------------------------------------------------------------------*/

#define COURSE_SCORE_NOSAVEGAME

/* Scorecard in a single course. */

struct course_score
{
    char course_name [256];

    char player   [MAXPLY][256];
    int  player_id[MAXPLY];

    int  score   [MAXPLY];              /* Total Stroke in session           */
    int  score_hs[MAXPLY];              /* Total Stroke HS                   */

    int  score_high[MAXPLY];
};

extern struct course_score course_score_v;

void  course_score_init_hs(struct course_score *);

void  course_score_update_name(struct course_score *, int, const char *);
void  course_score_insert     (struct course_score *, int);
void  course_score_finish     (struct course_score *);

/*---------------------------------------------------------------------------*/

void  hole_init(const char *);
void  hole_free(void);
int   hole_exists(int);
int   hole_load(int, const char *);

char *hole_player(int);
char *hole_score(int, int);
char *hole_tot(int);
char *hole_out(int);
char *hole_in(int);

void stroke_set_type(int);

int  curr_hole(void);
int  curr_party(void);
int  curr_player(void);
int  curr_stroke(void);
int  curr_count(void);
int  curr_stroke_type(void);

const char *curr_scr_profile(int);
const char *curr_scr(void);
const char *curr_par(void);
const char *curr_stroke_type_name(void);

int  hole_goto(int, int);
int  hole_next(void);
int  hole_move(void);
void hole_goal(void);
void hole_stop(void);
void hole_skip(void);
void hole_fall(int);
int  hole_retry_avail(int);
void hole_retry(int);
void hole_restart(void);

void hole_song(void);

/*---------------------------------------------------------------------------*/

#endif
