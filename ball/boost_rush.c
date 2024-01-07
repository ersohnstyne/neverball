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

#include "boost_rush.h"
#include "hud.h"
#include "progress.h"

/*---------------------------------------------------------------------------*/

static int   need_coins;
static float curr_speed;
static float sonic_timer;

void boost_rush_init(void)
{
    need_coins  = 0;
    curr_speed  = 0.f;
    sonic_timer = 0.f;
}

void boost_rush_stop(void)
{
    curr_speed  = 0.f;
    sonic_timer = 0.f;
}

void collect_coin_value(int coin_val)
{
    need_coins = need_coins + coin_val;

    while (need_coins > 9)
    {
        if (curr_speed >= 100.f)
        {
            /* Exceed some speed for 5 seconds */

            sonic_timer += 5.f;
            need_coins  -= 10;
            return;
        }

        /* Increase the speed */

        increase_speed();
        hud_speedup_pulse();
        need_coins -= 10;
    }
}

void increase_speed(void)
{
    curr_speed += 14.28571429f;
    
    if (curr_speed >= 100.f)
        curr_speed = 100.f;
}

void substract_speed(void)
{
    curr_speed  /= 2.f;
    sonic_timer  = 0;
}

void boost_rush_timer(float dt)
{
    if (sonic_timer > 0)
        sonic_timer += dt;
}

float get_speed_indicator(void)
{
    return curr_speed;
}

int is_sonic(void)
{
    return (sonic_timer > 0);
}
