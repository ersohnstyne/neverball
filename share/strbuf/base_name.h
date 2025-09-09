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

#ifndef BASE_NAME_H
#define BASE_NAME_H 1

#include "strbuf.h"
#include "common.h"

STRBUF_WRAP(base_name)

/*
 * Allocate a fixed-size buffer on the stack and fill it with the base name of
 * the given path.
 */
#define BASE_NAME(name) (base_name_strbuf((name)).buf)

#endif