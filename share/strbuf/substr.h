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

#ifndef SUBSTR_H
#define SUBSTR_H 1

#include <string.h>
#include "common.h"
#include "strbuf.h"

<<<<<<< HEAD
#if _DEBUG && _MSC_VER
#ifndef _CRTDBG_MAP_ALLOC
#pragma message(__FILE__": Missing _CRT_MAP_ALLOC, recreate: _CRTDBG_MAP_ALLOC + crtdbg.h")
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

=======
>>>>>>> e5b432cba6c61866704c6ea7bbcacbc0730edd87
static inline STRBUF substr(const char *str, size_t start, size_t count)
{
    STRBUF sb = { "" };

    if (str)
    {
        const size_t max_start = strlen(str);

        start = MIN(start, max_start);
        count = MIN(count, max_start - start);
        count = MIN(count, sizeof (sb.buf) - 1u);

        if (count > 0)
        {
            memcpy(sb.buf, str + start, count);
            sb.buf[count] = 0;
        }
    }

    return sb;
}

#endif