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

#ifndef STRBUF_H
#define STRBUF_H 1

#include <string.h>
#include "common.h"

struct strbuf
{
    char buf[128];
};

#define STRBUF_WRAP(fn) \
    inline struct strbuf fn ## _strbuf(const char *input) \
    { \
        struct strbuf sb = { "" }; \
        const char *output = fn(input); \
        if (output) \
        { \
            const size_t len = MIN(strlen(output), sizeof (sb.buf) - 1u); \
            memcpy(sb.buf, output, len); \
            sb.buf[len] = 0; \
        } \
        return sb; \
    }

inline struct strbuf strbuf(const char *input)
{
    struct strbuf sb = { "" };
    const size_t len = MIN(strlen(input), sizeof (sb.buf) - 1u);
    memcpy(sb.buf, input, len);
    sb.buf[len] = 0;
    return sb;
}

#define STR(sb) ((sb).buf)

#endif
