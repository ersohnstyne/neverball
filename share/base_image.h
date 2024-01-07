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

#ifndef BASE_IMAGE_H
#define BASE_IMAGE_H

/*---------------------------------------------------------------------------*/

void  image_size(int *, int *, int, int);
void  image_near2(int *, int *, int, int);

void *image_load(const char *, int *, int *, int *);

void *image_next2(const void *, int, int, int, int *, int *);
void *image_scale(const void *, int, int, int, int *, int *, int);
void  image_white(      void *, int, int, int);
void *image_flip (const void *, int, int, int, int, int);

/*---------------------------------------------------------------------------*/

#endif
