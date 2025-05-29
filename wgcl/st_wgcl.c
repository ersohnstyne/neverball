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

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "st_wgcl.h"

/*
 * HACK: Used with console version
 */
#include "console_control_gui.h"

#if NB_HAVE_PB_BOTH==1
#if (_WIN32 && _MSC_VER) || defined(__EMSCRIPTEN__)
#include "account_wgcl.h"
#endif
#include "networking.h"
#endif

#include "audio.h"
#include "config.h"
#include "gui.h"
#include "transition.h"
#include "lang.h"
#include "key.h"
#include "state.h"
#include "text.h"

#include "st_common.h"
#include "st_setup.h"

 //#define WGCL_ENABLE_CLIPBOARD

#define AUD_MENU     "snd/menu.ogg"
#define AUD_BACK     "snd/back.ogg"
#define AUD_DISABLED "snd/disabled.ogg"

/*
 * PB+NB WGCL: Pennyball + Neverball Web server Game Core Launcher
 * - Ersohn Styne
 */

/*###########################################################################*/

/* Macros helps with the action game menu. */

#define GENERIC_GAMEMENU_ACTION                      \
        if (st_global_animating()) {                 \
            audio_play(AUD_DISABLED, 1.0f);          \
            return 1;                                \
        } else audio_play(GUI_BACK == tok ?          \
                          AUD_BACK :                 \
                          (GUI_NONE == tok ?         \
                           AUD_DISABLED : AUD_MENU), \
                          1.0f)

#define GAMEPAD_GAMEMENU_ACTION_SCROLL(tok1, tok2, itemstep) \
        if (st_global_animating()) {                         \
            audio_play(AUD_DISABLED, 1.0f);                  \
            return 1;                                        \
        } else if (tok == tok1 || tok == tok2) {             \
            if (tok == tok1)                                 \
                audio_play(first > 1 ?                       \
                           AUD_MENU : AUD_DISABLED, 1.0f);   \
            if (tok == tok2)                                 \
                audio_play(first + itemstep < total ?        \
                           AUD_MENU : AUD_DISABLED, 1.0f);   \
        } else GENERIC_GAMEMENU_ACTION

/*###########################################################################*/

/*
 * 0 = None
 * 1 = Email or Playername
 * 2 = Password
 */
static int login_entertext_mode = 0;

/* WGCL keyboard screens */

static int lock = 1;
static int keyd[127];
static int keyd_en[127];

