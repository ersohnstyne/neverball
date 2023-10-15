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

#include "package.h"
#include "array.h"
#include "common.h"
#include "fetch.h"
#include "fs.h"
#include "lang.h"

#if _DEBUG && _MSC_VER
#ifndef _CRTDBG_MAP_ALLOC
#pragma message(__FILE__": Missing CRT-Debugger include header, recreate: crtdbg.h")
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

/*
 * Premium: pennyball.stynegame.de
 * Legacy downloads: neverball.github.io
 */
#define NB_CURRDOMAIN_PREMIUM "neverball.github.io"

struct package
{
    unsigned int size;

    char id      [64];
    char type    [64];
    char filename[MAXSTR];
    char files   [MAXSTR];
    char name    [64];
    char desc    [MAXSTR];
    char shot    [64];

#if NB_HAVE_PB_BOTH==1
    char category     [64];
#if ENABLE_FETCH>=2
    char fileid_gdrive[64];
    char shotid_gdrive[64];
#endif
#endif

    enum package_status status;
};

static Array available_addons;
static int   package_curr_category = PACKAGE_CATEGORY_LEVELSET;

#define PACKAGE_GET(a, i) ((struct package *) array_get((a), (i)))

#define PACKAGE_DIR "DLC"
#define NB_DOWNLOADPATH PACKAGE_DIR "/"
#define NB_DOWNLOADPATH_ROOT "/" PACKAGE_DIR "/"

#if NB_HAVE_PB_BOTH==1 && ENABLE_FETCH>=2

// TODO: Change the Google Drive package file ID preprocessor definitions

#ifndef NB_GDRIVE_PACKAGE_FILEID_LEVELSET
#error Must specify Level set file ID from the Google Drive website.
#endif

#ifndef NB_GDRIVE_PACKAGE_FILEID_CAMPAIGN
#error Must specify Campaign file ID from the Google Drive website.
#endif

#endif

/*---------------------------------------------------------------------------*/

/*
 * Get a download URL.
 */
static const char *get_package_url(const char *filename, int category)
{
    if (filename && *filename)
    {
        static char url[MAXSTR];

        memset(url, 0, sizeof (url));

#ifdef __EMSCRIPTEN__
#if defined(NB_PACKAGES_PREMIUM)
        switch (category)
        {
        case PACKAGE_CATEGORY_LEVELSET:
            /* Uses premium sets */
            SAFECPY(url, "/addons/levelsets/");
            break;
        case PACKAGE_CATEGORY_CAMPAIGN:
            /* Uses campaign */
            SAFECPY(url, "/addons/campaign/");
            break;
        case PACKAGE_CATEGORY_PROFILE:
            /* Uses ball models */
            SAFECPY(url, "/addons/ball/");
            break;
        case PACKAGE_CATEGORY_COURSE:
            /* Uses ball models */
            SAFECPY(url, "/addons/course/");
            break;
        }
#else
        /* Uses standard vanilla game */
        SAFECPY(url, "/addons/");
#endif
#else
#if defined(NB_PACKAGES_PREMIUM)
        switch (category)
        {
        case PACKAGE_CATEGORY_LEVELSET:
            /* Uses premium sets */
            SAFECPY(url, "https://" NB_CURRDOMAIN_PREMIUM "/addons/levelsets/");
            break;
        case PACKAGE_CATEGORY_CAMPAIGN:
            /* Uses campaign */
            SAFECPY(url, "https://" NB_CURRDOMAIN_PREMIUM "/addons/campaign/");
            break;
        case PACKAGE_CATEGORY_PROFILE:
            /* Uses ball models */
            SAFECPY(url, "https://" NB_CURRDOMAIN_PREMIUM "/addons/ball/");
            break;
        case PACKAGE_CATEGORY_COURSE:
            /* Uses ball models */
            SAFECPY(url, "https://" NB_CURRDOMAIN_PREMIUM "/addons/course/");
            break;
        }
#else
        /* Uses legacy vanilla game */
        SAFECPY(url, "https://neverball.github.io/addons/");
#endif
#endif
        SAFECAT(url, filename);

        return url;
    }

    return NULL;
}

