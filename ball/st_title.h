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

#ifndef ST_TITLE_H
#define ST_TITLE_H

#include "lang.h"
#include "state.h"

#if _WIN32
/* Editions */

#define EDITION_HOME               0         /* Neverball Home Edition       */
#define EDITION_PRO                1         /* Neverball Pro Edition        */
#define EDITION_ENTERPRISE         2         /* Neverball Enterprise Edition */
#define EDITION_EDUCATION          3         /* Neverball Education Edition  */
#define EDITION_SERVER_ESSENTIAL   4         /* Neverball Server Essential   */
#define EDITION_SERVER_STANDARD    5         /* Neverball Server Standard    */
#define EDITION_SERVER_DATACENTER  6         /* Neverball Server Datacenter  */

#if WINAPI_FAMILY == WINAPI_FAMILY_PC_APP || \
    WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP
#define EDITION_CURRENT \
        EDITION_PRO                          /* Current Edition              */
#elif WINAPI_FAMILY == WINAPI_FAMILY_SERVER
#define EDITION_CURRENT \
        EDITION_SERVER_STANDARD              /* Current Server Edition       */
#else
#error Security compilation error: No Windows Editions supported
#endif
#endif

/* TODO: Uncomment, if you have an community project edition */
//#define COMMUNITY_EDITION

/*---------------------------------------------------------------------------*/

/* For Windows PC Gaming */

#define TITLE_PLATFORM_WINDOWS _("Windows")

/* For Mac OS X Gaming */

#define TITLE_PLATFORM_OSX_CATALINA  _("Mac OS X Catalina")

/* For iOS Gaming */

#define TITLE_PLATFORM_IPAD      _("iPad")
#define TITLE_PLATFORM_IPAD_AIR  _("iPad Air")
#define TITLE_PLATFORM_IPAD_PRO  _("iPad Pro")
#define TITLE_PLATFORM_IPHONE    _("iPhone")
#define TITLE_PLATFORM_IPHONE_X  _("iPhone X")

/* For Console Games */

#define TITLE_PLATFORM_PS          _("PlayStation")
#define TITLE_PLATFORM_STEAMDECK   _("Steam Deck")
#define TITLE_PLATFORM_XBOX_360    _("Xbox 360")
#define TITLE_PLATFORM_XBOX_ONE    _("Xbox One")
#define TITLE_PLATFORM_XBOX_ONE_X  _("Xbox One X")

/* For Linux and Unix Gaming */

#define TITLE_PLATFORM_CINMAMON   _("Linux Mint Cinnamon")
#define TITLE_PLATFORM_FEDORA     _("Fedora")
#define TITLE_PLATFORM_KUBUNTU    _("Kubuntu")
#define TITLE_PLATFORM_MINT       _("Linux Mint")
#define TITLE_PLATFORM_RASPBERRY  _("Raspberry")
#define TITLE_PLATFORM_TESSA      _("Linux Mint Tessa")
#define TITLE_PLATFORM_UNIX       _("Ubuntu")

/* For Nintendo */

#define TITLE_PLATFORM_SWITCH  _("Nintendo Switch")
#define TITLE_PLATFORM_SWITCH2 _("Nintendo Switch 2")
#define TITLE_PLATFORM_WII     _("Wii")
#define TITLE_PLATFORM_WIIU    _("Wii U")

/* For Handsets when in aircraft */

#define TITLE_PLATFORM_AIRFRANCE _("Air France")
#define TITLE_PLATFORM_ANA       _("ANA")
#define TITLE_PLATFORM_ASIANA    _("Asiana Airlines")
#define TITLE_PLATFORM_CATHAY    _("Cathay Pacific")
#define TITLE_PLATFORM_EMIRATES  _("Emirates")
#define TITLE_PLATFORM_ETIHAD    _("Etihad")
#define TITLE_PLATFORM_SAUDIA    _("Saudia Airlines")
#define TITLE_PLATFORM_THAI      _("Thai")

/* For Amazon Games */

#define TITLE_PLATFORM_ANAZON_FIRE  _("Amazon Fire")

extern struct state st_title;

int load_title_background(void);

int title_check_wgcl(void);

#endif
