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

/*
 * HACK: Used with console version
 */
#include "console_control_gui.h"

/* Have some Switchball guides? */

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
#include "transition.h"
#include "audio.h"
#include "config.h"
#include "demo.h"
#include "video.h"
#include "key.h"

#include "game_common.h"
#include "game_server.h"
#include "game_client.h"

#include "st_common.h"
#include "st_title.h"
#include "st_help.h"
#include "st_shared.h"

#if NB_HAVE_PB_BOTH==1 && defined(LEVELGROUPS_INCLUDES_CAMPAIGN) &&\
    defined(SWITCHBALL_GUI)
//#define SWITCHBALL_HELP
#endif

#if NB_HAVE_PB_BOTH!=1 && \
    (defined(SWITCHBALL_GUI) || defined(SWITCHBALL_HELP) || \
     defined(LEVELGROUPS_INCLUDES_CAMPAIGN) || defined(CONFIG_INCLUDES_ACCOUNT))
#error Security compilation error: Preprocessor definitions can be used it, \
       once you have transferred or joined into the target Discord Server, \
       and verified and promoted as Developer Role. \
       This invite link can be found under https://discord.gg/qnJR263Hm2/.
#endif

#if defined(__WII__)
/* We're using SDL 1.2 on Wii, which has SDLKey instead of SDL_Keycode. */
typedef SDLKey SDL_Keycode;
#endif

/*---------------------------------------------------------------------------*/

struct state st_help_demo;

/*---------------------------------------------------------------------------*/

enum
{
    HELP_SELECT = GUI_LAST,
    HELP_DEMO,
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
#if NB_HAVE_PB_BOTH==1
    PAGE_MODES_SPECIAL,
#endif
    PAGE_TRICKS
};
#endif

static const char demos[][16] = {
    "gui/rules1.nbr",
    "gui/rules2.nbr",
    "gui/tricks1.nbr",
    "gui/tricks2.nbr"
};

