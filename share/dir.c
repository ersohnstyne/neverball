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

#if _MSC_VER
#include <Windows.h>
#else
#include <dirent.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "dir.h"
#include "common.h"

/*
 * HACK: Thank god using FindFirstFileA by Microsoft Elite Developers!
 * On Unix and linux, or using MinGW, include headers will be used as: dirent.h
 * - Ersohn Styne
 */

#if _MSC_VER
#pragma message("Using directory list for code compilation: Microsoft Visual Studio")
#else
#pragma message("Using directory list for code compilation: MinGW")
#endif

/*
 * Enumerate files in a system directory. Returns a List of allocated filenames.
 */
List dir_list_files(const char *path)
{
#if __GNUC__
    DIR *dir;
#endif

    List files = NULL;

#if _WIN32 && _MSC_VER
    WIN32_FIND_DATAA findDataFiles;

    char outpath[MAXSTR];
    sprintf_s(outpath, 256U, "%s\\*", path);

    HANDLE hFind = FindFirstFileA(outpath, &findDataFiles);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        if (strcmp(findDataFiles.cFileName, ".") == 0 || strcmp(findDataFiles.cFileName, "..") == 0) {}
        else if (findDataFiles.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ||
            findDataFiles.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
            files = list_cons(strdup(findDataFiles.cFileName), files);

        while (FindNextFileA(hFind, &findDataFiles))
        {
            if (strcmp(findDataFiles.cFileName, ".") == 0 || strcmp(findDataFiles.cFileName, "..") == 0) {}
            else if (findDataFiles.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ||
                findDataFiles.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
                files = list_cons(strdup(findDataFiles.cFileName), files);
        }

        FindClose(hFind);
        hFind = 0;
    }
#elif __GNUC__
    if ((dir = opendir(path)))
    {
        struct dirent *ent;

        while ((ent = readdir(dir)))
        {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
                continue;

            files = list_cons(strdup(ent->d_name), files);
        }

        closedir(dir);
    }
#else
#error No implementation for dir_list_files found!
#endif

    return files;
}

/*
 * Free the allocated filenames and the List cells.
 */
void dir_list_free(List files)
{
    while (files)
    {
        free(files->data);
        files = list_rest(files);
    }
}

/*
 * Add a struct dir_item to the given Array.
 */
static struct dir_item *add_item(Array items, const char *dir, const char *name)
{
    struct dir_item *item = array_add(items);

    item->path = path_join(dir, name);
    item->data = NULL;

    return item;
}

/*
 * Remove a struct dir_item from the given array.
 */
static void del_item(Array items)
{
    struct dir_item *item = array_get(items, array_len(items) - 1);

    free((void *) item->path);
    assert(!item->data);

    array_del(items);
}

/*
 * Enumerate files in a directory. Returns an Array of struct dir_item.
 *
 * By passing `list_files`, an arbitrary list may be turned into an Array
 * of struct dir_item. By passing `filter`, any struct dir_item may be
 * excluded from the final Array.
 */
Array dir_scan(const char *path,
               int  (*filter)    (struct dir_item *),
               List (*list_files)(const char *),
               void (*free_files)(List))
{
    List files, file;
    Array items = NULL;

    assert((list_files && free_files) || (!list_files && !free_files));

    if (!list_files) list_files = dir_list_files;
    if (!free_files) free_files = dir_list_free;

    items = array_new(sizeof (struct dir_item));

    if ((files = list_files(path)))
    {
        for (file = files; file; file = file->next)
        {
            struct dir_item *item;

            item = add_item(items, path, file->data);

            if (filter && !filter(item))
                del_item(items);
        }

        free_files(files);
    }

    return items;
}

/*
 * Free the Array of struct dir_item.
 */
void dir_free(Array items)
{
    while (array_len(items))
        del_item(items);

    array_free(items);
}

/*
 * Test existence of a system directory.
 */
int dir_exists(const char *path)
{
#if _WIN32 && _MSC_VER
    DWORD file_attr = GetFileAttributesA(path);

    if (file_attr & FILE_ATTRIBUTE_OFFLINE
        || file_attr & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED
        || file_attr & FILE_ATTRIBUTE_NO_SCRUB_DATA)
        return 0;

    return file_attr & FILE_ATTRIBUTE_DIRECTORY
        || file_attr & FILE_ATTRIBUTE_ARCHIVE;
#elif __GNUC__
    DIR *dir;

    if ((dir = opendir(path)))
    {
        closedir(dir);
        return 1;
    }
    return 0;
#else
#error No implementation for dir_exists found!
#endif
}
