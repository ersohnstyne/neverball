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

#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

/*
 * HACK: Used with console version
 */
#include "console_control_gui.h"

#include "lang_switchball.h"

#include "campaign.h"
#include "audio.h"
#include "common.h"
#include "config.h"
#include "geom.h"
#include "gui.h"
#include "transition.h"
#include "hud.h"
#include "lang.h"
#include "progress.h"
#include "set.h"
#include "key.h"

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

const char tutorial_desc_touch[][128] =
{
    "",
    TUTORIAL_1_DESC_TOUCH,
    TUTORIAL_2_DESC_TOUCH,
    TUTORIAL_3_DESC_TOUCH,
    TUTORIAL_4_DESC_TOUCH,
    TUTORIAL_5_DESC,
    TUTORIAL_6_DESC,
    TUTORIAL_7_DESC,
    TUTORIAL_8_DESC,
    TUTORIAL_9_DESC,
    TUTORIAL_10_DESC,
};

const char tutorial_desc_touch_gyroscope[][160] =
{
    "",
    TUTORIAL_1_DESC_TOUCH_GYROSCOPE,
    TUTORIAL_2_DESC_TOUCH,
    TUTORIAL_3_DESC_TOUCH,
    TUTORIAL_4_DESC_TOUCH,
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

const char tutorial_desc_wii[][128] =
{
    "",
    TUTORIAL_1_DESC_WII,
    TUTORIAL_2_DESC,
    TUTORIAL_3_DESC_WII,
    TUTORIAL_4_DESC_WII,
    TUTORIAL_5_DESC,
    TUTORIAL_6_DESC,
    TUTORIAL_7_DESC,
    TUTORIAL_8_DESC,
    TUTORIAL_9_DESC,
    TUTORIAL_10_DESC,
};

const char tutorial_desc_wiiu[][128] =
{
    "",
    TUTORIAL_1_DESC_WIIU,
    TUTORIAL_2_DESC,
    TUTORIAL_3_DESC_WII,
    TUTORIAL_4_DESC_WII,
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

static int toggle_id;

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
            return hint_check();

        return 0;
    }

    const char *ln = level_name(curr_level());
    const char *sn = set_name(curr_set());

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    if (!campaign_used())
#endif
    {
        if (ln && *ln && sn && *sn)
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
                if (strcmp(ln, "3") == 0)
                {
                    goto_tutorial_before_play(4);
                    return 1;
                }
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
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
        return hint_check();

    return 0;
}

int goto_tutorial(int idx)
{
    tutorial_before_play = 0;
    tutorial_index       = idx;

    return goto_state(&st_tutorial);
}

int goto_tutorial_before_play(int idx)
{
    tutorial_before_play = 1;
    tutorial_index       = idx;

    st_continue = &st_play_ready;

    return goto_state(&st_tutorial);
}

static int tutorial_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case TUTORIAL_TOGGLE:
            config_tgl_d(CONFIG_ACCOUNT_TUTORIAL);
            gui_set_label(toggle_id, config_get_d(CONFIG_ACCOUNT_TUTORIAL) ?
                                     _("Tutorial Off") :
                                     _("Tutorial On"));
            return 1;
    }

    /* Next phase: hints */

    if (config_get_d(CONFIG_ACCOUNT_HINT) && hint_check())
        return 1;

    video_set_grab(1);
    return exit_state(st_continue);
}

static int tutorial_enter(struct state *st, struct state *prev, int intent)
{
    if (!tutorial_before_play)
        if (prev == &st_play_loop)
            st_continue = prev;

    video_clr_grab();

    int id, jd;
    if ((id = gui_vstack(0)))
    {
        gui_label(id, _(tutorial_title[tutorial_index]), GUI_MED, GUI_COLOR_DEFAULT);
        gui_space(id);

#if NB_HAVE_PB_BOTH==1
        if (opt_touch && !console_gui_shown())
        {
#ifdef __EMSCRIPTEN__
            const int gyroscope_supported = EM_ASM_INT({
                return Neverball._gyroscopeSupported ? 1 : 0;
            });

            if (gyroscope_supported)
                gui_multi(id, _(tutorial_desc_touch_gyroscope[tutorial_index]),
                                GUI_SML, GUI_COLOR_WHT);
            else
#endif
                gui_multi(id, _(tutorial_desc_touch[tutorial_index]),
                              GUI_SML, GUI_COLOR_WHT);
        }
        else if (console_gui_shown())
            gui_multi(id, _(tutorial_desc_xbox[tutorial_index]),
                          GUI_SML, GUI_COLOR_WHT);
#ifndef __EMSCRIPTEN__
        else if (current_platform == PLATFORM_PC)
            gui_multi(id, _(tutorial_desc[tutorial_index]),
                          GUI_SML, GUI_COLOR_WHT);
        else if (current_platform == PLATFORM_WII)
            gui_multi(id, _(tutorial_desc_wii[tutorial_index]),
                          GUI_SML, GUI_COLOR_WHT);
        else if (current_platform == PLATFORM_WIIU)
            gui_multi(id, _(tutorial_desc_wiiu[tutorial_index]),
                          GUI_SML, GUI_COLOR_WHT);
        else
            gui_multi(id, _(tutorial_desc_xbox[tutorial_index]),
                          GUI_SML, GUI_COLOR_WHT);
#else
        else
            gui_multi(id, _(tutorial_desc[tutorial_index]),
                          GUI_SML, GUI_COLOR_WHT);
#endif
#else
        gui_multi(id, _(opt_touch ? tutorial_desc_touch[tutorial_index] :
                                    tutorial_desc[tutorial_index]),
                        GUI_SML, GUI_COLOR_WHT);
#endif

        gui_space(id);
        if ((jd = gui_harray(id)))
        {
            const char *toggle_tutorial_text = config_get_d(CONFIG_ACCOUNT_TUTORIAL) ?
                                               N_("Tutorial Off") : N_("Tutorial On");
            toggle_id = gui_state(jd, _(toggle_tutorial_text), GUI_SML, TUTORIAL_TOGGLE, 0);
            gui_start(jd, _("OK"), GUI_SML, GUI_BACK, 0);
        }
    }

    gui_layout(id, 0, 0);

    hud_hide();
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    EM_ASM({ Neverball.WGCLhideGameHUD(); });
#endif

    return transition_slide(id, 1, intent);
}

