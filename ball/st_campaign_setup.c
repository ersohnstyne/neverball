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

#if _WIN32 && __MINGW32__
#include <SDL3/SDL.h>
#else
#include <SDL.h>
#endif

#include <string.h>
#include <assert.h>

#include "lang_switchball.h"

#include "campaign.h"
#include "progress.h"
#include "common.h"
#include "config.h"
#include "geom.h"
#include "gui.h"
#include "vec3.h"
#include "audio.h"
#include "image.h"
#include "video.h"

#include "game_client.h"
#include "game_common.h"

#include "st_campaign_setup.h"
#include "st_set.h"
#include "st_title.h"
#include "st_shared.h"

#define TARGET_DATA_OUTPUT "C:/Users/ersoh/source/C++ Games/neverball-mingw/data_ziparchives"
#define CAMPAIGN_MAPC_ARGS " --campaign --skip_verify"

int intro_phase;
int donot_id;

/*---------------------------------------------------------------------------*/

static int setup_intro_gui(void)
{
    audio_music_fade_out(1.0f);

    int txt_id = gui_title_header(0, _(intro_phase == 2 ? "Your Campaign will set up." : "Hi there!"), GUI_MED, 0, 0);
    gui_layout(txt_id, 0, 0);
    return txt_id;
}

static int setup_intro_enter(struct state *st, struct state *prev)
{
    return setup_intro_gui();
}

/*---------------------------------------------------------------------------*/

static int setup_progressions_gui(void)
{
    int txt_id = gui_title_header(0, _("Compiling campaign levels"), GUI_MED, 0, 0);
    donot_id = gui_multi(0, _("Do not turn off your PC"), GUI_SML, gui_wht, gui_wht);
    gui_layout(txt_id, 0, 0);

    gui_set_rect(donot_id, GUI_TOP);
    gui_layout(donot_id, 0, -1);

    return txt_id;
}

static int setup_progressions_enter(struct state *st, struct state *prev)
{
    if (st == &st_title)
    {
        audio_music_fade_to(0.5f, "bgm/setup.ogg");
        intro_phase = 1;
    }
    return setup_progressions_gui();
}

/*---------------------------------------------------------------------------*/

static int setup_done_gui(void)
{
    audio_music_fade_out(1.0f);

    int txt_id = gui_title_header(0, _("Let's start!"), GUI_MED, 0, 0);
    gui_layout(txt_id, 0, 0);

    return txt_id;
}

static int setup_done_enter(struct state *st, struct state *prev)
{
    if (st == &st_title)
    {
        audio_music_fade_out(1.0f);
        intro_phase = 1;
    }
    return setup_done_gui();
}

/*---------------------------------------------------------------------------*/

static void setup_timer(int id, float dt)
{
    if (time_state() > 2)
    {
        if (curr_state() == &st_setup_intro)
        {
            if (intro_phase == 2)
                goto_state(&st_setup_progressions);
            else
            {
                intro_phase = 2;
                goto_state(&st_setup_intro);
            }
        }
        else if (curr_state() == &st_setup_progressions)
        {
            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/sky-01.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);
            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/sky-02.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);
            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/sky-03.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);
            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/sky-04.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);
            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/sky-05.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);
            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/sky-06.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);

            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/ice-01.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);
            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/ice-02.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);
            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/ice-03.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);
            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/ice-04.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);
            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/ice-05.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);
            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/ice-06.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);

            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/cave-01.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);
            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/cave-02.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);
            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/cave-03.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);
            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/cave-04.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);
            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/cave-05.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);
            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/cave-06.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);

            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/cloud-01.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);
            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/cloud-02.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);
            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/cloud-03.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);
            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/cloud-04.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);
            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/cloud-05.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);
            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/cloud-06.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);

            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/lava-01.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);
            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/lava-02.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);
            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/lava-03.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);
            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/lava-04.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);
            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/lava-05.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);
            system("mapc \"" TARGET_DATA_OUTPUT "/buildin-map-campaign/lava-06.cmap\" " "\"" TARGET_DATA_OUTPUT "\"" CAMPAIGN_MAPC_ARGS);

            goto_state(&st_setup_done);
        }
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        else if (curr_state() == &st_setup_done)
            goto_playmenu(MODE_CAMPAIGN);
#endif
    }
}

static void intro_fade(float alpha)
{
    if (donot_id)
        gui_set_alpha(donot_id, alpha, GUI_ANIMATION_S_CURVE);
}

/*---------------------------------------------------------------------------*/

struct state st_setup_intro = {
    setup_intro_enter,
    shared_leave,
    shared_paint,
    setup_timer,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

struct state st_setup_progressions = {
    setup_progressions_enter,
    shared_leave,
    shared_paint,
    setup_timer,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

struct state st_setup_done = {
    setup_done_enter,
    shared_leave,
    shared_paint,
    setup_timer,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};