/*
 * Get a download filename.
 */
static const char *get_package_path(const char *filename)
{
    if (filename && *filename)
    {
        static char path[MAXSTR];

        memset(path, 0, sizeof (path));

        SAFECPY(path, NB_DOWNLOADPATH);
        SAFECAT(path, filename);

        return path;
    }
    return NULL;
}

/*---------------------------------------------------------------------------*/

/*
 * We also track the installed addons separately, just so we don't have to scan
 * the package directory and figure out which ZIP files can be added to the FS
 * and which ones can't.
 */
struct local_package
{
    char id[64];
    char filename[MAXSTR];
};

static List installed_addons;

static struct local_package *create_local_package(const char *package_id, const char *filename)
{
    struct local_package *lpkg = calloc(sizeof (*lpkg), 1);

    if (lpkg)
    {
        SAFECPY(lpkg->id, package_id);
        SAFECPY(lpkg->filename, filename);
    }

    return lpkg;
}

static void free_local_package(struct local_package **lpkg)
{
    if (lpkg && *lpkg)
    {
        free(*lpkg);
        *lpkg = NULL;
    }
}

/*
 * Add package file to FS path.
 */
static int mount_package_file(const char *filename)
{
    const char *write_dir = fs_get_write_dir();
    int added = 0;

    if (filename && *filename && write_dir)
    {
        char *path = concat_string(write_dir, NB_DOWNLOADPATH_ROOT,
                                   filename, NULL);

        if (path)
        {
            added = fs_add_path(path);

            free(path);
            path = NULL;
        }
    }

    return added;
}

/*
 * Remove package file from the FS read path.
 */
static void unmount_package_file(const char *filename)
{
    const char *write_dir = fs_get_write_dir();

    if (filename && *filename && write_dir)
    {
        char *path = concat_string(write_dir, NB_DOWNLOADPATH_ROOT, filename, NULL);

        if (path)
        {
            fs_remove_path(path);

            free(path);
            path = NULL;
        }
    }
}

/*
 * Unmount and uninstall other instances of the given local package.
 */
static void unmount_duplicate_local_addons(const struct local_package* keep_lpkg)
{
    List p, l;

    /* Unmount and uninstall other instances of this package ID. */

    for (p = NULL, l = installed_addons; l; p = l, l = l->next)
    {
        struct local_package* test_lpkg = l->data;

        if (test_lpkg != keep_lpkg && strcmp(test_lpkg->id, keep_lpkg->id) == 0)
        {
            unmount_package_file(test_lpkg->filename);

            free_local_package(&test_lpkg);

            l->data = NULL;

            if (p)
            {
                p->next = list_rest(l);
                l = p;
            }
            else
            {
                installed_addons = list_rest(l);
                l = installed_addons;
            }
        }
    }
}

/*
 * Add a package to the FS path and to the list, if not yet added.
 */
static int mount_local_package(struct local_package *lpkg)
{
    if (lpkg && mount_package_file(lpkg->filename))
    {
        installed_addons = list_cons(lpkg, installed_addons);
        unmount_duplicate_local_addons(lpkg);
        return 1;
    }

    return 0;
}

/*
 * Load the list of installed addons.
 */
