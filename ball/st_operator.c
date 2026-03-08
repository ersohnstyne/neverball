/*
 * Copyright (C) 2026 Microsoft / Neverball authors / Jānis Rūcis
 *
 * PENNYBALL is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */

#include "log.h"
#include "key.h"
#include "config.h"
#include "state.h"
#include "gui.h"
#include "transition.h"
#include "common.h"
#include "demo.h"

#include "audio.h"

#include "progress.h"

#include "st_shared.h"

#include "game_common.h"
#include "game_client.h"

#if (_WIN32 && _MSC_VER) && NB_HAVE_PB_BOTH==1

#include "st_operator.h"
#include "account_wgcl.h"

/*---------------------------------------------------------------------------*/

static int operator_mode;
static int operator_cansend_incidents;
static int operator_norepeat;
static int operator_readytosnap;

static char operator_map_name[MAXSTR];

static struct state *operator_back_state;

/*---------------------------------------------------------------------------*/

struct state st_operator_search;
struct state st_operator_incidents_alldone;
struct state st_operator_incidents_found;
struct state st_operator_incidents_error;
struct state st_operator_incidents_snap;

void demo_operator_init     (void) { operator_mode = 1;    }
int  demo_operator_activated(void) { return operator_mode; }

void demo_operator_quit(void)
{
    /*if (operator_map_name)
    {
        free(operator_map_name);
        operator_map_name = NULL;
    }*/

    operator_mode = 0;
}

int  demo_operator_map_name(const char *map_name)
{
    operator_cansend_incidents = 0;
    operator_readytosnap       = 0;
    operator_norepeat          = 0;

    /*if (operator_map_name)
    {
        free(operator_map_name);
        operator_map_name = NULL;
    }*/

    if (!operator_mode) return 0;

    SAFECPY(operator_map_name, map_name);

    return 1;
}

int goto_operator_search(void)
{
    return goto_state(&st_operator_search);
}

int operator_send_incident_from_replay(int   status,
                                       float x,
                                       float y,
                                       float z)
{
    if (!operator_mode) return 1;

    operator_cansend_incidents = (status == GAME_TIME || status == GAME_FALL);
    operator_norepeat          = 1;

    if (status == GAME_GOAL)
        return goto_state(&st_operator_incidents_alldone);

    operator_readytosnap = operator_cansend_incidents;

    log_printf("WGCL Operator: Dingo!: %s\n", operator_map_name);

    struct state *st_next = (account_wgcl_mapmarkers_place(operator_map_name,
                                                           status,
                                                           ROUND(x * 100), ROUND(y * 100), ROUND(z * 100))) ?
        &st_operator_incidents_found :
        &st_operator_incidents_error;

    return goto_state(st_next);

    /*return goto_state(operator_cansend_incidents ?
                      &st_operator_incidents_found :
                      &st_operator_incidents_error);*/
}

/*---------------------------------------------------------------------------*/

static int operator_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    if (tok == GUI_BACK && operator_back_state)
    {
        demo_replay_stop(0);
        game_client_free(NULL);
        goto_state(operator_back_state);
        operator_back_state = NULL;
    }

    if (tok == 9999 && val == 9999 && operator_readytosnap)
    {
        operator_mode = 0;
        game_kill_fade();
        video_clear();
        video_can_swap_window = 1;
        game_client_draw(0, 0.0f);
        video_snap("Screenshots/incidence.png");
        video_swap();
        operator_mode = 1;

        return goto_state(&st_operator_incidents_snap);
    }

    return 1;
}

static int operator_search_enter(struct state *st, struct state *prev, int intent)
{
    if (!operator_back_state) operator_back_state = prev;

    operator_mode = 1;
    demo_replay_speed(SPEED_FASTESTESTEST);

    int id, jd;

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("Please wait..."), GUI_MED, GUI_COLOR_DEFAULT);

        gui_space(id);

        if ((jd = gui_hstack(id)))
        {
            gui_filler(jd);
            const int icn_id = gui_label(jd, GUI_CROSS, GUI_SML, GUI_COLOR_RED);
            gui_label(jd, _("Cancel"), GUI_SML, GUI_COLOR_WHT);
            gui_filler(jd);

            gui_set_font(icn_id, "ttf/DejaVuSans-Bold.ttf");

            gui_set_state(jd, GUI_BACK, 0);
            gui_set_rect(jd, GUI_ALL);
        }

        gui_layout(id, 0, 0);
    }

    return transition_slide(id, 1, intent);
}

static int operator_search_leave(struct state *st, struct state *next, int id, int intent)
{
    return transition_slide(id, 0, intent);
}

static void operator_search_paint(int id, float t)
{
    game_client_draw(0, t);
    gui_paint(id);
}

static void operator_search_timer(int id, float dt)
{
    gui_timer(id, dt);

    if (!demo_replay_step(dt)) {
        if (!operator_norepeat) {
            demo_replay_stop(0);
            progress_replay(curr_demo());
        }
    } else {
        game_client_blend(demo_replay_blend());
    }
}

