/*
 * Copyright (C) 2023 Microsoft / Neverball authors
 *
 * PENNYBALL is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */

#ifndef COMMON_H
#define COMMON_H

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS && !_MSC_VER
#include <sec_api/stdlib_s.h>
#endif

#if _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "fs.h"

/* Neverball Platform API */

#define PENNYBALL_PC_FAMILY_API 0
#define PENNYBALL_XBOX_FAMILY_API 1
#define PENNYBALL_XBOX_360_FAMILY_API 2
#define PENNYBALL_PS_FAMILY_API 3
#define PENNYBALL_STEAMDECK_FAMILY_API 4
#define PENNYBALL_SWITCH_FAMILY_API 5
#define PENNYBALL_HANDSET_FAMILY_API 6

#if _WIN32
//#define PENNYBALL_FAMILY_API PENNYBALL_PC_FAMILY_API
#endif

/* Random stuff. */

#if !defined(MAX_STR_BLOCKREASON)
#define MAX_STR_BLOCKREASON 256
#endif
#ifndef MAXSTR
#define MAXSTR MAX_STR_BLOCKREASON
#endif

#ifdef __GNUC__
#define NULL_TERMINATED __attribute__ ((__sentinel__))
#else
#define NULL_TERMINATED
#endif

/* Math. */

#ifdef __min
#define MIN __min
#else
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifdef __max
#define MAX __max
#else
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#define CLAMP(a, b, c) MIN(MAX(a, b), c)

#define SIGN(n) ((n) < 0 ? -1 : ((n) ? +1 : 0))
#define ROUND(f) ((int) ((f) + 0.5f * SIGN(f)))

#define TIME_TO_MS(t) ROUND((t) * 1000.0f)
#define MS_TO_TIME(m) ((m) * 0.001f)

int rand_between(int low, int high);

/* Arrays and strings. */

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof (a) / sizeof ((a)[0]))
#endif

#ifndef MAXSTRLEN
#define MAXSTRLEN(a) (sizeof (a) - 1)
#endif

#if UNICODE
/**
 * Copy a wstring SRC into a zero-terminated fixed-size array of char DST.
 */
#define WSAFECPY(dst, src) do { \
    size_t _len = wcslen(src); \
    size_t _max = MIN(sizeof (dst) - 1, _len); \
    memcpy((dst), (src), _max); \
    (dst)[_max] = 0; \
} while (0)

/**
 * Append a wstring SRC to a zero-terminated fixed-size array of char DST.
 */
#define WSAFECAT(dst, src) do { \
    size_t _len = wcslen(dst); \
    size_t _max = MIN(sizeof (dst) - 1u - _len, wcslen(src)); \
    memcpy((dst) + _len, (src), _max); \
    (dst)[_len + _max] = 0; \
} while (0)
#endif

/**
 * Copy a string SRC into a zero-terminated fixed-size array of char DST.
 */
#define SAFECPY(dst, src) do { \
    size_t _len = strlen(src); \
    size_t _max = MIN(sizeof (dst) - 1, _len); \
    memcpy((dst), (src), _max); \
    (dst)[_max] = 0; \
} while (0)

/**
 * Append a string SRC to a zero-terminated fixed-size array of char DST.
 */
#define SAFECAT(dst, src) do { \
    size_t _len = strlen(dst); \
    size_t _max = MIN(sizeof (dst) - 1u - _len, strlen(src)); \
    memcpy((dst) + _len, (src), _max); \
    (dst)[_len + _max] = 0; \
} while (0)

#if UNICODE
wchar_t *wcsip_newline(wchar_t *);
#if _MSC_VER && !_NONSTDC
#define dupe_wstring _wcsdup
#else
wchar_t *dupe_wstring(const wchar_t *);
#endif
#endif

int   read_line(char **, fs_file);
char *strip_newline(char *);
#if _MSC_VER && !_NONSTDC
#define dupe_string _strdup
#else
char *dupe_string(const char *);
#endif
char *concat_string(const char *first, ...) NULL_TERMINATED;

#ifdef strdup
#undef strdup
#endif
#if _MSC_VER && !_NONSTDC
#define strdup _strdup
#else 
#define strdup dupe_string
#endif
#if UNICODE
#ifdef wcsdup
#undef wcsdup
#endif
#if _MSC_VER && !_NONSTDC
#define wcsdup _wcsdup
#else 
#define wcsdup dupe_wstring
#endif
#endif

#if UNICODE
#define wcs_starts_with(s, h) \
    (wcsncmp((s), (h), wcslen(h)) == 0)
#define wcs_ends_with(s, t) \
    ((wcslen(s) >= wcslen(t)) && wcscmp((s) + wcslen(s) - wcslen(t), (t)) == 0)
#endif
#define str_starts_with(s, h) \
    (strncmp((s), (h), strlen(h)) == 0)
#define str_ends_with(s, t) \
    ((strlen(s) >= strlen(t)) && strcmp((s) + strlen(s) - strlen(t), (t)) == 0)

/*
 * Declaring vsnprintf with the C99 signature, even though we're
 * claiming to be ANSI C. This is probably bad but is not known to not
 * work.
 */
/*#ifndef __APPLE__
extern int vsnprintf(char *, size_t, const char *, va_list);
#endif*/

/* Time. */

time_t make_time_from_utc(struct tm *);
const char *date_to_str(time_t);

/* Files. */

int  file_exists(const char *);
int  file_rename(const char *, const char *);
#ifndef FS_VERSION_1
int  file_size(const char *path);
#endif
void file_copy(FILE *fin, FILE *fout);

/* Paths. */

int path_is_sep(int);
int path_is_abs(const char *);

char *path_join(const char *, const char *);
char *path_normalize(char *);

const char *path_last_sep(const char *);
const char *path_next_sep(const char *);

const char *base_name(const char *name);
const char *base_name_sans(const char *name, const char *suffix);
const char *dir_name(const char *name);

/* Environment */

int set_env_var(const char *, const char *);

#endif
