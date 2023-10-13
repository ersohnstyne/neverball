/*
 * Copyright (C) 2023 Microsoft / Neverball authors
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

#ifndef BOOST_RUSH_H
#define BOOST_RUSH_H

void boost_rush_init(void);
void boost_rush_stop(void);
void collect_coin_value(int);
void increase_speed(void);
void substract_speed(void);
void boost_rush_timer(float);

float get_speed_indicator(void);
int is_sonic(void);

#endif
