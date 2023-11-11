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

/*
 * HACK: Remembering the code file differences:
 * Developers  who  programming  C++  can see more bedrock declaration
 * than C.  Developers  who  programming  C  can  see  few  procedural
 * declaration than  C++.  Keep  in  mind  when making  sure that your
 * extern code must associated. The valid file types are *.c and *.cpp,
 * so it's always best when making cross C++ compiler to keep both.
 * - Ersohn Styne
 */

#if __cplusplus
#include <vcruntime_exception.h>
#endif

#include <string.h>
#include <locale.h>
#include <stdlib.h>
#include <stdio.h>
#if !_WIN32
#include <errno.h>
#endif

#if __cplusplus
extern "C" {
#endif
#include "dbg_config.h"
#include "lang.h"
#include "common.h"
#include "config.h"
#include "base_config.h"
#include "fs.h"
#include "log.h"
#if __cplusplus
}
#endif

#if _DEBUG && _MSC_VER
#ifndef _CRTDBG_MAP_ALLOC
#pragma message(__FILE__": Missing CRT-Debugger include header, recreate: crtdbg.h")
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

/*---------------------------------------------------------------------------*/

#if ENABLE_NLS==1
#if NLS_GETTEXT!=1
#error Complete the compiled source code by using -DNLS_GETTEXT=1
#endif
#endif

#if NLS_GETTEXT==1

#if _DEBUG
#pragma comment(lib, "libintl_a_debug.lib")
#else
#pragma comment(lib, "libintl_a.lib")
#endif

#define GT_CODESET "UTF-8"

void gt_init(const char *domain, const char *pref)
{
    static char default_lang[MAXSTR];
    static int  default_lang_init;

    char *dir = strdup(getenv("PENNYBALL_LOCALE"));

    /* Select the location of message catalogs. */

    if (!dir)
    {
        if (path_is_abs(CONFIG_LOCALE))
            dir = strdup(CONFIG_LOCALE);
        else
            dir = concat_string(fs_base_dir(), "/", CONFIG_LOCALE, NULL);
    }

    if (!dir_exists(dir))
    {
        log_errorf("Failure to find locale path: %s - %s\n",
                   dir, errno ? strerror(errno) : "Unknown error");
        free(dir);
        return;
    }

    /* Set up locale. */

#if !_WIN32
    errno = 0;
#endif

    if (!setlocale(LC_ALL, ""))
    {
        log_errorf("Failure to set LC_ALL to native locale (%s)\n",
                   errno ? strerror(errno) : "Unknown error");
    }
    else
    {
        /* The C locale is guaranteed (sort of) to be available. */

        setlocale(LC_NUMERIC, "C");

        /* Tell gettext of our language preference. */

        if (!default_lang_init)
        {
            const char* env;

            if ((env = getenv("LANGUAGE")))
                SAFECPY(default_lang, env);

            default_lang_init = 1;
        }

        if (pref && *pref)
            set_env_var("LANGUAGE", pref);
        else
            set_env_var("LANGUAGE", default_lang);

        /* Set up gettext. */

#if ENABLE_NLS
        bindtextdomain(domain, dir);
        bind_textdomain_codeset(domain, GT_CODESET);
        textdomain(domain);
#endif
    }

    free(dir);
}

#if __cplusplus
extern "C"
#endif
const char *gt_prefix(const char *msgid)
{
#if ENABLE_NLS
    const char *msgval = gettext(msgid);
#else
    const char *msgval = msgid;
#endif

    if (msgval == msgid)
    {
        if ((msgval = strrchr(msgid, '^')))
            msgval++;
        else msgval = msgid;
    }
    return msgval;
}

/*---------------------------------------------------------------------------*/

#if __cplusplus
extern "C"
#endif
const char *lang_path(const char *code)
{
    static char path[MAXSTR];

    SAFECPY(path, "lang/");
    SAFECAT(path, code);
    SAFECAT(path, ".txt");

    return path;
}