/* Britain Keyboards */
void wgcl_gui_keyboard_en(int id)
{
    int jd, kd, ld;

    lock = 1;

    if ((jd = gui_hstack(id)))
    {
        gui_filler(jd);

        if ((kd = gui_vstack(jd)))
        {
            if ((ld = gui_hstack(kd)))
            {
                gui_filler(ld);
                const int bksp_id = gui_state(ld, GUI_TRIANGLE_LEFT, GUI_SML, GUI_BS, 0);
                keyd_en['='] = gui_state(ld, "+", GUI_SML, GUI_CHAR, '+');
                keyd_en['-'] = gui_state(ld, "_", GUI_SML, GUI_CHAR, '_');
                keyd_en['0'] = gui_state(ld, ")", GUI_SML, GUI_CHAR, ')');
                keyd_en['9'] = gui_state(ld, "(", GUI_SML, GUI_CHAR, '(');
                keyd_en['8'] = gui_state(ld, "*", GUI_SML, GUI_CHAR, '*');
                keyd_en['7'] = gui_state(ld, "&", GUI_SML, GUI_CHAR, '&');
                keyd_en['6'] = gui_state(ld, "^", GUI_SML, GUI_CHAR, '^');
                keyd_en['5'] = gui_state(ld, "%", GUI_SML, GUI_CHAR, '%');
                keyd_en['4'] = gui_state(ld, "$", GUI_SML, GUI_CHAR, '$');
                keyd_en['3'] = gui_state(ld, "#", GUI_SML, GUI_CHAR, '#');
                keyd_en['2'] = gui_state(ld, "@", GUI_SML, GUI_CHAR, '@');
                keyd_en['1'] = gui_state(ld, "!", GUI_SML, GUI_CHAR, '!');
                gui_filler(ld);

                gui_set_font(bksp_id, "ttf/DejaVuSans-Bold.ttf");
            }
            if ((ld = gui_hstack(kd)))
            {
                gui_filler(ld);
                gui_space(ld);
                keyd_en[']'] = gui_state(ld, "}", GUI_SML, GUI_CHAR, '}');
                keyd_en['['] = gui_state(ld, "{", GUI_SML, GUI_CHAR, '{');
                keyd_en['P'] = gui_state(ld, "P", GUI_SML, GUI_CHAR, 'P');
                keyd_en['O'] = gui_state(ld, "O", GUI_SML, GUI_CHAR, 'O');
                keyd_en['I'] = gui_state(ld, "I", GUI_SML, GUI_CHAR, 'I');
                keyd_en['U'] = gui_state(ld, "U", GUI_SML, GUI_CHAR, 'U');
                keyd_en['Y'] = gui_state(ld, "Y", GUI_SML, GUI_CHAR, 'Y');
                keyd_en['T'] = gui_state(ld, "T", GUI_SML, GUI_CHAR, 'T');
                keyd_en['R'] = gui_state(ld, "R", GUI_SML, GUI_CHAR, 'R');
                keyd_en['E'] = gui_state(ld, "E", GUI_SML, GUI_CHAR, 'E');
                keyd_en['W'] = gui_state(ld, "W", GUI_SML, GUI_CHAR, 'W');
                keyd_en['Q'] = gui_state(ld, "Q", GUI_SML, GUI_CHAR, 'Q');
                gui_space(ld);
                gui_space(ld);
                gui_filler(ld);
            }
            if ((ld = gui_hstack(kd)))
            {
                gui_filler(ld);
                keyd_en['\\'] = gui_state(ld, "|", GUI_SML, GUI_CHAR, '|');
                keyd_en['\''] = gui_state(ld, "\"", GUI_SML, GUI_CHAR, '\"');
                keyd_en[';'] = gui_state(ld, ":", GUI_SML, GUI_CHAR, ':');
                keyd_en['L'] = gui_state(ld, "L", GUI_SML, GUI_CHAR, 'L');
                keyd_en['K'] = gui_state(ld, "K", GUI_SML, GUI_CHAR, 'K');
                keyd_en['J'] = gui_state(ld, "J", GUI_SML, GUI_CHAR, 'J');
                keyd_en['H'] = gui_state(ld, "H", GUI_SML, GUI_CHAR, 'H');
                keyd_en['G'] = gui_state(ld, "G", GUI_SML, GUI_CHAR, 'G');
                keyd_en['F'] = gui_state(ld, "F", GUI_SML, GUI_CHAR, 'F');
                keyd_en['D'] = gui_state(ld, "D", GUI_SML, GUI_CHAR, 'D');
                keyd_en['S'] = gui_state(ld, "S", GUI_SML, GUI_CHAR, 'S');
                keyd_en['A'] = gui_state(ld, "A", GUI_SML, GUI_CHAR, 'A');
                const int caps_id = gui_state(ld, "⇩", GUI_SML, GUI_CL, 0);
                gui_filler(ld);

                gui_set_font(caps_id, "ttf/DejaVuSans-Bold.ttf");
            }
            if ((ld = gui_hstack(kd)))
            {
                gui_filler(ld);
                keyd_en['/'] = gui_state(ld, "?", GUI_SML, GUI_CHAR, '?');
                keyd_en['.'] = gui_state(ld, ">", GUI_SML, GUI_CHAR, '>');
                keyd_en[','] = gui_state(ld, "<", GUI_SML, GUI_CHAR, '<');
                keyd_en['M'] = gui_state(ld, "M", GUI_SML, GUI_CHAR, 'M');
                keyd_en['N'] = gui_state(ld, "N", GUI_SML, GUI_CHAR, 'N');
                keyd_en['B'] = gui_state(ld, "B", GUI_SML, GUI_CHAR, 'B');
                keyd_en['V'] = gui_state(ld, "V", GUI_SML, GUI_CHAR, 'V');
                keyd_en['C'] = gui_state(ld, "C", GUI_SML, GUI_CHAR, 'C');
                keyd_en['X'] = gui_state(ld, "X", GUI_SML, GUI_CHAR, 'X');
                keyd_en['Z'] = gui_state(ld, "Z", GUI_SML, GUI_CHAR, 'Z');
                keyd_en['`'] = gui_state(ld, "~", GUI_SML, GUI_CHAR, '~');
                gui_space(ld);
                gui_filler(ld);
            }
        }
    }
}

