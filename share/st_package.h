/*
 * Copyright (C) 2026 Microsoft / Neverball authors / Jānis Rūcis
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

#ifndef ST_PACKAGE
#define ST_PACKAGE 1

#include "state.h"

/* === PREFIX/SUFFIX CONVERTER === */

/* This suffix variable name "*_package" will be replaced into "*_addons". */
#define st_package st_addons

/* This suffix function name "*_package" will be replaced into "*_addons". */
#define goto_package goto_addons

/* This prefix function name "package_*" will be replaced into "addons_*". */
#define package_set_installed_action addons_set_installed_action

/* === END PREFIX/SUFFIX CONVERTER === */

extern struct state st_package;

void goto_package(int, struct state *);
void package_set_installed_action(int (*installed_action_fn)(int pi));

#endif