static int load_installed_addons(void)
{
#if defined(NB_PACKAGES_PREMIUM)
    char default_filename[64];

    switch (package_curr_category)
    {
    case PACKAGE_CATEGORY_CAMPAIGN:
        SAFECPY(default_filename, "installed-addons_campaign.txt");
        break;
    case PACKAGE_CATEGORY_PROFILE:
        SAFECPY(default_filename, "installed-addons_ball.txt");
        break;
    case PACKAGE_CATEGORY_COURSE:
        SAFECPY(default_filename, "installed-addons_course.txt");
        break;
    default:
        SAFECPY(default_filename, "installed-addons.txt");
        break;
    }

#else
    const char *default_filename = "installed-addons.txt";
#endif

#ifdef FS_VERSION_1
    fs_file fp = fs_open(get_package_path(default_filename), "r");
#else
    fs_file fp = fs_open_read(get_package_path(default_filename));
#endif

    if (fp)
    {
        char line[MAXSTR] = "";

        Array pkgs = array_new(sizeof(struct local_package));
        struct local_package* lpkg = NULL;
        int i, n;

        while (fs_gets(line, sizeof(line), fp))
        {
            strip_newline(line);

            if (strncmp(line, "package ", 8) == 0)
            {
                lpkg = array_add(pkgs);

                if (lpkg)
                    SAFECPY(lpkg->id, line + 8);
            }
            else if (strncmp(line, "filename ", 9) == 0)
            {
                if (lpkg)
                    SAFECPY(lpkg->filename, line + 9);
            }
            else if (fs_exists(get_package_path(line)))
            {
                /* Backward compatibility: the entire line is the filename. */

                if ((lpkg = array_add(pkgs)))
                {
                    char *delim;

                    SAFECPY(lpkg->filename, line);

                    /* Extract package ID from the filename. */

                    if ((delim = strrchr(lpkg->filename, '-')))
                    {
                        size_t len = delim - lpkg->filename;
                        memcpy(lpkg->id, lpkg->filename, MIN(sizeof(lpkg->id) - 1, len));
                    }

                    lpkg = NULL;
                }
            }
        }

        for (i = 0, n = array_len(pkgs); i < n; ++i)
        {
            const struct local_package *src = array_get(pkgs, i);
            struct local_package       *dst = create_local_package(src->id, src->filename);

            if (!mount_local_package(dst))
                free_local_package(&dst);
        }

        if (pkgs)
        {
            array_free(pkgs);
            pkgs = NULL;
        }

        fs_close(fp);
        fp = NULL;

        return 1;
    }

    return 0;
}

/*
 * Save the list of installed addons.
 */
static int save_installed_addons(void)
{
    if (installed_addons)
    {
#if defined(NB_PACKAGES_PREMIUM)
        char default_filename[64];

        switch (package_curr_category)
        {
        case PACKAGE_CATEGORY_CAMPAIGN:
            SAFECPY(default_filename, "installed-addons_campaign.txt");
            break;
        case PACKAGE_CATEGORY_PROFILE:
            SAFECPY(default_filename, "installed-addons_ball.txt");
            break;
        case PACKAGE_CATEGORY_COURSE:
            SAFECPY(default_filename, "installed-addons_course.txt");
            break;
        default:
            SAFECPY(default_filename, "installed-addons.txt");
            break;
        }

#else
        const char* default_filename = "installed-addons.txt";
#endif

        fs_file fp = fs_open_write(get_package_path(default_filename));

        if (fp)
        {
            List l;

            for (l = installed_addons; l; l = l->next)
            {
                struct local_package* lpkg = l->data;

                if (lpkg)
                    fs_printf(fp, "package %s\nfilename %s\n", lpkg->id, lpkg->filename);
            }

            fs_close(fp);
            fp = NULL;

            fs_persistent_sync();

            return 1;
        }
    }

    return 0;
}

/*
 * Free the list of installed addons.
 */
static void free_installed_addons(void)
{
    List l = installed_addons;

    while (l)
    {
        struct local_package *lpkg = l->data;

        free_local_package(&lpkg);

        l = list_rest(l);
    }

    installed_addons = NULL;
}

/*---------------------------------------------------------------------------*/

/*
 * Figure out package statuses.
 */
