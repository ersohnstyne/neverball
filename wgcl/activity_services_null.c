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

#include "activity_services.h"

#if ENABLE_ACTIVITY_SERVICES==0

/*---------------------------------------------------------------------------*/

int activity_services_init(void)
{
    return 1;
}

void activity_services_quit(void)
{
}

void activity_services_step(const float dt)
{
}

void activity_services_partyid(const char *uuid4)
{
}

void activity_services_group_update(const enum activity_services_group g)
{
}

void activity_services_mode_update(const enum activity_services_mode m)
{
}

void activity_services_setname_update(const char *name)
{
}

void activity_services_level_update(const int b)
{
}

void activity_services_status_update(const int s)
{
}

void activity_services_powerup_update(const int t)
{
}

void activity_services_event_overlay_lock(const int locked)
{
}

#endif
