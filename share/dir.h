/*
 * Copyright (C) 2023 Microsoft / Neverball authors
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

#ifndef DIR_H
#define DIR_H

#include "array.h"
#include "list.h"

struct dir_item
{
    const char *path;
    void *data;
};

/*
 * A minor convenience for quick member access, as in "DIR_ITEM_GET(a,
 * i)->member".  Most of the time this macro is not what you want to
 * use.
 */
#define DIR_ITEM_GET(a, i) ((struct dir_item *) array_get((a), (i)))

Array dir_scan(const char *,
               int  (*filter)    (struct dir_item *),
               List (*list_files)(const char *),
               void (*free_files)(List));
void  dir_free(Array);

List dir_list_files(const char *);
void dir_list_free (List);

int dir_exists(const char *);

#if _WIN32 && _MSC_VER
// 1 - 0: Directory created
// 0 - 1: Directory already exists
// 0 - 0 or 1 - 1

#include <Windows.h>
#define dir_make(path) \
    !(CreateDirectoryA(path, 0) != 0 || GetLastError() == ERROR_ALREADY_EXISTS)
#elif _WIN32 && __MINGW32__
#include <direct.h>
#define dir_make(path) _mkdir(path)
#else
#include <sys/stat.h>
#define dir_make(path) mkdir(path, 0777)
#endif

#endif
