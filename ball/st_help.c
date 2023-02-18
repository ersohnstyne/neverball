/*
 * Copyright (C) 2023 Microsoft / Neverball authors
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

/* Have some Switchball guides? */

#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
#include "console_control_gui.h"
#endif

#if NB_HAVE_PB_BOTH==1
#include "campaign.h"
#include "account.h"

#include "networking.h"
#include "accessibility.h"
#include "lang_switchball.h"
#endif

#include "progress.h"
#include "geom.h"
#include "gui.h"
#include "audio.h"
#include "config.h"
#include "demo.h"
#include "video.h"

#include "game_common.h"
#include "game_server.h"
#include "game_client.h"

#include "st_title.h"
#include "st_help.h"
#include "st_shared.h"

#if defined(LEVELGROUPS_INCLUDES_CAMPAIGN) && NB_HAVE_PB_BOTH==1
#define SWITCHBALL_HELP
#endif

/*---------------------------------------------------------------------------*/

struct state st_help_demo;

/*---------------------------------------------------------------------------*/

enum
{
    HELP_SELECT = GUI_LAST,
    HELP_DEMO,
    HELP_RULES_PREMIUM,
    HELP_NEXT
};

#if defined(SWITCHBALL_HELP)
enum
{
    PAGE_INTRODUCTION,
    PAGE_MORPHS_AND_GENERATORS,
    PAGE_MACHINES,
    PAGE_CONTROLS,
    PAGE_MODES,
    PAGE_MODES_SPECIAL,
    PAGE_TRICKS
};
#else
enum
{
    PAGE_RULES,
    PAGE_CONTROLS,
    PAGE_MODES,
    PAGE_MODES_SPECIAL,
    PAGE_TRICKS
};
#endif

static const char demos[][16] = {
    "gui/rules1.nbr",
    "gui/rules2.nbr",
    "gui/tricks1.nbr",
    "gui/tricks2.nbr"
};

static const char demos_xbox[][21] = {
    "gui/rules1_xbox.nbr",
    "gui/rules2_xbox.nbr",
    "gui/tricks1_xbox.nbr",
    "gui/tricks2_xbox.nbr"
};

#if !defined(SWITCHBALL_HELP)
static int page = PAGE_RULES;
#else
static int help_open = 0;
static int help_page_current = 1;
static int help_page_limit = 1;
static int help_page_category = PAGE_INTRODUCTION;
#endif


/*---------------------------------------------------------------------------*/

static int help_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