#ifndef __EMSCRIPTEN__
static const char demos_xbox[][21] = {
    "gui/rules1_xbox.nbr",
    "gui/rules2_xbox.nbr",
    "gui/tricks1_xbox.nbr",
    "gui/tricks2_xbox.nbr"
};
#endif

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
                return exit_state(&st_help);
            }
            else
                return exit_state(&st_title);

        case HELP_DEMO:
            progress_reinit(MODE_NONE);
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            if (progress_replay_full(current_platform == PLATFORM_PC ?
                                     demos[val] : demos_xbox[val],
#else
            if (progress_replay_full(demos[val],
#endif
                                     0, 0, 0, 0, 0, 0))

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

        case HELP_NEXT:
            help_page_current++;
            return goto_state(&st_help);
    }
#else
    switch (tok)
    {
        case GUI_BACK:
            return exit_state(&st_title);

        case HELP_DEMO:
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            if (progress_replay_full(current_platform == PLATFORM_PC ?
                                     demos[val] : demos_xbox[val],
#else
            if (progress_replay_full(demos[val],
#endif
                                     0, 0, 0, 0, 0, 0))

                return goto_state(&st_help_demo);
            break;

        case HELP_SELECT:
            if (val != page)
            {
                int old_page = page;

                page = val;

                return page < old_page ? exit_state(&st_help) : goto_state(&st_help);
            }
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
#ifndef __EMSCRIPTEN__
        if (console_gui_shown())
#endif
            gui_filler(jd);

        if ((kd = gui_harray(jd)))
        {
            help_button(kd, _("Tricks"),   HELP_SELECT, PAGE_TRICKS);
#if NB_HAVE_PB_BOTH==1
            help_button(kd, _("Special"),  HELP_SELECT, PAGE_MODES_SPECIAL);
#endif
            help_button(kd, _("Modes"),    HELP_SELECT, PAGE_MODES);
            help_button(kd, _("Controls"), HELP_SELECT, PAGE_CONTROLS);
            help_button(kd, _("Rules"),    HELP_SELECT, PAGE_RULES);
        }

        if (!console_gui_shown())
            gui_space(jd);

        gui_filler(jd);

        if (!console_gui_shown())
            gui_back_button(jd);
    }

    return jd;
}
#endif

/*---------------------------------------------------------------------------*/

#ifdef SWITCHBALL_HELP
static int page_introduction(int id)
{
    int w = video.device_w;
    int h = video.device_h;

    int jd, kd;

    gui_space(id);

    const int ww = 5 * MIN(w, h) / 16;
    const int hh = ww / 4 * 3;

    if (help_page_current == 1)
        gui_multi(id, _("The goal of Switchball is to get your ball to the\n"
                        "end of the level and board the gyrocopter.\n"
                        "Once you have completed a level\n"
                        "the next will be unlocked and so on.\n"
                        "Eventually, new worlds will be unlocked as well."),
                      GUI_SML, GUI_COLOR_WHT);
    if (help_page_current == 2)
    {
        if ((jd = gui_hstack(id)))
        {
            if ((kd = gui_vstack(jd)))
            {
                gui_multi(kd, _("The Checkpoint resurrects\n"
                                "your ball. If anything bad\n"
                                "happens, you will start over\n"
                                "from here."),
                              GUI_SML, GUI_COLOR_WHT);
                gui_filler(kd);
            }
            gui_space(jd);
            if ((kd = gui_vstack(jd)))
            {
                gui_image(kd, "gui/help/introduction1.jpg", ww, hh);
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
                gui_multi(kd, _("The Timer shows how fast you have\n"
                                "completed the level. By completing\n"
                                "a level as fast as you can, you can\n"
                                "earn bronze, silver and gold medals,\n"
                                "which will reward Achievements\n"
                                "if you collect enough of them."),
                              GUI_SML, GUI_COLOR_WHT);
                gui_filler(kd);
            }
            gui_space(jd);
            if ((kd = gui_vstack(jd)))
            {
                gui_image(kd, "gui/help/introduction2.jpg", ww, hh);
                gui_filler(kd);
            }
        }
    }

    return id;
}
#else
static int help_allow_control_demos(int id)
{
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    return fs_exists(current_platform == PLATFORM_PC ?
                     demos[id] : demos_xbox[id]);
#else
    return fs_exists(demos[id]);
#endif
}

static int page_rules(int id)
{
    const char *s0 = _("Move the mouse or joystick\n"
                       "or use keyboard arrows to\n"
                       "tilt the floor causing the\n"
                       "ball to roll.\n");
    const char *s1 = _("Roll over coins to collect\n"
                       "them.  Collect coins to\n"
                       "unlock the goal and finish\n"
                       "the level.\n");

#if defined(__WII__)
    const char *s_wii = _("Tilt the Wii Remote or move\n"
                          "the nunchuck stick to\n"
                          "tilt the floor causing the\n"
                          "ball to roll.\n");
    const char *s_wii_notilt = _("Tilt the Wii Remote or move\n"
                                 "the nunchuck stick to\n"
                                 "move the ball.\n");
#else
    const char *s_xbox = _("Use the left stick to\n"
                           "tilt the floor causing the\n"
                           "ball to roll.\n");
    const char *s_pc = _("Move the mouse or use keyboard\n"
                         "to tilt the floor causing the\n"
                         "ball to roll.\n");

    const char *s_xbox_notilt = _("Use the left stick to\n"
                                  "move the ball.\n");
    const char *s_pc_notilt = _("Move the mouse or use keyboard\n"
                                "to move the ball.\n");
#endif

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
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
#if defined(__WII__)
                gui_multi(ld, config_get_d(CONFIG_TILTING_FLOOR) ? s_wii :
                                                                   s_wii_notilt,
                              GUI_SML, GUI_COLOR_WHT);
#else
                gui_multi(ld, current_platform == PLATFORM_PC ? (config_get_d(CONFIG_TILTING_FLOOR) ? s_pc :
                                                                                                      s_pc_notilt) :
                                                                (config_get_d(CONFIG_TILTING_FLOOR) ? s_xbox :
                                                                                                      s_xbox_notilt),
                              GUI_SML, GUI_COLOR_WHT);
#endif
#else
                gui_multi(ld, s0, GUI_SML, GUI_COLOR_WHT);
#endif
                gui_filler(ld);
            }

            if ((ld = gui_vstack(kd)))
            {
                gui_space(ld);
                gui_multi(ld, s1, GUI_SML, GUI_COLOR_WHT);
                gui_filler(ld);
            }
        }

        gui_space(jd);

        if ((kd = gui_varray(jd)))
        {
#if defined(HELP_RULES_WITH_DEMO) || NB_HAVE_PB_BOTH==1
            if (help_allow_control_demos(0) && help_allow_control_demos(1))
            {
                const int ww = MIN(w, h) / 4;
                const int hh = ww / 4 * 3;

                /* This rule guides contains replays and is used with Premium */
                if ((ld = gui_vstack(kd)))
                {
                    gui_space(ld);
                    gui_image(ld, "gui/rules1.jpg", ww, hh);
                    gui_state(ld, _("Watch demo"), GUI_SML, 0, 0);
                    gui_filler(ld);
                    gui_set_state(ld, HELP_DEMO, 0);
                }

                if ((ld = gui_vstack(kd)))
                {
                    gui_space(ld);
                    gui_image(ld, "gui/rules2.jpg", ww, hh);
                    gui_state(ld, _("Watch demo"), GUI_SML, 0, 0);
                    gui_filler(ld);
                    gui_set_state(ld, HELP_DEMO, 1);
                }
            }
            else
#endif
            {
                const int ww = MIN(w, h) * 5 / 12;
                const int hh = ww / 4 * 3;

                if ((ld = gui_vstack(kd)))
                {
                    gui_space(ld);
                    gui_image(ld, "gui/help1.jpg", ww, hh);
                    gui_filler(ld);
                }

                if ((ld = gui_vstack(kd)))
                {
                    gui_space(ld);
                    gui_image(ld, "gui/help2.jpg", ww, hh);
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

    const SDL_Keycode k_exit    = KEY_EXIT;
    const SDL_Keycode k_auto    = config_get_d(CONFIG_KEY_CAMERA_TOGGLE);
    const SDL_Keycode k_cam1    = config_get_d(CONFIG_KEY_CAMERA_1);
    const SDL_Keycode k_cam2    = config_get_d(CONFIG_KEY_CAMERA_2);
    const SDL_Keycode k_cam3    = config_get_d(CONFIG_KEY_CAMERA_3);
    const SDL_Keycode k_restart = config_get_d(CONFIG_KEY_RESTART);
    const SDL_Keycode k_shot    = KEY_SCREENSHOT;
    const SDL_Keycode k_rot_l   = config_get_d(CONFIG_KEY_CAMERA_R);
    const SDL_Keycode k_rot_r   = config_get_d(CONFIG_KEY_CAMERA_L);

    const char *s_rotate = _("Left and right mouse buttons rotate the view.\n"
                             "Hold Shift for faster view rotation.");
    const char *s_exit    = _("Exit / Pause");
    const char *s_camAuto = _("Auto-Camera");
    const char *s_camera1 = cam_to_str(CAM_1);
    const char *s_camera2 = cam_to_str(CAM_2);
    const char *s_camera3 = cam_to_str(CAM_3);
    const char *s_restart = _("Restart Level");
    const char *s_shot    = _("Screenshot");

    const char *ks_unassigned = _("Unassigned");
    char       *ks_exit       = strdup(SDL_GetKeyName(k_exit));
    char       *ks_restart    = strdup(SDL_GetKeyName(k_restart));
    char       *ks_auto       = strdup(SDL_GetKeyName(k_auto));
    char       *ks_cam1       = strdup(SDL_GetKeyName(k_cam1));
    char       *ks_cam2       = strdup(SDL_GetKeyName(k_cam2));
    char       *ks_cam3       = strdup(SDL_GetKeyName(k_cam3));
    char       *ks_rot_l      = strdup(SDL_GetKeyName(k_rot_l));
    char       *ks_rot_r      = strdup(SDL_GetKeyName(k_rot_r));

    char temp_k_rot_l[32]; SAFECPY(temp_k_rot_l, ks_rot_l && *ks_rot_l ? ks_rot_l : ks_unassigned);
    char temp_k_rot_r[32]; SAFECPY(temp_k_rot_r, ks_rot_r && *ks_rot_r ? ks_rot_r : ks_unassigned);

    char s_rotate_new[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(s_rotate_new, MAXSTR,
#else
    sprintf(s_rotate_new,
#endif
            _("Use %s / %s buttons to rotate the view.\n"
              "Hold %s for faster view rotation."),
            config_get_d(CONFIG_CAMERA_ROTATE_MODE) == 1 ? temp_k_rot_r :
                                                           temp_k_rot_l,
            config_get_d(CONFIG_CAMERA_ROTATE_MODE) == 1 ? temp_k_rot_l :
                                                           temp_k_rot_r,
            SDL_GetKeyName(config_get_d(CONFIG_KEY_ROTATE_FAST)));

    int jd, kd;

    gui_space(id);

    if ((jd = gui_vstack(id)))
    {
        if ((kd = gui_harray(jd)))
        {
            gui_label(kd, s_exit, GUI_SML, GUI_COLOR_WHT);
            gui_label(kd, ks_exit && *ks_exit ? ks_exit : ks_unassigned, GUI_SML, GUI_COLOR_YEL);
        }
#if NB_HAVE_PB_BOTH==1
        if ((kd = gui_harray(jd)))
        {
            gui_label(kd, s_restart, GUI_SML, GUI_COLOR_WHT);
            gui_label(kd, ks_restart && *ks_restart ? ks_restart : ks_unassigned, GUI_SML, GUI_COLOR_YEL);
        }
#endif
        if ((kd = gui_harray(jd)))
        {
            gui_label(kd, s_camAuto, GUI_SML, GUI_COLOR_WHT);
            gui_label(kd, ks_auto && *ks_auto ? ks_auto : ks_unassigned, GUI_SML, GUI_COLOR_YEL);
        }
        if ((kd = gui_harray(jd)))
        {
            gui_label(kd, s_camera1, GUI_SML, GUI_COLOR_WHT);
            gui_label(kd, ks_cam1 && *ks_cam1 ? ks_cam1 : ks_unassigned, GUI_SML, GUI_COLOR_YEL);
        }
        if ((kd = gui_harray(jd)))
        {
            gui_label(kd, s_camera2, GUI_SML, GUI_COLOR_WHT);
            gui_label(kd, ks_cam2 && *ks_cam2 ? ks_cam2 : ks_unassigned, GUI_SML, GUI_COLOR_YEL);
        }
        if ((kd = gui_harray(jd)))
        {
            gui_label(kd, s_camera3, GUI_SML, GUI_COLOR_WHT);
            gui_label(kd, ks_cam3 && *ks_cam3 ? ks_cam3 : ks_unassigned, GUI_SML, GUI_COLOR_YEL);
        }

        if ((kd = gui_harray(jd)))
        {
            gui_label(kd, _("Max Speed"), GUI_SML, GUI_COLOR_WHT);
            gui_label(kd, _("LMB"), GUI_SML, GUI_COLOR_YEL);
        }

#if NB_HAVE_PB_BOTH!=1
#ifndef __EMSCRIPTEN__
        if ((kd = gui_harray(jd)))
        {
            gui_label(kd, s_shot, GUI_SML, GUI_COLOR_WHT);
            gui_label(kd, ks_shot && *ks_shot ? ks_shot : ks_unassigned, GUI_SML, GUI_COLOR_YEL);
        }
#endif
#endif

        gui_set_rect(jd, GUI_ALL);
    }

    gui_space(id);

    gui_multi(id, s_rotate_new, GUI_SML, GUI_COLOR_WHT);
    gui_space(id);
    gui_multi(id, _("Note that you can change keyboard and\n"
                    "controller controls in the Controls Settings menu."),
                  GUI_SML, GUI_COLOR_WHT);
}

static void controls_touch(int id)
{
    gui_space(id);

    gui_multi(id, _("Tap and hold the second finger to rotate the view."),
                  GUI_SML, GUI_COLOR_WHT);
    gui_space(id);
    gui_multi(id, _("Note that you can change touch controls\n"
                    "in the Controls Settings menu."),
                  GUI_SML, GUI_COLOR_WHT);
}

#ifndef __EMSCRIPTEN__
static void controls_console(int id)
{
#if defined(__WII__)
    const char *s_rotate    = _("Use D-Pad to rotate the view.");
#else
    const char *s_rotate    = _("Move the right stick left or right\n"
                                "to rotate the view.");
#endif
    const char *s_exit      = _("Exit");
    const char *s_pause     = _("Pause");
    const char *s_camToggle = _("Cycle Camera Mode");

    int jd, kd;

    gui_space(id);

    if ((jd = gui_vstack(id)))
    {
        if ((kd = gui_harray(jd)))
        {
            gui_label(kd, s_exit, GUI_SML, GUI_COLOR_WHT);
            console_gui_create_b_button(kd, config_get_d(CONFIG_JOYSTICK_BUTTON_B));
        }
        if ((kd = gui_harray(jd)))
        {
            gui_label(kd, s_pause, GUI_SML, GUI_COLOR_WHT);
            console_gui_create_start_button(kd, config_get_d(CONFIG_JOYSTICK_BUTTON_START));
        }
        if ((kd = gui_harray(jd)))
        {
            gui_label(kd, s_camToggle, GUI_SML, GUI_COLOR_WHT);
            console_gui_create_x_button(kd, config_get_d(CONFIG_JOYSTICK_BUTTON_X));
        }

        gui_set_rect(jd, GUI_ALL);
    }

    gui_space(id);

    gui_multi(id, s_rotate, GUI_SML, GUI_COLOR_WHT);
    gui_space(id);
    gui_multi(id, _("Note that you can change controller controls\n"
                    "in the Controls Settings menu."),
                  GUI_SML, GUI_COLOR_WHT);
}
#endif

static int page_controls(int id)
{
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
#if defined(__WII__)
    controls_console(id);
#else
    if (opt_touch) controls_touch(id);
    else if (current_platform == PLATFORM_PC)
        controls_pc(id);
    else controls_console(id);
#endif
#else
    if (opt_touch) controls_touch(id);
    else           controls_pc(id);
#endif

    return id;
}

static int page_modes(int id)
{
    int jd;

    gui_space(id);

    if ((jd = gui_vstack(id)))
    {
        if (server_policy_get_d(SERVER_POLICY_EDITION) == 0 &&
            account_get_d(ACCOUNT_SET_UNLOCKS) < 1)
        {
            gui_label(jd, _("Classic Mode"), GUI_SML, gui_gry, gui_red);
            gui_multi(jd, _("Complete the game to unlock this Mode."),
                          GUI_SML, GUI_COLOR_WHT);
        }
        else
        {
            gui_label(jd, _("Classic Mode"), GUI_SML, 0, 0);
            gui_multi(jd, _("Finish a level before the time runs out.\n"
                            "You need to collect coins in order to open the goal."),
                          GUI_SML, GUI_COLOR_WHT);
        }

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
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
                || config_cheat()
#endif
            ))
        {
            if ((accessibility_get_d(ACCESSIBILITY_SLOWDOWN) < 100 ||
                 config_cheat()) ||
                !server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_CHALLENGE) ||
                CHECK_ACCOUNT_BANKRUPT)
                gui_label(jd, _("Challenge Mode"), GUI_SML, gui_gry, gui_red);
            else
                gui_label(jd, _("Challenge Mode"), GUI_SML, 0, 0);

            if (!server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_CHALLENGE) &&
                server_policy_get_d(SERVER_POLICY_EDITION) == 0)
                gui_multi(jd, _("Upgrade to Pro edition to play this Mode."),
                              GUI_SML, GUI_COLOR_WHT);
            else if (!server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_CHALLENGE))
                gui_multi(jd, _("Challenge Mode is not available\n"
                                "with server group policy."),
                              GUI_SML, GUI_COLOR_WHT);
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
            else if (accessibility_get_d(ACCESSIBILITY_SLOWDOWN) < 100 ||
                     config_cheat())
                gui_multi(jd, _("Challenge Mode is not available\n"
                                "with slowdown or cheat."),
                              GUI_SML, GUI_COLOR_WHT);
#else
            else if (accessibility_get_d(ACCESSIBILITY_SLOWDOWN) < 100 &&
                     !config_cheat())
                gui_multi(jd, _("Challenge Mode is not available\n"
                                "with slowdown."),
                              GUI_SML, GUI_COLOR_WHT);
#endif
            else if (CHECK_ACCOUNT_BANKRUPT)
                gui_multi(jd,
                          _("Your player account is bankrupt.\n"
                            "Restore from the backup or delete the\n"
                            "local account and start over from scratch."),
                          GUI_SML, GUI_COLOR_WHT);
            else if (account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES) > 0 &&
                     server_policy_get_d(SERVER_POLICY_EDITION) > 0)
                gui_multi(jd,
                          _("Start playing from the first level of the set.\n"
                            "You start with which you've already purchased from the shop.\n"
                            "Earn an extra ball for each 100 coins collected."),
                          GUI_SML, GUI_COLOR_WHT);
            else
                gui_multi(jd,
                    _("Start playing from the first level of the set.\n"
                      "You start with only three balls, do not extra paid of them.\n"
                      "Earn an extra ball for each 100 coins collected."),
                    GUI_SML, GUI_COLOR_WHT);
        }
        else
#endif
        {
            gui_label(jd, _("Challenge Mode"), GUI_SML, gui_gry, gui_red);

            if (!server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_CHALLENGE) &&
                server_policy_get_d(SERVER_POLICY_EDITION) == 0)
                gui_multi(jd, _("Upgrade to Pro edition to play this Mode"),
                              GUI_SML, GUI_COLOR_WHT);
            else if (!server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_CHALLENGE))
                gui_multi(jd, _("Challenge Mode is not available\n"
                                "with server group policy."),
                              GUI_SML, GUI_COLOR_WHT);
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
            else if (accessibility_get_d(ACCESSIBILITY_SLOWDOWN) < 100 ||
                     config_cheat())
                gui_multi(jd, _("Challenge Mode is not available\n"
                                "with slowdown or cheat."),
                              GUI_SML, GUI_COLOR_WHT);
