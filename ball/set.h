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

#ifndef SET_H
#define SET_H

#include "base_config.h"
#include "level.h"
#include "package.h"

#define SET_FILE "sets.txt"
#define SET_MISC "set-misc.txt"

#define BOOST_FILE "boost-rushes.txt"

#define MAXLVL_SET 150

#define SET_UNLOCKABLE_EDITION 2

/*---------------------------------------------------------------------------*/

int  set_init(int);
void set_quit(void);

/*---------------------------------------------------------------------------*/

int  set_exists(int);
void set_goto(int);
void set_scan_level_files(void);
int  set_find(const char *);

int  curr_set(void);

const char         *set_id(int);
const char         *set_name(int);
const char         *set_name_n(int);
const char         *set_file(int);
const char         *set_desc(int);
const char         *set_shot(int);
#if NB_HAVE_PB_BOTH==1
int                 set_star(int);
int                 set_star_curr(int);
int                 set_star_gained(int);
int                 set_balls_needed(int);
#endif
const struct score *set_score(int, int);

struct level *set_find_level(const char *basename);

#if NB_HAVE_PB_BOTH==1
int  set_star_update  (int);
#endif
int  set_score_update (int, int, int *, int *);
void set_rename_player(int, int, const char *);

void set_store_hs(void);

/*---------------------------------------------------------------------------*/

struct level *get_level(int);

void level_snap(int, const char *);
void set_cheat(void);
void set_detect_bonus_product(void);

/*---------------------------------------------------------------------------*/

#endif