/* Britain Keyboards */
void wgcl_gui_keyboard_lock_en(void)
{
    lock = lock ? 0 : 1;

    gui_set_label(keyd_en['-'], lock ? "_" : "-"); gui_set_state(keyd_en['-'], GUI_CHAR, lock ? '_' : '-');
    gui_set_label(keyd_en['='], lock ? "+" : "="); gui_set_state(keyd_en['='], GUI_CHAR, lock ? '+' : '=');
    gui_set_label(keyd_en['1'], lock ? "!" : "1");
    gui_set_label(keyd_en['2'], lock ? "@" : "2");
    gui_set_label(keyd_en['3'], lock ? "#" : "3");
    gui_set_label(keyd_en['4'], lock ? "$" : "4");
    gui_set_label(keyd_en['5'], lock ? "%" : "5");
    gui_set_label(keyd_en['6'], lock ? "^" : "6");
    gui_set_label(keyd_en['7'], lock ? "&" : "7");
    gui_set_label(keyd_en['8'], lock ? "*" : "8");
    gui_set_label(keyd_en['9'], lock ? "(" : "9");
    gui_set_label(keyd_en['0'], lock ? ")" : "0");
    gui_set_label(keyd_en['A'], lock ? "A" : "a");
    gui_set_label(keyd_en['B'], lock ? "B" : "b");
    gui_set_label(keyd_en['C'], lock ? "C" : "c");
    gui_set_label(keyd_en['D'], lock ? "D" : "d");
    gui_set_label(keyd_en['E'], lock ? "E" : "e");
    gui_set_label(keyd_en['F'], lock ? "F" : "f");
    gui_set_label(keyd_en['G'], lock ? "G" : "g");
    gui_set_label(keyd_en['H'], lock ? "H" : "h");
    gui_set_label(keyd_en['I'], lock ? "I" : "i");
    gui_set_label(keyd_en['J'], lock ? "J" : "j");
    gui_set_label(keyd_en['K'], lock ? "K" : "k");
    gui_set_label(keyd_en['L'], lock ? "L" : "l");
    gui_set_label(keyd_en['M'], lock ? "M" : "m");
    gui_set_label(keyd_en['N'], lock ? "N" : "n");
    gui_set_label(keyd_en['O'], lock ? "O" : "o");
    gui_set_label(keyd_en['P'], lock ? "P" : "p");
    gui_set_label(keyd_en['Q'], lock ? "Q" : "q");
    gui_set_label(keyd_en['R'], lock ? "R" : "r");
    gui_set_label(keyd_en['S'], lock ? "S" : "s");
    gui_set_label(keyd_en['T'], lock ? "T" : "t");
    gui_set_label(keyd_en['U'], lock ? "U" : "u");
    gui_set_label(keyd_en['V'], lock ? "V" : "v");
    gui_set_label(keyd_en['W'], lock ? "W" : "w");
    gui_set_label(keyd_en['X'], lock ? "X" : "x");
    gui_set_label(keyd_en['Y'], lock ? "Y" : "y");
    gui_set_label(keyd_en['Z'], lock ? "Z" : "z");
    gui_set_label(keyd_en['`'], lock ? "~" : "`"); gui_set_state(keyd_en['`'], GUI_CHAR, lock ? '~' : '`');
    gui_set_label(keyd_en['['], lock ? "{" : "["); gui_set_state(keyd_en['['], GUI_CHAR, lock ? '{' : '[');
    gui_set_label(keyd_en[']'], lock ? "}" : "]"); gui_set_state(keyd_en[']'], GUI_CHAR, lock ? '}' : ']');
    gui_set_label(keyd_en[';'], lock ? ":" : ";"); gui_set_state(keyd_en[';'], GUI_CHAR, lock ? ':' : ';');
    gui_set_label(keyd_en['\\'], lock ? "|" : "\\"); gui_set_state(keyd_en['\\'], GUI_CHAR, lock ? '|' : '\\');
    gui_set_label(keyd_en['\''], lock ? "\"" : "'"); gui_set_state(keyd_en['\''], GUI_CHAR, lock ? '"' : '\'');
    gui_set_label(keyd_en[','], lock ? "<" : ","); gui_set_state(keyd_en[','], GUI_CHAR, lock ? '<' : ',');
    gui_set_label(keyd_en['.'], lock ? ">" : "."); gui_set_state(keyd_en['.'], GUI_CHAR, lock ? '>' : '.');
    gui_set_label(keyd_en['/'], lock ? "?" : "/"); gui_set_state(keyd_en['/'], GUI_CHAR, lock ? '?' : '/');
}

