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
#include "campaign.h"
#include "st_intro_covid.h"
#endif

#include "log.h"
#include "gui.h"
#include "transition.h"
#include "util.h"
#include "audio.h"
#include "config.h"
#include "demo.h"
#include "progress.h"
#include "text.h"
#include "common.h"
#include "key.h"

#include "game_server.h"
#include "game_client.h"
#include "game_common.h"

#include "st_conf.h"
#include "st_save.h"
#include "st_shared.h"

/*---------------------------------------------------------------------------*/

struct state st_save;
struct state st_clobber;
struct state st_lockdown;
struct state st_save_error;

/*---------------------------------------------------------------------------*/

static struct state *ok_state;
static struct state *cancel_state;

int goto_save(struct state *ok, struct state *cancel)
{
    ok_state     = ok;
    cancel_state = cancel;

#if defined(COVID_HIGH_RISK)
    return goto_state(&st_lockdown);
#elif defined(DEMO_QUARANTINED_MODE)
    /* Lockdown duration time. DO NOT EDIT! */
    int nolockdown; DEMO_LOCKDOWN_RANGE_NIGHT(nolockdown, 16, 8);
    if (!nolockdown && curr_status() == GAME_FALL)
        return goto_state(&st_lockdown);
    else return goto_state(&st_save);
#else
    return goto_state(&st_save);
#endif
}

/*---------------------------------------------------------------------------*/

static int allow_entertext;
static int file_id;
static int enter_id;

enum
{
    SAVE_OK = GUI_LAST
};

static void save_update_enter_btn(void)
{
    int name_accepted = text_length(text_input) > 2 &&
                        strcmp("Last", text_input) != 0;

    for (int i = 0; i < text_length(text_input); i++)
    {
        if (text_input[i] == '\\' || text_input[i] == '/' || text_input[i] == ':'  ||
            text_input[i] == '*'  || text_input[i] == '?' || text_input[i] == '"'  ||
            text_input[i] == '<'  || text_input[i] == '>' || text_input[i] == '|')
        {
            name_accepted = 0;
            break;
        }
    }

    gui_set_state(enter_id, name_accepted ? SAVE_OK : GUI_NONE, 0);
    gui_set_color(enter_id,
                  name_accepted ? gui_wht : gui_gry,
                  name_accepted ? gui_wht : gui_gry);
}

static int save_action(int tok, int val)
{
    KEYBOARD_GAMEMENU_ACTION(SAVE_OK);

    int save_nolockdown = 0;

    switch (tok)
    {
        case GUI_BACK:
            return exit_state(cancel_state);

        case SAVE_OK:
#if NB_HAVE_PB_BOTH==1 && defined(DEMO_QUARANTINED_MODE) && !defined(DEMO_LOCKDOWN_COMPLETE)
            /* Lockdown duration time. DO NOT EDIT! */

            DEMO_LOCKDOWN_RANGE_NIGHT(
                save_nolockdown,
                DEMO_LOCKDOWN_RANGE_NIGHT_START_HOUR_DEFAULT,
                DEMO_LOCKDOWN_RANGE_NIGHT_END_HOUR_DEFAULT
            );

            if (!save_nolockdown && curr_status() == GAME_FALL)
                return goto_state(&st_lockdown);
#endif

            for (int i = 0; i < text_length(text_input); i++)
            {
                if (text_input[i] == '\\' || text_input[i] == '/' || text_input[i] == ':' ||
                    text_input[i] == '*'  || text_input[i] == '?' || text_input[i] == '"' ||
                    text_input[i] == '<'  || text_input[i] == '>' || text_input[i] == '|')
                {
                    log_errorf("Can't accept other charsets!: %c\n", text_input[i]);
                    return 1;
                }
            }

            if (text_length(text_input) < 3 || strcmp("Last", text_input) == 0)
                return 1;

            if (demo_exists(text_input))
                return goto_state(&st_clobber);
            else
                return demo_rename(text_input) ? exit_state(ok_state) :
                                                 goto_state(&st_save_error);

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

static int save_gui(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        allow_entertext = 1;

        gui_title_header(id, _("Replay Name"), GUI_MED, GUI_COLOR_DEFAULT);
        gui_space(id);

        file_id = gui_label(id, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
                                GUI_SML, GUI_COLOR_YEL);

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
            enter_id = gui_start(jd, _("Save"), GUI_SML, SAVE_OK, 0);
            gui_space(jd);
            gui_state(jd, _("Cancel"), GUI_SML, GUI_BACK, 0);
        }

        gui_layout(id, 0, 0);

        gui_set_trunc(file_id, TRUNC_HEAD);
        gui_set_label(file_id, text_input);
    }

    return id;
}

