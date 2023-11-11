/*
 * Copyright (C) 2023 Microsoft / Neverball authors
 *
 * PENNYBALL is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */

#ifndef BALL_H
#define BALL_H

#include "solid_draw.h"

/*---------------------------------------------------------------------------*/

#define BALL_FUDGE 0.001f

#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_MULTIBALLS)
void ball_multi_init(void);
void ball_multi_free(void);

void ball_multi_step(float);

void ball_multi_equip(int);
int  ball_multi_curr(void);
#endif

#if NB_HAVE_PB_BOTH!=1 || !defined(CONFIG_INCLUDES_MULTIBALLS)
void ball_init(void);
void ball_free(void);

void ball_step(float);
#endif

void ball_draw(struct s_rend *,
               const float *,
               const float *,
               const float *, float);

#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_MULTIBALLS)
void ball_multi_draw(struct s_rend *,
                     const float *,
                     const float *,
                     const float *, float);

void ball_multi_draw_single(int, struct s_rend *,
                            const float *,
                            const float *,
                            const float *, float);
#endif

/*---------------------------------------------------------------------------*/

#endif
