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

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS && !_MSC_VER
#include <sec_api/stdlib_s.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdarg.h>
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

#if _DEBUG && _MSC_VER
#ifndef _CRTDBG_MAP_ALLOC
#pragma message(__FILE__": Missing _CRT_MAP_ALLOC, recreate: _CRTDBG_MAP_ALLOC + crtdbg.h")
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

#include "common.h"
#include "fs.h"

#if ENABLE_OPENDRIVEAPI!=0 && !defined(__EMSCRIPTEN__)
/*
 * https://gitea.stynegame.de/StyneGameHamburg/opendrivepi
 */
#include <opendriveapi.h>
#elif _WIN32
#if !defined(_MSC_VER)
#error Security compilation error: This was already done with GetFileAttributesA \
       or using OpenDriveAPI. \
       Install Visual Studio 2022 Community or later version to build it there. \
       Or download the OpenDriveAPI project: \
       https://gitea.stynegame.de/StyneGameHamburg/opendrivepi
#else
#pragma message(__FILE__ ": Using code compilation: Microsoft Visual Studio")
#endif
#else
/*
 * Relying on MinGW to provide, that uses from GetFileAttributes() (Windows.h).
 */
#include <sys/stat.h> /* stat() */
#include <unistd.h>   /* access() */

#pragma message(__FILE__ ": Using code compilation: GCC + G++")
#endif

#if _DEBUG && _MSC_VER
#ifndef _CRTDBG_MAP_ALLOC
#pragma message(__FILE__": Missing _CRT_MAP_ALLOC, recreate: _CRTDBG_MAP_ALLOC + crtdbg.h")
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

#if ENABLE_OPENDRIVEAPI!=0
#pragma comment(lib, "opendriveapi_ascii.lib")
#endif

/*---------------------------------------------------------------------------*/

int read_line(char **dst, fs_file fin)
{
    char buff[MAXSTR];

    char *line, *newLine;
    size_t len0, len1;

    line = NULL;

    while (fs_gets(buff, sizeof (buff), fin))
    {
        /* Append to data read so far. */

        if (line)
        {
            newLine = concat_string(line, buff, NULL);
            free(line);
            line = newLine;
        }
        else
            line = strdup(buff);

        /* Strip newline, if any. */

        len0 = strlen(line);
        strip_newline(line);
        len1 = strlen(line);

        if (len1 != len0)
        {
            /* We hit a newline, clean up and break. */
            line = (char *) realloc(line, len1 + 1);
            break;
        }
    }

    return (*dst = line) ? 1 : 0;
}

#if UNICODE
wchar_t *wcsip_newline(wchar_t *wstr)
{
    if (wstr && *wstr)
    {
        wchar_t *p = wstr + wcslen(wstr) - 1;

        while (p >= wstr && (*p == L'\n' || *p == L'\r'))
            *p-- = L'\0';
    }

    return wstr;
}

wchar_t *wcsip_spaces(wchar_t * wstr)
{
    if (wstr && *wstr)
    {
        wchar_t *p = wstr + wcslen(wstr) - 1;

        while (p >= wstr && isspace(*p))
            *p-- = '\0';
    }

    return wstr;
}
#endif

#if !_MSC_VER || _NONSTDC
#if UNICODE
wchar_t *dupe_wstring(const wchar_t *src)
{
    wchar_t *dst = NULL;

    if (src && (dst = (wchar_t *) malloc(wcslen(src) + 1)))
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        wcscpy_s(dst, wcslen(src) + 1, src);
#else
        wcscpy(dst, src);
#endif

    return dst;
}
#endif
#endif

char *strip_newline(char *str)
{
    if (str && *str)
    {
        char *p = str + strlen(str) - 1;

        while (p >= str && (*p == '\n' || *p == '\r'))
            *p-- = '\0';
    }

    return str;
}

