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

#ifndef IMAGE_H
#define IMAGE_H

#if _WIN32 && __MINGW32__
#include <SDL3/SDL.h>
#include <SDL3/SDL_ttf.h>
#else
#include <SDL.h>
#include <SDL_ttf.h>
#endif

#include "glext.h"
#include "base_image.h"

/*---------------------------------------------------------------------------*/

#define IF_MIPMAP 0x01

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define RMASK 0xFF000000
#define GMASK 0x00FF0000
#define BMASK 0x0000FF00
#define AMASK 0x000000FF
#else
#define RMASK 0x000000FF
#define GMASK 0x0000FF00
#define BMASK 0x00FF0000
#define AMASK 0xFF000000
#endif

extern int donot_allow_mip_and_aniso_during_gui;

void   image_snap(const char *);

GLuint make_image_from_file(const char *, int);
GLuint make_image_from_font(int *, int *,
                            int *, int *, const char *, TTF_Font *, int);
void   size_image_from_font(int *, int *,
                            int *, int *, const char *, TTF_Font *);
GLuint make_texture(const void *, int, int, int, int);

SDL_Surface *load_surface(const char *);

/*---------------------------------------------------------------------------*/

#endif
