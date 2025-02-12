/*
 * Copyright (C) 2025 Microsoft / Neverball authors / Jānis Rūcis
 *
 * NEVERPUTT is  free software; you can redistribute  it and/or modify
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

#include "state.h"

#pragma region Title Edition Text

/* For Windows PC Gaming */

#define TITLE_PLATFORM_WINDOWS _("Windows")

/* For Mac OS X Gaming */

#define TITLE_PLATFORM_OSX_CATALINA  _("Mac OS X Catalina")

/* For iOS Gaming */

#define TITLE_PLATFORM_IPAD     _("iPad")
#define TITLE_PLATFORM_IPAD_PRO _("iPad Pro")
#define TITLE_PLATFORM_IPHONE   _("iPhone")
#define TITLE_PLATFORM_IPHONE_X _("iPhone X")

/* For Console Games */

#define TITLE_PLATFORM_PS         _("PlayStation")
#define TITLE_PLATFORM_STEAMDECK  _("Steam Deck")
#define TITLE_PLATFORM_XBOX_360   _("Xbox 360")
#define TITLE_PLATFORM_XBOX_ONE   _("Xbox One")
#define TITLE_PLATFORM_XBOX_ONE_X _("Xbox One X")

/* For Linux and Unix Gaming */

#define TITLE_PLATFORM_CINMAMON  _("Linux Mint Cinnamon")
#define TITLE_PLATFORM_FEDORA    _("Fedora")
#define TITLE_PLATFORM_KUBUNTU   _("Kubuntu")
#define TITLE_PLATFORM_MINT      _("Linux Mint")
#define TITLE_PLATFORM_RASPBERRY _("Raspberry")
#define TITLE_PLATFORM_TESSA     _("Linux Mint Tessa")
#define TITLE_PLATFORM_UNIX      _("Ubuntu")

/* For Nintendo */

#define TITLE_PLATFORM_SWITCH _("Nintendo Switch")

/* For Google Games */

#define TITLE_PLATFORM_STADIA _("Stadia")

/* For Handsets when in aircraft */

#define TITLE_PLATFORM_EMIRATES _("Emirates")
#define TITLE_PLATFORM_ETIHAD   _("Etihad")
#define TITLE_PLATFORM_THAI     _("Thai")

/* For Amazon Games */

#define TITLE_PLATFORM_ANAZON_FIRE _("Amazon Fire")

#pragma endregion

extern struct state st_title;
extern struct state st_next;

int goto_pause(int);

#endif