char *strip_spaces(char *str)
{
    if (str && *str)
    {
        char *p = str + strlen(str) - 1;

        while (p >= str && isspace(*p))
            *p-- = 0;
    }

    return str;
}

#if !_MSC_VER || _NONSTDC
char *dupe_string(const char *src)
{
    char *dst = NULL;

    if (src && (dst = (char *) malloc(strlen(src) + 1)))
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        strcpy_s(dst, strlen(src) + 1, src);
#else
        strcpy(dst, src);
#endif

    return dst;
}
#endif

char *concat_string(const char *first, ...)
{
    char *full = NULL;

    if ((full = strdup(first)))
    {
        const char *part;
        va_list ap;

        va_start(ap, first);

        while ((part = va_arg(ap, const char *)))
        {
            char *new;

            if (!part) continue;

            if ((new = (char *) realloc(full, strlen(full) + strlen(part) + 1)))
            {
                full = new;
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                strcat_s(full, strlen(full) + strlen(part) + 1, part);
#else
                strcat(full, part);
#endif
            }
            else
            {
                free(full);
                full = NULL;
                break;
            }
        }

        va_end(ap);
    }

    return full;
}

time_t make_time_from_utc(struct tm *tm)
{
    struct tm local, utc;
    time_t t;

    t = mktime(tm);

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    localtime_s(&local, &t);
    gmtime_s   (&utc, &t);
#else
    local = *localtime(&t);
    utc   = *gmtime(&t);
#endif

    local.tm_year += local.tm_year - utc.tm_year;
    local.tm_mon  += local.tm_mon  - utc.tm_mon ;
    local.tm_mday += local.tm_mday - utc.tm_mday;
    local.tm_hour += local.tm_hour - utc.tm_hour;
    local.tm_min  += local.tm_min  - utc.tm_min ;
    local.tm_sec  += local.tm_sec  - utc.tm_sec ;

    return mktime(&local);
}

const char *date_to_str(time_t i)
{
    char str[sizeof ("dd.mm.YYYY HH:MM:SS")];
#if _MSC_VER
    struct tm output_tm;
    localtime_s(&output_tm, &i);
    strftime   (str, sizeof (str), "%d.%m.%Y %H:%M:%S", &output_tm);
#else
    strftime(str, sizeof (str), "%d.%m.%Y %H:%M:%S", localtime(&i));
#endif
    return str;
}

int file_exists(const char *path)
{
#if ENABLE_OPENDRIVEAPI!=0
    return opendriveapi_file_exists(path);
#else
#if _MSC_VER
    DWORD file_attr = GetFileAttributesA(path);

    if (file_attr & FILE_ATTRIBUTE_OFFLINE             ||
        file_attr & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED ||
        file_attr & FILE_ATTRIBUTE_NO_SCRUB_DATA)
        return 0;

    return (file_attr & FILE_ATTRIBUTE_NORMAL   ||
            file_attr & FILE_ATTRIBUTE_ARCHIVE  ||
            file_attr & FILE_ATTRIBUTE_READONLY ||
            file_attr & FILE_ATTRIBUTE_HIDDEN) ? 1 : 0;
#else
    return (access(path, F_OK) == 0);
#endif
#endif
}

int file_rename(const char *src, const char *dst)
{
#if _WIN32
    if (fs_exists(dst))
        remove(dst);
#endif
    return rename(src, dst) == 0;
}

int file_size(const char *path)
{
    struct stat buf;

    if (stat(path, &buf) == 0)
        return (int) buf.st_size;
    else
        return -1;

    return 0;
}

void file_copy(FILE* fin, FILE* fout)
{
    char   buff[MAXSTR];
    size_t size = 0;

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    while (fread_s(buff, size, 1, sizeof (buff), fin) > 0)
#else
    while ((size = fread(buff, 1, sizeof (buff), fin)) > 0)
#endif
        fwrite(buff, 1, size, fout);
}

/*---------------------------------------------------------------------------*/

