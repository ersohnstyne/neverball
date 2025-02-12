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

#ifndef POWERUP_H
#define POWERUP_H

#define ENABLE_POWERUP

void powerup_stop(void);

void powerup_init_earninator(void);
void powerup_init_floatifier(void);
void powerup_init_speedifier(void);

/*---------------------------------------------------------------------------*/

float powerup_get_coin_multiply(void);
float powerup_get_grav_multiply(void);
float powerup_get_tilt_multiply(void);

#endif
