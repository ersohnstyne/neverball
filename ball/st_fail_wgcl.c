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
#include "progress.h"

#include "st_level.h"

/*---------------------------------------------------------------------------*/

/*
 * This file implements from WGCL's javascript files, that supports
 * modern browser.
 */

void WGCL_ST_FAIL_StartRespawn(void)
{
    if (checkpoints_load() && progress_same_avail() && !progress_dead())
    {
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
#ifdef __EMSCRIPTEN__
    if (demo_exists(fileName))
        EM_ASM({ CoreLauncherGameplay_ST_FAIL_Classic_RequestOverwriteReplay(UTF8ToString($0)); }, fileName);
    else if (demo_saved()) demo_rename(fileName);
#endif
}

void WGCL_ST_FAIL_StartOverwriteReplay(const char *fileName)
{
    if (demo_saved()) demo_rename(fileName);
}

void WGCL_ST_FAIL_StartRestart(void)
{
    if (progress_same_avail() && !progress_dead())
    {
        checkpoints_stop();
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
    goto_exit();
}

/*---------------------------------------------------------------------------*/
