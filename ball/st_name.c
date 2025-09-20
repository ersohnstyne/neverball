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

#include <string.h>
#include <ctype.h>

/*
 * HACK: Used with console version
 */
#include "console_control_gui.h"

#if NB_HAVE_PB_BOTH==1
#include "networking.h"
#include "account.h"
#include "account_wgcl.h"
#include "campaign.h"
#endif

#include "ball.h"

#include "common.h"
#include "gui.h"
#include "transition.h"
#include "util.h"
#include "audio.h"
#include "config.h"
#include "video.h"
#include "text.h"
#include "geom.h"
#include "key.h"

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

static int allow_entertext;

static unsigned int draw_back;

static int newplayers;
static int name_error;
static int name_readonly;
static int name_lockedby_systemos;

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

int goto_name_setup(struct state *finish,
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

    KEYBOARD_GAMEMENU_ACTION(NAME_OK);

    switch (tok)
    {
        case GUI_BACK:
            allow_entertext = 0;
            text_input_stop();

            if (name_error)
                return exit_state(&st_name);

            if (newplayers)
                return ok_fn ? ok_fn(ok_state) : goto_state(ok_state);

            account_init();
            account_load();

            return cancel_fn ? cancel_fn(cancel_state) : exit_state(cancel_state);
            break;

        case NAME_OK:
            newplayers = 0;
            name_error = 0;

            if (strcmp(config_get_s(CONFIG_PLAYER), text_input) != 0)
                player_renamed = 1;

            allow_entertext = 0;
            text_input_stop();

#ifdef CONFIG_INCLUDES_ACCOUNT
            if (text_length(text_input) < 3)
                name_error = 1;
            else if (player_renamed)
            {
                account_wgcl_save();

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

                account_wgcl_save();
            }
#endif
            config_save();

            return !name_error && !newplayers && ok_fn ? ok_fn(ok_state) :
                                                         goto_state(newplayers || name_error ? &st_name : ok_state);
            break;

        case NAME_CONTINUE:
            newplayers = 0;

            return !name_error && ok_fn ? ok_fn(ok_state) :
                                          goto_state(name_error ? &st_name : ok_state);
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
        if (!newplayers && !name_error &&
            !name_readonly && !name_lockedby_systemos)
        {
            allow_entertext = 1;

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
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
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
        else if (name_readonly)
        {
            gui_title_header(id, _("Read-Only"), GUI_MED, gui_red, gui_blk);
            gui_space(id);
            gui_multi(id, _("The player name is read-only and\n"
                            "can be changed with WGCL's account settings."),
                          GUI_SML, GUI_COLOR_WHT);
            gui_space(id);
            gui_start(id, _("OK"), GUI_SML, GUI_BACK, 0);
        }
        else if (name_lockedby_systemos)
        {
            gui_title_header(id, _("Read-Only"), GUI_MED, gui_red, gui_blk);
            gui_space(id);
            gui_multi(id, _("The player name is read-only and\n"
                            "can be changed with system settings."),
                          GUI_SML, GUI_COLOR_WHT);
            gui_space(id);
            gui_start(id, _("OK"), GUI_SML, GUI_BACK, 0);
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
    if (!allow_entertext) return;

    if (name_id)
    {
        gui_set_label(name_id, text_input);

        if (typing)
            audio_play(AUD_2_2_0_KEYBD_CHAR, 1.0f);

        name_update_enter_btn();
    }
}

static int name_enter(struct state *st, struct state *prev, int intent)
{
    player_renamed = 0;

    name_lockedby_systemos = config_playername_locked();

#if NB_HAVE_PB_BOTH==1
    name_readonly = account_wgcl_name_read_only();
#else
    name_readonly = 0;
#endif

    if (name_error || name_readonly || name_lockedby_systemos)
        audio_play("snd/uierror.ogg", 1.0f);

    if (draw_back)
    {
        game_client_free(NULL);
        back_init("back/gui.png");
    }

    if (!newplayers && !name_readonly && !name_lockedby_systemos)
    {
        text_input_start(on_text_input);
        text_input_str(config_get_s(CONFIG_PLAYER), 0);
    }

    return transition_slide(name_gui(), 1, intent);
}

static int name_leave(struct state *st, struct state *next, int id, int intent)
{
    if (draw_back)
        back_free();

    if (next == &st_null)
    {
        progress_exit();

        campaign_quit();
        set_quit();

        game_server_free(NULL);
        game_client_free(NULL);
    }

#if NB_HAVE_PB_BOTH==1 && ENABLE_DEDICATED_SERVER==1
    if (player_renamed)
    {
        Sleep(1000);
        networking_dedicated_refresh_login(config_get_s(CONFIG_PLAYER));
    }
#endif

    return transition_slide(id, 0, intent);
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
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    if ((current_platform != PLATFORM_PC || console_gui_shown()) &&
        allow_entertext)
        console_gui_keybd_paint();
#endif
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

        else if (allow_entertext)
        {
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

            return name_action(tok, (tok == GUI_CHAR && allow_entertext ?
                                     gui_keyboard_char(val) :
                                     val));
        }
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            name_action(GUI_BACK, 0);

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_X, b) &&
            allow_entertext)
            name_action(GUI_BS, 0);
        else if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L2, b) &&
                 allow_entertext)
            name_action(GUI_CL, 0);
        else if (config_tst_d(CONFIG_JOYSTICK_BUTTON_R2, b) &&
                 allow_entertext)
            name_action(NAME_OK, 0);
    }
    else
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L2, b) &&
            allow_entertext)
            name_action(GUI_CL, 0);
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