#if defined(SWITCHBALL_HELP)
    switch (tok)
    {
    case GUI_BACK:
        if (help_open)
        {
            help_open = 0;
            help_page_current = 1;
            return goto_state(&st_help);
        }
        else
            return goto_state(&st_title);
        break;
    case HELP_DEMO:
        progress_init(MODE_NONE);
#ifndef __EMSCRIPTEN__
        if (demo_replay_init(current_platform == PLATFORM_PC ?
                             demos[val] : demos_xbox[val],
#else
        if (demo_replay_init(demos[val],
#endif
            NULL, NULL, NULL, NULL, NULL, NULL))
            return goto_state(&st_help_demo);
        break;

    case HELP_SELECT:
        help_page_category = val;

        switch (val)
        {
        case PAGE_INTRODUCTION:
            help_page_limit = 3;
            break;
        case PAGE_MORPHS_AND_GENERATORS:
        case PAGE_MACHINES:
            help_page_limit = 2;
            break;
        default:
            help_page_limit = 1;
            break;
        }

        help_open = 1;
        return goto_state(&st_help);
        break;
    case HELP_NEXT:
        help_page_current++;
        return goto_state(&st_help);
        break;
    }
#else
    switch (tok)
    {
    case GUI_BACK:
        page = PAGE_RULES;
        return goto_state(&st_title);

    case HELP_DEMO:
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
        if (demo_replay_init(current_platform == PLATFORM_PC ?
                             demos[val] : demos_xbox[val],
#else
        if (demo_replay_init(demos[val],
#endif
            NULL, NULL, NULL, NULL, NULL, NULL))
            return goto_state(&st_help_demo);
        break;

    case HELP_SELECT:
        page = val;
        return goto_state(&st_help);
    }
#endif
    return 1;
}

/*---------------------------------------------------------------------------*/

#if !defined(SWITCHBALL_HELP)
static int help_button(int id, const char *text, int token, int value)
{
    int jd = gui_state(id, text, GUI_SML, token, value);

    /* Hilight current page. */

    if (token == HELP_SELECT && value == page)
    {
        gui_set_hilite(jd, 1);
        gui_focus(jd);
    }

    return jd;
}

static int help_menu(int id)
{
    int jd, kd;

    gui_space(id);

    if ((jd = gui_hstack(id)))
    {
        if (xbox_show_gui())
            gui_filler(jd);

        if ((kd = gui_harray(jd)))
        {
            help_button(kd, _("Tricks"),   HELP_SELECT, PAGE_TRICKS);
            help_button(kd, _("Special"),  HELP_SELECT, PAGE_MODES_SPECIAL);
            help_button(kd, _("Modes"),    HELP_SELECT, PAGE_MODES);
            help_button(kd, _("Controls"), HELP_SELECT, PAGE_CONTROLS);
            help_button(kd, _("Rules"),    HELP_SELECT, PAGE_RULES);
        }

        if (!xbox_show_gui())
            gui_space(jd);

        gui_filler(jd);

        if (!xbox_show_gui())
            help_button(jd, _("Back"),     GUI_BACK, 0);
    }

    return jd;
}
#endif

static int get_premium_rules()
{
    return 1;
}

/*---------------------------------------------------------------------------*/

#if defined(SWITCHBALL_HELP)
static int page_introduction(int id)
{
    int w = video.device_w;
    int h = video.device_h;

    int jd, kd;

    gui_space(id);

    if (help_page_current == 1)
        gui_multi(id,
                  _("The goal of Switchball is to get your ball to the\\"
                    "end of the level and board the gyrocopter.\\"
                    "Once you have completed a level\\"
                    "the next will be unlocked and so on.\\"
                    "Eventually, new worlds will be unlocked as well."),
                  GUI_SML, gui_wht, gui_wht);
    if (help_page_current == 2)
    {
        if ((jd = gui_hstack(id)))
        {
            if ((kd = gui_vstack(jd)))
            {
                gui_multi(kd,
                          _("The Checkpoint resurrects\\"
                            "your ball. If anything bad\\"
                            "happens, you will start over\\"
                            "from here."),
                          GUI_SML, gui_wht, gui_wht);
                gui_filler(kd);
            }
            gui_space(jd);
            if ((kd = gui_vstack(jd)))
            {
                gui_image(kd, "gui/help/introduction1.jpg", 5 * w / 16, 5 * h / 16);
                gui_filler(kd);
            }
        }
    }
    if (help_page_current == 3)
    {
        if ((jd = gui_hstack(id)))
        {
            if ((kd = gui_vstack(jd)))
            {
                gui_multi(kd,
                          _("The Timer shows how fast you have\\"
                            "completed the level. By completing\\"
                            "a level as fast as you can, you can\\"
                            "earn bronze, silver and gold medals,\\"
                            "which will reward Achievements\\"
                            "if you collect enough of them."),
                          GUI_SML, gui_wht, gui_wht);
                gui_filler(kd);
            }
            gui_space(jd);
            if ((kd = gui_vstack(jd)))
            {
                gui_image(kd, "gui/help/introduction2.jpg", 5 * w / 16, 5 * h / 16);
                gui_filler(kd);
            }
        }
    }

    return id;
}
#else
static int page_rules(int id)
{
    const char *s0 = _(
        "Move the mouse or joystick\\"
        "or use keyboard arrows to\\"
        "tilt the floor causing the\\"
        "ball to roll.\\");
    const char *s1 = _(
        "Roll over coins to collect\\"
        "them.  Collect coins to\\"
        "unlock the goal and finish\\"
        "the level.\\");

    const char *s_xbox = _(
        "Move the left stick to\\"
        "tilt the floor causing the\\"
        "ball to roll.\\");
    const char *s_pc = _(
        "Move the mouse or use keyboard\\"
        "to tilt the floor causing the\\"
        "ball to roll.\\");

    int w = video.device_w;
    int h = video.device_h;

    int jd, kd, ld;

    if ((jd = gui_hstack(id)))
    {
        gui_filler(jd);

        if ((kd = gui_varray(jd)))
        {
            if ((ld = gui_vstack(kd)))
            {
                gui_space(ld);
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
                gui_multi(ld, current_platform == PLATFORM_PC ? s_pc : s_xbox, GUI_SML, gui_wht, gui_wht);
#else
                gui_multi(ld, s_pc, GUI_SML, gui_wht, gui_wht);
#endif
                gui_filler(ld);
            }

            if ((ld = gui_vstack(kd)))
            {
                gui_space(ld);
                gui_multi(ld, s1, GUI_SML, gui_wht, gui_wht);
                gui_filler(ld);
            }
        }

        gui_space(jd);

        if ((kd = gui_varray(jd)))
        {
            if (get_premium_rules())
            {
                /* This rule guides contains replays and is used with Premium */
                if ((ld = gui_vstack(kd)))
                {
                    gui_space(ld);
                    gui_image(ld, "gui/rules1.jpg", w / 4, h / 4);
                    gui_state(ld, _("Watch demo"), GUI_SML, 0, 0);
                    gui_filler(ld);
                    gui_set_state(ld, HELP_DEMO, 0);
                }

                if ((ld = gui_vstack(kd)))
                {
                    gui_space(ld);
                    gui_image(ld, "gui/rules2.jpg", w / 4, h / 4);
                    gui_state(ld, _("Watch demo"), GUI_SML, 0, 0);
                    gui_filler(ld);
                    gui_set_state(ld, HELP_DEMO, 1);
                }
            } else {
                if ((ld = gui_vstack(kd)))
                {
                    gui_space(ld);
                    gui_image(ld, "gui/help1.jpg", 5 * w / 16, 5 * h / 16);
                    gui_filler(ld);
                }

                if ((ld = gui_vstack(kd)))
                {
                    gui_space(ld);
                    gui_image(ld, "gui/help2.jpg", 5 * w / 16, 5 * h / 16);
                    gui_filler(ld);
                }
            }
        }

        gui_filler(jd);
    }
    return id;
}
#endif

static void controls_pc(int id)
{
    /*
     * For PC, use SDL_GetKeyName
     */

    const SDL_Keycode k_exit = KEY_EXIT;
    const SDL_Keycode k_auto = config_get_d(CONFIG_KEY_CAMERA_TOGGLE);
    const SDL_Keycode k_cam1 = config_get_d(CONFIG_KEY_CAMERA_1);
    const SDL_Keycode k_cam2 = config_get_d(CONFIG_KEY_CAMERA_2); /* DEPRECATED!: Replaced to static camera in the next version */
    const SDL_Keycode k_cam3 = config_get_d(CONFIG_KEY_CAMERA_3);
    const SDL_Keycode k_restart = config_get_d(CONFIG_KEY_RESTART);
    const SDL_Keycode k_shot = KEY_SCREENSHOT;

    const char *s_rotate = _("Left and right mouse buttons rotate the view.\\"
                             "Hold Shift for faster view rotation.");
    const char *s_exit = _("Exit / Pause");
    const char *s_camAuto = _("Auto-Camera");
    const char *s_camera1 = _("Chase Camera");
    const char *s_camera2 = _("Static Camera"); /* DEPRECATED!: Replaced to static camera in the next version */
    const char *s_camera3 = _("Manual Camera");
    const char *s_restart = _("Restart Level");
    const char *s_shot = _("Screenshot");

    const SDL_Keycode k_rot_l = config_get_d(CONFIG_KEY_CAMERA_R);
    const SDL_Keycode k_rot_r = config_get_d(CONFIG_KEY_CAMERA_L);
    char temp_k_rot_l[32]; SAFECPY(temp_k_rot_l, SDL_GetKeyName(k_rot_l));
    char temp_k_rot_r[32]; SAFECPY(temp_k_rot_r, SDL_GetKeyName(k_rot_r));

    char s_rotate_new[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(s_rotate_new, dstSize,
#else
    sprintf(s_rotate_new,
#endif
        _("Use %s / %s buttons to rotate the view.\\"
          "Hold %s for faster view rotation."),
        temp_k_rot_l,
        temp_k_rot_r,
        SDL_GetKeyName(config_get_d(CONFIG_KEY_ROTATE_FAST)));

    int jd, kd;

    gui_space(id);

    if ((jd = gui_vstack(id)))
    {
        if ((kd = gui_harray(jd)))
        {
            gui_label(kd, s_exit, GUI_SML, gui_wht, gui_wht);
            gui_label(kd, SDL_GetKeyName(k_exit), GUI_SML, gui_yel, gui_yel);
        }
        if ((kd = gui_harray(jd)))
        {
            gui_label(kd, s_restart, GUI_SML, gui_wht, gui_wht);
            gui_label(kd, SDL_GetKeyName(k_restart), GUI_SML, gui_yel, gui_yel);
        }
        if ((kd = gui_harray(jd)))
        {
            gui_label(kd, s_camAuto, GUI_SML, gui_wht, gui_wht);
            gui_label(kd, SDL_GetKeyName(k_auto), GUI_SML, gui_yel, gui_yel);
        }
        if ((kd = gui_harray(jd)))
        {
            gui_label(kd, s_camera1, GUI_SML, gui_wht, gui_wht);
            gui_label(kd, SDL_GetKeyName(k_cam1), GUI_SML, gui_yel, gui_yel);
        }
        if ((kd = gui_harray(jd)))
        {
            gui_label(kd, s_camera2, GUI_SML, gui_wht, gui_wht);
            gui_label(kd, SDL_GetKeyName(k_cam2), GUI_SML, gui_yel, gui_yel);
        }
        if ((kd = gui_harray(jd)))
        {
            gui_label(kd, s_camera3, GUI_SML, gui_wht, gui_wht);
            gui_label(kd, SDL_GetKeyName(k_cam3), GUI_SML, gui_yel, gui_yel);
        }

        /* Screenshot won't be able to do that. We don't need this. */

        if ((kd = gui_harray(jd)))
        {
            gui_label(kd, _("Max Speed"), GUI_SML, gui_wht, gui_wht);
            gui_label(kd, _("LMB"), GUI_SML, gui_yel, gui_yel);
        }

        gui_set_rect(jd, GUI_ALL);
    }

    gui_space(id);

    gui_multi(id, s_rotate_new, GUI_SML, gui_wht, gui_wht);
    gui_space(id);
    gui_multi(id, _("Note that you can change keyboard and\\controller controls in the Controls Settings menu."), GUI_SML, gui_wht, gui_wht);
}

#ifndef __EMSCRIPTEN__
static void controls_console(int id)
{
    const char *s_rotate = _("Move the right stick left or right to rotate the view.");
    const char *s_exit = _("Exit");
    const char *s_pause = _("Pause");
    const char *s_camToggle = _("Cycle Camera Mode");

    int jd, kd;

    gui_space(id);

    if ((jd = gui_vstack(id)))
    {
        if ((kd = gui_harray(jd)))
        {
            gui_label(kd, s_exit, GUI_SML, gui_wht, gui_wht);
            create_b_button(kd, config_get_d(CONFIG_JOYSTICK_BUTTON_B));
        }
        if ((kd = gui_harray(jd)))
        {
            gui_label(kd, s_pause, GUI_SML, gui_wht, gui_wht);
            create_start_button(kd, config_get_d(CONFIG_JOYSTICK_BUTTON_START));
        }
        if ((kd = gui_harray(jd)))
        {
            gui_label(kd, s_camToggle, GUI_SML, gui_wht, gui_wht);
            create_x_button(kd, config_get_d(CONFIG_JOYSTICK_BUTTON_X));
        }

        gui_set_rect(jd, GUI_ALL);
    }

    gui_space(id);

    gui_multi(id, s_rotate, GUI_SML, gui_wht, gui_wht);
    gui_space(id);
    gui_multi(id, _("Note that you can change controller controls\\in the Controls Settings menu."), GUI_SML, gui_wht, gui_wht);
}
#endif

static int page_controls(int id)
{
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
    if (current_platform == PLATFORM_PC)
        controls_pc(id);
    else
        controls_console(id);
#else
    controls_pc(id);
#endif

    return id;
}

static int page_modes(int id)
{
    int jd;

    gui_space(id);

    if ((jd = gui_vstack(id)))
    {
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        if ((server_policy_get_d(SERVER_POLICY_PLAYMODES_UNLOCKED_MODE_CAREER)
             || campaign_career_unlocked())
            && server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_CAREER))
        {
            gui_label(jd, _("Classic Mode"), GUI_SML, 0, 0);
            gui_multi(jd,
                      _("Finish a level before the time runs out.\\"
                        "You need to collect coins in order to open the goal."),
                      GUI_SML, gui_wht, gui_wht);
        }
        else if (server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_CAREER))
        {
            gui_label(jd, _("Classic Mode"), GUI_SML, gui_gry, gui_red);
            gui_multi(jd,
                      _("Complete the game to unlock this Mode."),
                      GUI_SML, gui_wht, gui_wht);
        }
        else
        {
            gui_label(jd, _("Classic Mode"), GUI_SML, gui_gry, gui_red);
            gui_multi(jd,
                      _("Career mode is not available\\with server group policy."),
                      GUI_SML, gui_wht, gui_wht);
        }
#else
        gui_label(jd, _("Classic Mode"), GUI_SML, 0, 0);
        gui_multi(jd,
                  _("Finish a level before the time runs out.\\"
                    "You need to collect coins in order to open the goal."),
                  GUI_SML, gui_wht, gui_wht);
#endif

        gui_set_rect(jd, GUI_ALL);
    }

    gui_space(id);

    if ((jd = gui_vstack(id)))
    {
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
#ifdef CONFIG_INCLUDES_ACCOUNT
        if (account_get_d(ACCOUNT_SET_UNLOCKS) > 0
            && (server_policy_get_d(SERVER_POLICY_PLAYMODES_UNLOCKED_MODE_CAREER)
                || campaign_career_unlocked()
#if NB_STEAM_API==0 && NB_EOS_SDK==0
                || config_cheat()
#endif
            ))
        {
            if ((accessibility_get_d(ACCESSIBILITY_SLOWDOWN) < 100 || config_cheat())
                || !server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_CHALLENGE))
                gui_label(jd, _("Challenge Mode"), GUI_SML, gui_gry, gui_red);
            else
                gui_label(jd, _("Challenge Mode"), GUI_SML, 0, 0);

            if (!server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_CHALLENGE)
                && server_policy_get_d(SERVER_POLICY_EDITION) == 0)
                gui_multi(jd, _("Upgrade to Pro edition to play this Mode."), GUI_SML, gui_wht, gui_wht);
            else if (!server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_CHALLENGE))
                gui_multi(jd, _("Challenge Mode is not available\\with server group policy."), GUI_SML, gui_wht, gui_wht);
#if NB_STEAM_API==0 && NB_EOS_SDK==0
            else if (accessibility_get_d(ACCESSIBILITY_SLOWDOWN) < 100 || config_cheat())
                gui_multi(jd, _("Challenge Mode is not available\\with slowdown or cheat."), GUI_SML, gui_wht, gui_wht);
#else
            else if (accessibility_get_d(ACCESSIBILITY_SLOWDOWN) < 100 || config_cheat())
                gui_multi(jd, _("Challenge Mode is not available\\with slowdown."), GUI_SML, gui_wht, gui_wht);
#endif
#ifdef CONFIG_INCLUDES_ACCOUNT
            else if (account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES))
                gui_multi(jd,
                          _("Start playing from the first level of the set.\\"
                            "You start with which you've already purchased from the shop.\\"
                            "Earn an extra ball for each 100 coins collected."),
                          GUI_SML, gui_wht, gui_wht);
#endif
            else
                gui_multi(jd,
                    _("Start playing from the first level of the set.\\"
                      "You start with only three balls, do not lose them.\\"
                      "Earn an extra ball for each 100 coins collected."),
                    GUI_SML, gui_wht, gui_wht);
        }
        else
#endif
        {
            gui_label(jd, _("Challenge Mode"), GUI_SML, gui_gry, gui_red);

            if (!server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_CHALLENGE)
                && server_policy_get_d(SERVER_POLICY_EDITION) == 0)
                gui_multi(jd, _("Upgrade to Pro edition to play this Mode"), GUI_SML, gui_wht, gui_wht);
            else if (!server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_CHALLENGE))
                gui_multi(jd, _("Challenge Mode is not available\\with server group policy."), GUI_SML, gui_wht, gui_wht);