static int operator_search_keybd(int c, int d)
{
    if (d && c == KEY_EXIT)
        if (operator_back_state) {
            demo_replay_stop(0);
            game_client_free(NULL);
            goto_state(operator_back_state);
            operator_back_state = NULL;
        }

    return 1;
}

static int operator_search_buttn(int b, int d)
{
    int active = gui_active();

    if (d) {
        if (b == config_get_d(CONFIG_JOYSTICK_BUTTON_B) && operator_back_state) {
            demo_replay_stop(0);
            game_client_free(NULL);
            goto_state(operator_back_state);
            operator_back_state = NULL;
        }

        if (b == config_get_d(CONFIG_JOYSTICK_BUTTON_A))
            operator_action(gui_token(active), gui_value(active));
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int operator_incidents_alldone_enter(struct state *st, struct state *prev, int intent)
{
    audio_play("snd/uierror.ogg", 1.0f);

    int id, kd;

    if ((id = gui_vstack(0)))
    {
        if ((kd = gui_title_header(id, _("Case solved!"), GUI_MED, gui_grn, gui_grn)))
            gui_pulse(kd, 1.2f);

        gui_space(id);

        const char *s0 = _("Nothing to be coming incidents.");

        gui_multi(id, s0, GUI_SML, GUI_COLOR_WHT);

        gui_space(id);

        const int backbtn_id = gui_back_button(id);
        gui_focus(backbtn_id);

        gui_layout(id, 0, 0);
    }

    return transition_slide(id, 1, intent);
}

static int operator_incidents_found_enter(struct state *st, struct state *prev, int intent)
{
    audio_play("snd/2.2/ui_operator_spawn_incident.ogg", 1.0f);

    int id, kd;

    if ((id = gui_vstack(0)))
    {
        if ((kd = gui_title_header(id, _("We found them!"), GUI_MED, GUI_COLOR_GRN)))
            gui_pulse(kd, 1.2f);

        gui_space(id);

        const char *s0 = _("Reported incidents have been\n"
                           "added to WGCL server.");

        gui_multi(id, s0, GUI_SML, GUI_COLOR_WHT);

        gui_space(id);

        if (operator_readytosnap)
            gui_start(id, _("Take photo!"), GUI_SML, 9999, 9999);

        const int backbtn_id = gui_back_button(id);

        if (!operator_readytosnap) gui_focus(backbtn_id);

        gui_layout(id, 0, 0);
    }

    return transition_slide(id, 1, intent);
}

static int operator_incidents_error_enter(struct state *st, struct state *prev, int intent)
{
    audio_play("snd/uierror.ogg", 1.0f);

    int id, kd;

    if ((id = gui_vstack(0)))
    {
        if ((kd = gui_title_header(id, _("Something went wrong!"), GUI_MED, gui_red, gui_blk)))
            gui_pulse(kd, 1.2f);

        gui_space(id);

        const char *s0 = _("Check internet connections\n"
                           "and on your WGCL server online.\n"
                           "If they already online,\n"
                           "then it's already exists.");
        
        const char *s1 = _("Only failure attempts with map name\n"
                           "are supported.");

        gui_multi(id, operator_cansend_incidents ? s0 : s1, GUI_SML, GUI_COLOR_WHT);

        gui_space(id);

        const int backbtn_id = gui_back_button(id);
        gui_focus(backbtn_id);

        gui_layout(id, 0, 0);
    }

    return transition_slide(id, 1, intent);
}

static int operator_incidents_snap_enter(struct state *st, struct state *prev, int intent)
{
    int id, kd;

    if ((id = gui_vstack(0)))
    {
        if ((kd = gui_title_header(id, _("Saved!"), GUI_MED, GUI_COLOR_GRN)))
            gui_pulse(kd, 1.2f);

        gui_space(id);

        const char *s0 = _("Your screenshot incidence\n"
                           "has been saved!");

        gui_multi(id, s0, GUI_SML, GUI_COLOR_WHT);

        gui_space(id);

        const int backbtn_id = gui_back_button(id);
        gui_focus(backbtn_id);

        gui_layout(id, 0, 0);
    }

    return transition_slide(id, 1, intent);
}

/*---------------------------------------------------------------------------*/

struct state st_operator_search = {
    operator_search_enter,
    operator_search_leave,
    operator_search_paint,
    operator_search_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    operator_search_keybd,
    operator_search_buttn
};

struct state st_operator_incidents_alldone = {
    operator_incidents_alldone_enter,
    operator_search_leave,
    operator_search_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    operator_search_keybd,
    operator_search_buttn
};

struct state st_operator_incidents_found = {
    operator_incidents_found_enter,
    operator_search_leave,
    operator_search_paint,
    operator_search_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    operator_search_keybd,
    operator_search_buttn
};

struct state st_operator_incidents_error = {
    operator_incidents_error_enter,
    operator_search_leave,
    operator_search_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    operator_search_keybd,
    operator_search_buttn
};

struct state st_operator_incidents_snap = {
    operator_incidents_snap_enter,
    operator_search_leave,
    operator_search_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    operator_search_keybd,
    operator_search_buttn
};

#endif