#else
            else if (accessibility_get_d(ACCESSIBILITY_SLOWDOWN) < 100 &&
                     !config_cheat())
                gui_multi(jd, _("Challenge Mode is not available\n"
                                "with slowdown."),
                              GUI_SML, GUI_COLOR_WHT);
#endif
            else
            gui_multi(jd, _("Complete the game to unlock this Mode."),
                          GUI_SML, GUI_COLOR_WHT);
        }
#else
#if NB_HAVE_PB_BOTH==1
        if (!server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_CHALLENGE)
            && server_policy_get_d(SERVER_POLICY_EDITION) == 0)
        {
            gui_label(jd, _("Challenge Mode"), GUI_SML, gui_gry, gui_red);
            gui_multi(jd, _("Upgrade to Pro edition to play this Mode"),
                          GUI_SML, GUI_COLOR_WHT);
        }
        else if (!server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_CHALLENGE))
        {
            gui_label(jd, _("Challenge Mode"), GUI_SML, gui_gry, gui_red);
            gui_multi(jd, _("Challenge Mode is not available\n"
                            "with server group policy."),
                          GUI_SML, GUI_COLOR_WHT);
        }
#if NB_STEAM_API==0 && NB_EOS_SDK==0
        else if (accessibility_get_d(ACCESSIBILITY_SLOWDOWN) < 100 ||
                 config_cheat())
        {
            gui_label(jd, _("Challenge Mode"), GUI_SML, gui_gry, gui_red);
            gui_multi(jd, _("Challenge Mode is not available\n"
                            "with slowdown or cheat."),
                          GUI_SML, GUI_COLOR_WHT);
        }
