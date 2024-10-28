/*
<<<<<<< HEAD
 * Copyright (C) 2024 Microsoft / Neverball Authors
=======
 * Copyright (C) 2024 Jānis Rūcis
>>>>>>> b7d565d1c0298d675625db737a6460be6ff92e50
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

#include "transition.h"
#include "common.h"
#include "gui.h"
#include "log.h"

<<<<<<< HEAD
#include "config.h"

=======
>>>>>>> b7d565d1c0298d675625db737a6460be6ff92e50
/*
 * How to create a screen transition:
 *
 * 1. the `leave` handler sets up an "exit" animation (e.g. via `transition_slide`)
 * 2. the `leave` handler DOES NOT use `gui_delete` on its widget ID
 * 3. the `enter` handler of the next screen sets up an "enter" animation (e.g.
 *    via `transition_slide`)
 *
 * You can also set up custom exit animations in the `leave` handler as long as
 * you schedule your widget to be removed at the end with the `GUI_REMOVE` flag.
 *
 * When a screen has an exit animation, the old GUI needs to coexist with the
 * new GUI of the following screen for the duration of the transition. It needs
 * to be painted to be visible and it needs to be stepped forward to animate.
 * That's what all this is for.
 */

/* Widget IDs with exit animations. */
static int widget_ids[16];

/*---------------------------------------------------------------------------*/

void transition_init(void)
{
    memset(widget_ids, 0, sizeof (widget_ids));
}

void transition_quit(void)
{
    memset(widget_ids, 0, sizeof (widget_ids));
}

void transition_add(int id)
{
    int i;

    for (i = 0; i < ARRAYSIZE(widget_ids); ++i)
        if (!widget_ids[i])
        {
            widget_ids[i] = id;
            break;
        }

    if (i == ARRAYSIZE(widget_ids))
    {
        log_printf("Out of transition slots\n");

        gui_remove(id);
    }
}

void transition_remove(int id)
{
    int i;

    for (i = 0; i < ARRAYSIZE(widget_ids); ++i)
        if (widget_ids[i] == id)
        {
            widget_ids[i] = 0;
            break;
        }
}

void transition_timer(float dt)
{
    int i;

    for (i = 0; i < ARRAYSIZE(widget_ids); ++i)
        if (widget_ids[i])
            gui_timer(widget_ids[i], dt);
}

void transition_paint(void)
{
    int i;

    for (i = 0; i < ARRAYSIZE(widget_ids); ++i)
        if (widget_ids[i])
            gui_paint(widget_ids[i]);
}

/*---------------------------------------------------------------------------*/

int transition_slide(int id, int in, int intent)
{
<<<<<<< HEAD
    return transition_slide_full(id, in,
                                 (intent == INTENT_BACK ? GUI_W : GUI_E),
                                 (intent == INTENT_BACK ? GUI_E : GUI_W));
}

int transition_slide_full(int id, int in,
                          int enter_flags, int exit_flags)
{
    if (in)
    {
        // Was: (intent == INTENT_BACK ? GUI_W : GUI_E)

        if (config_get_d(CONFIG_SCREEN_ANIMATIONS))
            gui_slide(id, (enter_flags) | GUI_FLING, 0, 0.16f, 0);
    }
    else
    {
        // Was: (intent == INTENT_BACK ? GUI_E : GUI_W)

        if (config_get_d(CONFIG_SCREEN_ANIMATIONS))
        {
            gui_slide(id, (exit_flags) | GUI_BACKWARD | GUI_FLING | GUI_REMOVE, 0, 0.16f, 0);
            transition_add(id);
        }
        else gui_delete(id);
=======
    if (in)
    {
        gui_slide(id, (intent == INTENT_BACK ? GUI_W : GUI_E) | GUI_FLING, 0, 0.16f, 0);
    }
    else
    {
        gui_slide(id, (intent == INTENT_BACK ? GUI_E : GUI_W) | GUI_BACKWARD | GUI_FLING | GUI_REMOVE, 0, 0.16f, 0);

        transition_add(id);
>>>>>>> b7d565d1c0298d675625db737a6460be6ff92e50
    }

    return id;
}

<<<<<<< HEAD
/*---------------------------------------------------------------------------*/
=======
/*---------------------------------------------------------------------------*/
>>>>>>> b7d565d1c0298d675625db737a6460be6ff92e50