char wgcl_gui_keyboard_char(char c)
{
    return lock ? toupper(c) : tolower(c);
}

/*###########################################################################*/

struct state st_wgcl_login;
struct state st_wgcl_login_result;
struct state st_wgcl_logout_confirm;

/*###########################################################################*/

static int wgcl_error_offline_enter(struct state *st, struct state *prev, int intent)
{
    audio_play("snd/uierror.ogg", 1.0f);

    int id;

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("No internet connection"),
                             GUI_MED, gui_gry, gui_red);
        gui_space(id);
        gui_multi(id, _("Please check your internet connection\n"
                        "or configure your router first.\n"
                        "(e.g. Wi-Fi settings or ethernet)"),
                      GUI_SML, GUI_COLOR_WHT);

        gui_layout(id, 0, 0);
    }
    
#ifdef __EMSCRIPTEN__
    EM_ASM({
        if (navigator.userAgent.includes("Windows") || navigator.platform.startsWith("Win")) {
            /* Go to Windows Settings! */
            window.open("ms-settings:network", "_blank");
        }
    });
#endif

    return transition_slide(id, 1, intent);
}

/*===========================================================================*/

/* Login screens (with no online session active) */

enum
{
    WGCL_LOGIN_TEXTFIELD = GUI_LAST,
    WGCL_LOGIN_SUBMIT,
    WGCL_LOGIN_SIGNUP,
    WGCL_LOGIN_DONE
};

static struct state *login_back;
static struct state *login_next;

static int (*login_back_fn)(struct state *);
static int (*login_next_fn)(struct state *);

static int login_introduction = 1;

static int login_result = 0;

static int login_enter_id;

static int login_field_id;
static int login_field_name_id;
static int login_field_password_id;

static char login_field_name[MAXSTR];
static char login_field_password[MAXSTR];

static int login_write_protected;

#ifdef WGCL_ENABLE_CLIPBOARD
static int login_write_hold_lctrl;
static int login_write_hold_rctrl;
#endif

int goto_wgcl_login(struct state *back_state, int (*back_fn)(struct state *),
                    struct state *next_state, int (*next_fn)(struct state *))
{
    login_back = back_state;
    login_next = next_state;

    login_back_fn = back_fn;
    login_next_fn = next_fn;

    return goto_state(&st_wgcl_login);
}

static int wgcl_login_action(int tok, int val)
{
    if (tok == GUI_BACK && login_entertext_mode == 0 &&
        (game_setup_process()
#if NB_HAVE_PB_BOTH==1
     && server_policy_get_d(SERVER_POLICY_EDITION) == 0
#endif
        ))
    {
        audio_play(AUD_DISABLED, 1.0f);
        return 1;
    }

    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
            if (login_entertext_mode == 0)
            {
                login_introduction = 1;
                login_write_protected = 0;
                return login_back_fn ? login_back_fn(login_back) :
                                       login_back ? exit_state(login_back) : 0;
            }
            else
            {
                login_write_protected = 1;
                login_entertext_mode = 0;
                text_input_stop();
#ifdef WGCL_ENABLE_CLIPBOARD
                login_write_hold_lctrl = 0;
                login_write_hold_rctrl = 0;
#endif
                return exit_state(&st_wgcl_login);
            }
            break;

        case GUI_CL:
            wgcl_gui_keyboard_lock_en();
            break;

        case GUI_BS:
            text_input_del();
            break;

        case GUI_CHAR:
#ifdef WGCL_ENABLE_CLIPBOARD
            if ((login_write_hold_lctrl || login_write_hold_rctrl) &&
                (val == 'V' || val == 'v'))
                text_input_paste();
            else
#endif
                text_input_char(val);
            break;

        case WGCL_LOGIN_DONE:
            return login_next_fn ? login_next_fn(login_next) :
                                   login_next ? goto_state(login_next) : 0;
            break;

        case WGCL_LOGIN_TEXTFIELD:
            login_write_protected = 1;
            login_entertext_mode = val;
            return goto_state(&st_wgcl_login);
            break;

        case WGCL_LOGIN_SIGNUP:
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
#if defined(__EMSCRIPTEN__)
            EM_ASM({ window.open("https://pennyball.stynegame.de/signup"); });
#elif _WIN32
            system("explorer https://pennyball.stynegame.de/signup");
#elif defined(__APPLE__)
            system("open https://pennyball.stynegame.de/signup");
#elif defined(__linux__)
            system("x-www-browser https://pennyball.stynegame.de/signup");
#endif
#endif
            break;

        case WGCL_LOGIN_SUBMIT:
            if (login_introduction)
            {
                login_introduction = 0;
                return goto_state(&st_wgcl_login);
            }
            else switch (login_entertext_mode)
            {
                case 1:
                    login_write_protected = 1;
                    SAFECPY(login_field_name, text_input);

                    login_entertext_mode = 0;
                    text_input_stop();
                    return exit_state(&st_wgcl_login);
                    break;

                case 2:
                    login_write_protected = 1;
                    SAFECPY(login_field_password, text_input);

                    login_entertext_mode = 0;
                    text_input_stop();
                    return exit_state(&st_wgcl_login);
                    break;

                default:
#ifndef __EMSCRIPTEN__
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__)
                    if (text_length(login_field_name) < 3 ||
                        text_length(login_field_password) < 14)
                    {
                        audio_play("snd/uierror.ogg", 1.0f);
                        return 1;
                    }

                    login_result = account_wgcl_login(login_field_name, login_field_password);

                    if (login_result)
                    {
                        login_introduction = 1;
                        login_write_protected = 0;
                        return goto_state(&st_wgcl_login_result);
                    }
                    else audio_play("snd/uierror.ogg", 1.0f);
#endif
#endif
            }
            break;
    }

    return 1;
}

