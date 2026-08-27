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

#include <stdlib.h>
#ifndef NDEBUG
#include <assert.h>
#elif defined(_MSC_VER) && defined(_AFXDLL)
#include <afx.h>
/**
 * HACK: assert() for Microsoft Windows Apps in Release builds
 * will be replaced to VERIFY() - Ersohn Styne
 */
#define assert VERIFY
#else
#define assert(_x) (_x)
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

<<<<<<< HEAD
    if ((new = malloc(sizeof(*new))))
=======
    if ((new = malloc(sizeof (*new))))
>>>>>>> 329a96e40fcc28c8f6d30d3fcc0d2ca03914ee27
    {
        if (!(new->head = new->tail = list_cons(NULL, NULL)))
        {
            free(new);
            return NULL;
        }
    }

    return new;
}

void queue_free(Queue q)
{
    if (!q)
        return;

<<<<<<< HEAD
#ifndef NDEBUG
=======
>>>>>>> 329a96e40fcc28c8f6d30d3fcc0d2ca03914ee27
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
    if (!q || !q->head)
        return 1;
<<<<<<< HEAD
    return q ? q->head == q->tail : 0;
=======
    return q->head == q->tail;
>>>>>>> 329a96e40fcc28c8f6d30d3fcc0d2ca03914ee27
}

void queue_enq(Queue q, void *data)
{
    List next_node;

    if (!q || !q->tail)
        return;

    if ((next_node = list_cons(NULL, NULL)))
    {
        q->tail->data = data;
        q->tail->next = next_node;
        q->tail = next_node;
    }
}

void *queue_deq(Queue q)
{
    void *data = NULL;

<<<<<<< HEAD
    if (!queue_empty(q) && !q->head)
=======
    if (!queue_empty(q) && q->head)
>>>>>>> 329a96e40fcc28c8f6d30d3fcc0d2ca03914ee27
    {
        data    = q->head->data;
        q->head = list_rest(q->head);
    }

    return data;
}