static void tutorial_paint(int id, float t)
{
    game_client_draw(0, t);

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    if (console_gui_shown())
        console_gui_death_paint();
#endif

    hud_paint();
    gui_paint(id);
}

static void tutorial_timer(int id, float dt)
{
    if (tutorial_before_play || hint_before_play)
        geom_step(dt);

    hud_timer(dt);
    gui_timer(id, dt);
}

static int tutorial_keybd(int c, int d)
{
    if (d && (c == KEY_EXIT
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
           && current_platform == PLATFORM_PC
#endif
        ))
        return tutorial_action(GUI_BACK, 0);

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
    HINT_PITFALLS_TEXT,
    HINT_PLATFORM_PARTY,
    HINT_HALF_PIPE_TEXT,
    HINT_UPWARD_SPIRAL_TEXT,
    HINT_SURVIVAL_FITTEST,
    HINT_FORKROAD_TEXT,
    HINT_ENDURANCE_TEXT,
    HINT_UNDERCONSTRUCTION_TEXT
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

    const char *ln = level_name(curr_level());
    const char *sn = set_name(curr_set());

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    if (!campaign_used())
#endif
    {
        if (ln && *ln && sn && *sn)
        {
            if (strcmp(sn, _("Neverball Easy")) == 0)
            {
                if (strcmp(ln, "10") == 0)
                {
                    goto_hint_before_play(3);
                    return 1;
                }
                if (strcmp(ln, "13") == 0)
                {
                    goto_hint_before_play(5);
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
            else if (strcmp(sn, _("Tour de force")) == 0)
            {
                if (strcmp(ln, "2") == 0 ||
                    strcmp(ln, "V") == 0)
                {
                    goto_hint_before_play(9);
                    return 1;
                }
            }
            else if (strcmp(sn, _("Nevermania")) == 0)
            {
                if (strcmp(ln, "6") == 0)
                {
                    goto_hint_before_play(6);
                    return 1;
                }
                if (strcmp(ln, "11") == 0)
                {
                    goto_hint_before_play(10);
                    return 1;
                }
            }
            else if (strcmp(sn, _("Tones Levels")) == 0)
            {
                if (strcmp(ln, "15") == 0)
                {
                    goto_hint_before_play(8);
                    return 1;
                }
            }
        }
    }

    /* No hints available! */

    return 0;
}

int goto_hint(int idx)
{
    hint_before_play = 0;
    hint_index       = idx;

    return goto_state(&st_hint);
}

int goto_hint_before_play(int idx)
{
    hint_before_play = 1;
    hint_index       = idx;

    st_continue = &st_play_ready;

    return goto_state(&st_hint);
}

static int hint_action(int tok, int val)
{
    audio_play(AUD_MENU, 1.0f);
    switch (tok)
    {
        case HINT_TOGGLE:
            config_tgl_d(CONFIG_ACCOUNT_HINT);
            gui_set_label(toggle_id, config_get_d(CONFIG_ACCOUNT_HINT) ?
                                     _("Hint Off") :
                                     _("Hint On"));
            return 1;
    }

    video_set_grab(1);
    return exit_state(st_continue);
}

static int hint_enter(struct state *st, struct state *prev, int intent)
{
    if (!tutorial_before_play)
        if (prev == &st_play_loop)
            st_continue = prev;

    video_clr_grab();

    int id, jd;
    if ((id = gui_vstack(0)))
    {
        gui_label(id, _("Hint"), GUI_MED, GUI_COLOR_DEFAULT);
        gui_space(id);
        gui_multi(id, _(hint_desc[hint_index]), GUI_SML, GUI_COLOR_WHT);

        gui_space(id);
        if ((jd = gui_harray(id)))
        {
            const char *toggle_hint_text = config_get_d(CONFIG_ACCOUNT_HINT) ?
                                           N_("Hint Off") : N_("Hint On");
            toggle_id = gui_state(jd, _(toggle_hint_text), GUI_SML, HINT_TOGGLE, 0);
            gui_start(jd, _("OK"), GUI_SML, GUI_BACK, 0);
        }
    }

    gui_layout(id, 0, 0);

    hud_hide();
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    EM_ASM({ Neverball.WGCLhideGameHUD(); });
#endif

    return transition_slide(id, 1, intent);
}

static int hint_keybd(int c, int d)
{
    if (d && (c == KEY_EXIT
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
           && current_platform == PLATFORM_PC
#endif
        ))
        return hint_action(GUI_BACK, 0);

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