static int wgcl_login_gui_introduction(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("Login to WGCL?"), GUI_MED, GUI_COLOR_DEFAULT);
        gui_space(id);
        gui_multi(id, _("With using Pennyball + Neverball WGCL\n"
                        "(Web server Game Core Launcher),\n"
                        "you can sync your wallets and consumables\n"
                        "whenever you go and where you left off."),
                      GUI_SML, GUI_COLOR_WHT);
        gui_space(id);

        if ((jd = gui_harray(id)))
        {
            gui_start(jd, _("Login"), GUI_SML, WGCL_LOGIN_SUBMIT, 0);

            if (!game_setup_process()
#if NB_HAVE_PB_BOTH==1
#ifndef __EMSCRIPTEN__
                && current_platform == PLATFORM_PC
#endif
                && server_policy_get_d(SERVER_POLICY_EDITION) != 0
#endif
                )
                gui_state(jd, _("Cancel"), GUI_SML, GUI_BACK, 0);
        }

        gui_layout(id, 0, 0);
    }

    return id;
}

static int wgcl_login_gui_keyboard(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        switch (login_entertext_mode)
        {
            case 1:
                gui_title_header(id, _("Name / E-Mail"), GUI_MED, GUI_COLOR_DEFAULT);
                break;
            case 2:
                gui_title_header(id, _("Password"), GUI_MED, GUI_COLOR_DEFAULT);
                break;
        }

        gui_space(id);

        login_field_id = gui_label(id, "XXXXXXXXXXXXXXXX",
                                       GUI_MED, GUI_COLOR_YEL);

        gui_space(id);
        if ((jd = gui_hstack(id)))
        {
            gui_filler(jd);
            wgcl_gui_keyboard_en(jd);
            gui_filler(jd);
        }
        gui_space(id);

        gui_set_trunc(login_field_id, TRUNC_HEAD);

        char password_field_val[MAXSTR];

        switch (login_entertext_mode)
        {
            case 1:
                gui_set_label(login_field_id, text_input);
                break;
            case 2:
                if (!(text_length(text_input) > 0))
                    SAFECPY(password_field_val, "");
                else for (int i = 0; i < text_length(text_input); i++)
                {
                    if (i == 0) SAFECPY(password_field_val, "*");
                    else        SAFECAT(password_field_val, "*");
                }
                gui_set_label(login_field_id, password_field_val);
                break;
        }

        if ((jd = gui_harray(id)))
        {
            login_enter_id = gui_start(jd, _("OK"), GUI_SML, WGCL_LOGIN_SUBMIT, 0);

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            if (current_platform == PLATFORM_PC)
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

        gui_layout(id, 0, 0);
    }

    return id;
}

