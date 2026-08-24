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

#if ENABLE_FETCH!=1

#include "fetch.h"

int fetch_enable(int enable)
{
    return 0;
}

int fetch_init(void)
{
    return 0;
}

int fetch_reinit(void)
{
    return 0;
}

void fetch_handle_event(void *data)
{
}

void fetch_quit(void)
{
}

unsigned int fetch_file(const char *url, const char *dst, struct fetch_callback callback)
{
    return 0;
}

#endif