#else
        else if (accessibility_get_d(ACCESSIBILITY_SLOWDOWN) < 100)
        {
            gui_label(jd, _("Challenge Mode"), GUI_SML, gui_gry, gui_red);
            gui_multi(jd, _("Challenge Mode is not available\n"
                            "with slowdown."),
                          GUI_SML, GUI_COLOR_WHT);
        }
#endif
        else
#endif
        {
            gui_label(jd, _("Challenge Mode"), GUI_SML, 0, 0);
            gui_multi(jd, _("Start playing from the first level of the set.\n"
                            "You start with only three balls, do not lose them.\n"
                            "Earn an extra ball for each 100 coins collected."),
                      GUI_SML, GUI_COLOR_WHT);
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

#ifdef CONFIG_INCLUDES_ACCOUNT
    /*if ((jd = gui_vstack(id)))
    {
        if (account_get_d(ACCOUNT_SET_UNLOCKS) > 0 &&
            (server_policy_get_d(SERVER_POLICY_PLAYMODES_UNLOCKED_MODE_CAREER) ||
             campaign_career_unlocked()) &&
            !CHECK_ACCOUNT_BANKRUPT)
        {
            if (account_get_d(ACCOUNT_PRODUCT_LEVELS))
            {
                gui_label(jd, _("Boost Rush Mode"), GUI_SML, 0, 0);
                gui_multi(jd,
                          _("Same as Challenge Mode, do not decrease them.\n"
                            "Increase 14,3% the acceleration for each 10\n"
                            "coins collected."),
                          GUI_SML, GUI_COLOR_WHT);
            }
            else if (server_policy_get_d(SERVER_POLICY_EDITION) > 0)
            {
                gui_label(jd, _("Boost Rush Mode"), GUI_SML, gui_gry, gui_red);
                gui_multi(jd, _("Buy Extra Levels in the Shop to unlock this Mode."),
                              GUI_SML, GUI_COLOR_WHT);
            }
            else
            {
                gui_label(jd, _("Boost Rush Mode"), GUI_SML, gui_gry, gui_red);
                gui_multi(jd, _("Upgrade to Pro edition to play this Mode."),
                              GUI_SML, GUI_COLOR_WHT);
            }
        }
        else
        {
            gui_label(jd, _("Boost Rush Mode"), GUI_SML, gui_gry, gui_red);

            if (server_policy_get_d(SERVER_POLICY_EDITION) < 0 &&
                CHECK_ACCOUNT_ENABLED && !CHECK_ACCOUNT_BANKRUPT)
                gui_multi(jd, _("Upgrade to Pro edition to play this Mode."),
                              GUI_SML, GUI_COLOR_WHT);
            else if (CHECK_ACCOUNT_ENABLED && !CHECK_ACCOUNT_BANKRUPT)
                gui_multi(jd, _("Boost Rush Mode is not available\n"
                                "with server group policy."),
                              GUI_SML, GUI_COLOR_WHT);
            else if (CHECK_ACCOUNT_ENABLED && CHECK_ACCOUNT_BANKRUPT)
                gui_multi(jd, _("Your player account is bankrupt.\n"
                               "Restore from the backup or delete the\n"
                               "local account and start over from scratch."),
                              GUI_SML, GUI_COLOR_WHT);
            else
                gui_multi(jd, _("Boost Rush Mode is not available.\n"
                                "Please check your account settings!"),
                              GUI_SML, GUI_COLOR_WHT);
        }

        gui_set_rect(jd, GUI_ALL);
    }*/
#endif

#if defined(CONFIG_INCLUDES_ACCOUNT) && defined(LEVELGROUPS_INCLUDES_CAMPAIGN)
    //gui_space(id);
#endif

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    if ((jd = gui_vstack(id)))
    {
        if (server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED_MODE_HARDCORE)
#ifdef CONFIG_INCLUDES_ACCOUNT
            && !CHECK_ACCOUNT_BANKRUPT && CHECK_ACCOUNT_ENABLED
#endif
            )
        {
            if ((accessibility_get_d(ACCESSIBILITY_SLOWDOWN) < 100 ||
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
                 config_cheat() ||
#endif
                 (config_get_d(CONFIG_SMOOTH_FIX) && video_perf() < NB_FRAMERATE_MIN)))
            {
                gui_label(jd, _("Hardcore Mode"), GUI_SML, gui_gry, gui_red);
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
                gui_multi(jd, _("Hardcore Mode is not available\n"
                                "with slowdown, cheat or smooth fix."),
                              GUI_SML, GUI_COLOR_WHT);
#else
                gui_multi(jd, _("Hardcore Mode is not available\n"
                                "with slowdown or smooth fix."),
                              GUI_SML, GUI_COLOR_WHT);
#endif
            }
            else if ((server_policy_get_d(SERVER_POLICY_PLAYMODES_UNLOCKED_MODE_HARDCORE) ||
                      campaign_hardcore_unlocked()))
            {
                gui_label(jd, _("Hardcore Mode"), GUI_SML, 0, 0);
                gui_multi(jd, _("Same as Challenge Mode. You must play the entire game\n"
                                "with only one ball. All checkpoints were removed\n"
                                "and the game cannot be saved."),
                              GUI_SML, GUI_COLOR_WHT);
            }
            else
            {
                gui_label(jd, _("Hardcore Mode"), GUI_SML, gui_gry, gui_red);
                gui_multi(jd, _("Achieve all Silver Medals or above in Best Time\n"
                                "to unlock this Mode."),
                              GUI_SML, GUI_COLOR_WHT);
            }
        }
        else
        {
            gui_label(jd, _("Hardcore Mode"), GUI_SML, gui_gry, gui_red);

            if (server_policy_get_d(SERVER_POLICY_EDITION) < 0 &&
                CHECK_ACCOUNT_ENABLED && !CHECK_ACCOUNT_BANKRUPT)
                gui_multi(jd, _("Upgrade to Pro edition to play this Mode."),
                              GUI_SML, GUI_COLOR_WHT);
            else if (CHECK_ACCOUNT_ENABLED && !CHECK_ACCOUNT_BANKRUPT)
                gui_multi(jd, _("Hardcore Mode is not available\n"
                                "with server group policy."),
                              GUI_SML, GUI_COLOR_WHT);
            else if (CHECK_ACCOUNT_ENABLED && CHECK_ACCOUNT_BANKRUPT)
                gui_multi(jd, _("Your player account is bankrupt.\n"
                                "Restore from the backup or delete the\n"
                                "local account and start over from scratch."),
                              GUI_SML, GUI_COLOR_WHT);
            else
                gui_multi(jd, _("Hardcore Mode is not available.\n"
                                "Please check your account settings!"),
                              GUI_SML, GUI_COLOR_WHT);
        }
        gui_set_rect(jd, GUI_ALL);
    }
