/*
 * Copyright (C) 2023 Microsoft / Neverball authors
 *
 * this file is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */

#ifndef LANG_H
#define LANG_H

/*---------------------------------------------------------------------------*/

#if ENABLE_NLS==1 && NLS_GETTEXT==1

#if _MSC_VER
/*
 * TODO: To compile single localization file on WSL2 or Ubuntu CLI, run:
 *       msgfmt --directory=./po <LANG_NAME>.po --output-file=./locale/<LANG_NAME>/LC_MESSAGES/neverball.mo
 */

#include <libgnuintl.h>
#else
#include <libintl.h>
#endif
#define _(s) gettext(s)
#define gt_plural(msgid, msgid_plural, n) ngettext(msgid, msgid_plural, n)

#elif _WIN32

const char *ms_nls_gettext(const char *);

#define _(s) ms_nls_gettext(s)
#define gt_plural(msgid, msgid_plural, n) ((n) == 1 ? (msgid) : (msgid_plural))

#else

#define _(s) (s)
#define gt_plural(msgid, msgid_plural, n) ((n) == 1 ? (msgid) : (msgid_plural))

#endif /* ENABLE_NLS==1 && NLS_GETTEXT==1 */

/* No-op, useful for marking up strings for extraction-only. */
#define N_(s) s

/* Disambiguate strings with a caret-separated prefix. */
const char *gt_prefix(const char *);

/*---------------------------------------------------------------------------*/

#include "common.h"
#include "array.h"
#include "dir.h"

struct lang_desc
{
    char code[32];

    char name1[MAXSTR];
    char name2[MAXSTR];
    char font[MAXSTR];
};

#define lang_name(desc) (*(desc)->name2 ? (desc)->name2 : (desc)->name1)

const char *lang_path(const char *code);
const char *lang_code(const char *path);

int  lang_load(struct lang_desc *, const char *);
void lang_free(struct lang_desc *);

/*---------------------------------------------------------------------------*/

#define LANG_GET(a, i) ((struct lang_desc *) DIR_ITEM_GET((a), (i))->data)

Array lang_dir_scan(void);
void  lang_dir_free(Array);

/*---------------------------------------------------------------------------*/

extern struct lang_desc curr_lang;

void lang_init(void);
void lang_quit(void);

/*---------------------------------------------------------------------------*/

#endif
