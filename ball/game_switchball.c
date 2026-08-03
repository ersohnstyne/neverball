/*
 * Copyright (C) 2026 Microsoft / Neverball authors / Ersohn Styne
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

#include "common.h"
#include "config.h"

#include "game_switchball.h"

#if NB_HAVE_PB_BOTH!=1 || !_WIN32 || defined(__EMSCRIPTEN__)
#pragma message(__FILE__ ": Speeding alert is not available on Mac, Linux, web browser or other Pennyball builds.")
#endif

/*---------------------------------------------------------------------------*/

static int check_game_installed(const char *install_path_classic,
                                const char *install_path_epic,
                                const char *install_path_steam)
{
#if NB_HAVE_PB_BOTH==1 && _WIN32 && !defined(__EMSCRIPTEN__)
    char program_path[MAXSTR];
    char gameapp_path_classic[MAXSTR];
    char gameapp_path_epic[MAXSTR];
    char gameapp_path_steam[MAXSTR];

    SAFECPY(program_path, "C:\\Program Files\\");

    SAFECPY(gameapp_path_classic, program_path);
    SAFECPY(gameapp_path_epic,    program_path);
    SAFECPY(gameapp_path_steam,   program_path);

    SAFECAT(gameapp_path_classic, install_path_classic);

    SAFECAT(gameapp_path_epic, "Epic Games\\");
    SAFECAT(gameapp_path_epic, install_path_epic);

    SAFECAT(gameapp_path_steam, "Steam\\steamapps\\common\\");
    SAFECAT(gameapp_path_steam, install_path_steam);

    return (str_ends_with(gameapp_path_classic, ".exe") ? file_exists(gameapp_path_classic) : 0) ||
           (str_ends_with(gameapp_path_epic,    ".exe") ? file_exists(gameapp_path_epic)    : 0) ||
           (str_ends_with(gameapp_path_steam,   ".exe") ? file_exists(gameapp_path_steam)   : 0);
#else
    /* Not available on Mac, Linux, web browser or other Pennyball builds. */

    return 0;
#endif
}

/*---------------------------------------------------------------------------*/

static int allow_ticks;

static int   speeding_nowarning;
static int   speeding_detected;

static float pos_altitude;

int game_switchball_installed(void)
{
#if NB_HAVE_PB_BOTH==1 && _WIN32 && !defined(__EMSCRIPTEN__)
    return check_game_installed("Switchball\\switchball.exe",
                                "SwitchballHD\\switchball.exe",
                                "Switchball HD\\switchball.exe");
#else
    /* Not available on Mac, Linux, web browser or other Pennyball builds. */

    return 0;
#endif
}

void game_switchball_set_fixed_altitude(float y)
{
#if NB_HAVE_PB_BOTH==1 && _WIN32 && !defined(__EMSCRIPTEN__)
    if (!game_switchball_installed())
        return;

    speeding_nowarning = 0;
    speeding_detected  = 0;
    pos_altitude       = y;
#endif
}

void game_switchball_set_speeding(int e)
{
#if NB_HAVE_PB_BOTH==1 && _WIN32 && !defined(__EMSCRIPTEN__)
    if (speeding_nowarning)
        return;

    if (e && !game_switchball_installed())
        return;

    if (e && !config_get_d(CONFIG_ADVANCEDGAMING_GAMEPLAY_SWITCHBALL_DROPSPEEDING))
        return;

    speeding_detected = e;
#endif
}

void game_switchball_ignore_speeding(void)
{
#if NB_HAVE_PB_BOTH==1 && _WIN32 && !defined(__EMSCRIPTEN__)
    speeding_nowarning = 0;
    speeding_detected  = 0;
#endif
}

float game_switchball_altitude(void)
{
#if NB_HAVE_PB_BOTH==1 && _WIN32 && !defined(__EMSCRIPTEN__)
    return pos_altitude;
#else
    /* Not available on Mac, Linux, web browser or other Pennyball builds. */

    return 0.0f;
#endif
}

int game_switchball_speeding(void)
{
#if NB_HAVE_PB_BOTH==1 && _WIN32 && !defined(__EMSCRIPTEN__)
    return game_switchball_installed() && speeding_detected &&
           config_get_d(CONFIG_ADVANCEDGAMING_GAMEPLAY_SWITCHBALL_DROPSPEEDING);
#else
    /* Not available on Mac, Linux, web browser or other Pennyball builds. */

    return 0;
#endif
}

void game_switchball_toggle_ticks(int e)
{
#if NB_HAVE_PB_BOTH==1 && _WIN32 && !defined(__EMSCRIPTEN__)
    allow_ticks = e;
#endif
}

int game_switchball_haveticks(void)
{
#if NB_HAVE_PB_BOTH==1 && _WIN32 && !defined(__EMSCRIPTEN__)
    return allow_ticks;
#else
    /* Not available on Mac, Linux, web browser or other Pennyball builds. */

    return 1;
#endif
}

/*---------------------------------------------------------------------------*/