#endif

#if !defined(CONFIG_INCLUDES_ACCOUNT) && !defined(LEVELGROUPS_INCLUDES_CAMPAIGN)
    gui_multi(id, _("Special game modes requires\n"
                   "LEVELGROUPS_INCLUDES_CAMPAIGN\n"
                   "or CONFIG_INCLUDES_ACCOUNT\n"
                   "preprocessor definitions"),
             GUI_SML, GUI_COLOR_RED);
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

    const int ww = 5 * MIN(w, h) / 16;
    const int hh = ww / 4 * 3;

    if (help_page_current == 1)
    {
        if ((jd = gui_hstack(id)))
        {
            if ((kd = gui_vstack(jd)))
            {
                gui_multi(kd, _("Roll into a morph to switch\n"
                                "to the ball presented by the\n"
                                "hologram. There are four different\n"
                                "balls, Marbleball, Metalball,\n"
                                "Airball and Powerball."),
                              GUI_SML, GUI_COLOR_WHT);
                gui_filler(kd);
            }
            gui_space(jd);
            if ((kd = gui_vstack(jd)))
            {
                gui_image(kd, "gui/help/mandg1.jpg", ww, hh);
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
                gui_multi(kd, _("When you have the Powerball,\n"
                                "roll into a generator to charge\n"
                                "the ball with a powerup.\n"
                                "There are three different powerups,\n"
                                "Jump, Boost and Magnetic."),
                              GUI_SML, GUI_COLOR_WHT);
                gui_filler(kd);
            }
            gui_space(jd);
            if ((kd = gui_vstack(jd)))
            {
                gui_image(kd, "gui/help/mandg2.jpg", ww, hh);
                gui_filler(kd);
            }
        }*/

        gui_multi(id, _("When you have the Powerball,\n"
                        "roll into a generator to charge\n"
                        "the ball with a powerup.\n"
                        "There are three different powerups,\n"
                        "Jump, Boost and Magnetic."),
                      GUI_SML, GUI_COLOR_WHT);
    }

    return id;
}

