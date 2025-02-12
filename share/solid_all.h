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

#ifndef SOLID_ALL_H
#define SOLID_ALL_H

#include "solid_vary.h"

#ifdef MAPC_INCLUDES_CHKP
extern int last_path_enabled_orcondition[1024];
#endif
extern int curr_path_enabled_orcondition[1024];

typedef void (*cmd_fn)(const union cmd *);

void sol_body_p(float p[3],
                const struct s_vary *,
                int mi,
                float);
void sol_body_v(float v[3],
                const struct s_vary *,
                int mi,
                float);
void sol_body_e(float e[4],
                const struct s_vary *,
                int mi,
                float);
int  sol_body_w(const struct s_vary *,
                int mi);

void sol_entity_p(float dest_p[3],
                  const struct s_vary *vary,
                  int mi, int mj);
void sol_entity_e(float e[4],
                  const struct s_vary *vary,
                  int mi, int mj);

void sol_entity_world(float w[3],
                      const struct s_vary *vary,
                      int mi,int mj,
                      const float v[3]);
void sol_entity_local(float w[3],
                      const struct s_vary *vary,
                      int mi,int mj,
                      const float v[3]);

void sol_rotate(float e[3][3], const float w[3], float dt);

void sol_pendulum(struct v_ball *up,
                  const float a[3],
                  const float g[3], float dt);

void sol_swch_step(struct s_vary *, cmd_fn, float dt, int ms);
void sol_move_step(struct s_vary *, cmd_fn, float dt, int ms);
void sol_ball_step(struct s_vary *, cmd_fn, float dt);

enum
{
    JUMP_OUTSIDE = 0,
    JUMP_INSIDE,
    JUMP_TOUCH
};

enum
{
    SWCH_OUTSIDE = 0,
    SWCH_INSIDE,
    SWCH_TOUCH
};

enum
{
    CHKP_OUTSIDE = 0,
    CHKP_INSIDE,
    CHKP_TOUCH
};

int            sol_item_test(struct s_vary *, float *p, float item_r);
struct b_goal *sol_goal_test(struct s_vary *, float *p, int ui);
int            sol_jump_test(struct s_vary *, float *p, int ui);
int            sol_swch_test(struct s_vary *, cmd_fn, int ui);
#ifdef MAPC_INCLUDES_CHKP
int            sol_chkp_test(struct s_vary *, cmd_fn, int ui, int *ci);
#endif

#endif
