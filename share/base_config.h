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

#ifndef BASE_CONFIG_H
#define BASE_CONFIG_H

/*
 * This file contains some constants
 * And the way to access to directories
 */

#if _MSC_VER
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#if _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

/* Shall the Xbox live begin? */
#define NB_PB_WITH_XBOX 1
#include <XInput.h>
#endif

#include <stdio.h>

#include "dbg_config.h"

/*---------------------------------------------------------------------------*/

#ifndef CONFIG_DATA
#ifdef _WIN32
#define CONFIG_DATA   ".\\data"       /* Game data directory */
#else
#define CONFIG_DATA   "./data"        /* Game data directory */
#endif
#endif

#ifndef CONFIG_LOCALE
#ifdef _WIN32
#define CONFIG_LOCALE ".\\locale"     /* Game localisation */
#else
#define CONFIG_LOCALE "./locale"     /* Game localisation */
#endif
#endif

/* User config directory */
#ifndef CONFIG_USER
#if NDEBUG
#ifdef _WIN32
#define CONFIG_USER   "Neverball"
#else
#define CONFIG_USER   ".neverball"
#endif
#endif
#ifdef _WIN32
#define CONFIG_USER   "Neverball-dev"
#else
#define CONFIG_USER   ".neverball-dev"
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

#define AUDIO_BUFF_HI 2048
#define AUDIO_BUFF_LO 1024

#define JOY_VALUE(k) ((float) (k) / ((k) < 0 ? 32768 : 32767))

#if !defined(MAX_STR_BLOCKREASON)
#if _WIN32 && !_MSC_VER
#error This preprocessor is superceded with WinUser.h include headers. \
       Consider using MAX_STR_BLOCKREASON from Windows.h with IDE \
       or build tools for Microsoft Visual Studio Community instead.
#endif
#define MAX_STR_BLOCKREASON 256
#endif

#if !defined(MAX_PATH)
#if _WIN32 && !_MSC_VER
#error This preprocessor is superceded with minwindef.h include headers. \
       Consider using MAX_PATH from Windows.h with IDE or build tools \
       for Microsoft Visual Studio Community instead.
#endif
#define MAX_PATH 260
#endif

#define MAXSTR MAX_STR_BLOCKREASON
#define PATHMAX MAX_PATH

/*---------------------------------------------------------------------------*/

void config_paths(const char *);
void config_log_userpath();

/*---------------------------------------------------------------------------*/

#endif
