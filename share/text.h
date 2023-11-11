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

#ifndef TEXT_H
#define TEXT_H

#if _WIN32 && __MINGW32__
#include <SDL3/SDL.h>
#else
#include <SDL.h>
#endif

#ifndef MAXSTR
#define MAXSTR MAX_STR_BLOCKREASON
#endif

/*---------------------------------------------------------------------------*/

int text_add_char(Uint32, char *, int);
int text_del_char(char *);
int text_length(const char *);

/*---------------------------------------------------------------------------*/

extern char text_input[MAXSTR];

void text_input_start(void (*cb)(int typing));
void text_input_stop(void);
int  text_input_str(const char *, int typing);
int  text_input_char(int);
int  text_input_del(void);

#endif
