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

 /*
  * HACK: Used with console version
  */
#include "console_control_gui.h"

#include "key.h"

#include "audio.h"
#include "config.h"
#include "state.h"

#include "gui.h"
#include "transition.h"

#include "demo.h"
#include "progress.h"

#include "hud.h"

#include "game_client.h"
#include "game_common.h"
#include "game_proxy.h"
#include "game_switchball.h"

#include "st_fail.h"

#include "st_switchball.h"
#include "st_shared.h"

/*---------------------------------------------------------------------------*/

struct state st_switchball_speeding;

static struct state *back_state;

/*---------------------------------------------------------------------------*/

enum
{
    SWITCHBALL_SPEEDING_CANCELRUN = GUI_LAST
};

static int switchball_speeding_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
            game_switchball_ignore_speeding();
            return goto_state(back_state);
            break;

        case SWITCHBALL_SPEEDING_CANCELRUN:
            /*
             * HACK: Turn off ticks and don't send reported incidents!
             *
             * Use if (game_switchball_haveticks()), to ensure
             * that game physics must be disabled, when value was set to false.
             */

            game_switchball_toggle_ticks(0);

            union cmd cmd = { CMD_STATUS };
            cmd.status.t  = GAME_FALL;
            game_proxy_enq(&cmd);

            game_client_sync(
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                             !campaign_hardcore_norecordings() &&
#endif
                             curr_mode() != MODE_NONE ? demo_fp : NULL);

            progress_stat(curr_status());
            return goto_state(&st_fail);
            break;
    }

    return 1;
}

static int switchball_speeding_gui(void)
{
    int id = 0, jd = 0, kd = 0;

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("Speeding!"), GUI_MED, GUI_COLOR_RED);
        gui_space(id);
        gui_multi(id, _("You're trying to drop too far,\n"
                        "but it's not recommended for\n"
                        "AE-Speedruns or AE-Fast-Rewinds.\n"
                        "Cancel this run for now?\n"
                        "(This doesn't send reported incidents)"),
                      GUI_SML, GUI_COLOR_WHT);
        gui_space(id);

        if ((jd = gui_harray(id)))
        {
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            if (current_platform == PLATFORM_PC && !console_gui_shown())
#endif
            {
                gui_state(jd, _("No"),  GUI_SML, GUI_BACK, 0);
                const int btn0 = gui_start(jd, _("Yes"), GUI_SML, SWITCHBALL_SPEEDING_CANCELRUN, 0);
                gui_set_color(btn0, GUI_COLOR_RED);
            }
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            else {
                if ((kd = gui_hstack(jd))) {
                    gui_filler(kd);
                    gui_label(kd, _("No"), GUI_SML, GUI_COLOR_WHT);
                    gui_space(kd);
                    console_gui_create_b_button(kd, config_get_d(CONFIG_JOYSTICK_BUTTON_B), 0);
                    gui_filler(kd);
                    gui_set_rect(kd, GUI_ALL);
                }
                if ((kd = gui_hstack(jd))) {
                    gui_filler(kd);
                    const int btn0 = gui_label(kd, _("Yes"), GUI_SML, GUI_COLOR_WHT);
                    gui_space(kd);
                    console_gui_create_a_button(kd, config_get_d(CONFIG_JOYSTICK_BUTTON_A), 0);
                    gui_filler(kd);
                    gui_set_rect(kd, GUI_ALL);
                    gui_set_color(btn0, GUI_COLOR_RED);
                }
            }
#endif
        }

        gui_layout(id, 0, 0);
    }

    return id;
}

static int switchball_speeding_enter(struct state *st, struct state *prev, int intent)
{
    hud_hide();
    return transition_slide(switchball_speeding_gui(), 1, intent);
}

static void switchball_speeding_paint(int id, float t)
{
    game_client_draw(0, t);
    gui_paint(id);
}

static int switchball_speeding_keybd(int c, int d)
{
    if (d && c == KEY_EXIT)
        return switchball_speeding_action(GUI_BACK, 0);

    return 1;
}

static int switchball_speeding_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
        {
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            if (current_platform == PLATFORM_PC && !console_gui_shown())
#endif
            {
                return switchball_speeding_action(gui_token(active) == GUI_BACK ? GUI_BACK : SWITCHBALL_SPEEDING_CANCELRUN, 0);
            }
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            else return switchball_speeding_action(SWITCHBALL_SPEEDING_CANCELRUN, 0);
#endif
        }

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return switchball_speeding_action(GUI_BACK, 0);
    }

    return 1;
}

/*---------------------------------------------------------------------------*/

int goto_switchball_speeding(struct state *back)
{
    audio_play(AUD_WARNING, 1.0f);
    audio_play("snd/2.2/ui_operator_spawn_incident.ogg", 1.0f);

    back_state = back;

    return goto_state(&st_switchball_speeding);
}

/*---------------------------------------------------------------------------*/

struct state st_switchball_speeding = {
    switchball_speeding_enter,
    shared_leave,
    switchball_speeding_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    switchball_speeding_keybd,
    switchball_speeding_buttn
};

/*---------------------------------------------------------------------------*/