static int page_machines(int id)
{
    int w = video.device_w;
    int h = video.device_h;

    int jd, kd;

    const int ww = 5 * MIN(w, h) / 16;
    const int hh = ww / 4 * 3;

    gui_space(id);

    if (help_page_current == 1)
    {
        if ((jd = gui_hstack(id)))
        {
            if ((kd = gui_vstack(jd)))
            {
                gui_multi(kd, _("When you have the Airball,\n"
                                "roll onto the pump to inflate\n"
                                "the Airball with helium.\n"
                                "The Airball will then fly\n"
                                "for a brief period of time."),
                              GUI_SML, GUI_COLOR_WHT);
                gui_filler(kd);
            }
            gui_space(jd);
            if ((kd = gui_vstack(jd)))
            {
                gui_image(kd, "gui/help/machines1.jpg", ww, hh);
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
                gui_multi(kd, _("There are many different machines\n"
                                "in Switchball. Some machines\n"
                                "have a switch connected to them.\n"
                                "Roll onto the button to turn\n"
                                "on and off."),
                              GUI_SML, GUI_COLOR_WHT);
                gui_filler(kd);
            }
            gui_space(jd);
            if ((kd = gui_vstack(jd)))
            {
                gui_image(kd, "gui/help/machines2.jpg", ww, hh);
                gui_filler(kd);
            }
        }
    }

    return id;
}
#endif