static int wgcl_login_gui_forms(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        if ((jd = gui_harray(id)))
        {
            login_field_name_id = gui_start(jd, "XXXXXXXXXXXXXX", GUI_SML, WGCL_LOGIN_TEXTFIELD, 1);
            gui_label(jd, _("Name / E-Mail"), GUI_SML, GUI_COLOR_DEFAULT);
            gui_set_trunc(login_field_name_id, TRUNC_HEAD);
        }
        if ((jd = gui_harray(id)))
        {
            login_field_password_id = gui_state(jd, "XXXXXXXXXXXXXX", GUI_SML, WGCL_LOGIN_TEXTFIELD, 2);
            gui_label(jd, _("Password"), GUI_SML, GUI_COLOR_DEFAULT);
            gui_set_trunc(login_field_password_id, TRUNC_HEAD);
        }

        gui_space(id);
        gui_state(id, _("Sign up"), GUI_SML, WGCL_LOGIN_SIGNUP, 0);
        gui_space(id);

        if ((jd = gui_harray(id)))
        {
            gui_state(jd, _("Login"), GUI_SML, WGCL_LOGIN_SUBMIT, 0);
            gui_space(jd);

            if (!game_setup_process()
#if NB_HAVE_PB_BOTH==1
                && server_policy_get_d(SERVER_POLICY_EDITION) != 0
#endif
                )
                gui_state(jd, _("Cancel"), GUI_SML, GUI_BACK, 0);
            else
                gui_space(jd);
        }

        gui_set_label(login_field_name_id, login_field_name);

        char password_field_val[MAXSTR];

        if (!(text_length(login_field_password) > 0))
            SAFECPY(password_field_val, "");
        else for (int i = 0; i < text_length(login_field_password); i++)
        {
            if (i == 0) SAFECPY(password_field_val, "*");
            else        SAFECAT(password_field_val, "*");
        }

        gui_set_label(login_field_password_id, password_field_val);

        gui_layout(id, 0, 0);
    }

    return id;
}

static void on_text_input(int typing)
{
    if (login_field_id)
    {
        char password_field_val[MAXSTR];

        if (login_entertext_mode == 1)
            gui_set_label(login_field_id, text_input);
        else
        {
            for (int i = 0; i < text_length(text_input); i++)
            {
                if (i == 0) SAFECPY(password_field_val, "*");
                else        SAFECAT(password_field_val, "*");
            }

            gui_set_label(login_field_id, password_field_val);
        }

        if (typing)
            audio_play(AUD_MENU, 1.0f);
    }
}

static int wgcl_login_enter(struct state *st, struct state *prev, int intent)
{
    conf_common_init(wgcl_login_action, 1);

    if (login_entertext_mode != 0)
        text_input_start(on_text_input);

    if (login_introduction)
        return transition_slide(wgcl_login_gui_introduction(), 1, intent);

    return transition_slide(login_entertext_mode == 0 ? wgcl_login_gui_forms() :
                                                        wgcl_login_gui_keyboard(),
                            1, intent);
}

static int wgcl_login_leave(struct state *st, struct state *next, int id, int intent)
{
    if (login_entertext_mode == 0 &&
        !login_write_protected)
    {
        SAFECPY(login_field_name,     "");
        SAFECPY(login_field_password, "");
    }

    login_write_protected = 0;

    return conf_common_leave(st, next, id, intent);
}

static void wgcl_login_paint(int id, float t)
{
    conf_common_paint(id, t);

    if ((current_platform != PLATFORM_PC || console_gui_shown()) &&
        login_entertext_mode != 0)
        console_gui_keybd_paint();
}

