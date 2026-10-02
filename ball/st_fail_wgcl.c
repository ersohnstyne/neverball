/*
 * Copyright (C) 2026 Microsoft / Neverball authors / Jānis Rūcis
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

#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

#if NB_HAVE_PB_BOTH==1
#include "account.h"
#include "account_wgcl.h"
#include "campaign.h" /* New: Campaign */
#endif

#ifdef MAPC_INCLUDES_CHKP
#include "checkpoints.h" /* New: Checkpoints */
#endif

#ifdef CONFIG_INCLUDES_ACCOUNT
#include "powerup.h"
#include "mediation.h"
#endif

#include "demo.h"
#include "demo_dir.h"
#include "progress.h"

#include "game_server.h"

#include "st_fail.h"
#include "st_level.h"

/*---------------------------------------------------------------------------*/

/*
 * This file implements WGCL's javascript files, that supports
 * modern client browser.
 */

int WGCL_ST_FAIL_CheckOverlayElement(void)
{
#ifdef __EMSCRIPTEN__
#if NB_HAVE_PB_BOTH==1
    return EM_ASM_INT({
        const elem_overlay = document.getElementById("wgcl_ui_newmenu_st_fail_overlay");
        return elem_overlay != undefined && elem_overlay != null &&
               CoreLauncherGameplay_ST_FAIL_ElemState ? 1 : 0;
    });
#else
    return EM_ASM_INT({
        const elem_overlay = document.getElementById("wgcl_ui_newmenu_st_fail_overlay");
        return elem_overlay != undefined && elem_overlay != null ? 1 : 0;
    });
#endif
#else
    /* Not available on outside Web Browser. */

    return 0;
#endif
}

void WGCL_ST_FAIL_StartRespawn(void)
{
    if (WGCL_ST_FAIL_CheckOverlayElement() && game_server_state() &&
        progress_same_avail() && !progress_dead()) {
#if NB_HAVE_PB_BOTH==1 && \
    defined(CONFIG_INCLUDES_ACCOUNT) && defined(ENABLE_POWERUP)
        powerup_stop();
#endif
        if (progress_same())
            goto_play_level();
    }
}

void WGCL_ST_FAIL_StartSaveReplay(const char *fileName)
{
    if (WGCL_ST_FAIL_CheckOverlayElement() && game_server_state()) {
#ifdef __EMSCRIPTEN__
        if (demo_exists(fileName))
            EM_ASM({
                CoreLauncherGameplay_ST_FAIL_Classic_RequestOverwriteReplay(UTF8ToString($0));
            }, fileName);
        else if (demo_saved()) {
            demo_rename(fileName);

#ifdef __EMSCRIPTEN__
            EM_ASM({ CoreLauncherGameplay_ST_FAIL_StaticInt_SaveLocked = true; });
#endif
        }
#endif
    }
}

void WGCL_ST_FAIL_StartOverwriteReplay(const char *fileName)
{
    if (WGCL_ST_FAIL_CheckOverlayElement() && game_server_state() &&
        demo_saved()) {
        demo_rename(fileName);

#ifdef __EMSCRIPTEN__
        EM_ASM({ CoreLauncherGameplay_ST_FAIL_StaticInt_SaveLocked = true; });
#endif
    }
}

void WGCL_ST_FAIL_StartRestart(void)
{
    if (WGCL_ST_FAIL_CheckOverlayElement() && game_server_state() &&
        progress_same_avail() && !progress_dead()) {
#ifdef MAPC_INCLUDES_CHKP
        checkpoints_stop();
#endif
#if NB_HAVE_PB_BOTH==1 && \
    defined(CONFIG_INCLUDES_ACCOUNT) && defined(ENABLE_POWERUP)
        powerup_stop();
#endif
        if (progress_same())
            goto_play_level();
    }
}

void WGCL_ST_FAIL_CloseLevel(void)
{
    if (!WGCL_ST_FAIL_CheckOverlayElement()) return;

#ifdef MAPC_INCLUDES_CHKP
    checkpoints_stop();
#endif
#if NB_HAVE_PB_BOTH==1 && \
    defined(CONFIG_INCLUDES_ACCOUNT) && defined(ENABLE_POWERUP)
    powerup_stop();
#endif
    if (game_server_state())
        goto_exit();
}

/*---------------------------------------------------------------------------*/
