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

#if ENABLE_FETCH!=1

#include "fetch.h"

void fetch_init(void (*dispatch_event) (void *))
{
}

void fetch_reinit(void)
{
}

void fetch_handle_event(void *data)
{
}

void fetch_quit(void)
{
}

unsigned int fetch_url(const char *url, const char *dst, struct fetch_callback callback)
{
    return 0;
}

#endif
