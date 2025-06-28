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

/*
 * HACK: Remembering the code file differences:
 * Developers  who  programming  C++  can see more bedrock declaration
 * than C.  Developers  who  programming  C  can  see  few  procedural
 * declaration than  C++.  Keep  in  mind  when making  sure that your
 * extern code must associated. The valid file types are *.c and *.cpp,
 * so it's always best when making cross C++ compiler to keep both.
 * - Ersohn Styne
 */

#include <string.h>
#include <locale.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
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

#include <sstream>
#include <vector>

extern "C" {
#include "lang.h"

#include "array.h"
#include "common.h"
#include "config.h"
#include "base_config.h"
#include "fs.h"
#include "log.h"
}

#if _DEBUG && _MSC_VER
#ifndef _CRTDBG_MAP_ALLOC
#pragma message(__FILE__": Missing _CRT_MAP_ALLOC, recreate: _CRTDBG_MAP_ALLOC + crtdbg.h")
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

/*---------------------------------------------------------------------------*/

#if _WIN32 && _MSC_VER && NLS_GETTEXT!=1

static std::ostringstream lang_splitstr(std::string str, std::string delim = "\n")
{
    std::ostringstream finalres;

    size_t start = 0, end = str.find(delim);

    bool writeable = true;

    while (end != -1)
    {
        if (writeable)
            finalres << str.substr(start, end - start);

        start = end + delim.size();
        end = str.find(delim, start);

        if (writeable) finalres << '\n';

        writeable = !writeable;
    }

    finalres << str.substr(start, end - start);
    return finalres;
}

/*---------------------------------------------------------------------------*/

std::vector<bool> lang_text_found;
std::vector<char *> src_lang_text;
std::vector<char *> targ_lang_text;

static char *targ_lang_code;

#if _MSC_VER
#define MSVC_DEBUG_NLS 1
#endif

void ms_nls_free(void);