static void on_text_input(int typing)
{
    if (file_id)
    {
        gui_set_label(file_id, text_input);

        if (typing)
            audio_play(AUD_2_2_0_KEYBD_CHAR, 1.0f);

        save_update_enter_btn();
    }
}

static int save_enter(struct state *st, struct state *prev, int intent)
{
    const char *name;
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    if (campaign_used())
    {
        name = demo_format_name(config_get_s(CONFIG_REPLAY_NAME),
                                "campaign",
                                level_name(curr_level()), curr_status());

        if (curr_mode() == MODE_HARDCORE)
            name = demo_format_name(config_get_s(CONFIG_REPLAY_NAME),
                                    "hardcore",
                                    level_name(curr_level()), curr_status());
    }
    else
#endif
    if (curr_mode() == MODE_STANDALONE)
        name = "standalone";
    else
    {

        const char *curr_setid = set_id(curr_set());
        char curr_setid_final[MAXSTR];

        if (!curr_setid)
        {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(curr_setid_final, MAXSTR,
#else
            sprintf(curr_setid_final,
#endif
                    _("none_%d"), curr_set());
        }
        else SAFECPY(curr_setid_final, curr_setid);

        const char *curr_lvlname = level_name(curr_level());

        name = demo_format_name(config_get_s(CONFIG_REPLAY_NAME),
                                curr_setid_final,
                                curr_lvlname ? curr_lvlname : "0", curr_status());
    }

    text_input_start(on_text_input);
    text_input_str(name, 0);

    return transition_slide(save_gui(), 1, intent);
}

static int save_leave(struct state *st, struct state *next, int id, int intent)
{
    allow_entertext = 1;
    text_input_stop();

    if (next == &st_null)
    {
        progress_exit();

        campaign_quit();
        set_quit();

        game_server_free(NULL);
        game_client_free(NULL);
    }

    return transition_slide(id, 0, intent);
}

static void save_paint(int id, float t)
{
    game_client_draw(0, t);

    gui_paint(id);
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    if ((current_platform != PLATFORM_PC || console_gui_shown()) &&
        allow_entertext)
        console_gui_keybd_paint();
#endif
}

static int save_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT)
            return save_action(GUI_BACK, 0);

        if (c == '\b' || c == 0x7F)
        {
            gui_focus(enter_id);
            return save_action(GUI_BS, 0);
        }
        else
        {
            gui_focus(enter_id);
            return 1;
        }
    }
    return 1;
}

static int save_buttn(int b, int d)
{
    if (d)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
        {
            int tok = gui_token(gui_active());
            int val = gui_value(gui_active());

            return save_action(tok, (tok == GUI_CHAR && allow_entertext ?
                                     gui_keyboard_char(val) :
                                     val));
        }
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return save_action(GUI_BACK, 0);

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_X, b) &&
            allow_entertext)
            save_action(GUI_BS, 0);
        else if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L2, b) &&
                 allow_entertext)
            save_action(GUI_CL, 0);
        else if (config_tst_d(CONFIG_JOYSTICK_BUTTON_R2, b) &&
                 allow_entertext)
            save_action(SAVE_OK, 0);
    }
    else
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L2, b) &&
            allow_entertext)
            save_action(GUI_CL, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int clobber_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    if (tok == SAVE_OK)
    {
#ifdef DEMO_QUARANTINED_MODE
        /* Lockdown duration time. DO NOT EDIT! */
        int nolockdown; DEMO_LOCKDOWN_RANGE_NIGHT(nolockdown, 16, 8);
        if (!nolockdown && curr_status() == GAME_FALL)
            return goto_state(&st_lockdown);
#endif

        return demo_rename(text_input) ? exit_state(ok_state) :
                                         goto_state(&st_save_error);
    }

    return exit_state(&st_save);
}

