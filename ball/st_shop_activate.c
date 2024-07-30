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

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
#include "console_control_gui.h"
#endif

#include "audio.h"
#include "account.h"
#include "account_wgcl.h"
#include "config.h"
#include "gui.h"
#include "lang.h"
#include "state.h"
#include "text.h"
#include "vec3.h"
#include "key.h"

#include "game_common.h"
#include "game_payment.h"

#include "st_shop.h"
#include "st_shared.h"

/*---------------------------------------------------------------------------*/

struct state shop_activate;

/*---------------------------------------------------------------------------*/

static int activate_introducory = 1;

static struct state *ok_state;
static struct state *cancel_state;

static int (*curr_ok_fn)     (struct state *) = NULL;
static int (*curr_cancel_fn) (struct state *) = NULL;

int goto_shop_activate(struct state *ok, struct state *cancel,
                       int (*new_ok_fn) (struct state *),
                       int (*new_cancel_fn) (struct state *))
{
    activate_introducory = 1;

    ok_state     = ok;
    cancel_state = cancel;

    curr_ok_fn     = new_ok_fn;
    curr_cancel_fn = new_cancel_fn;

    return goto_state(&shop_activate);
}

/*---------------------------------------------------------------------------*/

enum
{
    ACTIVATE_OK = GUI_LAST,
};

static int ordercode_id;
static int enter_id;

static char prev_text_input[MAXSTR];
static int additive = 0;

static void on_text_input(int typing);

static void shop_activate_update_enter_btn(void)
{
    int ordercode_accepted = text_length(text_input) >= 29;

    for (int i = 0; i < text_length(text_input) && ordercode_accepted; i++)
    {
        if (text_input[i] != '-'
         && text_input[i] != 'B'
         && text_input[i] != 'C'
         && text_input[i] != 'D'
         && text_input[i] != 'F'
         && text_input[i] != 'G'
         && text_input[i] != 'H'
         && text_input[i] != 'J'
         && text_input[i] != 'K'
         && text_input[i] != 'M'
         && text_input[i] != 'N'
         && text_input[i] != 'P'
         && text_input[i] != 'Q'
         && text_input[i] != 'R'
         && text_input[i] != 'T'
         && text_input[i] != 'V'
         && text_input[i] != 'W'
         && text_input[i] != 'X'
         && text_input[i] != 'Y'
         && text_input[i] != '2'
         && text_input[i] != '3'
         && text_input[i] != '4'
         && text_input[i] != '5'
         && text_input[i] != '6'
         && text_input[i] != '7'
         && text_input[i] != '8'
         && text_input[i] != '9')
        {
            ordercode_accepted = 0;
            break;
        }
    }

    gui_set_state(enter_id, ordercode_accepted ? ACTIVATE_OK : GUI_NONE, 0);
    gui_set_color(enter_id,
                  ordercode_accepted ? gui_wht : gui_gry,
                  ordercode_accepted ? gui_wht : gui_gry);
}

static int shop_activate_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    int tmp_txtlen = text_length(text_input);

    switch (tok)
    {
        case GUI_BACK:
            text_input_stop();

            if (curr_cancel_fn)
                curr_cancel_fn(cancel_state);

            return goto_state(cancel_state);

        case ACTIVATE_OK:
            if (activate_introducory)
            {
                activate_introducory = 0;
                return goto_state(curr_state());
            }
#if ENABLE_IAP!=0 && \
    !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
            else if (game_payment_activate(text_input) == 0)
            {
                text_input_stop();

                int new_coins = account_get_d(ACCOUNT_DATA_WALLET_COINS)      + curr_orderpack().Wallets[0];
                int new_gems  = account_get_d(ACCOUNT_DATA_WALLET_GEMS)       + curr_orderpack().Wallets[1];
                int new_lives = account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES) + curr_orderpack().Balls;

                int prev_powers[3], new_powers[3];
                prev_powers[0] = account_get_d(ACCOUNT_CONSUMEABLE_EARNINATOR);
                prev_powers[1] = account_get_d(ACCOUNT_CONSUMEABLE_FLOATIFIER);
                prev_powers[2] = account_get_d(ACCOUNT_CONSUMEABLE_SPEEDIFIER);
                v_add(new_powers, prev_powers, curr_orderpack().Powerups);

                /* Do the same as well? */

                account_wgcl_do_set(new_coins, new_gems, new_lives,
                                    new_powers[0], new_powers[1], new_powers[2]);

                account_wgcl_save();

                audio_play("snd/import.ogg", 1.0f);

                if (curr_ok_fn)
                    curr_ok_fn(ok_state);

                return goto_state(ok_state);
            }
#endif
            break;

        case GUI_BS:
            text_input_del();
            break;

        case GUI_CHAR:
            if (tmp_txtlen < 29)
                text_input_char(val);

            break;
    }

    return 1;
}

static int shop_activate_gui_introducory(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("Activate order code"), GUI_MED, GUI_COLOR_DEFAULT);
        gui_space(id);
        gui_multi(id,
                  _("Your order code should be in an email\n"
                    "from whoever sold Pennyball IAP to you.\n\n"
                    "The order code should be following like this:\n"
                    "XXXXX-XXXXX-XXXXX-XXXXX-XXXXX"), 
                  GUI_SML, GUI_COLOR_WHT);
        gui_space(id);

        if ((jd = gui_harray(id)))
        {
            gui_start(jd, _("Enter"), GUI_SML, ACTIVATE_OK, 0);
            gui_state(jd, _("Cancel"), GUI_SML, GUI_BACK, 0);
        }
    }

    gui_layout(id, 0, 0);

    return id;
}

