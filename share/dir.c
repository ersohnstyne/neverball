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

#if _MSC_VER
#define NOMINMAX
#include <Windows.h> /* FindClose(), FindFirstFile(), FindNextFile() */
#else
#include <dirent.h> /* opendir(), readdir(), closedir() */
#endif

#include <string.h>
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

#include <sys/stat.h>

#include "dir.h"
#include "common.h"

/*
 * HACK: Thank god using FindFirstFileA by Microsoft Elite Developers!
 * On Unix and linux, or using MinGW, include headers will be used as: dirent.h
 * - Ersohn Styne
 */

#if ENABLE_OPENDRIVEAPI!=0
/*
 * https://gitea.stynegame.de/StyneGameHamburg/opendrivepi
 */
#include <opendriveapi.h>
#elif _WIN32
#if !defined(_MSC_VER)
#error Security compilation error: This was already done with FindClose, \
       FindFirstFileA, FindNextFile and GetFileAttributesA or using OpenDriveAPI. \
       Install Visual Studio 2022 Community or later version to build it there. \
       Or download the OpenDriveAPI project: \
       https://gitea.stynegame.de/StyneGameHamburg/opendrivepi
#else
#pragma message(__FILE__ ": Using code compilation: Microsoft Visual Studio")
#endif
#else
#pragma message(__FILE__ ": Using code compilation: GCC + G++")
#endif

/*
 * Enumerate files in a system directory. Returns a List of allocated filenames.
 */
List dir_list_files(const char *path)
{
    List files = NULL;

#if ENABLE_OPENDRIVEAPI!=0
    OpenDriveAPI_PDIR          dir;
    struct OpenDriveAPI_DirEnt ent;

    if ((dir = opendriveapi_opendir(path, &ent)))
    {
        while ((opendriveapi_readdir(dir, &ent)) != 0)
        {
            if (strcmp(ent.dir_ent_d_name, ".") == 0 || strcmp(ent.dir_ent_d_name, "..") == 0)
                continue;

            files = list_cons(strdup(ent.dir_ent_d_name), files);
        }

        opendriveapi_closedir(dir);
    }
#elif _WIN32 && _MSC_VER
    char outpath[MAXSTR];
    sprintf_s(outpath, MAXSTR, "%s\\*", path);

    WIN32_FIND_DATAA find_data;
    HANDLE hFind;

    if ((hFind = FindFirstFileA(outpath, &find_data)) != INVALID_HANDLE_VALUE)
    {
        do {
            if (strcmp(find_data.cFileName, ".")  == 0 ||
                strcmp(find_data.cFileName, "..") == 0)
                continue;

            struct stat _buf;
            char tmp_real[256];

            sprintf_s(tmp_real, 256, "%s\\%s", path, find_data.cFileName);

            if (stat(tmp_real, &_buf) == 0)
                files = list_cons(strdup(find_data.cFileName), files);
        }
        while (FindNextFileA(hFind, &find_data));

        FindClose(hFind);
        hFind = 0;
    }
#else
    DIR *dir;
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
        files->data = NULL;
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
#ifndef NDEBUG
    assert(!item->data);
#endif
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

#ifndef NDEBUG
    assert((list_files && free_files) || (!list_files && !free_files));
#endif

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
#if ENABLE_OPENDRIVEAPI!=0
    return opendriveapi_dir_exists(path);
#else
#if _WIN32 && _MSC_VER
    DWORD file_attr = GetFileAttributesA(path);

    if (file_attr & FILE_ATTRIBUTE_OFFLINE             ||
        file_attr & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED ||
        file_attr & FILE_ATTRIBUTE_NO_SCRUB_DATA)
        return 0;

    return file_attr & FILE_ATTRIBUTE_DIRECTORY ? 1 : 0;
#else
    DIR *dir;

    if ((dir = opendir(path)))
    {
        closedir(dir);
        return 1;
    }
    return 0;
#endif
#endif
}