#if NB_STEAM_API==0 && NB_EOS_SDK==0
            else if (accessibility_get_d(ACCESSIBILITY_SLOWDOWN) < 100 || config_cheat())
                gui_multi(jd, _("Challenge Mode is not available\\with slowdown or cheat."), GUI_SML, gui_wht, gui_wht);
#else
            else if (accessibility_get_d(ACCESSIBILITY_SLOWDOWN) < 100 || config_cheat())
                gui_multi(jd, _("Challenge Mode is not available\\with slowdown."), GUI_SML, gui_wht, gui_wht);
#endif
            else
            gui_multi(jd,
                _("Complete the game to unlock this Mode."),
                GUI_SML, gui_wht, gui_wht);
        }
#else
#if NB_HAVE_PB_BOTH == 1
        if (!server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_CHALLENGE)
            && server_policy_get_d(SERVER_POLICY_EDITION) == 0)
        {
            gui_label(jd, _("Challenge Mode"), GUI_SML, gui_gry, gui_red);
            gui_multi(jd, _("Upgrade to Pro edition to play this Mode"), GUI_SML, gui_wht, gui_wht);
        }
        else if (!server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_CHALLENGE))
        {
            gui_label(jd, _("Challenge Mode"), GUI_SML, gui_gry, gui_red);
            gui_multi(jd, _("Challenge Mode is not available\\with server group policy."), GUI_SML, gui_wht, gui_wht);
        }
