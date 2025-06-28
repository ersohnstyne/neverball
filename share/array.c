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

#include <stdlib.h>
#include <string.h>
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

#include "array.h"
#include "common.h"

#if _DEBUG && _MSC_VER
#ifndef _CRTDBG_MAP_ALLOC
#pragma message(__FILE__": Missing _CRT_MAP_ALLOC, recreate: _CRTDBG_MAP_ALLOC + crtdbg.h")
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

/*----------------------------------------------------------------------------*/

void alloc_new(struct alloc *alloc,
               int block,
               void **data,
               int   *count)
{
    memset(alloc, 0, sizeof (*alloc));

    alloc->data  = data;
    alloc->count = count;

    *alloc->data  = NULL;
    *alloc->count = 0;

    alloc->size   = 0;
    alloc->block  = block;
}

void alloc_free(struct alloc *alloc)
{
    if (alloc->data)
    {
        free(*alloc->data);
        *alloc->data = NULL;
    }

    if (alloc->count)
        *alloc->count = 0;
}

void *alloc_add(struct alloc *alloc)
{
    if ((*alloc->count + 1) * alloc->block > alloc->size)
    {
        void *new_data;
        int   new_size = (alloc->size > 0 ?
                          alloc->size * 2 :
                          alloc->block);

        if ((new_data = realloc(*alloc->data, new_size)))
        {
            *alloc->data = new_data;
            alloc->size  = new_size;
        }
        else
            return NULL;
    }

    return (((unsigned char *) *alloc->data) +
            ((*alloc->count)++ * alloc->block));
}

void alloc_del(struct alloc *alloc)
{
    if (*alloc->count > 0)
    {
        if ((*alloc->count - 1) * alloc->block == alloc->size / 4)
            *alloc->data  = realloc(*alloc->data, (alloc->size /= 4));

        (*alloc->count)--;
    }
}

/*----------------------------------------------------------------------------*/

struct array
{
    unsigned char *data;

    int elem_num;
    int elem_len;

    struct alloc alloc;
};

Array array_new(int elem_len)
{
    Array a;

#ifndef NDEBUG
    assert(elem_len > 0);
#endif

    if ((a = malloc(sizeof (*a))))
    {
        a->elem_num = 0;
        a->elem_len = elem_len;

        alloc_new(&a->alloc, MAX(elem_len, 1), (void **) &a->data, &a->elem_num);
    }

    return a;
}

void array_free(Array a)
{
#ifndef NDEBUG
    assert(a);
#endif

    if (a)
    {
        alloc_free(&a->alloc);
        free(a); a = NULL;
    }
}

void *array_add(Array a)
{
#ifndef NDEBUG
    assert(a);
#endif

    return a ? alloc_add(&a->alloc) : NULL;
}

void array_del(Array a)
{
#ifndef NDEBUG
    assert(a);
    assert(a->elem_num > 0);
#endif

    if (a && a->elem_num > 0)
        alloc_del(&a->alloc);
}

void *array_get(Array a, int i)
{
#ifndef NDEBUG
    assert(a);
    assert(i >= 0 && i < a->elem_num);
#endif

    return a && (i >= 0 && i < a->elem_num) ? &a->data[i * a->elem_len] : NULL;
}

void *array_rnd(Array a)
{
#ifndef NDEBUG
    assert(a);
#endif

    return a && a->elem_num ? array_get(a, rand_between(0, a->elem_num - 1)) : NULL;
}

int array_len(Array a)
{
#ifndef NDEBUG
    assert(a);
#endif

    return a ? a->elem_num : 0;
}

void array_sort(Array a, int (*cmp)(const void *, const void *))
{
#ifndef NDEBUG
    assert(a);
#endif

    if (a)
        qsort(a->data, a->elem_num, a->elem_len, cmp);
}

/*----------------------------------------------------------------------------*/
