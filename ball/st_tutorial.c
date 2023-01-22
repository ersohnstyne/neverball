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

#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
#include "console_control_gui.h"
#endif

#include <assert.h>

#include "lang_switchball.h"

#include "campaign.h"
#include "audio.h"
#include "common.h"
#include "config.h"
#include "geom.h"
#include "gui.h"
#include "hud.h"
#include "lang.h"
#include "progress.h"
#include "set.h"

#include "game_client.h"
#include "game_common.h"

#include "st_tutorial.h"
#include "st_play.h"
#include "st_shared.h"

/*---------------------------------------------------------------------------*/

enum {
    TUTORIAL_TOGGLE = GUI_LAST
};

static struct state *st_continue;

const char tutorial_title[][24] =
{
    "",
    TUTORIAL_1_TITLE,
    TUTORIAL_2_TITLE,
    TUTORIAL_3_TITLE,
    TUTORIAL_4_TITLE,
    TUTORIAL_5_TITLE,
    TUTORIAL_6_TITLE,
    TUTORIAL_7_TITLE,
    TUTORIAL_8_TITLE,
    TUTORIAL_9_TITLE,
    TUTORIAL_10_TITLE,
};

const char tutorial_desc[][128] =
{
    "",
    TUTORIAL_1_DESC,
    TUTORIAL_2_DESC,
    TUTORIAL_3_DESC,
    TUTORIAL_4_DESC,
    TUTORIAL_5_DESC,
    TUTORIAL_6_DESC,
    TUTORIAL_7_DESC,
    TUTORIAL_8_DESC,
    TUTORIAL_9_DESC,
    TUTORIAL_10_DESC,
};

const char tutorial_desc_xbox[][128] =
{
    "",
    TUTORIAL_1_DESC_XBOX,
    TUTORIAL_2_DESC,
    TUTORIAL_3_DESC_XBOX,
    TUTORIAL_4_DESC_XBOX,
    TUTORIAL_5_DESC,
    TUTORIAL_6_DESC,
    TUTORIAL_7_DESC,
    TUTORIAL_8_DESC,
    TUTORIAL_9_DESC,
    TUTORIAL_10_DESC,
};

static int tutorial_index = 0;
static int hint_index = 0;

static int tutorial_before_play = 0;
static int hint_before_play = 0;

/*---------------------------------------------------------------------------*/

int tutorial_check(void)
{
    if (config_get_d(CONFIG_ACCOUNT_TUTORIAL) == 0
        || curr_mode() == MODE_CHALLENGE
        || curr_mode() == MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        || curr_mode() == MODE_HARDCORE
#endif
        )
    {
        if (config_get_d(CONFIG_ACCOUNT_HINT) == 1)
        {
            return hint_check();
        }
        return 0;
    }

    assert(config_get_d(CONFIG_ACCOUNT_TUTORIAL) == 1);

    const char *ln = level_name(curr_level());
    const char *sn = set_name(curr_set());

    if (!campaign_used())
    {
        assert(ln && sn);

        if (ln && sn)
        {
            if (strcmp(sn, _("Neverball Easy")) == 0)
            {
                if (strcmp(ln, "1") == 0)
                {
                    goto_tutorial_before_play(1);
                    return 1;
                }
                if (strcmp(ln, "2") == 0)
                {
                    goto_tutorial_before_play(3);
                    return 1;
                }
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
                if (strcmp(ln, "4") == 0 && current_platform == PLATFORM_PC)
#else
                if (strcmp(ln, "4") == 0)
#endif
                {
                    goto_tutorial_before_play(2);
                    return 1;
                }
            }
            else if (strcmp(sn, _("Neverball Medium")) == 0)
            {
                if (strcmp(ln, "1") == 0)
                {
                    goto_tutorial_before_play(5);
                    return 1;
                }
                if (strcmp(ln, "14") == 0)
                {
                    goto_tutorial_before_play(7);
                    return 1;
                }
            }
            else if (strcmp(sn, _("Neverball Hard")) == 0)
            {
                if (strcmp(ln, "IV") == 0)
                {
                    goto_tutorial_before_play(6);
                    return 1;
                }
            }
        }
    }
    
    /* No tutorial available, use hint instead! */

    if (config_get_d(CONFIG_ACCOUNT_HINT) == 1)
    {
        return hint_check();
    }

    return 0;
}

int goto_tutorial(int idx)
{
    tutorial_before_play = 0;
    tutorial_index = idx;

    return goto_state(&st_tutorial);
}

int goto_tutorial_before_play(int idx)
{
    tutorial_before_play = 1;
    tutorial_index = idx;

    st_continue = &st_play_ready;

    return goto_state_full(&st_tutorial, GUI_ANIMATION_W_CURVE, GUI_ANIMATION_E_CURVE, 0);
}

static int tutorial_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
    case TUTORIAL_TOGGLE:
        config_set_d(CONFIG_ACCOUNT_TUTORIAL, !config_get_d(CONFIG_ACCOUNT_TUTORIAL));
        return goto_state_full(&st_tutorial, 0, 0, 1);
        break;
    }

    if (config_get_d(CONFIG_ACCOUNT_HINT))
    {
        if (hint_check())
            return 1;
    }

    video_set_grab(!tutorial_before_play);
    return goto_state_full(st_continue, GUI_ANIMATION_E_CURVE, GUI_ANIMATION_E_CURVE, 0);
}