static void ms_nls_init(const char *pref)
{
    ms_nls_free();

    char *dir = strdup(getenv("NEVERBALL_LOCALE"));

    /* Select the location of message catalogs. */

    if (!dir)
    {
        if (path_is_abs(CONFIG_LOCALE "\\..\\po"))
            dir = strdup(CONFIG_LOCALE "\\..\\po");
        else
            dir = concat_string(fs_base_dir(), "\\", CONFIG_LOCALE, "\\..\\po",
                                NULL);
    }

    if (!dir_exists(dir))
    {
        log_errorf("Failure to find locale path: %s - %s\n",
                   dir, errno ? strerror(errno) : "Unknown error");
#if !MSVC_DEBUG_NLS
        free(dir);
        dir = NULL;
#endif
        return;
    }

    bool lang_available = false;

    size_t  pWLocaleNameC;
    wchar_t pWLocaleName[MAXSTR];
    size_t  pCharC;
    char    pCharExt[MAXSTR], pChar[MAXSTR];

#define LANG_RESET_DEFAULTS \
    do {                                                        \
        GetSystemDefaultLocaleName(pWLocaleName, 85);           \
        wcstombs_s(&pCharC, pChar, MAXSTR, pWLocaleName, 2);    \
        wcstombs_s(&pCharC, pCharExt, MAXSTR, pWLocaleName, 5); \
        for (size_t i = 0; i < strlen(pCharExt); i++)           \
            if (pCharExt[i] == '-')                             \
                pCharExt[i] = '_';                              \
    } while (0)

    if (strlen(pref) < 2)
    {
        LANG_RESET_DEFAULTS;
        log_printf("Choosed system locale code: %s\n", pCharExt);
        lang_available = true;
    }
    else
    {
        mbstowcs_s(&pWLocaleNameC, pWLocaleName, pref, 5);

        for (size_t i = 0; i < wcslen(pWLocaleName); i++)
        {
            if (pWLocaleName[i] == '_')
                pWLocaleName[i] = '-';
        }

        if (!IsValidLocaleName(pWLocaleName))
        {
            log_errorf("Invalid locale code: %s\n", pWLocaleName);
            LANG_RESET_DEFAULTS;
        }
        else
        {
            wcstombs_s(&pCharC, pChar, pWLocaleName, 2);
            wcstombs_s(&pCharC, pCharExt, pWLocaleName, 5);

            for (size_t i = 0; i < strlen(pCharExt); i++)
            {
                if (pCharExt[i] == '-')
                    pCharExt[i] = '_';
            }

            log_printf("Used locale code: %s\n", pCharExt);
            lang_available = true;
        }
    }
#undef LANG_RESET_DEFAULTS

    fs_file fp;
    bool lang_first = true, lang_found = false;
    char *inPath    = strdup(concat_string(dir, "\\", pChar, ".po", NULL));
    char *inPathExt = strdup(concat_string(dir, "\\", pCharExt, ".po", NULL));

    int lock_it = 0;

    for (lock_it = 0; inPath[lock_it]; ++lock_it)
        if (inPath[lock_it] == '\\')
            inPath[lock_it] = '/';

    for (lock_it = 0; inPathExt[lock_it]; ++lock_it)
        if (inPathExt[lock_it] == '\\')
            inPathExt[lock_it] = '/';

    bool lang_duplicated = false, lang_src_empty = false,
         lang_src_hold   = false, lang_targ_hold = false;

    char src_lang_stack_final[MAXSTR];
    char targ_lang_stack_final[MAXSTR];
    memset(src_lang_stack_final, 0, MAXSTR);
    memset(targ_lang_stack_final, 0, MAXSTR);

    if (!lang_available)
    {
#if !MSVC_DEBUG_NLS
        free(dir);
        dir = NULL;
#endif
        return;
    }

#define LANG_FUNC_LOADSEG_FINALIZE \
        do { \
            lang_src_empty = strlen(src_lang_stack_final) < 1; \
            if (lang_found && !lang_src_empty) \
            { \
                for (int i = 0; i < src_lang_text.size() && !lang_duplicated; i++) \
                { \
                    if (strncmp(src_lang_text[i], src_lang_stack_final, strlen(src_lang_stack_final)) == 0 && \
                        strlen(src_lang_stack_final) == strlen(src_lang_text[i])) \
                        lang_duplicated = true; \
                } \
                char temp_targ0[MAXSTR], \
                     temp_targ1[MAXSTR]; \
                SAFECPY(temp_targ0, buf + 8); \
                strncpy_s(temp_targ1, MAXSTR, temp_targ0, strlen(temp_targ0) - 1); \
                if (!lang_duplicated) \
                { \
                    std::string _src_std_lang(src_lang_stack_final); \
                    std::string _targ_std_lang(targ_lang_stack_final); \
                    std::ostringstream _src_std_final = lang_splitstr(_src_std_lang); \
                    std::ostringstream _targ_std_final = lang_splitstr(_targ_std_lang); \
                    src_lang_text.push_back(strdup (_src_std_final.str().c_str())); \
                    targ_lang_text.push_back(strdup (_targ_std_final.str().c_str())); \
                    lang_text_found.push_back(strlen(_targ_std_final.str().c_str()) > 0); \
                } \
            } \
            else if (!lang_src_empty) \
            { \
                for (int i = 0; i < src_lang_text.size() && !lang_duplicated; i++) \
                { \
                    if (strncmp(src_lang_text[i], src_lang_stack_final, strlen(src_lang_stack_final)) == 0 && \
                        strlen(src_lang_stack_final) == strlen(src_lang_text[i])) \
                        lang_duplicated = true; \
                } \
                if (!lang_duplicated) \
                { \
                    std::string _src_std_lang(src_lang_stack_final); \
                    std::ostringstream _src_std_final = lang_splitstr(_src_std_lang); \
                    src_lang_text.push_back(strdup (_src_std_final.str().c_str())); \
                    targ_lang_text.push_back(strdup ("")); \
                    lang_text_found.push_back(false); \
                } \
            } \
        } while (0)

#define LANG_FUNC_LOAD_LOOP \
        do { \
            char temp_code[MAXSTR]; \
            SAFECPY(temp_code, base_name_sans(inPath, ".po")); \
            while (fs_gets(buf, sizeof (buf), fp)) \
            { \
                strip_newline(buf); \
                if (str_starts_with(buf, "msgid ")) \
                { \
                    LANG_FUNC_LOADSEG_FINALIZE; \
                    memset(src_lang_stack_final, 0, MAXSTR); \
                    memset(targ_lang_stack_final, 0, MAXSTR); \
                    lang_targ_hold = false; \
                    lang_src_hold = true; \
                    lang_src_empty = false; \
                    lang_duplicated = false; \
                    lang_found = false; \
                    char temp_src0[MAXSTR], \
                         temp_src1[MAXSTR]; \
                    SAFECPY(temp_src0, buf + 7); \
                    strncpy_s(temp_src1, MAXSTR, temp_src0, strlen(temp_src0) - 1); \
                    strncpy_s(src_lang_stack_final, MAXSTR, temp_src1, strlen(temp_src1)); \
                    lang_first = false; \
                } \
                if (str_starts_with(buf, "\"") && buf[0] == '"' && lang_src_hold) \
                { \
                    char temp_src0[MAXSTR], \
                         temp_src1[MAXSTR], \
                         temp_stack_final[MAXSTR]; \
                    SAFECPY(temp_src0, buf+1); \
                    strncpy_s(temp_src1, MAXSTR, temp_src0, strlen(temp_src0) - 1); \
                    strncpy_s(temp_stack_final, MAXSTR, strdup(temp_src1), strlen(temp_src1)); \
                    SAFECAT(src_lang_stack_final, temp_stack_final); \
                } \
                if (str_starts_with(buf, "msgstr ") && lang_src_hold) \
                { \
                    lang_src_hold = false; \
                    lang_targ_hold = true; \
                    if (!lang_found) \
                    { \
                        lang_found = true; \
                        char temp_targ0[MAXSTR], \
                             temp_targ1[MAXSTR]; \
                        SAFECPY(temp_targ0, buf + 8); \
                        strncpy_s(temp_targ1, MAXSTR, temp_targ0, strlen(temp_targ0) - 1); \
                        strncpy_s(targ_lang_stack_final, MAXSTR, temp_targ1, strlen(temp_targ1)); \
                    } \
                } \
                if (str_starts_with(buf, "\"") && buf[0] == '"' && lang_targ_hold) \
                { \
                    char temp_targ0[MAXSTR], \
                         temp_targ1[MAXSTR], \
                         temp_stack_final[MAXSTR]; \
                    SAFECPY(temp_targ0, buf + 1); \
                    strncpy_s(temp_targ1, MAXSTR, temp_targ0, strlen(temp_targ0)); \
                    strncpy_s(temp_stack_final, MAXSTR, strdup(temp_targ1), strlen(temp_targ1)); \
                    SAFECAT(targ_lang_stack_final, temp_stack_final); \
                } \
            } \
        } while (0)

    if ((fp = fs_open_read(inPathExt)))
    {
        char buf[MAXSTR];
        LANG_FUNC_LOAD_LOOP;

        LANG_FUNC_LOADSEG_FINALIZE;
        fs_close(fp);
    }
    else if ((fp = fs_open_read(inPath)))
    {
        char buf[MAXSTR];
        LANG_FUNC_LOAD_LOOP;

        LANG_FUNC_LOADSEG_FINALIZE;
        fs_close(fp);
    }
    else
        log_errorf("Can't load poedit file!: %s / %s\n", inPath, fs_error());

    free(inPath);    inPath    = NULL;
    free(inPathExt); inPathExt = NULL;

#undef LANG_FUNC_LOAD_LOOP
#undef LANG_FUNC_LOADSEG_FINALIZE

#ifndef NDEBUG
    /* HACK: Must match the element count in order. */

    assert(lang_text_found.size() == src_lang_text.size() &&
           lang_text_found.size() == targ_lang_text.size());
#endif

#if !MSVC_DEBUG_NLS
    free(dir);
    dir = NULL;
#endif
}

