/*
 * Copyright (C) 2023 Microsoft / Valve / Neverball authors
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

/*
 * HACK: Remembering the code file differences:
 * Developers  who  programming  C++  can see more bedrock declaration
 * than C.  Developers  who  programming  C  can  see  few  procedural
 * declaration than  C++.  Keep  in  mind  when making  sure that your
 * extern code must associated. The valid file types are *.c and *.cpp,
 * so it's always best when making cross C++ compiler to keep both.
 * - Ersohn Styne
 */

#if NB_STEAM_API==1 && !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
#include <steam/steam_api.h>

/*---------------------------------------------------------------------------*/

namespace SteamMicroTxConf
{
    constexpr char ReqLink_Development[] =
                   "https://partner.steam-api.com/ISteamMicroTxnSandbox";
    constexpr char ReqLink_Production[] =
                   "https://partner.steam-api.com/ISteamMicroTxn";
    constexpr char Keys[6][256] = {
                   "",
                   "",
                   "",
                   "",
                   "",
                   "" };
}

#endif

/*---------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C"
{
#endif

int purchase_gems(int index)
{
    return 0;
}

#ifdef __cplusplus
}
#endif