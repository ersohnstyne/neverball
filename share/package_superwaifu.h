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

#ifndef PACKAGE_SUPERWAIFU_H
#define PACKAGE_SUPERWAIFU_H

void package_superwaifu_init(void);
void package_superwaifu_quit(void);

/*
 * HACK: Should be redirected, if any other among things instead of
 * package_superwaifu_game_installed():
 *
 * * game_common_superwaifu_game_installed()
 */
int package_superwaifu_game_installed(void);

#define game_common_superwaifu_game_installed package_superwaifu_game_installed

#endif