static int tutorial_enter(struct state *st, struct state *prev)
{
    if (!tutorial_before_play)
        if (prev == &st_play_loop)
            st_continue = prev;

    video_clr_grab();

    int id, jd;
    if ((id = gui_vstack(0)))
    {
        gui_label(id, _(tutorial_title[tutorial_index]), GUI_MED, 0, 0);
        gui_space(id);
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
        if (current_platform == PLATFORM_PC)
            gui_multi(id, _(tutorial_desc[tutorial_index]), GUI_SML, gui_wht, gui_wht);
        else
#endif
            gui_multi(id, _(tutorial_desc_xbox[tutorial_index]), GUI_SML, gui_wht, gui_wht);
        
        gui_space(id);
        if ((jd = gui_harray(id)))
        {
            gui_state(jd, config_get_d(CONFIG_ACCOUNT_TUTORIAL) ? _("Tutorial Off") : _("Tutorial On"), GUI_SML, TUTORIAL_TOGGLE, 0);
            gui_start(jd, _("OK"), GUI_SML, GUI_BACK, 0);
        }
    }

    gui_layout(id, 0, 0);

    return id;
}

static void tutorial_paint(int id, float t)
{
    game_client_draw(0, t);

    gui_paint(id);
#ifndef __EMSCRIPTEN__
    if (xbox_show_gui())
        xbox_control_death_gui_paint();
#endif
}

static void tutorial_timer(int id, float dt)
{
    if (tutorial_before_play || hint_before_play)
        geom_step(dt);

    gui_timer(id, dt);
}

static int tutorial_keybd(int c, int d)
{
    if (d)
    {
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
        if (c == KEY_EXIT && current_platform == PLATFORM_PC)
#else
        if (c == KEY_EXIT)
#endif
            return tutorial_action(GUI_BACK, 0);
    }
    return 1;
}

static int tutorial_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return tutorial_action(gui_token(active), gui_value(active));
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

enum {
    HINT_TOGGLE = GUI_LAST
};

const char hint_desc[][128] =
{
    "",
    HINT_BUNNY_SLOPE_TEXT,
    HINT_HAZARDOUS_CLIMB_TEXT,
    HINT_HALFPIPE_TEXT,
    HINT_UPWARD_SPIRAL_TEXT
};

/*---------------------------------------------------------------------------*/

int hint_check(void)
{
    if (config_get_d(CONFIG_ACCOUNT_HINT) == 0
        || curr_mode() == MODE_CHALLENGE
        || curr_mode() == MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        || curr_mode() == MODE_HARDCORE
#endif
        )
        return 0;

    assert(config_get_d(CONFIG_ACCOUNT_HINT) == 1);

    const char *ln = level_name(curr_level());
    const char *sn = set_name(curr_set());

    if (!campaign_used())
    {
        assert(ln && sn);

        if (ln && sn)
        {
            if (strcmp(sn, _("Neverball Easy")) == 0)
            {
                if (strcmp(ln, "13") == 0)
                {
                    goto_hint_before_play(3);
                    return 1;
                }
            }
            else if (strcmp(sn, _("Neverball Medium")) == 0)
            {
                if (strcmp(ln, "9") == 0)
                {
                    goto_hint_before_play(1);
                    return 1;
                }
            }
            else if (strcmp(sn, _("Neverball Hard")) == 0)
            {
            }
        }
    }

    /* No hints available! */

    return 0;
}

int goto_hint(int idx)
{
    hint_before_play = 0;
    hint_index = idx;

    return goto_state(&st_hint);
}

int goto_hint_before_play(int idx)
{
    hint_before_play = 1;
    hint_index = idx;

    st_continue = &st_play_ready;

    return goto_state_full(&st_hint, GUI_ANIMATION_W_CURVE, GUI_ANIMATION_E_CURVE, 0);
}

static int hint_action(int tok, int val)
{
    audio_play(AUD_MENU, 1.0f);
    switch (tok)
    {
    case HINT_TOGGLE:
        config_set_d(CONFIG_ACCOUNT_TUTORIAL, !config_get_d(CONFIG_ACCOUNT_TUTORIAL));
        return goto_state_full(&st_hint, 0, 0, 1);
        break;
    }

    video_set_grab(!hint_before_play);
    return goto_state_full(st_continue, GUI_ANIMATION_E_CURVE, GUI_ANIMATION_E_CURVE, 0);
}

static int hint_enter(struct state *st, struct state *prev)
{
    if (!tutorial_before_play)
        if (prev == &st_play_loop)
            st_continue = prev;

    video_clr_grab();

    int id, jd;
    if ((id = gui_vstack(0)))
    {
        gui_label(id, _("Hint"), GUI_MED, 0, 0);
        gui_space(id);
        gui_multi(id, _(hint_desc[hint_index]), GUI_SML, gui_wht, gui_wht);

        gui_space(id);
        if ((jd = gui_harray(id)))
        {
            gui_state(jd, config_get_d(CONFIG_ACCOUNT_TUTORIAL) ? _("Hint Off") : _("Hint On"), GUI_SML, HINT_TOGGLE, 0);
            gui_start(jd, _("OK"), GUI_SML, GUI_BACK, 0);
        }
    }

    gui_layout(id, 0, 0);

    return id;
}

static int hint_keybd(int c, int d)
{
    if (d)
    {
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
        if (c == KEY_EXIT && current_platform == PLATFORM_PC)
#else
        if (c == KEY_EXIT)
#endif
            return hint_action(GUI_BACK, 0);
    }
    return 1;
}

static int hint_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return hint_action(gui_token(active), gui_value(active));
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

struct state st_tutorial = {
    tutorial_enter,
    shared_leave,
    tutorial_paint,
    tutorial_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    tutorial_keybd,
    tutorial_buttn
};

struct state st_hint = {
    hint_enter,
    shared_leave,
    tutorial_paint,
    tutorial_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    hint_keybd,
    hint_buttn
};