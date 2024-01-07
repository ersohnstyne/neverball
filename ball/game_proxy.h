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

#ifndef GAME_PROXY_H
#define GAME_PROXY_H

#include "cmd.h"

void       game_proxy_filter(int (*fn)(const union cmd *));
void       game_proxy_enq(const union cmd *);
union cmd *game_proxy_deq(void);
void       game_proxy_clr(void);

#endif