static void on_text_input(int typing)
{
    if (ordercode_id)
    {
        if (text_length(prev_text_input) < text_length(text_input))
        {
            int start_i =
                text_length(text_input) > 23 ? 24 :
                                              (text_length(text_input) > 17 ? 18 :
                                                                             (text_length(text_input) > 11 ? 12 :
                                                                                                            (text_length(text_input) > 5 ? 6 : 0)));

            for (int i = start_i; i < text_length(text_input); i++)
            {
                if (text_length(text_input) == 6
                 || text_length(text_input) == 12
                 || text_length(text_input) == 18
                 || text_length(text_input) == 24)
                {
                    if ((i == 5
                      || i == 11
                      || i == 17
                      || i == 23)
                     && text_input[i] != '-')
                        text_input_del();
                }
                else if (text_input[i] == 'b'
                      || text_input[i] == 'c'
                      || text_input[i] == 'd'
                      || text_input[i] == 'f'
                      || text_input[i] == 'g'
                      || text_input[i] == 'h'
                      || text_input[i] == 'j'
                      || text_input[i] == 'k'
                      || text_input[i] == 'm'
                      || text_input[i] == 'n'
                      || text_input[i] == 'p'
                      || text_input[i] == 'q'
                      || text_input[i] == 'r'
                      || text_input[i] == 't'
                      || text_input[i] == 'v'
                      || text_input[i] == 'w'
                      || text_input[i] == 'x'
                      || text_input[i] == 'y'

                      || text_input[i] == 'B'
                      || text_input[i] == 'C'
                      || text_input[i] == 'D'
                      || text_input[i] == 'F'
                      || text_input[i] == 'G'
                      || text_input[i] == 'H'
                      || text_input[i] == 'J'
                      || text_input[i] == 'K'
                      || text_input[i] == 'M'
                      || text_input[i] == 'N'
                      || text_input[i] == 'P'
                      || text_input[i] == 'Q'
                      || text_input[i] == 'R'
                      || text_input[i] == 'T'
                      || text_input[i] == 'V'
                      || text_input[i] == 'W'
                      || text_input[i] == 'X'
                      || text_input[i] == 'Y'

                      || text_input[i] == '2'
                      || text_input[i] == '3'
                      || text_input[i] == '4'
                      || text_input[i] == '5'
                      || text_input[i] == '6'
                      || text_input[i] == '7'
                      || text_input[i] == '8'
                      || text_input[i] == '9')
                {
                    if (text_input[i] == 'b'
                     || text_input[i] == 'c'
                     || text_input[i] == 'd'
                     || text_input[i] == 'f'
                     || text_input[i] == 'g'
                     || text_input[i] == 'h'
                     || text_input[i] == 'j'
                     || text_input[i] == 'k'
                     || text_input[i] == 'm'
                     || text_input[i] == 'n'
                     || text_input[i] == 'p'
                     || text_input[i] == 'q'
                     || text_input[i] == 'r'
                     || text_input[i] == 't'
                     || text_input[i] == 'v'
                     || text_input[i] == 'w'
                     || text_input[i] == 'x'
                     || text_input[i] == 'y')
                        text_input[i] = toupper(text_input[i]);
                }
                else
                    text_input_del();

                if (text_input[i] == ' ')
                    text_input_del();

                if (text_length(text_input) > 29)
                    text_input_del();
            }
        }

        SAFECPY(prev_text_input, text_input);

        gui_set_label(ordercode_id, text_input);

        if (typing)
            audio_play(AUD_MENU, 1.0f);
    }
}

static int shop_activate_gui(void)
{
    int id, jd;

    text_input_start(on_text_input);

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("Activate order code"), GUI_MED, GUI_COLOR_DEFAULT);
        gui_space(id);

        ordercode_id = gui_label(id, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
                                 GUI_SML, GUI_COLOR_GRN);
        gui_set_label(ordercode_id, text_input);

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
            enter_id = gui_start(jd, _("OK"), GUI_SML, ACTIVATE_OK, 0);
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            if (current_platform == PLATFORM_PC)
#endif
            {
                gui_space(jd);
                gui_state(jd, _("Cancel"), GUI_SML, GUI_BACK, 0);
            }
        }
    }

    gui_layout(id, 0, 0);

    return id;
}

static int shop_activate_enter(struct state *st, struct state *prev)
{
    additive = 0;

    return activate_introducory ?
        shop_activate_gui_introducory() : shop_activate_gui();
}

static void shop_activate_timer(int id, float dt)
{
    shop_activate_update_enter_btn();
    gui_timer(id, dt);
}

static int shop_activate_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT)
            return shop_activate_action(GUI_BACK, 0);

        if (c == '\b' || c == 0x7F)
        {
            gui_focus(enter_id);
            return shop_activate_action(GUI_BS, 0);
        }
        else
        {
            gui_focus(enter_id);
            return 1;
        }
    }
    return 1;
}

static int shop_activate_buttn(int b, int d)
{
    if (d)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
        {
            int tok = gui_token(gui_active());
            int val = gui_value(gui_active());

            return shop_activate_action(tok, (tok == GUI_CHAR ?
                                        gui_keyboard_char(val) :
                                        val));
        }
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            shop_activate_action(GUI_BACK, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

struct state shop_activate = {
    shop_activate_enter,
    shared_leave,
    shared_paint,
    shop_activate_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    shop_activate_keybd,
    shop_activate_buttn
};