void ms_nls_free(void)
{
    int i;

    for (i = 0; i < lang_text_found.size(); i++)
        lang_text_found[i] = false;

    lang_text_found.clear();

    for (i = 0; i < src_lang_text.size(); i++)
    {
        free(src_lang_text[i]);
        src_lang_text[i] = NULL;
    }

    src_lang_text.clear();

    for (i = 0; i < targ_lang_text.size(); i++)
    {
        free(targ_lang_text[i]);
        targ_lang_text[i] = NULL;
    }

    targ_lang_text.clear();
}

const char *ms_nls_gettext(const char *s)
{
    bool lang_was_found = false;
    const char *outS;

    for (int i = 0; i < src_lang_text.size(); i++)
    {
        if (strncmp(src_lang_text[i], s, strlen(src_lang_text[i])) == 0 &&
            strlen(s) == strlen(src_lang_text[i]))
            if (lang_text_found[i])
            {
                outS = targ_lang_text[i];
                lang_was_found = true;
                break;
            }
    }

    if (!lang_was_found)
        outS = s;

    return outS;
}

const char *gt_prefix(const char *msgid)
{
    const char *msgval = ms_nls_gettext(msgid);

    if (msgval == msgid)
    {
        if ((msgval = strrchr(msgid, '^')))
            msgval++;
        else msgval = msgid;
    }
    return msgval;
}

