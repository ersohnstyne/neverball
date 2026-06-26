/*
 * Copyright (C) 2026 Microsoft / Neverball authors / Jānis Rūcis
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

#include "common.h"

#include "package_superwaifu.h"

/*---------------------------------------------------------------------------*/

static int superwaifu_game_installed = 0;

void package_superwaifu_init(void) {
    if (superwaifu_game_installed)
        package_superwaifu_quit();

#ifdef _WIN32
    char program_path[MAXSTR];
    char gameapp_path_classic[MAXSTR];
    char gameapp_path_epic[MAXSTR];
    char gameapp_path_steam[MAXSTR];

    SAFECPY(program_path, "C:\\Program Files\\");

    SAFECPY(gameapp_path_steam, program_path);
    SAFECAT(gameapp_path_steam, "Steam\\steamapps\\common\\Super Waifu Ball\\SuperWaifuBall.exe");

    superwaifu_game_installed = str_ends_with(gameapp_path_steam, ".exe") ? file_exists(gameapp_path_classic) : 0;
#else
    /* Not available outside Windows OS. */
#endif
}

void package_superwaifu_quit(void) {
    superwaifu_game_installed = 0;
}

int package_superwaifu_game_installed(void) {
	return superwaifu_game_installed;
}

/*---------------------------------------------------------------------------*/