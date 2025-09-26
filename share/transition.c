/*
 * Copyright (C) 2024 Microsoft / Neverball Authors / Jānis Rūcis
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

#include "config.h"

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

static int widget_flags_ids[16];

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
    transition_add_full(id, 0);
}

void transition_add_full(int id, int flags)
{
    int i;

    /*
     * HACK: Sollte diesen alten GUI flags vor dem erstellen eines Übergang
     * "GUI_REMOVE" versehen, wird sofort gelöscht.
     * - Ersohn Styne
     */

    /*for (i = 0; i < ARRAYSIZE(widget_ids); ++i)
        if (widget_flags_ids[i] | GUI_REMOVE)
        {
            gui_remove(widget_ids[i]);
            widget_flags_ids[i] = 0;
        }*/

    for (i = 0; i < ARRAYSIZE(widget_ids); ++i)
        if (!widget_ids[i])
        {
            widget_ids[i]       = id;
            widget_flags_ids[i] = flags;
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
    else if (config_get_d(CONFIG_SCREEN_ANIMATIONS))
    {
        // Was: (intent == INTENT_BACK ? GUI_E : GUI_W)

        gui_slide(id, (exit_flags) | GUI_BACKWARD | GUI_FLING | GUI_REMOVE, 0, 0.16f, 0);
        transition_add_full(id, (exit_flags) | GUI_BACKWARD | GUI_FLING | GUI_REMOVE);
    }
    else gui_delete(id);

    return id;
}

int transition_page(int id, int in, int intent)
{
    const int head_id = gui_child(id, 0);
    const int body_id = gui_child(id, 1);

    if (in)
    {
        // Slide in page content.

        if (config_get_d(CONFIG_SCREEN_ANIMATIONS))
            gui_slide(body_id, (intent == INTENT_BACK ? GUI_W : GUI_E) | GUI_FLING, 0, 0.16f, 0);
    }
    else if (config_get_d(CONFIG_SCREEN_ANIMATIONS))
    {
        // Just hide the header, header from the next page takes over immediately.
        gui_set_hidden(head_id, 1);

        // Remove GUI after timeout (this doesn't do a slide).
        gui_slide(id, GUI_REMOVE, 0, 0.16f, 0);

        // Slide out page content.
        gui_slide(body_id, (intent == INTENT_BACK ? GUI_E : GUI_W) | GUI_BACKWARD | GUI_FLING, 0, 0.16f, 0);

        transition_add_full(id, (intent == INTENT_BACK ? GUI_E : GUI_W) | GUI_BACKWARD | GUI_FLING);
    }
    else gui_delete(id);

    return id;
}

/*---------------------------------------------------------------------------*/