static void load_package_statuses(Array addons)
{
    if (addons)
    {
        int i, n;

        for (i = 0, n = array_len(addons); i < n; ++i)
        {
            struct package* pkg = array_get(addons, i);
            const char* dest_filename = get_package_path(pkg->filename);

            pkg->status = PACKAGE_AVAILABLE;

            for (List l = installed_addons; l; l = l->next)
            {
                struct local_package* lpkg = l->data;

                if (strcmp(pkg->id, lpkg->id) == 0)
                {
                    pkg->status = PACKAGE_UPDATE;

                    if (strcmp(pkg->filename, lpkg->filename) == 0)
                    {
                        pkg->status = PACKAGE_INSTALLED;
                        break;
                    }
                }
            }
        }
    }
}

/*
 * Load an array of addons from a manifest file.
 */
static Array load_addons_from_file(const char *filename)
{
    Array addons = array_new(sizeof (struct package));
    fs_file fp;

    if (!addons)
        return NULL;

    if ((fp = fs_open_read(filename)))
    {
        struct package *pkg = NULL;
        char line[MAXSTR];

        while (fs_gets(line, sizeof (line), fp))
        {
            strip_newline(line);

            if (str_starts_with(line, "package "))
            {
                /* Start reading a new package. */

                pkg = array_add(addons);

                if (pkg)
                {
                    size_t prefix_len;

                    memset(pkg, 0, sizeof (*pkg));

                    SAFECPY(pkg->id, line + 8);
                    prefix_len = strcspn(pkg->id, "-");
#if _MSC_VER && _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                    strncpy_s(pkg->type, 64,
                              pkg->id, MIN(sizeof(pkg->type) - 1, prefix_len));
#else
                    strncpy(pkg->type,
                            pkg->id, MIN(sizeof(pkg->type) - 1, prefix_len));
#endif
                    pkg->size = 0;
                    pkg->filename[0] = 0;
                    pkg->files[0] = 0;
                    pkg->name[0] = 0;
                    pkg->desc[0] = 0;
                    pkg->shot[0] = 0;
#if NB_HAVE_PB_BOTH==1
                    pkg->category[0] = 0;
#if ENABLE_FETCH>=2
                    pkg->fileid_gdrive[0] = 0;
                    pkg->shotid_gdrive[0] = 0;
#endif
#endif
                }
            }
#if NB_HAVE_PB_BOTH==1
            else if (str_starts_with(line, "category "))
            {
                if (pkg)
                    SAFECPY(pkg->category, line + 9);
            }
#if ENABLE_FETCH>=2
            else if (str_starts_with(line, "fileid-gdrive "))
            {
                if (pkg)
                    SAFECPY(pkg->fileid_gdrive, line + 14);
            }
#endif
#endif
            else if (str_starts_with(line, "filename "))
            {
                if (pkg)
                    SAFECPY(pkg->filename, line + 9);
            }
            else if (str_starts_with(line, "size "))
            {
                if (pkg)
#if _MSC_VER && _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                    sscanf_s(line + 5,
#else
                    sscanf(line + 5,
#endif
                           "%u", &pkg->size);
            }
            else if (str_starts_with(line, "files "))
            {
                if (pkg)
                    SAFECPY(pkg->files, line + 6);
            }
            else if (str_starts_with(line, "name "))
            {
                if (pkg)
                    SAFECPY(pkg->name, line + 5);
            }
            else if (str_starts_with(line, "desc "))
            {
                if (pkg)
                {
                    char* s = NULL;

                    SAFECPY(pkg->desc, line + 5);

                    /* Replace "\\n" with "\r\n" in place. I really just need the "\n", but don't want to move bytes around. */

                    for (s = pkg->desc; (s = strstr(s, "\\n")); s += 2)
                    {
                        s[0] = '\r';
                        s[1] = '\n';
                    }
                }
            }
            else if (str_starts_with(line, "shot "))
            {
                if (pkg)
                    SAFECPY(pkg->shot, line + 5);
            }
#if NB_HAVE_PB_BOTH==1 && ENABLE_FETCH>=2
            else if (str_starts_with(line, "shotid-gdrive "))
            {
                if (pkg)
                    SAFECPY(pkg->shotid_gdrive, line + 14);
            }
#endif
        }

        fs_close(fp);
    }

    load_package_statuses(addons);

    return addons;
}

