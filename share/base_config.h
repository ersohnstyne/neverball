/*
 * Copyright (C) 2022 Microsoft / Neverball authors
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

#ifndef BASE_CONFIG_H
#define BASE_CONFIG_H

/*
 * This file contains some constants
 * And the way to access to directories
 */

#if _MSC_VER
#include <Windows.h>
#if _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

#include <stdio.h>

#include "log.h"

/*---------------------------------------------------------------------------*/

#ifndef VERSION
#define VERSION "unknown"
#endif

#ifndef CONFIG_DATA
#define CONFIG_DATA   "./data"        /* Game data directory */
#endif

#ifndef CONFIG_LOCALE
#if _WIN32
#define CONFIG_LOCALE "./po"          /* Game localisation */
#else
#define CONFIG_LOCALE "./locale"      /* Game localisation */
#endif
#endif

/* User config directory */
#ifndef CONFIG_USER
#ifdef _WIN32
#define CONFIG_USER   "Pennyball_2.1.0"
#else
#define CONFIG_USER   ".neverball_2.1.0"
#endif
#endif

/*
 * Global settings are stored in USER_CONFIG_FILE.  Replays are stored
 * in  USER_REPLAY_FILE.  These files  are placed  in the  user's home
 * directory as given by the HOME environment var.  If the config file
 * is deleted, it will be recreated using the defaults.
 */

#define USER_CONFIG_FILE    "neverballrc"
#define USER_REPLAY_FILE    "Last"
#define CHKP_REPLAY_FILE    "chkp-last-active"

/*---------------------------------------------------------------------------*/

#ifdef _WIN32
#define FMODE_RB "rb"
#define FMODE_WB "wb"
#else
#define FMODE_RB "r"
#define FMODE_WB "w"
#endif

#define AUDIO_BUFF_HI 2048
#define AUDIO_BUFF_LO 1024

#define JOY_VALUE(k) ((float) (k) / ((k) < 0 ? 32768 : 32767))

#if !defined(MAX_STR_BLOCKREASON)
#if _WIN32
#pragma message("Missing MAX_STR_BLOCKREASON! Redefined back to Microsoft Windows!")
#endif
#define MAX_STR_BLOCKREASON 256
#endif
#define MAXSTR MAX_STR_BLOCKREASON

#if !defined(MAX_PATH)
#if _WIN32
#pragma message("Missing MAX_PATH! Redefined back to Microsoft Windows!")
#endif
#define MAX_PATH 260
#endif
#define PATHMAX MAX_PATH

/*---------------------------------------------------------------------------*/

void config_paths(const char *);
void config_log_userpath();

/*---------------------------------------------------------------------------*/

#endif
