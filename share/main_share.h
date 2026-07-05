/*
 * Copyright (C) 2026 Microsoft / Neverball authors / Ersohn Styne
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

#ifndef MAIN_SHARE_H
#define MAIN_SHARE_H

/**
 * Used for linux or win32 cli main entry function point
 */
int main_share(int, char **);

#ifdef __EMSCRIPTEN__

//void WGCL_CallMain(const char *);
void WGCL_CallMain(const int);

#endif

#endif