int path_is_sep(int c)
{
#ifdef _WIN32
    return c == '/' || c == '\\';
#else
    return c == '/';
#endif
}

int path_is_abs(const char *path)
{
    if (path_is_sep(path[0]))
        return 1;

#ifdef _WIN32
    if (isalpha(path[0]) && path[1] == ':' && path_is_sep(path[2]))
        return 1;
#endif

    return 0;
}

char *path_join(const char *head, const char *tail)
{
#ifdef _WIN32
    return (head && *head) ? concat_string(head, "\\", tail, NULL) : strdup(tail);
#else
    return (head && *head) ? concat_string(head, "/", tail, NULL) : strdup(tail);
#endif
}

const char *path_last_sep(const char *path)
{
    const char *sep;

    sep = strrchr(path, '/');

#ifdef _WIN32
    if (!sep)
        sep = strrchr(path, '\\');
    else
    {
        const char *tmp;

        if ((tmp = strrchr(path, '\\')) && sep < tmp)
            sep = tmp;
    }
#endif

    return sep;
}

const char *path_next_sep(const char *path)
{
    size_t skip;

#ifdef _WIN32
    skip = strcspn(path, "/\\");
#else
    skip = strcspn(path, "/");
#endif

    return *(path + skip) ? path + skip : NULL;
}

char *path_normalize(char *path)
{
    char *sep = path;

    while ((sep = (char *) path_next_sep(sep)))
        *sep++ = '/';

    return path;
}

const char *base_name_sans(const char *name, const char *suffix)
{
    static char base[MAXSTR];
    size_t blen, slen;

    if (!name)
        return NULL;
    if (!suffix)
        return base_name(name);

    /* Remove the directory part. */

    SAFECPY(base, base_name(name));

    /* Remove the suffix. */

    blen = strlen(base);
    slen = strlen(suffix);

    if (blen >= slen && strcmp(base + blen - slen, suffix) == 0)
        base[blen - slen] = '\0';

    return base;
}

const char *base_name(const char *name)
{
    static char buff[MAXSTR];

    char *sep;

    if (!name) return name;

    SAFECPY(buff, name);

    /* Remove trailing slashes. */

    while ((sep = (char *) path_last_sep(buff)) && !sep[1])
        *sep = 0;

    return (sep = (char *) path_last_sep(buff)) ? sep + 1 : buff;
}

const char *dir_name(const char *name)
{
    static char buff[MAXSTR];

    if (name && *name)
    {
        char *sep;

        SAFECPY(buff, name);

        /* Remove trailing slashes. */

        while ((sep = (char *) path_last_sep(buff)) && !sep[1])
            *sep = 0;

        if ((sep = (char *) path_last_sep(buff)))
        {
            if (sep == buff)
                return "/";

            *sep = '\0';

            return buff;
        }
    }

    return ".";
}

/*---------------------------------------------------------------------------*/

int rand_between(int low, int high)
{
    return low + rand() / (RAND_MAX / (high - low + 1) + 1);
}

/*---------------------------------------------------------------------------*/

#ifdef _WIN32

/* MinGW hides this from ANSI C. MinGW-w64 doesn't. */
_CRTIMP int _putenv(const char *envstring);

int set_env_var(const char *name, const char *value)
{
    if (name)
    {
        char str[MAXSTR];

        if (value)
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(str, MAXSTR,
#else
            sprintf(str,
#endif
                    "%s=%s", name, value);
        else
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(str, MAXSTR,
#else
            sprintf(str,
#endif
                    "%s=", name);

        return (_putenv(str) == 0);
    }
    return 0;
}

#else

extern int setenv(const char *name, const char *value, int overwrite);
extern int unsetenv(const char *name);

int set_env_var(const char *name, const char *value)
{
    if (name)
    {
        if (value)
            return (setenv(name, value, 1) == 0);
        else
            return (unsetenv(name) == 0);
    }
    return 0;
}

#endif

/*---------------------------------------------------------------------------*/
