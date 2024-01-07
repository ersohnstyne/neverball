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

#ifndef POWERUP_H
#define POWERUP_H

#define ENABLE_POWERUP

void powerup_stop(void);

void init_earninator(void);
void init_floatifier(void);
void init_speedifier(void);

/*---------------------------------------------------------------------------*/

float get_coin_multiply(void);
float get_grav_multiply(void);
float get_tilt_multiply(void);

#endif