#if NB_STEAM_API==0 && NB_EOS_SDK==0
        else if (accessibility_get_d(ACCESSIBILITY_SLOWDOWN) < 100 || config_cheat())
        {
            gui_label(jd, _("Challenge Mode"), GUI_SML, gui_gry, gui_red);
            gui_multi(jd, _("Challenge Mode is not available\\with slowdown or cheat."), GUI_SML, gui_wht, gui_wht);
        }
#else
        else if (accessibility_get_d(ACCESSIBILITY_SLOWDOWN) < 100)
        {
            gui_label(jd, _("Challenge Mode"), GUI_SML, gui_gry, gui_red);
            gui_multi(jd, _("Challenge Mode is not available\\with slowdown."), GUI_SML, gui_wht, gui_wht);
        }
#endif
        else
#endif
        {
            gui_label(jd, _("Challenge Mode"), GUI_SML, 0, 0);
            gui_multi(jd,
                      _("Start playing from the first level of the set.\\"
                        "You start with only three balls, do not lose them.\\"
                        "Earn an extra ball for each 100 coins collected."),
                      GUI_SML, gui_wht, gui_wht);
        }
#endif

        gui_set_rect(jd, GUI_ALL);
    }

    return id;
}

static int page_modes_special(int id)
{
    int jd;

    gui_space(id);

    if ((jd = gui_vstack(id)))
    {
#ifdef CONFIG_INCLUDES_ACCOUNT
        if (account_get_d(ACCOUNT_SET_UNLOCKS) > 0
            && (server_policy_get_d(SERVER_POLICY_PLAYMODES_UNLOCKED_MODE_CAREER)
                || campaign_career_unlocked()))
        {
            if (account_get_d(ACCOUNT_PRODUCT_LEVELS))
            {
                gui_label(jd, _("Boost Rush Mode"), GUI_SML, 0, 0);
                gui_multi(jd,
                          _("Same as Challenge Mode, do not decrease them.\\"
                            "Increase 14,3% the acceleration for each 10\\coins collected."),
                          GUI_SML, gui_wht, gui_wht);
            }
            else if (server_policy_get_d(SERVER_POLICY_EDITION) > 0)
            {
                gui_label(jd, _("Boost Rush Mode"), GUI_SML, gui_gry, gui_red);
                gui_multi(jd,
                          _("Buy Extra Levels in the Shop to unlock this Mode."),
                          GUI_SML, gui_wht, gui_wht);
            }
            else
            {
                gui_label(jd, _("Boost Rush Mode"), GUI_SML, gui_gry, gui_red);
                gui_multi(jd,
                          _("Upgrade to Pro edition to play this Mode."),
                          GUI_SML, gui_wht, gui_wht);
            }
        }
        else
        {
            gui_label(jd, _("Boost Rush Mode"), GUI_SML, gui_gry, gui_red);

            if (account_get_d(ACCOUNT_PRODUCT_LEVELS))
            {
                gui_multi(jd,
                          _("Complete the game to unlock this Mode."),
                          GUI_SML, gui_wht, gui_wht);
            }
            else if (server_policy_get_d(SERVER_POLICY_EDITION) > -1)
            {
                gui_multi(jd,
                          _("Buy Extra Levels in the Shop to unlock this Mode."),
                          GUI_SML, gui_wht, gui_wht);
            }
            else
            {
                gui_multi(jd,
                          _("Upgrade to Home edition to play this Mode."),
                          GUI_SML, gui_wht, gui_wht);
            }
        }
#else
        gui_label(jd, _("Boost Rush Mode"), GUI_SML, gui_gry, gui_red);
        gui_multi(jd, _("Neverball Game Modes\\requires Account configuration!"), GUI_SML, gui_red, gui_red);
#endif

        gui_set_rect(jd, GUI_ALL);
    }
    
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    gui_space(id);

    if ((jd = gui_vstack(id)))
    {
        if (server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_HARDCORE))
        {
            if ((accessibility_get_d(ACCESSIBILITY_SLOWDOWN) < 100
#if NB_STEAM_API==0 && NB_EOS_SDK==0
                || config_cheat()
#endif
                || (config_get_d(CONFIG_SMOOTH_FIX) && video_perf() < 25)))
            {
                gui_label(jd, _("Hardcore Mode"), GUI_SML, gui_gry, gui_red);
#if NB_STEAM_API==0 && NB_EOS_SDK==0
                gui_multi(jd,
                          _("Hardcore Mode is not available\\with slowdown, cheat or smooth fix."),
                          GUI_SML, gui_wht, gui_wht);
#else
                gui_multi(jd,
                          _("Hardcore Mode is not available\\with slowdown or smooth fix."),
                          GUI_SML, gui_wht, gui_wht);
#endif
            }
            else if ((server_policy_get_d(SERVER_POLICY_PLAYMODES_UNLOCKED_MODE_HARDCORE) || campaign_hardcore_unlocked()))
            {
                gui_label(jd, _("Hardcore Mode"), GUI_SML, 0, 0);
                gui_multi(jd,
                          _("Same as Challenge Mode. You must play the entire game\\"
                            "with only one ball. All checkpoints were removed\\"
                            "and the game cannot be saved."),
                          GUI_SML, gui_wht, gui_wht);
            }
            else
            {
                gui_label(jd, _("Hardcore Mode"), GUI_SML, gui_gry, gui_red);
                gui_multi(jd,
                          _("Achieve all Silver Medals or above in Best Time\\to unlock this Mode."),
                          GUI_SML, gui_wht, gui_wht);
            }
        }
        else
        {
            if (server_policy_get_d(SERVER_POLICY_EDITION) < 0)
            {
                gui_label(jd, _("Hardcore Mode"), GUI_SML, gui_gry, gui_red);
                gui_multi(jd,
                          _("Upgrade to Pro edition to play this Mode."),
                          GUI_SML, gui_wht, gui_wht);
            }
            else
            {
                gui_label(jd, _("Hardcore Mode"), GUI_SML, gui_gry, gui_red);
                gui_multi(jd,
                          _("Hardcore Mode is not available\\with server group policy."),
                          GUI_SML, gui_wht, gui_wht);
            }
        }
        gui_set_rect(jd, GUI_ALL);
    }
