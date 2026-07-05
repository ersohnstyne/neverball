/*
 * Copyright (C) 2026 Microsoft / Neverball authors / Ersohn Styne
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

#ifndef ST_PLAY_SYNC_H
#define ST_PLAY_SYNC_H

/* Synchronized via st_play.c and st_demo.c */

#pragma message(__FILE__": This code file was synced belonging the header file.")

#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

#include "hud.h"
#include "set.h"
#include "demo.h"
#include "progress.h"
#include "audio.h"
#include "config.h"
#include "video.h"
#include "cmd.h"
#include "key.h"

#include "geom.h"
#include "vec3.h"

#include "game_draw.h"
#include "game_common.h"
#include "game_server.h"
#include "game_proxy.h"
#include "game_client.h"

#define ST_PLAY_SYNC_SMOOTH_FIX_TIMER(time_total) \
    if (config_get_d(CONFIG_SMOOTH_FIX) &&        \
        video_perf() < NB_FRAMERATE_MIN) {        \
        time_total += dt;                         \
        if (time_total >= 10) {                   \
            config_set_d(CONFIG_SMOOTH_FIX,       \
                         config_get_d(CONFIG_FORCE_SMOOTH_FIX));\
            time_total = 0; } }                   \
    else time_total = 0

#endif