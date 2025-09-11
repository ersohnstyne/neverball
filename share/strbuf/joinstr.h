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

#ifndef JOINSTR_H
#define JOINSTR_H 1

#include <string.h>
#include "common.h"
#include "strbuf.h"

static inline struct strbuf joinstr(const char *head, const char *tail)
{
    struct strbuf sb = { "" };

    const size_t max_len = sizeof (sb.buf) - 1u;
    const size_t head_len = head ? MIN(strlen(head), max_len) : 0u;
    const size_t tail_len = tail ? MIN(strlen(tail), max_len - head_len) : 0u;

    if (head_len > 0)
        memcpy(sb.buf, head, head_len);

    if (tail_len > 0)
        memcpy(sb.buf + head_len, tail, tail_len);

    sb.buf[head_len + tail_len] = 0;

    return sb;
}

/*
 * Allocate a fixed-size buffer on the stack and join two strings into it.
 */
#define JOINSTR(head, tail) (joinstr((head), (tail)).buf)

#endif