/*
 * Free a loaded array of addons.
 */
static void free_addons(Array addons)
{
    if (addons)
    {
        array_free(addons);
        addons = NULL;
    }
}

/*---------------------------------------------------------------------------*/

/*
 * Queue missing package images for download.
 */
static void fetch_package_images(Array addons)
{
    if (addons)
    {
        int i, n = array_len(addons);

        for (i = 0; i < n; ++i)
        {
            struct package *pkg      = array_get(addons, i);
            const char     *filename = package_get_shot_filename(i);

            if (filename && *filename && !fs_exists(filename))
            {
#if NB_HAVE_PB_BOTH==1 && ENABLE_FETCH>=2
                if (pkg->shotid_gdrive[0])
                {
                    // Google Drive package support
                    struct fetch_callback gdrive_callback = { 0 };
                    fetch_gdrive(pkg->shotid_gdrive, filename,
                                 gdrive_callback);
                }
                else
#endif
                {
#if NB_HAVE_PB_BOTH!=1 || ENABLE_FETCH<3
                    const char* url = get_package_url(pkg->shot,
                                                      package_curr_category);

                    if (url)
                    {
                        struct fetch_callback callback = { 0 };
                        fetch_url(url, filename, callback);
                    }
#endif
                }
            }
        }
    }
}

/*---------------------------------------------------------------------------*/

static void available_addons_done(void *data, void *extra_data)
{
    struct fetch_done *fd = extra_data;

    if (fd && fd->finished)
    {
        const char *filename = get_package_path("available-addons.txt");

        if (filename)
        {
            Array addons = load_addons_from_file(filename);

            if (addons)
            {
                available_addons = addons;

                /* TODO: notify the player somehow about this fact. */

                fetch_package_images(available_addons);
            }
        }
    }
}

/*
 * Download the package list.
 */
static void fetch_available_addons(int category)
{
    const char* filename = get_package_path("available-addons.txt");

#if NB_HAVE_PB_BOTH==1 && ENABLE_FETCH>=2
    if (filename && category == PACKAGE_CATEGORY_CAMPAIGN
     && NB_GDRIVE_PACKAGE_FILEID_CAMPAIGN[0])
    {
        // Google Drive campaign package support

        struct fetch_callback gdrive_callback = { 0 };
        gdrive_callback.done = available_addons_done;
        fetch_gdrive(NB_GDRIVE_PACKAGE_FILEID_CAMPAIGN, filename,
                     gdrive_callback);
        return;
    }
    else if (filename && category == PACKAGE_CATEGORY_LEVELSET
          && NB_GDRIVE_PACKAGE_FILEID_LEVELSET[0])
    {
        // Google Drive level set package support

        struct fetch_callback gdrive_callback = { 0 };
        gdrive_callback.done = available_addons_done;
        fetch_gdrive(NB_GDRIVE_PACKAGE_FILEID_LEVELSET, filename,
                     gdrive_callback);
        return;
    }
#endif
    
#if NB_HAVE_PB_BOTH!=1 || ENABLE_FETCH<3
#ifdef __EMSCRIPTEN__
    const char *url = get_package_url("available-addons-emscripten.txt",
                                      category);
#else
    const char *url = get_package_url("available-addons.txt", category);
#endif

    if (url && filename)
    {
        struct fetch_callback callback = { 0 };
        callback.done = available_addons_done;
        fetch_url(url, filename, callback);
    }
#endif
}

/*---------------------------------------------------------------------------*/

/*
 * Initialize package stuff.
 */
void package_init(void)
{
    const char *write_dir;

    /* Create the downloads directory. */

    write_dir = fs_get_write_dir();

    if (write_dir)
    {
        char *package_dir = concat_string(write_dir, "/", PACKAGE_DIR, NULL);

        if (package_dir)
        {
            if (!dir_exists(package_dir))
                fs_mkdir(PACKAGE_DIR);

            free(package_dir);
            package_dir = NULL;
        }
    }

    /* Load the list of installed addons. */

    load_installed_addons();

    /* Download package list. */

    fetch_available_addons(package_curr_category);
}

