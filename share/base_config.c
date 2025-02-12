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
#include <stdio.h>
#include <math.h>

#include "base_config.h"
#include "common.h"
#include "log.h"
#include "fs.h"

#if _DEBUG && _MSC_VER
#ifndef _CRTDBG_MAP_ALLOC
#pragma message(__FILE__": Missing _CRT_MAP_ALLOC, recreate: _CRTDBG_MAP_ALLOC + crtdbg.h")
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

#ifdef _WIN32
#include <shlobj.h>

#if _MSC_VER && !defined(_CONSOLE)
#pragma message("Neverball " VERSION " for Microsoft Visual Studio")
#endif
#endif

/*---------------------------------------------------------------------------*/

static const char *pick_data_path(const char *arg_data_path)
{
#if defined(__EMSCRIPTEN__)
    return "/data";
#else
    static char dir[MAX_PATH];

    if (arg_data_path)
        return arg_data_path;

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    size_t requiredSize = MAX_PATH;
    char *data_env_dir;
    errno_t result = getenv_s(&requiredSize, 0, 0, "NEVERBALL_DATA");

    if (result == 0 && requiredSize != 0)
    {
        data_env_dir = (char *) malloc(requiredSize * sizeof (char));
        if (getenv_s(&requiredSize, data_env_dir, requiredSize,
                     "NEVERBALL_DATA") == 0)
            return data_env_dir;
    }
#else
    char *data_env_dir;
    if ((data_env_dir = getenv("NEVERBALL_DATA")))
        return data_env_dir;
#endif

    if (path_is_abs(CONFIG_DATA))
        return CONFIG_DATA;

    SAFECPY(dir, fs_base_dir());
    SAFECAT(dir, "/");
    SAFECAT(dir, CONFIG_DATA);

    return dir;
#endif
}

static const char *pick_home_path(void)
{
#ifdef _WIN32
    size_t requiredSize;
    static char path[MAX_PATH];

#if !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    char *userdir_env;
    errno_t result = getenv_s(&requiredSize, 0, 0, "NEVERBALL_USERDIR");

    if (result == 0 && requiredSize != 0)
    {
        userdir_env = (char *) malloc(requiredSize * sizeof (char));
        if (getenv_s(&requiredSize, userdir_env, requiredSize,
                     "NEVERBALL_USERDIR") == 0)
            return userdir_env;
    }
#else
    char *userdir_env;
    if ((userdir_env = getenv("NEVERBALL_USERDIR")))
        return userdir_env;
#endif

    if (SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, path) == S_OK)
    {
        static char gamepath[MAX_PATH];

        SAFECPY(gamepath, path);
        SAFECAT(gamepath, "\\My Games");

        if (dir_exists(gamepath) || dir_make(gamepath) == 0)
            return gamepath;

        return path;
    }
    else
        return fs_base_dir();
#elif defined(__WII__)
    return "/";
#else
    char *userdir_env;
    if ((userdir_env = getenv("NEVERBALL_USERDIR")))
        return userdir_env;

    const char *path;

    return (path = getenv("HOME")) ? path : fs_base_dir();
#endif
}

void config_paths(const char *arg_data_path)
{
    const char *data, *home, *user;

    /*
     * Scan in turn the game data and user directories for archives,
     * adding each archive to the search path.  Archives with names
     * further down the alphabet take precedence.  After each scan,
     * add the directory itself, taking precedence over archives added
     * so far.
     */

    /* Data directory. */

    data = pick_data_path(arg_data_path);

    fs_add_path_with_archives(data);

    /* User directory. */

    home = pick_home_path();
#if defined(__EMSCRIPTEN__)
    /* Force persistent store created during Module['preInit']. */
    user = strdup("/neverball");
#else
#if _WIN32
    user = concat_string(home, "\\", CONFIG_USER, NULL);
#else
    user = concat_string(home, "/", CONFIG_USER, NULL);
#endif
#endif

    /* Set up directory for writing, create if needed. */

    if (!fs_set_write_dir(user))
    {
        int success = 0;

        if (fs_set_write_dir(home))
            if (fs_mkdir(CONFIG_USER))
                if (fs_set_write_dir(user))
                    success = 1;

        if (success)
            log_printf("Write directory established at %s\n", user);
        else
        {
            log_errorf("Write directory not established at %s\n", user);
            fs_set_write_dir(NULL);
        }
    }

    fs_add_path_with_archives(user);

    free((void *) user);
    user = NULL;
}

void config_log_userpath()
{
    const char *user;

#if defined(__EMSCRIPTEN__)
    /* Force persistent store created during Module['preInit']. */
    user = strdup("/pennyball");
#else
    const char *home = pick_home_path();

#if _WIN32
    user = concat_string(home, "\\", CONFIG_USER, NULL);
#else
    user = concat_string(home, "/", CONFIG_USER, NULL);
#endif
#endif

    free((void *) user);
    user = NULL;
}

/*---------------------------------------------------------------------------*/