#endif

    return id;
}

#if defined(SWITCHBALL_HELP)
static int page_morphs_and_generators(int id)
{
    int w = video.device_w;
    int h = video.device_h;

    int jd, kd;

    gui_space(id);

    if (help_page_current == 1)
    {
        if ((jd = gui_hstack(id)))
        {
            if ((kd = gui_vstack(jd)))
            {
                gui_multi(kd,
                          _("Roll into a morph to switch\\"
                            "to the ball presented by the\\"
                            "hologram. There are four different\\"
                            "balls, Marbleball, Metalball,\\"
                            "Airball and Powerball."),
                          GUI_SML, gui_wht, gui_wht);
                gui_filler(kd);
            }
            gui_space(jd);
            if ((kd = gui_vstack(jd)))
            {
                gui_image(kd, "gui/help/mandg1.jpg", 5 * w / 16, 5 * h / 16);
                gui_filler(kd);
            }
        }
    }
    if (help_page_current == 2)
    {
        /*if ((jd = gui_hstack(id)))
        {
            if ((kd = gui_vstack(jd)))
            {
                gui_multi(kd,
                          _("When you have the Powerball,\\"
                            "roll into a generator to charge\\"
                            "the ball with a powerup.\\"
                            "There are three different powerups,\\"
                            "Jump, Boost and Magnetic."),
                          GUI_SML, gui_wht, gui_wht);
                gui_filler(kd);
            }
            gui_space(jd);
            if ((kd = gui_vstack(jd)))
            {
                gui_image(kd, "gui/help/mandg2.jpg", 5 * w / 16, 5 * h / 16);
                gui_filler(kd);
            }
        }*/

        gui_multi(id,
                  _("When you have the Powerball,\\"
                    "roll into a generator to charge\\"
                    "the ball with a powerup.\\"
                    "There are three different powerups,\\"
                    "Jump, Boost and Magnetic."),
                  GUI_SML, gui_wht, gui_wht);
    }

    return id;
}