void package_quit(void)
{
    if (available_addons)
    {
        free_addons(available_addons);
        available_addons = NULL;
    }

    save_installed_addons();
    free_installed_addons();
}

int package_count(void)
{
    return available_addons ? array_len(available_addons) : 0;
}

/*
 * Find a package that has FILE in its "files" string.
 */
int package_search(const char *file)
{
    if (available_addons)
    {
        int i, n;

        for (i = 0, n = array_len(available_addons); i < n; ++i)
        {
            struct package *pkg = array_get(available_addons, i);

            if (pkg && strstr(pkg->files, file) != NULL)
                return i;
        }
    }

    return -1;
}

/*
 * Find a package by ID.
 */
int package_search_id(const char *package_id)
{
    if (available_addons)
    {
        int i, n;

        for (i = 0, n = array_len(available_addons); i < n; ++i)
        {
            struct package *pkg = array_get(available_addons, i);

            if (pkg && strcmp(pkg->id, package_id) == 0)
                return i;
        }
    }

    return -1;
}

/*
 * Find next package of TYPE (aka the first part of package ID).
 */
int package_next(const char *type, int start)
{
    if (available_addons)
    {
        int i, n;

        for (i = MAX(0, start + 1), n = array_len(available_addons); i < n;
             ++i)
        {
            struct package *pkg = array_get(available_addons, i);
            size_t prefix_len = strcspn(pkg->id, "-");

            if (strncmp(pkg->id, type, prefix_len) == 0)
                return i;
        }
    }

    return -1;
}

/*
 * Get package status.
 */
enum package_status package_get_status(int pi)
{
    if (pi >= 0 && pi < array_len(available_addons))
        return PACKAGE_GET(available_addons, pi)->status;

    return PACKAGE_NONE;
}

const char *package_get_id(int pi)
{
    if (pi >= 0 && pi < array_len(available_addons))
        return PACKAGE_GET(available_addons, pi)->id;

    return NULL;
}

const char *package_get_type(int pi)
{
    if (pi >= 0 && pi < array_len(available_addons))
        return PACKAGE_GET(available_addons, pi)->type;

    return NULL;
}

const char *package_get_name(int pi)
{
    if (pi >= 0 && pi < array_len(available_addons))
        return PACKAGE_GET(available_addons, pi)->name;

    return NULL;
}

const char *package_get_desc(int pi)
{
    if (pi >= 0 && pi < array_len(available_addons))
        return PACKAGE_GET(available_addons, pi)->desc;

    return NULL;
}

const char *package_get_shot(int pi)
{
    if (pi >= 0 && pi < array_len(available_addons))
        return PACKAGE_GET(available_addons, pi)->shot;

    return NULL;
}

const char *package_get_files(int pi)
{
    if (pi >= 0 && pi < array_len(available_addons))
        return PACKAGE_GET(available_addons, pi)->files;

    return NULL;
}

/*
 * Construct a package image filename relative to the write dir.
 */
const char *package_get_shot_filename(int pi)
{
    return get_package_path(package_get_shot(pi));
}

const char* package_get_formatted_type(int pi)
{
    const char* type = package_get_type(pi);

    if (type)
    {
        if (strcmp(type, "set") == 0)
            return _("Level Set");
        else if (strcmp(type, "ball") == 0)
            return _("Ball");
        else if (strcmp(type, "course") == 0)
            return _("Course");
        else if (strcmp(type, "base") == 0)
            return _("Base");
    }

    return type;
}

/*---------------------------------------------------------------------------*/

struct package_fetch_info
{
    struct fetch_callback callback;
    char                 *temp_filename;
    char                 *dest_filename;
    struct package       *pkg;
};

static struct package_fetch_info *create_pfi(struct package *pkg)
{
    struct package_fetch_info *pfi = malloc(sizeof (*pfi));

