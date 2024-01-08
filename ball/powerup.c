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

#include "powerup.h"

/*---------------------------------------------------------------------------*/

static float coinmultiply = 1.0f;
static float gravmultiply = 1.0f;
static float tiltmultiply = 1.0f;

void powerup_stop(void)
{
    coinmultiply = 1;
    gravmultiply = 1.0f;
    tiltmultiply = 1;
}

void init_earninator(void)
{
    /* If you use earninator, set the Coin Multiply to 2 */
    coinmultiply = 2;
}

void init_floatifier(void)
{
    /* If you use floatifier, reduce the Gravity to 50% */
    gravmultiply = 0.5f;
}

void init_speedifier(void)
{
    /* If you use speedifier, set the Tilt Multiply to 2 */
    tiltmultiply = 2;
}

float get_coin_multiply(void)
{
    return coinmultiply;
}

float get_grav_multiply(void)
{
    return gravmultiply;
}

float get_tilt_multiply(void)
{
    return tiltmultiply;
}