static int wgcl_login_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            && current_platform == PLATFORM_PC
#endif
            )
            return wgcl_login_action(GUI_BACK, 0);

        else if (login_entertext_mode != 0)
        {
            if (c == '\b' || c == 0x7F)
            {
                gui_focus(login_enter_id);
                return wgcl_login_action(GUI_BS, 0);
            }
            else
            {
                gui_focus(login_enter_id);
                return 1;
            }
#ifdef WGCL_ENABLE_CLIPBOARD
            if (c == SDLK_LCTRL) login_write_hold_lctrl = d;
            if (c == SDLK_RCTRL) login_write_hold_rctrl = d;
#endif
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

            if (login_entertext_mode != 0)
                return wgcl_login_action(tok, (tok == GUI_CHAR && login_entertext_mode != 0 ?
                                               wgcl_gui_keyboard_char(val) :
                                               val));
        }
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            wgcl_login_action(GUI_BACK, 0);

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_X, b) &&
            login_entertext_mode != 0)
            wgcl_login_action(GUI_BS, 0);
        else if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L2, b) &&
                 login_entertext_mode != 0)
            wgcl_login_action(GUI_CL, 0);
        else if (config_tst_d(CONFIG_JOYSTICK_BUTTON_R2, b) &&
                 login_entertext_mode != 0)
            wgcl_login_action(WGCL_LOGIN_SUBMIT, 0);
    }
    else
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L2, b) &&
            login_entertext_mode != 0)
            wgcl_login_action(GUI_CL, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

/* Login result screens (with no online session active) */

static int wgcl_login_result_gui_success(void)
{
    int id;

    if ((id = gui_vstack(0)))
    {
        char desc_info[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(desc_info, MAXSTR,
#else
        sprintf(desc_info,
#endif
                N_("Thank you for logging in.\n"
                   "You have now linked to your account:\n"
                   "%s"),
                config_get_s(CONFIG_PLAYER));

        gui_title_header(id, _("Logged in"), GUI_MED, GUI_COLOR_DEFAULT);
        gui_space(id);
        gui_multi(id, _(desc_info), GUI_SML, GUI_COLOR_WHT);
        gui_space(id);
        gui_start(id, _("OK"), GUI_SML, WGCL_LOGIN_DONE, 0);

        gui_layout(id, 0, 0);
    }

    return id;
}

static int wgcl_login_result_enter(struct state *st, struct state *prev, int intent)
{
    conf_common_init(wgcl_login_action, 1);

    return transition_slide(wgcl_login_result_gui_success(), 1, intent);
}

/*===========================================================================*/

enum
{
    WGCL_LOGOUT_SUBMIT = GUI_LAST,
};

static int logout_confirm_delete = 0;

/* Logout confirm screen */

int goto_wgcl_logout(struct state *back)
{
    login_back = back;

    return goto_state(&st_wgcl_logout_confirm);
}

static int wgcl_logout_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__)
    if (tok == WGCL_LOGOUT_SUBMIT)
    {
        if (!account_wgcl_logout())
            return 1;

        account_wgcl_init();
        account_wgcl_load();
    }
#endif

    return exit_state(login_back);
}

static int wgcl_logout_confirm_gui(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("Logout?"), GUI_MED, GUI_COLOR_RED);
        gui_space(id);
        gui_multi(id, _("While you logging out, you'll not be able\n"
                        "to save wallets into the player account's cloud."),
                      GUI_SML, GUI_COLOR_WHT);
        gui_space(id);

        if ((jd = gui_harray(id)))
        {
            gui_start(jd, _("Logout"), GUI_SML, WGCL_LOGOUT_SUBMIT, 0);
#if NB_HAVE_PB_BOTH==1
#ifndef __EMSCRIPTEN__
            if (current_platform == PLATFORM_PC)
#endif
#endif
            {
                gui_state(jd, _("Cancel"), GUI_SML, GUI_BACK, 0);
            }
        }

        gui_layout(id, 0, 0);
    }

    return id;
}

static int wgcl_logout_confirm_enter(struct state *st, struct state *prev, int intent)
{
    audio_play("snd/warning.ogg", 1.0f);

    conf_common_init(wgcl_logout_action, 1);

    return transition_slide(wgcl_logout_confirm_gui(), 1, intent);
}

/*###########################################################################*/

struct state st_wgcl_error_offline =
{
    wgcl_error_offline_enter,
    conf_common_leave,
    conf_common_paint,
    common_timer,
    common_point,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

struct state st_wgcl_login =
{
    wgcl_login_enter,
    wgcl_login_leave,
    wgcl_login_paint,
    common_timer,
    common_point,
    common_stick,
    NULL,
    common_click,
    wgcl_login_keybd,
    common_buttn
};

struct state st_wgcl_login_result =
{
    wgcl_login_result_enter,
    conf_common_leave,
    conf_common_paint,
    common_timer,
    common_point,
    common_stick,
    NULL,
    common_click,
    common_keybd,
    common_buttn
};

struct state st_wgcl_logout_confirm =
{
    wgcl_logout_confirm_enter,
    conf_common_leave,
    conf_common_paint,
    common_timer,
    common_point,
    common_stick,
    NULL,
    common_click,
    common_keybd,
    common_buttn
};
