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

#ifndef ACTIVITY_SERVICES_H
#define ACTIVITY_SERVICES_H

enum activity_services_group
{
    AS_GROUP_NONE = 0,

    AS_GROUP_CAMPAIGN,
    AS_GROUP_LEVELSET,

    AS_GROUP_MAX
};

enum activity_services_mode
{
    AS_MODE_NONE       = 0,

    AS_MODE_CHALLENGE  = 1,              /* Challenge Mode  */
    AS_MODE_NORMAL     = 2,              /* Classic Mode    */
    AS_MODE_STANDALONE = 3,              /* Standalone Mode */
//#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    AS_MODE_HARDCORE   = 4,              /* Hardcore Mode   */
//#endif
    AS_MODE_ZEN        = 5,              /* Zen Mode        */
    AS_MODE_BOOST_RUSH = 6,              /* Boost Rush Mode */
//#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    AS_MODE_CAMPAIGN   = 7,              /* Campaign Mode   */
//#endif

    AS_MODE_MAX
};

enum activity_services_status_code
{
    AS_GAME_STATCODE_NONE = 0,

    AS_GAME_STATCODE_G,
    AS_GAME_STATCODE_XT,
    AS_GAME_STATCODE_XF,

    AS_GAME_STATCODE_MAX
};

enum activity_services_powerup
{
    AS_POWERUP_NONE = 0,

    AS_POWERUP_EARNINATOR,
    AS_POWERUP_FLOATIFIER,
    AS_POWERUP_SPEEDIFIER,

    AS_POWERUP_MAX
};

int  activity_services_init(void);
void activity_services_quit(void);

void activity_services_step(const float);

void activity_services_partyid       (const char *);
void activity_services_group_update  (const enum activity_services_group);
void activity_services_mode_update   (const enum activity_services_mode);
void activity_services_setname_update(const char *);
void activity_services_level_update  (const int);
void activity_services_status_update (const int);
void activity_services_powerup_update(const int);

void activity_services_event_overlay_lock(const int);

#endif