static int page_machines(int id)
{
    int w = video.device_w;
    int h = video.device_h;

    int jd, kd;

    gui_space(id);

    if (help_page_current == 1)
    {
        if ((jd = gui_hstack(id)))
        {
            if ((kd = gui_vstack(jd)))
            {
                gui_multi(kd,
                          _("When you have the Airball,\\"
                            "roll onto the pump to inflate\\"
                            "the Airball with helium.\\"
                            "The Airball will then fly\\"
                            "for a brief period of time."),
                          GUI_SML, gui_wht, gui_wht);
                gui_filler(kd);
            }
            gui_space(jd);
            if ((kd = gui_vstack(jd)))
            {
                gui_image(kd, "gui/help/machines1.jpg", 5 * w / 16, 5 * h / 16);
                gui_filler(kd);
            }
        }
    }
    if (help_page_current == 2)
    {
        if ((jd = gui_hstack(id)))
        {
            if ((kd = gui_vstack(jd)))
            {
                gui_multi(kd,
                          _("There are many different machines\\"
                            "in Switchball. Some machines\\"
                            "have a switch connected to them.\\"
                            "Roll onto the button to turn\\"
                            "on and off."),
                          GUI_SML, gui_wht, gui_wht);
                gui_filler(kd);
            }
            gui_space(jd);
            if ((kd = gui_vstack(jd)))
            {
                gui_image(kd, "gui/help/machines2.jpg", 5 * w / 16, 5 * h / 16);
                gui_filler(kd);
            }
        }
    }

    return id;
}
#endif