    if (pfi)
    {
        memset(pfi, 0, sizeof (*pfi));

        pfi->temp_filename = concat_string(get_package_path(pkg->filename),
                                           ".tmp", NULL);
        pfi->dest_filename = strdup(get_package_path(pkg->filename));

        pfi->pkg = pkg;
    }

    return pfi;
}

static void free_pfi(struct package_fetch_info *pfi)
{
    if (pfi)
    {
        if (pfi->temp_filename)
        {
            free(pfi->temp_filename);
            pfi->temp_filename = NULL;
        }

        if (pfi->dest_filename)
        {
            free(pfi->dest_filename);
            pfi->dest_filename = NULL;
        }

        free(pfi);
    }
}

/*
 * Just call the caller's callback.
 */
static void package_fetch_progress(void *data, void *data2)
{
    struct package_fetch_info *pfi = data;

    if (pfi && pfi->callback.progress)
        pfi->callback.progress(pfi->callback.data, data2);
}

/*
 * Add downloaded package to FS.
 */
static void package_fetch_done(void *data, void *extra_data)
{
    struct package_fetch_info *pfi = data;
    struct fetch_done *dn = extra_data;

    if (pfi)
    {
        struct package *pkg = pfi->pkg;
        pkg->status = PACKAGE_ERROR;

        if (dn->finished)
        {
            struct local_package *lpkg = create_local_package(pkg->id, pkg->filename);

            /* Rename from temporary name to target name. */

            if (pfi->temp_filename && pfi->dest_filename)
                fs_rename(pfi->temp_filename, pfi->dest_filename);

            /* Add package to installed addons and to FS. */

            if (lpkg)
            {
                if (mount_local_package(lpkg))
                    pkg->status = PACKAGE_INSTALLED;
                else
                    free_local_package(&lpkg);

                lpkg = NULL;
            }
        }
        else if (pfi->temp_filename)
            fs_remove(pfi->temp_filename);

        if (pfi->callback.done)
            pfi->callback.done(pfi->callback.data, extra_data);

        free_pfi(pfi);
        pfi = NULL;
    }
}

unsigned int package_fetch(int pi, struct fetch_callback callback, int category)
{
    unsigned int fetch_id = 0;

    if (pi >= 0 && pi < array_len(available_addons))
    {
        struct package *pkg = array_get(available_addons, pi);

#if NB_HAVE_PB_BOTH==1 && ENABLE_FETCH>=2
        if (pkg->fileid_gdrive[0])
        {
            struct package_fetch_info *pfi = create_pfi(pkg);

            /* Store passed callback. */

            pfi->callback = callback;

            /* Reuse variable to pass our callbacks. */

            callback.progress = package_fetch_progress;
            callback.done     = package_fetch_done;
            callback.data     = pfi;

            fetch_id = fetch_gdrive(pkg->fileid_gdrive, pfi->temp_filename,
                                    callback);

            if (fetch_id)
                pkg->status = PACKAGE_DOWNLOADING;
            else
            {
                free_pfi(pfi);
                pfi = NULL;
                callback.data = NULL;
            }

            return fetch_id;
        }
#endif

#if NB_HAVE_PB_BOTH!=1 || ENABLE_FETCH<3
        const char *url = get_package_url(pkg->filename, category);

        if (url)
        {
            struct package_fetch_info *pfi = create_pfi(pkg);

            /* Store passed callback. */

            pfi->callback = callback;

            /* Reuse variable to pass our callbacks. */

            callback.progress = package_fetch_progress;
            callback.done     = package_fetch_done;
            callback.data     = pfi;

            fetch_id = fetch_url(url, pfi->temp_filename, callback);

            if (fetch_id)
                pkg->status = PACKAGE_DOWNLOADING;
            else
            {
                free_pfi(pfi);
                pfi = NULL;
                callback.data = NULL;
            }

            return fetch_id;
        }
#endif
    }

    return fetch_id;
}