static int clobber_enter(struct state *st, struct state *prev, int intent)
{
    audio_play(AUD_WARNING, 1.0f);

    int id, jd, kd, file_id;

    if ((id = gui_vstack(0)))
    {
        kd = gui_title_header(id, _("Overwrite?"), GUI_MED, GUI_COLOR_RED);
        gui_space(id);
        file_id = gui_label(id, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
                                GUI_SML, GUI_COLOR_YEL);
        gui_space(id);

        if ((jd = gui_harray(id)))
        {
            gui_start(jd, _("Cancel"),    GUI_SML, GUI_BACK, 0);
            gui_state(jd, _("Overwrite"), GUI_SML, SAVE_OK, 0);
        }

        gui_pulse(kd, 1.2f);
        gui_layout(id, 0, 0);

        gui_set_trunc(file_id, TRUNC_TAIL);
        gui_set_label(file_id, text_input);
    }

    return transition_slide(id, 1, intent);
}

static int clobber_keybd(int c, int d)
{
    if (d && c == KEY_EXIT)
        return clobber_action(GUI_BACK, 0);

    return 1;
}

static int clobber_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return clobber_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return clobber_action(GUI_BACK, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int lockdown_action(int tok, int val)
{
    audio_play(AUD_BACK, 1.0f);

    return exit_state(cancel_state);
}

static int lockdown_enter(struct state *st, struct state *prev, int intent)
{
    audio_play("snd/uierror.ogg", 1.0f);

    int id, jd;
    
    if ((id = gui_vstack(0)))
    {
        jd = gui_title_header(id, _("Locked"), GUI_MED, gui_red, gui_blk);
        gui_space(id);
#ifdef COVID_HIGH_RISK
        gui_multi(id, _("Replays have locked down\n"
                        "during high risks!"), GUI_SML, GUI_COLOR_RED);
#else
        gui_multi(id, _("Replays have locked down\n"
                        "between 16:00 - 8:00 (4:00 PM - 8:00 AM)."),
                      GUI_SML, GUI_COLOR_RED);
#endif
        gui_space(id);
        gui_start(id, _("OK"), GUI_SML, GUI_BACK, 0);
        gui_pulse(jd, 1.2f);
        gui_layout(id, 0, 0);
    }

    return transition_slide(id, 1, intent);
}

static int lockdown_keybd(int c, int d)
{
    return (d && c == KEY_EXIT) ? lockdown_action(GUI_BACK, 0) : 1;
}

static int lockdown_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return lockdown_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return lockdown_action(GUI_BACK, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int save_error_enter(struct state *st, struct state *prev, int intent)
{
    audio_play("snd/uierror.ogg", 1.0f);

    int id;

    if ((id = gui_vstack(0)))
    {
        char desc[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(desc, MAXSTR,
#else
        sprintf(desc,
#endif
                _("Please check your permissions\n"
                  "before save your replay.\n%s"),
                fs_error());

        gui_title_header(id, _("Save failed!"), GUI_MED, gui_gry, gui_red);
        gui_space(id);
        gui_multi(id, desc, GUI_SML, GUI_COLOR_WHT);
    }

    gui_layout(id, 0, 0);

    return transition_slide(id, 1, intent);
}

static int save_error_keybd(int c, int d)
{
    return (d && c == KEY_EXIT) ? exit_state(&st_save) : 1;
}

static int save_error_buttn(int b, int d)
{
    return (d && b == config_get_d(CONFIG_JOYSTICK_BUTTON_A)) ?
           exit_state(&st_save) : 1;
}

/*---------------------------------------------------------------------------*/

struct state st_save = {
    save_enter,
    save_leave,
    save_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    save_keybd,
    save_buttn
};

struct state st_clobber = {
    clobber_enter,
    shared_leave,
    shared_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    clobber_keybd,
    clobber_buttn
};

struct state st_lockdown = {
    lockdown_enter,
    shared_leave,
    shared_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    lockdown_keybd,
    lockdown_buttn
};

struct state st_save_error = {
    save_error_enter,
    shared_leave,
    shared_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click_basic,
    save_error_keybd,
    save_error_buttn
};