#if __cplusplus
extern "C"
#endif
const char *lang_code(const char *path)
{
    return base_name_sans(path, ".txt");
}

#if __cplusplus
extern "C"
#endif
int lang_load(struct lang_desc *desc, const char *path)
{
    if (desc && path && *path)
    {
        fs_file fp;

        memset(desc, 0, sizeof (*desc));
#ifdef FS_VERSION_1
        if ((fp = fs_open(path, "r")))
#else
        if ((fp = fs_open_read(path)))
#endif
        {
            char buf[MAXSTR];

            SAFECPY(desc->code, base_name_sans(path, ".txt"));

            while (fs_gets(buf, sizeof (buf), fp))
            {
                strip_newline(buf);

                if (str_starts_with(buf, "name1 "))
                    SAFECPY(desc->name1, buf + 6);
                else if (str_starts_with(buf, "name2 "))
                    SAFECPY(desc->name2, buf + 6);
                else if (str_starts_with(buf, "font "))
                    SAFECPY(desc->font, buf + 5);
            }

            fs_close(fp);

            if (*desc->name1)
                return 1;
        }
    }
    return 0;
}

#if __cplusplus
extern "C"
#endif
void lang_free(struct lang_desc *desc)
{
}

/*---------------------------------------------------------------------------*/

static int scan_item(struct dir_item *item)
{
    if (str_ends_with(item->path, ".txt"))
    {
        struct lang_desc *desc;

        if ((desc = (struct lang_desc *) calloc(1, sizeof (*desc))))
        {
            if (lang_load(desc, item->path))
            {
                item->data = desc;
                return 1;
            }

            free(desc);
        }
    }
    return 0;
}

static void free_item(struct dir_item *item)
{
    if (item && item->data)
    {
#if __cplusplus
        lang_free(reinterpret_cast<lang_desc*>(item->data));
#else
        lang_free(item->data);
#endif

        free(item->data);
        item->data = NULL;
    }
}

static int cmp_items(const void *A, const void *B)
{
#if __cplusplus
    const struct dir_item *a = static_cast<const struct dir_item *>(A),
                          *b = static_cast<const struct dir_item *>(B);
#else
    const struct dir_item *a = A, * b = B;
#endif
    return strcmp(a->path, b->path);
}

#if __cplusplus
extern "C"
#endif
Array lang_dir_scan(void)
{
    Array items;

    if ((items = fs_dir_scan("lang", scan_item)))
        array_sort(items, cmp_items);

    return items;
}

#if __cplusplus
extern "C"
#endif
void lang_dir_free(Array items)
{
    int i;

    for (i = 0; i < array_len(items); i++)
#if __cplusplus
        free_item(reinterpret_cast<dir_item*>(array_get(items, i)));
#else
        free_item(array_get(items, i));
#endif

    dir_free(items);
}

/*---------------------------------------------------------------------------*/

struct lang_desc curr_lang;

static int lang_status;

#if __cplusplus
extern "C"
#endif
void lang_init(void)
{
#if ENABLE_NLS
    lang_quit();
#if __cplusplus
    try {
#endif
    lang_load(&curr_lang, lang_path(config_get_s(CONFIG_LANGUAGE)));
#if _WIN32 && _MSC_VER
    GAMEDBG_CHECK_SEGMENTATIONS(gt_init("pennyball", curr_lang.code));
#endif
    lang_status = 1;
#if __cplusplus
    } catch (const std::exception& xO) {
        log_errorf("Failure to initialize locale!: Exception caught! (%s)\n", xO.what());
    } catch (const char* xS) {
        log_errorf("Failure to initialize locale!: Exception caught! (%s)\n", xS);
    } catch (...) {
        log_errorf("Failure to initialize locale!: Exception caught! (Unknown type)\n");
    }
#endif
#endif
}

#if __cplusplus
extern "C"
#endif
void lang_quit(void)
{
#if ENABLE_NLS
    if (lang_status)
    {
        lang_free(&curr_lang);
        lang_status = 0;
    }
#endif
}

#endif

/*---------------------------------------------------------------------------*/
