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

#ifndef __EMSCRIPTEN__
#include "console_control_gui.h"
#endif

#include "state.h"
#include "config.h"
#include "gui.h"
#include "audio.h"
#include "progress.h"

#include "game_common.h"
#include "game_client.h"

#include "st_malfunction.h"

#include "st_shared.h"
#include "st_conf.h"

/*---------------------------------------------------------------------------*/

struct state st_malfunction;
struct state st_handsoff;

/*---------------------------------------------------------------------------*/

int malfunction_locked;
int char_downcounter[128];
int arrow_downcounter[5];
int fwindow_downcounter[13];

static int handson_threshold;

int check_malfunctions(void)
{
    if (malfunction_locked) return 1;

    for (int ic = 0; ic < 128; ic++)
    {
        if (char_downcounter[ic] >= MALFUNCTION_THRESHOLD)
        {
            /* Malfunction locked during exceeding threshold */
            malfunction_locked = 1;
            return goto_state(&st_malfunction);
        }
    }

    for (int ia = 0; ia < 4; ia++)
    {
        if (arrow_downcounter[ia] >= MALFUNCTION_THRESHOLD)
        {
            /* Malfunction locked during exceeding threshold */
            malfunction_locked = 1;
            return goto_state(&st_malfunction);
        }
    }

    for (int ifw = 0; ifw < 4; ifw++)
    {
        if (fwindow_downcounter[ifw] >= MALFUNCTION_THRESHOLD)
        {
            /* Malfunction locked during exceeding threshold */
            malfunction_locked = 1;
            return goto_state(&st_malfunction);
        }
    }

    malfunction_locked = 0;
    return 0;
}

int check_handsoff(void)
{
    for (int ic = 0; ic < 128; ic++)
    {
        if (char_downcounter[ic] >= 1)
            return 1;
    }

    for (int ia = 0; ia < 4; ia++)
    {
        if (arrow_downcounter[ia] >= 1)
            return 1;
    }

    for (int ifw = 0; ifw < 4; ifw++)
    {
        if (fwindow_downcounter[ifw] >= 1)
            return 1;
    }

    handson_threshold = 0;
    return 0;
}

/*---------------------------------------------------------------------------*/

static int malfunction_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
            progress_exit();
            game_fade(+4.0f);
            goto_state_full(&st_null, 0, 0, 0);
            return 0;
    }

    return 1;
}

static int malfunction_gui(void)
{
    int id;
    if ((id = gui_vstack(0)))
    {
        int titid = gui_title_header(id, _("Malfunction detected!"),
                                         GUI_MED, gui_gry, gui_red);
        gui_space(id);
        gui_multi(id, _("The Keyboard has an malfunction!\n"
                        "Please fix your keyboard on your PC\n"
                        "and restart the game."),
                      GUI_SML, GUI_COLOR_WHT);
        gui_space(id);
        gui_start(id, _("Exit"), GUI_SML, GUI_BACK, 0);
        gui_pulse(titid, 1.2f);
    }
    gui_layout(id, 0, 0);
    return id;
}

static int malfunction_enter(struct state *st, struct state *prev)
{
    audio_music_fade_out(0.0f);
    audio_play(AUD_INTRO_SHATTER, 1.0f);
    return malfunction_gui();
}

static int malfunction_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT
#ifndef __EMSCRIPTEN__
         && current_platform == PLATFORM_PC
#endif
            )
            return malfunction_action(GUI_BACK, 0);
    }
    return 1;
}

static int malfunction_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return malfunction_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return malfunction_action(GUI_BACK, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static struct state *back_state;

int goto_handsoff(struct state *back)
{
    back_state = back;

    if (handson_threshold > 1)
        return goto_state(&st_malfunction);

    return goto_state(&st_handsoff);
}

static int handsoff_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
            return goto_state(back_state);
    }

    return 1;
}

static int handsoff_gui(void)
{
    int id;
    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("Hands off!"), GUI_MED, GUI_COLOR_RED);
        gui_space(id);
        gui_multi(id, _("Keep fingers away from the keyboard,\n"
                        "before you play this level!"),
                      GUI_SML, GUI_COLOR_WHT);
        gui_space(id);
        gui_start(id, _("OK"), GUI_SML, GUI_BACK, 0);
    }
    gui_layout(id, 0, 0);
    return id;
}

static int handsoff_enter(struct state *st, struct state *prev)
{
    handson_threshold++;

    return handsoff_gui();
}

static int handsoff_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
         && current_platform == PLATFORM_PC
#endif
            )
            return handsoff_action(GUI_BACK, 0);
    }
    return 1;
}

static int handsoff_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return handsoff_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return handsoff_action(GUI_BACK, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

struct state st_malfunction = {
    malfunction_enter,
    shared_leave,
    shared_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    malfunction_keybd,
    malfunction_buttn
};

struct state st_handsoff = {
    handsoff_enter,
    shared_leave,
    shared_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    handsoff_keybd,
    handsoff_buttn
};
