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

#ifndef STRBUF_H
#define STRBUF_H 1

#include <string.h>
#include "common.h"

struct strbuf
{
    char buf[256];
};

typedef struct strbuf STRBUF;

#define STRBUF_WRAP(fn) \
    static inline STRBUF fn ## _strbuf(const char *input) \
    { \
        STRBUF sb = { "" }; \
        const char *output = fn(input); \
        if (output) \
        { \
            const size_t len = MIN(strlen(output), sizeof (sb.buf) - 1u); \
            memcpy(sb.buf, output, len); \
            sb.buf[len] = 0; \
        } \
        return sb; \
    }

static inline STRBUF strbuf(const char *input)
{
    STRBUF sb = { "" };
    const size_t len = MIN(strlen(input), sizeof (sb.buf) - 1u);
    memcpy(sb.buf, input, len);
    sb.buf[len] = 0;
    return sb;
}

<<<<<<< HEAD
=======
/*
 * Convert a STRBUF to a char pointer.
 * The address-of operator &(sb) forces a compilation error if sb is an rvalue
 * temporary (e.g. returned by value from a function), preventing use-after-scope
 * dangling pointer bugs.
 */
>>>>>>> e5b432cba6c61866704c6ea7bbcacbc0730edd87
#define CSTR(sb) ((void)&(sb), (sb).buf)

#endif
