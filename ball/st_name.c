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

#include <string.h>
#include <ctype.h>

#if NB_HAVE_PB_BOTH == 1
#ifndef __EMSCRIPTEN__
#include "console_control_gui.h"
#endif
#include "networking.h"
#include "account.h"
#endif

#include "ball.h"

#include "common.h"
#include "gui.h"
#include "util.h"
#include "audio.h"
#include "config.h"
#include "video.h"
#include "text.h"
#include "geom.h"

#include "game_common.h"
#include "game_server.h"
#include "game_client.h"

#include "st_name.h"
#include "st_shared.h"

/*---------------------------------------------------------------------------*/

static struct state *ok_state;
static struct state *cancel_state;

int (*ok_fn)(struct state *);
int (*cancel_fn)(struct state *);

static unsigned int draw_back;
static int newplayers;

int goto_name(struct state *ok, struct state *cancel,
              int (*new_ok_fn)(struct state *), int (*new_cancel_fn)(struct state *),
              unsigned int back)
{
    ok_state     = ok;
    cancel_state = cancel;

    ok_fn     = new_ok_fn;
    cancel_fn = new_cancel_fn;

    draw_back = back;

    return goto_state(&st_name);
}

/*---------------------------------------------------------------------------*/

enum
{
    NAME_OK = GUI_LAST,
    NAME_CONTINUE
};

static int name_id;
static int enter_id;

static void on_text_input(int typing);

static void name_update_enter_btn(void)
{
    int name_accepted = text_length(text_input) > 2;

    for (int i = 0; i < text_length(text_input); i++)
    {
        if (text_input[i] == '\\' || text_input[i] == '/' || text_input[i] == ':' || text_input[i] == '*' || text_input[i] == '?' || text_input[i] == '"' || text_input[i] == '<' || text_input[i] == '>' || text_input[i] == '|')
        {
            name_accepted = 0;
            break;
        }
    }

    gui_set_state(enter_id, name_accepted ? NAME_OK : GUI_NONE, 0);
    gui_set_color(enter_id,
                  name_accepted ? gui_wht : gui_gry,
                  name_accepted ? gui_wht : gui_gry);
}

static int name_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
    case GUI_BACK:
        account_init();
        account_load();
        
        if (cancel_fn)
            return cancel_fn(cancel_state);

        return goto_state(cancel_state);

    case NAME_OK:
        newplayers = 0;

#if ENABLE_DEDICATED_SERVER==1
        //networking_quit();
#endif
        account_save();

        config_set_s(CONFIG_PLAYER, text_input);
        text_input_stop();

#ifdef CONFIG_INCLUDES_ACCOUNT
        account_init();
        if (!account_exists())
            newplayers = 1;
        account_load();
        account_set_s(ACCOUNT_PLAYER, text_input);
        ball_free();
        ball_init();

        account_save();
#endif
        config_save();

#if ENABLE_DEDICATED_SERVER==1 && NB_HAVE_PB_BOTH==1
        //networking_init(1);
        //networking_reinit_dedicated_event();
#endif
        if (!newplayers && ok_fn)
            return ok_fn(ok_state);

        return newplayers ? goto_state(&st_name) : goto_state(ok_state);

    case NAME_CONTINUE:
        newplayers = 0;

        if (ok_fn)
            return ok_fn(ok_state);

        return goto_state(ok_state);

    case GUI_CL:
        gui_keyboard_lock_en();
        break;

    case GUI_BS:
        text_input_del();
        break;

    case GUI_CHAR:
        text_input_char(val);
        break;
    }

    return 1;
}

static int name_gui(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        if (!newplayers)
        {
            gui_title_header(id, _("Player Name"), GUI_MED, 0, 0);
            gui_space(id);

            name_id = gui_label(id, "XXXXXXXXXXXXXXXX", GUI_MED, gui_yel, gui_yel);

            gui_space(id);
            if ((jd = gui_hstack(id)))
            {
                gui_filler(jd);
                gui_keyboard_en(jd);
                gui_filler(jd);
            }
            gui_space(id);

            if ((jd = gui_harray(id)))
            {
                enter_id = gui_start(jd, _("OK"), GUI_SML, NAME_OK, 0);
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
                if (current_platform == PLATFORM_PC)
#endif
                {
                    gui_space(jd);
                    gui_state(jd, _("Cancel"), GUI_SML, GUI_BACK, 0);
                }
            }


            gui_set_trunc(name_id, TRUNC_HEAD);
            gui_set_label(name_id, text_input);
        }
        else
        {
            gui_title_header(id, _("New Players!"), GUI_MED, 0, 0);
            gui_space(id);
            gui_multi(id, _("As of new players, you can\\start new Campaign levels first\\before select other game modes."), GUI_SML, gui_wht, gui_wht);
            gui_space(id);
            gui_start(id, _("OK"), GUI_SML, NAME_CONTINUE, 0);
        }
    }

    gui_layout(id, 0, 0);
    return id;
}

static void on_text_input(int typing)
{
    if (name_id)
    {
        gui_set_label(name_id, text_input);

        if (typing)
            audio_play(AUD_MENU, 1.0f);
    }
}

static int name_enter(struct state *st, struct state *prev)
{
    if (draw_back)
    {
        game_client_free(NULL);
        back_init(config_get_d(CONFIG_ACCOUNT_MAYHEM) ? "back/gui-mayhem.png" : "back/gui.png");
    }

    if (!newplayers)
    {
        text_input_start(on_text_input);
        text_input_str(config_get_s(CONFIG_PLAYER), 0);
    }

    return name_gui();
}

static void name_leave(struct state *st, struct state *next, int id)
{
    if (draw_back)
        back_free();

    gui_delete(id);
}

static void name_paint(int id, float t)
{
    if (draw_back)
    {
        video_push_persp((float) config_get_d(CONFIG_VIEW_FOV), 0.1f, FAR_DIST);
        {
            back_draw_easy();
        }
        video_pop_matrix();
    }
    else
        game_client_draw(0, t);

    gui_paint(id);
}

static void name_timer(int id, float dt)
{
    name_update_enter_btn();
    gui_timer(id, dt);
}

static int name_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT)
            return name_action(GUI_BACK, 0);

        if (c == '\b' || c == 0x7F)
        {
            gui_focus(enter_id);
            return name_action(GUI_BS, 0);
        }
        else
        {
            gui_focus(enter_id);
            return 1;
        }
    }
    return 1;
}

static int name_buttn(int b, int d)
{
    if (d)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
        {
            int tok = gui_token(gui_active());
            int val = gui_value(gui_active());

            return name_action(tok, (tok == GUI_CHAR ?
                                     gui_keyboard_char(val) :
                                     val));
        }
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            name_action(GUI_BACK, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

struct state st_name = {
    name_enter,
    name_leave,
    name_paint,
    name_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    name_keybd,
    name_buttn
};