static int page_tricks(int id)
{
    const char *s0 = _(
        "Corners can be used to jump.\\"
        "Get rolling and take aim\\"
        "at the angle. You may be able\\"
        "to reach new places.\\");
    const char *s1 = _(
        "Pushing in 2 directions increases\\"
        "the roll speed. Use the manual\\"
        "camera and turn the camera by 45\\"
        "degrees for best results.\\");

    if (config_get_d(CONFIG_TILTING_FLOOR))
        s1 = _(
            "Tilting in 2 directions increases\\"
            "the slope. Use the manual camera\\"
            "and turn the camera by 45\\"
            "degrees for best results.\\");

    int w = video.device_w;
    int h = video.device_h;

    int jd, kd, ld;

    if ((jd = gui_hstack(id)))
    {
        gui_space(id);

        if ((kd = gui_varray(jd)))
        {
            if ((ld = gui_vstack(kd)))
            {
                gui_space(ld);
                gui_image(ld, "gui/tricks1.jpg", w / 4, h / 4);
                gui_state(ld, _("Watch demo"), GUI_SML, 0, 0);
                gui_filler(ld);
                gui_set_state(ld, HELP_DEMO, 2);
            }

            if ((ld = gui_vstack(kd)))
            {
                gui_space(ld);
                gui_image(ld, "gui/tricks2.jpg", w / 4, h / 4);
                gui_state(ld, _("Watch demo"), GUI_SML, 0, 0);
                gui_filler(ld);
                gui_set_state(ld, HELP_DEMO, 3);
            }
        }

        gui_space(jd);

        if ((kd = gui_varray(jd)))
        {
            if ((ld = gui_vstack(kd)))
            {
                gui_space(ld);
                gui_multi(ld, s0, GUI_SML, gui_wht, gui_wht);
                gui_filler(ld);
            }

            if ((ld = gui_vstack(kd)))
            {
                gui_space(ld);
                gui_multi(ld, s1, GUI_SML, gui_wht, gui_wht);
                gui_filler(ld);
            }
        }

        gui_filler(jd);
    }
    return id;
}

/*---------------------------------------------------------------------------*/

