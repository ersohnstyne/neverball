#ifndef ST_TITLE_H
#define ST_TITLE_H

#include "lang.h"
#include "state.h"

#if _WIN32
/* Editions */

#define EDITION_HOME 0              /* Neverball Home Edition       */
#define EDITION_PRO 1               /* Neverball Pro Edition        */
#define EDITION_ENTERPRISE 2        /* Neverball Enterprise Edition */
#define EDITION_EDUCATION 3         /* Neverball Education Edition  */
#define EDITION_SERVER_ESSENTIAL 4  /* Neverball Server Essential    */
#define EDITION_SERVER_STANDARD 5   /* Neverball Server Standard    */
#define EDITION_SERVER_DATACENTER 6 /* Neverball Server Datacenter  */

#if WINAPI_FAMILY == WINAPI_FAMILY_PC_APP || WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP
#define EDITION_CURRENT EDITION_PRO /* Current Edition */
#elif WINAPI_FAMILY == WINAPI_FAMILY_SERVER
#define EDITION_CURRENT EDITION_SERVER_STANDARD // Current Server Edition
#else
#error No Windows Editions supported
#endif
#endif

/* Uncomment, if you have an community project edition */
//#define COMMUNITY_EDITION

/*---------------------------------------------------------------------------*/

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

/* For Handsets when in aircraft */

#define TITLE_PLATFORM_EMIRATES _("Emirates")
#define TITLE_PLATFORM_ETIHAD   _("Etihad")
#define TITLE_PLATFORM_THAI     _("Thai")

/* For Amazon Games */

#define TITLE_PLATFORM_ANAZON_FIRE _("Amazon Fire")

extern struct state st_title;

#endif
