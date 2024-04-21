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

#include <string.h>
#include <ctype.h>

#if NB_HAVE_PB_BOTH==1
#ifndef __EMSCRIPTEN__
#include "console_control_gui.h"
#endif

#include "networking.h"
#include "account.h"
#include "campaign.h"
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

#include "set.h"
#include "demo.h"
#include "progress.h"

#include "game_common.h"
#include "game_server.h"
#include "game_client.h"

#include "st_name.h"
#include "st_shared.h"
#include "st_common.h"
#include "st_setup.h"

/*---------------------------------------------------------------------------*/

static struct state *ok_state;
static struct state *cancel_state;

int (*ok_fn)     (struct state *);
int (*cancel_fn) (struct state *);

static unsigned int draw_back;
static int newplayers;
static int name_error;

int goto_name(struct state *ok, struct state *cancel,
              int (*new_ok_fn) (struct state *),
              int (*new_cancel_fn) (struct state *),
              unsigned int back)
{
    ok_state     = ok;
    cancel_state = cancel;

    ok_fn     = new_ok_fn;
    cancel_fn = new_cancel_fn;

    draw_back = back;

    return goto_state(&st_name);
}

int goto_name_setup(struct state* finish,
                    int (*new_finish_fn) (struct state *))
{
    ok_state     = finish;
    cancel_state = &st_null;

    ok_fn     = new_finish_fn;
    cancel_fn = 0;

    draw_back = 1;

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

static int player_renamed = 0;

static int keybd_typing = 0;

static void on_text_input(int typing);

static void name_update_enter_btn(void)
{
    int name_accepted = text_length(text_input) > 2;

    for (int i = 0; i < text_length(text_input); i++)
    {
        if (text_input[i] == '\\' ||
            text_input[i] == '/'  ||
            text_input[i] == ':'  ||
            text_input[i] == '*'  ||
            text_input[i] == '?'  ||
            text_input[i] == '"'  ||
            text_input[i] == '<'  ||
            text_input[i] == '>'  ||
            text_input[i] == '|')
        {
            name_accepted = 0;
            break;
        }
    }

    gui_set_state(enter_id, name_accepted &&
                            !player_renamed ? NAME_OK : GUI_NONE, 0);
    gui_set_color(enter_id,
                  name_accepted && !player_renamed ? gui_wht : gui_gry,
                  name_accepted && !player_renamed ? gui_wht : gui_gry);
}

static int name_action(int tok, int val)
{
    if (game_setup_process() && tok == GUI_BACK)
    {
        audio_play(AUD_DISABLED, 1.0f);
        return 1;
    }

    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
            if (name_error)
            return goto_state(&st_name);

            if (newplayers)
            return ok_fn ? ok_fn(ok_state) : goto_state(ok_state);

            account_init();
            account_load();

            if (cancel_fn)
                return cancel_fn(cancel_state);

            return goto_state(cancel_state);
            break;

        case NAME_OK:
            newplayers = 0;
            name_error = 0;

            if (strcmp(config_get_s(CONFIG_PLAYER), text_input) != 0)
            player_renamed = 1;

            text_input_stop();

#ifdef CONFIG_INCLUDES_ACCOUNT
            if (text_length(text_input) < 3)
                name_error = 1;
            else if (player_renamed)
            {
                account_save();

                config_set_s(CONFIG_PLAYER, text_input);

                account_init();
                if (text_length(text_input) < 3)
                    name_error = 1;
                else if (account_exists())
                    account_load();
                else
                    newplayers = 1;

                account_set_s(ACCOUNT_PLAYER, text_input);
#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT) && defined(CONFIG_INCLUDES_MULTIBALLS)
                ball_multi_free();
                ball_multi_init();
#else
                ball_free();
                ball_init();
#endif

                account_save();
            }
#endif
            config_save();

            if (!name_error && !newplayers && ok_fn)
                return ok_fn(ok_state);

            return goto_state(newplayers || name_error ? &st_name : ok_state);
            break;

        case NAME_CONTINUE:
            newplayers = 0;

            if (!name_error && ok_fn)
                return ok_fn(ok_state);

            return goto_state(name_error ? &st_name : ok_state);
            break;

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
        if (!newplayers && !name_error)
        {
            gui_title_header(id, _("Player Name"), GUI_MED, GUI_COLOR_DEFAULT);
            gui_space(id);

            name_id = gui_label(id, "XXXXXXXXXXXXXXXX",
                                    GUI_MED, GUI_COLOR_YEL);

            gui_space(id);
            if ((jd = gui_hstack(id)))
            {
                gui_filler     (jd);
                gui_keyboard_en(jd);
                gui_filler     (jd);
            }
            gui_space(id);

            if ((jd = gui_harray(id)))
            {
                enter_id = gui_start(jd, _("OK"), GUI_SML, NAME_OK, 0);
                
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
                if (current_platform == PLATFORM_PC && !game_setup_process())
#endif
                {
                    gui_space(jd);
                    gui_state(jd, _("Cancel"), GUI_SML, GUI_BACK, 0);
                }
#if NB_HAVE_PB_BOTH==1
                else
                {
                    gui_space(jd);
                    gui_space(jd);
                }
#endif
            }

            gui_set_trunc(name_id, TRUNC_HEAD);
            gui_set_label(name_id, text_input);

            name_update_enter_btn();
        }
        else
        {
            const char *t_header = name_error ?
                                   N_("Register failed!") : N_("New Players!"),
                       *t_desc   = name_error ?
                                   N_("Player names didn't meet the requirements!\n"
                                      "- Minimum length: 3 letters") :
                                   N_("As of new players, you can\n"
                                      "start new Campaign levels first\n"
                                      "before select other game modes.");

            gui_title_header(id, _(t_header), GUI_MED,
                             name_error ? gui_red : 0,
                             name_error ? gui_blk : 0);
            gui_space(id);
            gui_multi(id, _(t_desc), GUI_SML, GUI_COLOR_WHT);
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

        name_update_enter_btn();
    }
}

static int name_enter(struct state *st, struct state *prev)
{
    player_renamed = 0;

    if (draw_back)
    {
        game_client_free(NULL);
        back_init("back/gui.png");
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

    play_shared_leave(st, next, id);

#if NB_HAVE_PB_BOTH==1 && ENABLE_DEDICATED_SERVER==1
    if (player_renamed)
    {
        Sleep(1000);
        networking_dedicated_refresh_login(config_get_s(CONFIG_PLAYER));
    }
#endif
}

static void name_paint(int id, float t)
{
    if (draw_back)
    {
        video_set_perspective((float) config_get_d(CONFIG_VIEW_FOV), 0.1f, FAR_DIST);
        back_draw_easy();
    }
    else
        game_client_draw(0, t);

    gui_paint(id);
}

static int name_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
         && current_platform == PLATFORM_PC
#endif
            )
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
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    name_keybd,
    name_buttn
};