static int help_gui(void)
{
    int id, jd, kd;

    if ((id = gui_vstack(0)))
    {
#if defined(SWITCHBALL_HELP)
        if ((jd = gui_vstack(id)))
        {
            const char *help_header = gt_prefix("menu^Help");

            if (help_open)
            {
                switch (help_page_category)
                {
                case PAGE_INTRODUCTION:          help_header = _("Introduction"); break;
                case PAGE_MORPHS_AND_GENERATORS: help_header = _("Powerups"); break;
                case PAGE_MACHINES:              help_header = _("Special objects"); break;
                case PAGE_CONTROLS:              help_header = _("Controls"); break;
                case PAGE_MODES:                 help_header = _("Modes"); break;
                case PAGE_MODES_SPECIAL:         help_header = _("Special"); break;
                case PAGE_TRICKS:                help_header = _("Tricks"); break;
                }
            }

            if ((kd = gui_hstack(jd)))
            {
                gui_label(kd, help_header, GUI_SML, gui_yel, gui_red);
                gui_filler(kd);

                if (!help_open)
                {
#ifndef __EMSCRIPTEN__
                    if (current_platform == PLATFORM_PC)
#endif
                        gui_start(kd, _("Back"), GUI_SML, GUI_BACK, 0);
                }
            }
        }

        if (help_open)
        {
            switch (help_page_category)
            {
            case PAGE_INTRODUCTION:          page_introduction(id);          break;
            case PAGE_MORPHS_AND_GENERATORS: page_morphs_and_generators(id); break;
            case PAGE_MACHINES:              page_machines(id);              break;
            case PAGE_CONTROLS:              page_controls(id);              break;
            case PAGE_MODES:                 page_modes(id);                 break;
            case PAGE_MODES_SPECIAL:         page_modes_special(id);         break;
            case PAGE_TRICKS:                page_tricks(id);                break;
            }

            gui_space(id);

            if ((jd = gui_harray(id)))
            {
                if (help_page_current == help_page_limit)
                    gui_start(jd, _("OK"), GUI_SML, GUI_BACK, 0);
                else
                {
#ifndef __EMSCRIPTEN__
                    if (current_platform == PLATFORM_PC)
#endif
                        gui_state(jd, _("Skip"), GUI_SML, GUI_BACK, 0);

                    gui_start(jd, _("Next"), GUI_SML, HELP_NEXT, 0);
                }
            }
        }
        else
        {
            gui_space(id);
            gui_state(id, _("Introduction"), GUI_SML, HELP_SELECT, PAGE_INTRODUCTION);
            gui_state(id, _("Powerups"), GUI_SML, HELP_SELECT, PAGE_MORPHS_AND_GENERATORS);
            gui_state(id, _("Special objects"), GUI_SML, HELP_SELECT, PAGE_MACHINES);
            gui_state(id, _("Controls"), GUI_SML, HELP_SELECT, PAGE_CONTROLS);
            gui_state(id, _("Modes"), GUI_SML, HELP_SELECT, PAGE_MODES);
            gui_state(id, _("Special"), GUI_SML, HELP_SELECT, PAGE_MODES_SPECIAL);
            gui_state(id, _("Tricks"), GUI_SML, HELP_SELECT, PAGE_TRICKS);
        }
#else
        help_menu(id);

        switch (page)
        {
        case PAGE_RULES:         page_rules(id);         break;
        case PAGE_CONTROLS:      page_controls(id);      break;
        case PAGE_MODES:         page_modes(id);         break;
        case PAGE_MODES_SPECIAL: page_modes_special(id); break;
        case PAGE_TRICKS:        page_tricks(id);        break;
        }
#endif

#if defined(SWITCHBALL_HELP)
        gui_layout(id, 0, 0);
#else
        gui_layout(id, 0, +1);
#endif
    }

    return id;
}

static int help_enter(struct state *st, struct state *prev)
{
    return help_gui();
}

static void help_paint(int id, float t)
{
    game_client_draw(0, t);

    gui_paint(id);
#ifndef __EMSCRIPTEN__
    if (xbox_show_gui())
        xbox_control_list_gui_paint();
#endif
}

static int help_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
            && current_platform == PLATFORM_PC
#endif
            )
            return help_action(GUI_BACK, 0);
    }
    return 1;
}

static int help_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return help_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return help_action(GUI_BACK, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int demo_freeze_all;

static int help_demo_enter(struct state *st, struct state *prev)
{
    demo_freeze_all = 0;
    game_client_fly(0.0f);
    return 0;
}

static void help_demo_leave(struct state *st, struct state *next, int id)
{
    demo_replay_stop(0);
}

static void help_demo_paint(int id, float t)
{
    game_client_draw(0, t);
}

static void help_demo_timer(int id, float dt)
{
    game_step_fade(dt);

    if (!demo_freeze_all)
    {
        geom_step(dt);

        if (!demo_replay_step(dt))
        {
            demo_freeze_all = 1;
            goto_state(&st_help);
        }

        game_client_blend(demo_replay_blend());
    }
}

static int help_demo_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT)
            return goto_state(&st_help);
    }
    return 1;
}

static int help_demo_buttn(int b, int d)
{
    if (d)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return goto_state(&st_help);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

struct state st_help = {
    help_enter,
    shared_leave,
    help_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    help_keybd,
    help_buttn
};

struct state st_help_demo = {
    help_demo_enter,
    help_demo_leave,
    help_demo_paint,
    help_demo_timer,
    NULL,
    NULL,
    NULL,
    NULL,
    help_demo_keybd,
    help_demo_buttn
};
