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

#ifndef PART_H
#define PART_H

#include "solid_draw.h"

/*---------------------------------------------------------------------------*/

#define IMG_PART_STAR     "png/part"
#define IMG_PART_SQUIGGLE "png/squiggle"

#define PART_MAX_COIN  64
#define PART_MAX_GOAL  64
#define PART_MAX_JUMP  64

#define PART_SIZE      0.1f

/*---------------------------------------------------------------------------*/

void part_reset(void);
void part_init(void);
void part_free(void);

void part_burst(const float *, const float *);
void part_step(const float *, float);

void part_draw_coin(struct s_rend *);

void part_lerp_apply(float);

/*---------------------------------------------------------------------------*/

#endif