/*---------------------------------------------------------------------------*/

const char *lang_path(const char *code)
{
    static char path[MAXSTR];

    SAFECPY(path, "lang/");
    SAFECAT(path, code);
    SAFECAT(path, ".txt");

    return path;
}

const char *lang_code(const char *path)
{
    return base_name_sans(path, ".txt");
}

int lang_load(struct lang_desc *desc, const char *path)
{
    if (desc && path && *path)
    {
        fs_file fp;

        memset(desc, 0, sizeof (*desc));

        if ((fp = fs_open_read(path)))
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

void lang_free(struct lang_desc *desc)
{
}

/*---------------------------------------------------------------------------*/

static int scan_item(struct dir_item *item)
{
    if (str_ends_with(item->path, ".txt"))
    {
        struct lang_desc *desc;

        if ((desc = (lang_desc *) calloc(1, sizeof (*desc))))
        {
            if (lang_load(desc, item->path))
            {
                item->data = desc;
                return 1;
            }

            free(desc);
            desc = NULL;
        }
    }
    return 0;
}

static void free_item(struct dir_item *item)
{
    if (item && item->data)
    {
        lang_free((lang_desc *) item->data);

        free(item->data);
        item->data = NULL;
    }
}

static int cmp_items(const void *A, const void *B)
{
    const struct dir_item *a = (dir_item *) A, *b = (dir_item *) B;
    return strcmp(a->path, b->path);
}

Array lang_dir_scan(void)
{
    Array items;

    if ((items = fs_dir_scan("lang", scan_item)))
        array_sort(items, cmp_items);

    return items;
}

void lang_dir_free(Array items)
{
    int i;

    for (i = 0; i < array_len(items); i++)
        free_item((dir_item *) array_get(items, i));

    dir_free(items);
}

/*---------------------------------------------------------------------------*/

struct lang_desc curr_lang;

static int lang_status;

extern "C" void lang_init(void)
{
    lang_quit();
    lang_load(&curr_lang, lang_path(config_get_s(CONFIG_LANGUAGE)));
    ms_nls_init(curr_lang.code);
    lang_status = 1;
}

extern "C" void lang_quit(void)
{
    if (lang_status)
    {
        lang_free(&curr_lang);

        ms_nls_free();

        lang_status = 0;
    }
}

#endif

/*---------------------------------------------------------------------------*/
