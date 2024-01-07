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

#ifndef CAMPAIGN_H
#define CAMPAIGN_H

#if NB_HAVE_PB_BOTH==1
#define LEVELGROUPS_INCLUDES_CAMPAIGN
#endif

#include "base_config.h"
#include "level.h"

#include "solid_vary.h"

#define CAMPAIGN_FILE "campaign.txt"

#define MAXLVL 30

/*---------------------------------------------------------------------------*/

struct campaign_hardcore_mode
{
    int used;
    int norecordings;
    int level_theme;
    int level_number;
    float coordinates[2];
};

struct campaign_medal_data
{
    int unlocks;
    int bronze;
    int silver;
    int gold;

    int curr_rank;
};

struct campaign_cam_box_trigger
{
    int activated;         /* Check if the cam box trigger is activated */
    int inside;            /* Is the ball inside of it                  */

    float positions[3];
    float triggerSize[3];

    int cammode;           /* Uses multiple camera mode                 */
    float campositions[3]; /* Uses camera stationary                    */
    float camdirection;    /* Uses camera direction                     */
};

/*---------------------------------------------------------------------------*/

void campaign_store_hs(void);

/*---------------------------------------------------------------------------*/

int campaign_score_update(int, int, int *, int *);
void campaign_rename_player(int, const char *);
int campaign_rank(void);

/*---------------------------------------------------------------------------*/

int campaign_find(const char *);
int campaign_level_difficulty(void);
void campaign_upgrade_difficulty(int);

int campaign_used(void);
int campaign_theme_used(void);

int campaign_load(const char *);
int campaign_init(void);
void campaign_quit(void);
void campaign_theme_init(void);
void campaign_theme_quit(void);

/*---------------------------------------------------------------------------*/

struct level *campaign_get_level(int i);

int campaign_career_unlocked(void);

int campaign_hardcore(void);
void campaign_hardcore_nextlevel(void);
int campaign_hardcore_norecordings(void);
int campaign_hardcore_unlocked(void);
void campaign_hardcore_set_coordinates(float, float);
void campaign_hardcore_play(int);
void campaign_hardcore_quit(void);
struct campaign_hardcore_mode campaign_get_hardcore_data(void);

/*---------------------------------------------------------------------------*/

int campaign_load_camera_box_trigger(const char *levelname);
void campaign_reset_camera_box_trigger(void);
struct campaign_cam_box_trigger campaign_get_camera_box_trigger(int index);
int campaign_camera_box_trigger_count(void);
int campaign_camera_box_trigger_test(struct s_vary *vary, int ui);

/*---------------------------------------------------------------------------*/

#endif