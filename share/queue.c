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

#include <stdlib.h>
#ifndef NDEBUG
#include <assert.h>
#endif

#include "queue.h"
#include "list.h"

#if _DEBUG && _MSC_VER
#ifndef _CRTDBG_MAP_ALLOC
#pragma message(__FILE__": Missing _CRT_MAP_ALLOC, recreate: _CRTDBG_MAP_ALLOC + crtdbg.h")
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

struct queue
{
    List head;
    List tail;
};

Queue queue_new(void)
{
    Queue new;

    if ((new = malloc(sizeof (*new))))
        new->head = new->tail = list_cons(NULL, NULL);

    return new;
}

void queue_free(Queue q)
{
#ifndef NDEBUG
    assert(queue_empty(q));
#endif

    if (queue_empty(q))
    {
        free(q->head); q->head = NULL;
        free(q);
    }
}

int queue_empty(Queue q)
{
#ifndef NDEBUG
    assert(q);
#endif
    return q ? q->head == q->tail : 0;
}

void queue_enq(Queue q, void *data)
{
#ifndef NDEBUG
    assert(q);
#endif

    if (q)
    {
        q->tail->data = data;
        q->tail->next = list_cons(NULL, NULL);

        q->tail = q->tail->next;
    }
}

void *queue_deq(Queue q)
{
    void *data = NULL;

    if (!queue_empty(q))
    {
        data    = q->head->data;
        q->head = list_rest(q->head);
    }

    return data;
}
