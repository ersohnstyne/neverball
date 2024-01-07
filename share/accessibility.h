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

#ifndef ACCESSIBILITY_H
#define ACCESSIBILITY_H

#include "base_config.h"

extern int accessibility_busy;

extern int ACCESSIBILITY_SLOWDOWN;

void accessibility_init(void);
void accessibility_load(void);
void accessibility_save(void);

void accessibility_set_d(int, int);
void accessibility_tgl_d(int);
int  accessibility_tst_d(int, int);
int  accessibility_get_d(int);

#endif