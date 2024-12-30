/*
 * Copyright (C) 2024 Microsoft / Neverball authors
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

#if __cplusplus
extern "C" {
#endif

#include "game_payment.h"
#include "common.h"
#include "fetch.h"
#include "fs.h"
#include "log.h"

#if __cplusplus
}
#endif

#include <stdbool.h>

/*
 * Premium: pennyball.stynegame.de
 * Legacy IAP: play.neverball.org
 */
#define GAMEIAP_CURRDOMAIN "pennyball.stynegame.de"

/*---------------------------------------------------------------------------*/

void game_payment_browse(int index)
{
#if ENABLE_IAP!=0 && \
    !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
    char linkstr[MAXSTR];

#if defined(__EMSCRIPTEN__)
    sprintf(linkstr, "https://%s/iap/gems/overview?packid=%d",
            GAMEIAP_CURRDOMAIN, index + 1);

    EM_ASM({ window.open($0); }, linkstr);
#else
#if _WIN32 && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(linkstr, MAXSTR,
#else
    sprintf(linkstr,
#endif
#if _WIN32
            "explorer \"https://%s/iap/gems/overview?packid=%d\"",
#elif defined(__APPLE__)
            "open \"https://%s/iap/gems/overview?packid=%d\"",
#elif defined(__linux__)
            "x-www-browser \"https://%s/iap/gems/overview?packid=%d\"",
#endif
            GAMEIAP_CURRDOMAIN, index + 1);

    system(linkstr);
#endif
#endif
}

#if __cplusplus
}
#endif
