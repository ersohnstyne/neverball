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

#ifndef THEME_H
#define THEME_H

#include "base_config.h"
#include "glext.h"

#define THEME_IMAGES_MAX 4

struct theme
{
    GLuint tex[THEME_IMAGES_MAX];

    GLfloat t[4];
    GLfloat s[4];
};

int  theme_load(struct theme *, const char *name);
void theme_free(struct theme *);

#endif
