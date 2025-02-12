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

#ifndef PART_H
#define PART_H

#include "solid_draw.h"

/*---------------------------------------------------------------------------*/

#define IMG_PART_STAR     "png/part"
#define IMG_PART_SQUIGGLE "png/squiggle"

/* FIXME: It gonna being smashed 1GB RAM to be used. */
#define PART_MAX_FILE  1024
#define PART_MAX_COIN  256
#define PART_MAX_GOAL  2048

#define PART_SIZE      0.1f

/*---------------------------------------------------------------------------*/

void part_reset(void);
void part_init(void);
int  part_load(void);
void part_free(void);

void part_burst(const float *, const float *);
void part_goal(const float *);
void part_step(const float *, float);

void part_draw_coin(const struct s_draw *draw, struct s_rend *rend, const float *M, float t);
void part_draw_goal(const struct s_draw *draw, struct s_rend *rend, const float *M, float t);

void part_lerp_apply(float);

/*---------------------------------------------------------------------------*/

#endif
