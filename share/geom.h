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

#ifndef GEOM_H
#define GEOM_H

#include "solid_draw.h"
#include "common.h"

/*---------------------------------------------------------------------------*/

#define IMG_SHAD "png/shadow.png"

#define BACK_DIST   256.0f
#define FAR_DIST    10000.0f /* Default is 512; Setting to 10000 uses Unity! */
#define JUMP_HEIGHT   2.00f
#define SWCH_HEIGHT   2.00f
#define GOAL_HEIGHT   3.00f
#define CHKP_HEIGHT   2.00f
#define ITEM_RADIUS   0.15f
#define GOAL_SPARKS  64

/*---------------------------------------------------------------------------*/

void geom_init(void);
void geom_free(void);
void geom_step(float);

void beam_draw(struct s_rend *, const GLfloat *, const GLfloat *, GLfloat, GLfloat);
void goal_draw(struct s_rend *, const GLfloat *, GLfloat, GLfloat, GLfloat);
void jump_draw(struct s_rend *, const GLfloat *, GLfloat, GLfloat, GLfloat);
void chkp_draw(struct s_rend *, const GLfloat *, GLfloat, GLfloat);
void flag_draw(struct s_rend *, const GLfloat *);
void mark_draw(struct s_rend *);
void vect_draw(struct s_rend *);
void back_draw(struct s_rend *);

void item_color(const struct v_item *, float *);
void item_draw(struct s_rend *, const struct v_item *, const GLfloat *, float);

/*---------------------------------------------------------------------------*/

void back_init(const char *s);
void back_free(void);
void back_draw_easy(void);

/*---------------------------------------------------------------------------*/

void shad_init(void);
void shad_free(void);
void shad_draw_set(void);
void shad_draw_clr(void);

/*---------------------------------------------------------------------------*/

enum
{
    TEX_STAGE_TEXTURE,
    TEX_STAGE_SHADOW,
    TEX_STAGE_CLIP
};

struct tex_stage
{
    GLenum unit;
    int stage;
};

struct tex_env
{
    void (*conf)(int, int);
    size_t count;
    struct tex_stage stages[4];
};

extern const struct tex_env tex_env_default;
extern const struct tex_env tex_env_shadow;
extern const struct tex_env tex_env_shadow_clip;
extern const struct tex_env tex_env_pose;

extern const struct tex_env *curr_tex_env;

void tex_env_select(const struct tex_env *, ...) NULL_TERMINATED;
void tex_env_active(const struct tex_env *);
int  tex_env_stage(int);

/*---------------------------------------------------------------------------*/

void light_reset(void);
void light_conf(void);
void light_load(void);

/*---------------------------------------------------------------------------*/

#endif
