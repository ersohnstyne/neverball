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

#ifndef CONFIG_WGCL_H
#define CONFIG_WGCL_H

#include "state.h"

#ifdef __EMSCRIPTEN__
#define CONFIG_WGCL_GAME_OPTIONS
#endif

extern int WGCL_GameOptions_Exists;

void WGCL_InitGameOptions(struct state *returnable);
void WGCL_QuitGameOptions(void);
void WGCL_BackToGameOptions(const char *);

void WGCL_CallClassicPackagesUI(void);

void WGCL_CallSaveDataUI(void);
void WGCL_CallHighscoreDataUI_Campaign(void);
void WGCL_CallHighscoreDataUI_LevelSet(void);

void WGCL_TryEraseHighscoreFile_Campaign(void);
void WGCL_TryEraseHighscoreFile_LevelSet(const char *);

void WGCL_LoadGameSystemSettings(void);
void WGCL_SaveGameSystemSettings(void);

void WGCL_TryFormatUserData(void);

#endif