static int page_tricks(int id)
{
    const char *s0 = N_("Corners can be used to jump.\n"
                        "Get rolling and take aim\n"
                        "at the angle. You may be able\n"
                        "to reach new places.\n");
    const char *s1 = N_("Pushing in 2 directions increases\n"
                        "the roll speed. Use the manual\n"
                        "camera and turn the camera by 45\n"
                        "degrees for best results.\n");

    if (config_get_d(CONFIG_TILTING_FLOOR))
        s1 = N_("Tilting in 2 directions increases\n"
                "the slope. Use the manual camera\n"
                "and turn the camera by 45\n"
                "degrees for best results.\n");

    int w = video.device_w;
    int h = video.device_h;

    int jd, kd, ld;

    const int ww = MIN(w, h) * 2 / 8;
    const int hh = ww / 4 * 3;

    if ((jd = gui_hstack(id)))
    {
        gui_filler(jd);

        if ((kd = gui_varray(jd)))
        {
            if ((ld = gui_vstack(kd)))
            {
                gui_space(ld);
                gui_image(ld, "gui/tricks1.jpg", ww, hh);
                gui_state(ld, _("Watch demo"), GUI_SML, 0, 0);
                gui_filler(ld);
                gui_set_state(ld, HELP_DEMO, 2);
            }

            if ((ld = gui_vstack(kd)))
            {
                gui_space(ld);
                gui_image(ld, "gui/tricks2.jpg", ww, hh);
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
                gui_multi(ld, _(s0), GUI_SML, GUI_COLOR_WHT);
                gui_filler(ld);
            }

            if ((ld = gui_vstack(kd)))
            {
                gui_space(ld);
                gui_multi(ld, _(s1), GUI_SML, GUI_COLOR_WHT);
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
#ifdef SWITCHBALL_HELP
    int root_id, id, jd, kd;

    root_id = gui_root();

    if ((id = gui_vstack(root_id)))
    {
        if ((jd = gui_vstack(id)))
        {
            const char *help_header = gt_prefix("menu^Help");

            if (help_open)
            {
                switch (help_page_category)
                {
                    case PAGE_INTRODUCTION:
                        help_header = _("Introduction");    break;
                    case PAGE_MORPHS_AND_GENERATORS:
                        help_header = _("Powerups");        break;
                    case PAGE_MACHINES:
                        help_header = _("Special objects"); break;
                    case PAGE_CONTROLS:
                        help_header = _("Controls");        break;
                    case PAGE_MODES:
                        help_header = _("Modes");           break;
                    case PAGE_MODES_SPECIAL:
                        help_header = _("Special");         break;
                    case PAGE_TRICKS:
                        help_header = _("Tricks");          break;
                }
            }

            if ((kd = gui_hstack(jd)))
            {
                gui_label(kd, help_header, GUI_SML, GUI_COLOR_DEFAULT);
                gui_filler(kd);

                if (!help_open)
#ifndef __EMSCRIPTEN__
                    if (current_platform == PLATFORM_PC && !console_gui_shown())
#endif
                        gui_back_button(kd);
            }
        }

        if (help_open)
        {
            switch (help_page_category)
            {
                case PAGE_INTRODUCTION:
                    page_introduction(id);          break;
                case PAGE_MORPHS_AND_GENERATORS:
                    page_morphs_and_generators(id); break;
                case PAGE_MACHINES:
                    page_machines(id);              break;
                case PAGE_CONTROLS:
                    page_controls(id);              break;
                case PAGE_MODES:
                    page_modes(id);                 break;
                case PAGE_MODES_SPECIAL:
                    page_modes_special(id);         break;
                case PAGE_TRICKS:
                    page_tricks(id);                break;
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

            gui_state(id, _("Introduction"),    GUI_SML,
                          HELP_SELECT, PAGE_INTRODUCTION);
            gui_state(id, _("Powerups"),        GUI_SML,
                          HELP_SELECT, PAGE_MORPHS_AND_GENERATORS);
            gui_state(id, _("Special objects"), GUI_SML,
                          HELP_SELECT, PAGE_MACHINES);
            gui_state(id, _("Controls"),        GUI_SML,
                          HELP_SELECT, PAGE_CONTROLS);
            gui_state(id, _("Modes"),           GUI_SML,
                          HELP_SELECT, PAGE_MODES);
#if NB_HAVE_PB_BOTH==1
            gui_state(id, _("Special"),         GUI_SML,
                          HELP_SELECT, PAGE_MODES_SPECIAL);
#endif
            gui_state(id, _("Tricks"),          GUI_SML,
                          HELP_SELECT, PAGE_TRICKS);
        }

        gui_layout(id, 0, 0);
    }

    return root_id;
#else
    int id;

    if ((id = gui_vstack(0)))
    {
        help_menu(id);

        switch (page)
        {
            case PAGE_RULES:         page_rules(id);         break;
            case PAGE_CONTROLS:      page_controls(id);      break;
            case PAGE_MODES:         page_modes(id);         break;
#if NB_HAVE_PB_BOTH==1
            case PAGE_MODES_SPECIAL: page_modes_special(id); break;
#endif
            case PAGE_TRICKS:        page_tricks(id);        break;
        }

        gui_layout(id, 0, +1);
    }

    return id;
#endif
}

static int help_enter(struct state *st, struct state *prev, int intent)
{
#ifndef SWITCHBALL_HELP
    if (prev == &st_title)
        page = PAGE_RULES;
#endif

    return transition_slide(help_gui(), 1, intent);
}

static void help_paint(int id, float t)
{
    game_client_draw(0, t);

    gui_paint(id);
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    if (console_gui_shown())
        console_gui_list_paint();
#endif
}

static int help_keybd(int c, int d)
{
    if (d && c == KEY_EXIT)
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        if (current_platform == PLATFORM_PC)
#endif
            return help_action(GUI_BACK, 0);

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

#define ST_HELP_DEMO_SMOOTH_FIX_TIMER(time_total) \
    if (config_get_d(CONFIG_SMOOTH_FIX) &&        \
        video_perf() < NB_FRAMERATE_MIN) {        \
        time_total += dt;                         \
        if (time_total >= 10) {                   \
            config_set_d(CONFIG_SMOOTH_FIX,       \
                         config_get_d(CONFIG_FORCE_SMOOTH_FIX));\
            time_total = 0; } }                   \
    else time_total = 0

static float smoothfix_slowdown_time;

static int demo_freeze_all;

static int help_demo_enter(struct state *st, struct state *prev, int intent)
{
    demo_freeze_all         = 0;
    smoothfix_slowdown_time = 0;

    game_client_fly(0.0f);
    return 0;
}

static int help_demo_leave(struct state *st, struct state *next, int id, int intent)
{
    demo_replay_stop(0);

    if (next == &st_null)
        game_client_free(NULL);

    return 0;
}

static void help_demo_paint(int id, float t)
{
    game_client_draw(0, t);
}

static void help_demo_timer(int id, float dt)
{
    ST_HELP_DEMO_SMOOTH_FIX_TIMER(smoothfix_slowdown_time);

    game_step_fade(dt);

    if (!demo_freeze_all)
    {
        geom_step(dt);

        if (!demo_replay_step(dt))
        {
            demo_freeze_all = 1;
            exit_state(&st_help);
        }

        game_client_blend(demo_replay_blend());
    }
}

static int help_demo_keybd(int c, int d)
{
    if (d && c == KEY_EXIT)
    {
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        if (current_platform == PLATFORM_PC)
#endif
        {
            demo_freeze_all = 1;
            return exit_state(&st_help);
        }
    }
    return 1;
}

static int help_demo_buttn(int b, int d)
{
    if (d && config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
    {
        demo_freeze_all = 1;
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
    shared_click,
    help_demo_keybd,
    help_demo_buttn
};
