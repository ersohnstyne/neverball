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

#ifndef SOLID_SIM_H
#define SOLID_SIM_H

#include "solid_vary.h"
#include "solid_all.h"

/*---------------------------------------------------------------------------*/

#if _WIN32 && _MSC_VER && ENABLE_NVIDIA_PHYSX==1

struct npx_mesh
{
    float vp[3][3];
    float vn[3];
};

struct npx_body
{
    const struct b_body *base;

    int mc;

    struct npx_mesh *mv;
};

struct s_nvpx
{
    struct s_base *base;
    struct s_vary *vary;

    int bc;

    struct npx_body *bv;
};

#endif

/*---------------------------------------------------------------------------*/

void sol_init_sim(struct s_vary *);
void sol_quit_sim(void);

void  sol_move(struct s_vary *, cmd_fn, float);
float sol_step(struct s_vary *, cmd_fn, const float *, float, int, int *);

#if _WIN32 && _MSC_VER && ENABLE_NVIDIA_PHYSX==1
void sol_init_sim_physx(struct s_vary *);
void sol_quit_sim_physx(struct s_vary *);
float sol_step_physx(struct s_vary *, cmd_fn, const float *, float, int);
#endif

/*---------------------------------------------------------------------------*/

#endif
