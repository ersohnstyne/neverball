/*
 * Copyright (C) 2026 Microsoft / Neverball authors / Ersohn Styne
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

#ifndef MAPMARKERS_H
#define MAPMARKERS_H

#if NB_HAVE_PB_BOTH==1 && _WIN32 && _MSC_VER

struct mapmarker
{
    char supported;
    char s;

    int px;
    int py;
    int pz;
};

int  mapmarkers_init(void);
void mapmarkers_quit(void);
void mapmarkers_draw(struct s_rend*);

int mapmarkers_load_map(const char*);

#endif

#endif