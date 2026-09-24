/*
 * Copyright (C) 2026 Microsoft / Neverball authors / Jānis Rūcis
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

#if NB_HAVE_PB_BOTH==1 && NB_PB_SDL3==1
#define SDL_ENABLE_OLD_NAMES
#include <SDL3/SDL.h>
#elif _WIN32 && __MINGW32__
#include <SDL2/SDL.h>
#elif _WIN32 && _MSC_VER
#include <SDL.h>
#elif _WIN32
#error Security compilation error: No target include file in path for Windows specified!
#else
#include <SDL.h>
#endif

/*
 * HACK: Used with console version
 */
#include "console_control_gui.h"

#include "common.h"
#include "config.h"
#include "gui.h"
#include "key.h"
#include "lang.h"
#include "video.h"
#include "log.h"

#include "game_inputbindings.h"

/*---------------------------------------------------------------------------*/

#if defined(__WII__)
/* We're using SDL 1.2 on Wii, which has SDLKey instead of SDL_Keycode. */
typedef SDLKey SDL_Keycode;
#endif

/*---------------------------------------------------------------------------*/

static int level_msg_search_and_replace(char *src,
                                        char *search, char *replace,
                                        char *result)
{
    int done = 0, i, j, k;
    int src_len     = strlen(src);
    int search_len  = strlen(search);
    int replace_len = strlen(replace);

    char *src_copy    = src;
    char *search_copy = search;

    for (i = 0, j = 0; i < src_len; )
    {
        if (src_copy[i] == search_copy[0])
        {
            int match = 1;

            /* Check if the substring matches */

            for (k = 1; k < search_len; k++)
            {
                if (src_copy[i + k] != search_copy[k])
                {
                    match = 0;
                    break;
                }
            }

            if (match)
            {
                /* Substring found, replace it */

                for (k = 0; k < replace_len; k++)
                    result[j++] = replace[k];

                i += search_len;

                done = 1;
            }
            else
            {
                /* No match, copy and continue */
                result[j++] = src_copy[i++];
            }
        }
        else
        {
            /* No match, copy and continue */
            result[j++] = src_copy[i++];
        }
    }

    result[j] = '\0';
    return done;
}

/*---------------------------------------------------------------------------*/

/*
 * Xbox, Playstation and Nintendo contains
 * fully controller support, so you don't need to worry them.
 */
static int *conf_controllers_options[] = {
    &CONFIG_JOYSTICK_BUTTON_A,
    &CONFIG_JOYSTICK_BUTTON_B,
    &CONFIG_JOYSTICK_BUTTON_X,
    &CONFIG_JOYSTICK_BUTTON_Y,
    &CONFIG_JOYSTICK_BUTTON_L1,
    &CONFIG_JOYSTICK_BUTTON_R1,
    &CONFIG_JOYSTICK_BUTTON_L2,
    &CONFIG_JOYSTICK_BUTTON_R2,
    &CONFIG_JOYSTICK_BUTTON_SELECT,
    &CONFIG_JOYSTICK_BUTTON_START,
    NULL,

    &CONFIG_JOYSTICK_AXIS_X0,
    &CONFIG_JOYSTICK_AXIS_Y0,
    NULL,
    &CONFIG_JOYSTICK_AXIS_X1,
    &CONFIG_JOYSTICK_AXIS_Y1,
    NULL,
};

/*
 * This variable name will be redirected to conf_controllers_options for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_options` to `joystick_options`.
 */
#define joystick_options conf_controllers_options

static const char *conf_controllers_option_names[] = {
    N_("Button A"),
    N_("Button B"),
    N_("Button X"),
    N_("Button Y"),
    N_("Button LB"),
    N_("Button RB"),
    N_("Button LT"),
    N_("Button RT"),
    N_("Select"),
    N_("Start"),

    "",

    N_("X Axis 1"),
    N_("Y Axis 1"),
    N_("Left Stick"),
    N_("X Axis 2"),
    N_("Y Axis 2"),
    N_("Right Stick"),
};

/*
 * This variable name will be redirected to conf_controllers_option_names for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_option_names` to `joystick_option_names`.
 */
#define joystick_option_names conf_controllers_option_names

static const char *conf_controllers_option_values_energizelab[] = {
    "a",
    "b",
    "e",
    "i",
    "LB",
    "RB",
    "LT",
    "RT",
    "",
    "",

    "",

    "X (LS)",
    "Y (LS)",
    "",
    "X (RS)",
    "Y (RS)",
    "",
};

/*
 * This variable name will be redirected to conf_controllers_option_values_energizelab for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_option_values_energizelab` to `joystick_option_values_energizelab`.
 */
#define joystick_option_values_energizelab conf_controllers_option_values_energizelab

static const char *conf_controllers_option_values_xbox[] = {
    "A",
    "B",
    "X",
    "Y",
    "LB",
    "RB",
    "LT",
    "RT",
    GUI_TRIANGLE_LEFT,
    GUI_TRIANGLE_RIGHT,

    "",

    "X (LS)",
    "Y (LS)",
    "LS",
    "X (RS)",
    "Y (RS)",
    "RS",
};

/*
 * This variable name will be redirected to conf_controllers_option_values_xbox for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_option_values_xbox` to `joystick_option_values_xbox`.
 */
#define joystick_option_values_xbox conf_controllers_option_values_xbox

static const char *conf_controllers_option_values_ps[] = {
    "X",
    "△",
    "◻",
    "○",
    "L1",
    "R1",
    "L2",
    "R2",
    GUI_TRIANGLE_LEFT,
    GUI_TRIANGLE_RIGHT,

    "",

    "X (L3)",
    "Y (L3)",
    "L3",
    "X (R3)",
    "Y (R3)",
    "R3",
};

/*
 * This variable name will be redirected to conf_controllers_option_values_ps for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_option_values_ps` to `joystick_option_values_ps`.
 */
#define joystick_option_values_ps conf_controllers_option_values_ps

static const char *conf_controllers_option_values_steamdeck[] = {
    "A",
    "B",
    "X",
    "Y",
    "L1",
    "R1",
    "L2",
    "R2",
    "-",
    "+",

    "",

    "X (L3)",
    "Y (L3)",
    "L3",
    "X (R3)",
    "Y (R3)",
    "R3",
};

/*
 * This variable name will be redirected to conf_controllers_option_values_steamdeck for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_option_values_steamdeck` to `joystick_option_values_steamdeck`.
 */
#define joystick_option_values_steamdeck conf_controllers_option_values_steamdeck

static const char *conf_controllers_option_values_switch[] = {
    "A",
    "B",
    "X",
    "Y",
    "L",
    "R",
    "ZL",
    "ZR",
    "-",
    "+",

    "",

    "X (LS)",
    "Y (LS)",
    "LS",
    "X (RS)",
    "Y (RS)",
    "RS",
};

/*
 * This variable name will be redirected to conf_controllers_option_values_switch for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_option_values_switch` to `joystick_option_values_switch`.
 */
#define joystick_option_values_switch conf_controllers_option_values_switch

static const char *conf_controllers_option_values_handset[] = {
    "A",
    "B",
    "X",
    "Y",
    "L",
    "R",
    "",
    "",
    GUI_TRIANGLE_LEFT,
    GUI_TRIANGLE_RIGHT,

    "",

    "X (LS)",
    "Y (LS)",
    "LS",
    "",
    "",
    "",
};

/*
 * This variable name will be redirected to conf_controllers_option_values_handset for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_option_values_handset` to `joystick_option_values_handset`.
 */
#define joystick_option_values_handset conf_controllers_option_values_handset

static const char *conf_controllers_option_values_wii[] = {
    "A",
    "B",
    "C",
    "Z",
    "",
    "",
    "",
    "",
    "-",
    "+",

    "",

    "X",
    "Y",
    "",
    "",
    "",
    "",
};

/*
 * This variable name will be redirected to conf_controllers_option_values_wii for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `conf_controllers_option_values_wii` to `joystick_option_values_wii`.
 */
#define joystick_option_values_wii conf_controllers_option_values_wii

/*---------------------------------------------------------------------------*/

/**
 * @brief Mojang's Dynamic Level Description for Level Map
 *
 * @param msg The current level description
 *
 * @returns The dynamic level description, which came from System Settings
 */
const char *game_level_msg_inputbindings(const char *msg)
{
    const char *ks_unassigned = _("Unassigned");

    int input_key_exit_enbind_done = 0,
        input_key_cam1_enbind_done = 0,
        input_key_cam2_enbind_done = 0,
        input_key_cam3_enbind_done = 0,
        input_key_caml_enbind_done = 0,
        input_key_camr_enbind_done = 0;

    int input_mouse_cam_enbind_done             = 0,
        input_mouse_presstostart_enbind_done    = 0,
        input_mouse_presstocontinue_enbind_done = 0;

    int  prepare_replace_count, prepare_replace_done = 0,
         final_replace_count, final_replace_done = 0;
    char relay_lvl_message[2][MAXSTR],
         final_lvl_message[2][MAXSTR];

    int i;

    for (i = 0; i < 2; i++)
    {
        memset(relay_lvl_message[i], 0, MAXSTR);
        memset(final_lvl_message[i], 0, MAXSTR);
    }

    /* PHASE 1: STRING FORMATS */

    for (prepare_replace_count = 0; !prepare_replace_done && prepare_replace_count < 2048; )
    {
        input_key_exit_enbind_done = 0;
        input_key_cam1_enbind_done = 0;
        input_key_cam2_enbind_done = 0;
        input_key_cam3_enbind_done = 0;
        input_key_caml_enbind_done = 0;
        input_key_camr_enbind_done = 0;

        input_mouse_cam_enbind_done             = 0;
        input_mouse_presstostart_enbind_done    = 0;
        input_mouse_presstocontinue_enbind_done = 0;

        if (prepare_replace_count != 0)
            memcpy(relay_lvl_message[0], relay_lvl_message[1], strlen(relay_lvl_message[1]));

        /* vvv KEYBOARD INPUT vvv */

        if (strstr(prepare_replace_count == 0 ? msg : relay_lvl_message[0], "Press Escape") && !input_key_exit_enbind_done)
        {
            level_msg_search_and_replace(prepare_replace_count == 0 ? msg : relay_lvl_message[0], "Press Escape", "Press %KEYBD_KEY_ESCAPE%", relay_lvl_message[1]);
            input_key_exit_enbind_done = 1;
            prepare_replace_count++;
        }
        else if (strstr(prepare_replace_count == 0 ? msg : relay_lvl_message[0], "Press 1") && !input_key_cam1_enbind_done)
        {
            level_msg_search_and_replace(prepare_replace_count == 0 ? msg : relay_lvl_message[0], "Press 1", "Press %KEYBD_KEY_CAM_1%", relay_lvl_message[1]);
            input_key_cam1_enbind_done = 1;
            prepare_replace_count++;
        }
        else if (strstr(prepare_replace_count == 0 ? msg : relay_lvl_message[0], "Press 2") && !input_key_cam2_enbind_done)
        {
            level_msg_search_and_replace(prepare_replace_count == 0 ? msg : relay_lvl_message[0], "Press 2", "Press %KEYBD_KEY_CAM_2%", relay_lvl_message[1]);
            input_key_cam2_enbind_done = 1;
            prepare_replace_count++;
        }
        else if (strstr(prepare_replace_count == 0 ? msg : relay_lvl_message[0], "Press 3") && !input_key_cam3_enbind_done)
        {
            level_msg_search_and_replace(prepare_replace_count == 0 ? msg : relay_lvl_message[0], "Press 3", "Press %KEYBD_KEY_CAM_3%", relay_lvl_message[1]);
            input_key_cam3_enbind_done = 1;
            prepare_replace_count++;
        }

        /* ^^^ KEYBOARD INPUT ^^^ */

        /* vvv MOUSE INPUT vvv */

        else if (strstr(prepare_replace_count == 0 ? msg : relay_lvl_message[0], "Move the mouse") && !input_mouse_cam_enbind_done)
        {
            level_msg_search_and_replace(prepare_replace_count == 0 ? msg : relay_lvl_message[0], "Move the mouse", "%MOUSE_MOVEMENT%", relay_lvl_message[1]);
            input_mouse_cam_enbind_done = 1;
            prepare_replace_count++;
        }
        else if (strstr(prepare_replace_count == 0 ? msg : relay_lvl_message[0], "move the mouse") && !input_mouse_cam_enbind_done)
        {
            level_msg_search_and_replace(prepare_replace_count == 0 ? msg : relay_lvl_message[0], "move the mouse", "%MOUSE_MOVEMENT%", relay_lvl_message[1]);
            input_mouse_cam_enbind_done = 1;
            prepare_replace_count++;
        }
        else if (strstr(prepare_replace_count == 0 ? msg : relay_lvl_message[0], "mouse buttons") && !input_mouse_cam_enbind_done)
        {
            level_msg_search_and_replace(prepare_replace_count == 0 ? msg : relay_lvl_message[0], "mouse buttons", "%MOUSE_BUTTON_LR%", relay_lvl_message[1]);
            input_mouse_cam_enbind_done = 1;
            prepare_replace_count++;
        }
        else if (strstr(prepare_replace_count == 0 ? msg : relay_lvl_message[0], "Mouse buttons") && !input_mouse_cam_enbind_done)
        {
            level_msg_search_and_replace(prepare_replace_count == 0 ? msg : relay_lvl_message[0], "Mouse buttons", "%MOUSE_BUTTON_LR%", relay_lvl_message[1]);
            input_mouse_cam_enbind_done = 1;
            prepare_replace_count++;
        }
        else if (strstr(prepare_replace_count == 0 ? msg : relay_lvl_message[0], "Click to begin") && !input_mouse_presstostart_enbind_done)
        {
            level_msg_search_and_replace(prepare_replace_count == 0 ? msg : relay_lvl_message[0], "Click to begin", "%MOUSE_BUTTON_PRESSTOSTART%", relay_lvl_message[1]);
            input_mouse_presstostart_enbind_done = 1;
            prepare_replace_count++;
        }
        else if (strstr(prepare_replace_count == 0 ? msg : relay_lvl_message[0], "Click to continue") && !input_mouse_presstocontinue_enbind_done)
        {
            level_msg_search_and_replace(prepare_replace_count == 0 ? msg : relay_lvl_message[0], "Click to continue", "%MOUSE_BUTTON_PRESSTOCONTINUE%", relay_lvl_message[1]);
            input_mouse_presstocontinue_enbind_done = 1;
            prepare_replace_count++;
        }

        /* ^^^ MOUSE INPUT ^^^ */

        else prepare_replace_done = 1;

        memcpy(relay_lvl_message[0], relay_lvl_message[1], strlen(relay_lvl_message[1]));
    }

    /* PHASE 2: STRING FORMATTED TO PHYSICAL KEY INPUT */

    char *ks_keybd_exit  = strdup(SDL_GetKeyName(KEY_EXIT));
    char *ks_keybd_cam1  = strdup(SDL_GetKeyName(config_get_d(CONFIG_KEY_CAMERA_1)));
    char *ks_keybd_cam2  = strdup(SDL_GetKeyName(config_get_d(CONFIG_KEY_CAMERA_2)));
    char *ks_keybd_cam3  = strdup(SDL_GetKeyName(config_get_d(CONFIG_KEY_CAMERA_3)));
    char *ks_keybd_rot_l = strdup(SDL_GetKeyName(config_get_d(CONFIG_KEY_CAMERA_R)));
    char *ks_keybd_rot_r = strdup(SDL_GetKeyName(config_get_d(CONFIG_KEY_CAMERA_L)));

    while (final_replace_done != 0) final_replace_done = 0;

    for (i = 0, final_replace_count = 0; !final_replace_done && final_replace_count < 2048 && i < 2048; i++)
    {
        input_key_exit_enbind_done = 0;
        input_key_cam1_enbind_done = 0;
        input_key_cam2_enbind_done = 0;
        input_key_cam3_enbind_done = 0;
        input_key_caml_enbind_done = 0;
        input_key_camr_enbind_done = 0;

        input_mouse_cam_enbind_done             = 0;
        input_mouse_presstostart_enbind_done    = 0;
        input_mouse_presstocontinue_enbind_done = 0;

        if (final_replace_count != 0)
            memcpy(final_lvl_message[0], final_lvl_message[1], strlen(final_lvl_message[1]));
        else
            memcpy(final_lvl_message[0], relay_lvl_message[0], strlen(relay_lvl_message[0]));

        if (opt_touch && !console_gui_shown())
        {
            /* vvv KEYBOARD INPUT vvv */

            if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_ESCAPE%") && !input_key_exit_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_ESCAPE%", _("Pause button"), final_lvl_message[1]);
                input_key_exit_enbind_done = 1;
                final_replace_count++;
            }

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_1%") && !input_mouse_cam_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_1%", _("Camera button"), final_lvl_message[1]);
                input_mouse_cam_enbind_done = 1;
                final_replace_count++;
            }

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_2%") && !input_mouse_cam_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_2%", _("Camera button"), final_lvl_message[1]);
                input_mouse_cam_enbind_done = 1;
                final_replace_count++;
            }

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_3%") && !input_mouse_cam_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_3%", _("Camera button"), final_lvl_message[1]);
                input_mouse_cam_enbind_done = 1;
                final_replace_count++;
            }

            /* ^^^ KEYBOARD INPUT ^^^ */

            /* vvv MOUSE INPUT vvv */

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_MOVEMENT%") && !input_mouse_cam_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_MOVEMENT%", _("Drag"), final_lvl_message[1]);
                input_mouse_cam_enbind_done = 1;
                final_replace_count++;
            }

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_LR%") && !input_mouse_cam_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_LR%", _("Rotate button"), final_lvl_message[1]);
                input_mouse_cam_enbind_done = 1;
                final_replace_count++;
            }

            else if (strstr(prepare_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_PRESSTOSTART%") && !input_mouse_presstostart_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_PRESSTOSTART%", _("Tap \"Start\" to begin"), final_lvl_message[1]);
                input_mouse_presstostart_enbind_done = 1;
                prepare_replace_count++;
            }

            else if (strstr(prepare_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_PRESSTOCONTINUE%") && !input_mouse_presstocontinue_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_PRESSTOCONTINUE%", _("Tap \"Start\" to continue"), final_lvl_message[1]);
                input_mouse_presstocontinue_enbind_done = 1;
                prepare_replace_count++;
            }

            /* ^^^ MOUSE INPUT ^^^ */

            else final_replace_done = 1;
        }
        else if (current_platform == PLATFORM_PC && !console_gui_shown())
        {
            /* vvv KEYBOARD INPUT vvv */

            if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_ESCAPE%") && !input_key_exit_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_ESCAPE%", ks_keybd_exit, final_lvl_message[1]);
                input_key_exit_enbind_done = 1;
                final_replace_count++;
            }

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_1%") && !input_mouse_cam_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_1%", ks_keybd_cam1, final_lvl_message[1]);
                input_mouse_cam_enbind_done = 1;
                final_replace_count++;
            }

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_2%") && !input_mouse_cam_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_2%", ks_keybd_cam2, final_lvl_message[1]);
                input_mouse_cam_enbind_done = 1;
                final_replace_count++;
            }

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_3%") && !input_mouse_cam_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_3%", ks_keybd_cam3, final_lvl_message[1]);
                input_mouse_cam_enbind_done = 1;
                final_replace_count++;
            }

            /* ^^^ KEYBOARD INPUT ^^^ */

            /* vvv MOUSE INPUT vvv */

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_MOVEMENT%") && !input_mouse_cam_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_MOVEMENT%", _("Use your mouse"), final_lvl_message[1]);
                input_mouse_cam_enbind_done = 1;
                final_replace_count++;
            }

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_LR%") && !input_mouse_cam_enbind_done)
            {
                char ks_keybd_rotate[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(ks_keybd_rotate, MAXSTR,
#else
                sprintf(ks_keybd_rotate,
#endif
                        "%s / %s",
                        config_get_d(CONFIG_CAMERA_ROTATE_MODE) == 1 ? ks_keybd_rot_r :
                        ks_keybd_rot_l,
                        config_get_d(CONFIG_CAMERA_ROTATE_MODE) == 1 ? ks_keybd_rot_l :
                        ks_keybd_rot_r);

                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_LR%", ks_keybd_rotate, final_lvl_message[1]);
                input_mouse_cam_enbind_done = 1;
                final_replace_count++;
            }

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_PRESSTOSTART%") && !input_mouse_presstostart_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_PRESSTOSTART%", _("Click to begin"), final_lvl_message[1]);
                input_mouse_presstostart_enbind_done = 1;
                final_replace_count++;
            }

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_PRESSTOCONTINUE%") && !input_mouse_presstocontinue_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_PRESSTOCONTINUE%", _("Click to continue"), final_lvl_message[1]);
                input_mouse_presstocontinue_enbind_done = 1;
                final_replace_count++;
            }

            /* ^^^ MOUSE INPUT ^^^ */

            else final_replace_done = 1;
        }
#if defined(__WII__)
        else if (current_platform == PLATFORM_WII)
        {
            /* vvv KEYBOARD INPUT vvv */

            if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_ESCAPE%") && !input_key_exit_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_ESCAPE%", conf_controllers_option_values_wii[config_get_d(CONFIG_JOYSTICK_BUTTON_START)], final_lvl_message[1]);
                input_key_exit_enbind_done = 1;
                final_replace_count++;
            }

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_1%") && !input_mouse_cam_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_1%", conf_controllers_option_values_wii[config_get_d(CONFIG_JOYSTICK_BUTTON_X)], final_lvl_message[1]);
                input_mouse_cam_enbind_done = 1;
                final_replace_count++;
            }

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_2%") && !input_mouse_cam_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_2%", conf_controllers_option_values_wii[config_get_d(CONFIG_JOYSTICK_BUTTON_X)], final_lvl_message[1]);
                input_mouse_cam_enbind_done = 1;
                final_replace_count++;
            }

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_3%") && !input_mouse_cam_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_3%", conf_controllers_option_values_wii[config_get_d(CONFIG_JOYSTICK_BUTTON_X)], final_lvl_message[1]);
                input_mouse_cam_enbind_done = 1;
                final_replace_count++;
            }

            /* ^^^ KEYBOARD INPUT ^^^ */

            /* vvv MOUSE INPUT vvv */

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_MOVEMENT%") && !input_mouse_cam_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_MOVEMENT%", _("Use your Nunchuck Stick"), final_lvl_message[1]);
                input_mouse_cam_enbind_done = 1;
                final_replace_count++;
            }

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_LR%") && !input_mouse_cam_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_LR%", _("D-Pad"), final_lvl_message[1]);
                input_mouse_cam_enbind_done = 1;
                final_replace_count++;
            }

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_PRESSTOSTART%") && !input_mouse_presstostart_enbind_done)
            {
                char ks_wii_presstostart[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(ks_wii_presstostart, MAXSTR,
#else
                sprintf(ks_wii_presstostart,
#endif
                        _("Press %s to begin"), conf_controllers_option_values_wii[config_get_d(CONFIG_JOYSTICK_BUTTON_A)]);
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_PRESSTOSTART%", ks_wii_presstostart, final_lvl_message[1]);
                input_mouse_presstostart_enbind_done = 1;
                final_replace_count++;
            }

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_PRESSTOCONTINUE%") && !input_mouse_presstocontinue_enbind_done)
            {
                char ks_wii_presstocontinue[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(ks_wii_presstocontinue, MAXSTR,
#else
                sprintf(ks_wii_presstocontinue,
#endif
                        _("Press %s to continue"), conf_controllers_option_values_wii[config_get_d(CONFIG_JOYSTICK_BUTTON_A)]);
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_PRESSTOCONTINUE%", ks_wii_presstocontinue, final_lvl_message[1]);
                input_mouse_presstocontinue_enbind_done = 1;
                final_replace_count++;
            }

            /* ^^^ MOUSE INPUT ^^^ */

            else final_replace_done = 1;
        }
#endif
        else if (current_platform == PLATFORM_PS)
        {
            /* vvv KEYBOARD INPUT vvv */

            if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_ESCAPE%") && !input_key_exit_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_ESCAPE%", conf_controllers_option_values_ps[config_get_d(CONFIG_JOYSTICK_BUTTON_START)], final_lvl_message[1]);
                input_key_exit_enbind_done = 1;
                final_replace_count++;
            }

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_1%") && !input_mouse_cam_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_1%", conf_controllers_option_values_ps[config_get_d(CONFIG_JOYSTICK_BUTTON_X)], final_lvl_message[1]);
                input_mouse_cam_enbind_done = 1;
                final_replace_count++;
            }

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_2%") && !input_mouse_cam_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_2%", conf_controllers_option_values_ps[config_get_d(CONFIG_JOYSTICK_BUTTON_X)], final_lvl_message[1]);
                input_mouse_cam_enbind_done = 1;
                final_replace_count++;
            }

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_3%") && !input_mouse_cam_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_3%", conf_controllers_option_values_ps[config_get_d(CONFIG_JOYSTICK_BUTTON_X)], final_lvl_message[1]);
                input_mouse_cam_enbind_done = 1;
                final_replace_count++;
            }

            /* ^^^ KEYBOARD INPUT ^^^ */

            /* vvv MOUSE INPUT vvv */

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_MOVEMENT%") && !input_mouse_cam_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_MOVEMENT%", _("Use your Left Stick"), final_lvl_message[1]);
                input_mouse_cam_enbind_done = 1;
                final_replace_count++;
            }

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_LR%") && !input_mouse_cam_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_LR%", _("Right Stick"), final_lvl_message[1]);
                input_mouse_cam_enbind_done = 1;
                final_replace_count++;
            }
            
            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_PRESSTOSTART%") && !input_mouse_presstostart_enbind_done)
            {
                char ks_ps_presstostart[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(ks_ps_presstostart, MAXSTR,
#else
                sprintf(ks_ps_presstostart,
#endif
                        _("Press %s to begin"), conf_controllers_option_values_ps[config_get_d(CONFIG_JOYSTICK_BUTTON_A)]);
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_PRESSTOSTART%", ks_ps_presstostart, final_lvl_message[1]);
                input_mouse_presstostart_enbind_done = 1;
                final_replace_count++;
            }

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_PRESSTOCONTINUE%") && !input_mouse_presstocontinue_enbind_done)
            {
                char ks_ps_presstocontinue[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(ks_ps_presstocontinue, MAXSTR,
#else
                sprintf(ks_ps_presstocontinue,
#endif
                        _("Press %s to continue"), conf_controllers_option_values_ps[config_get_d(CONFIG_JOYSTICK_BUTTON_A)]);
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_PRESSTOCONTINUE%", ks_ps_presstocontinue, final_lvl_message[1]);
                input_mouse_presstocontinue_enbind_done = 1;
                final_replace_count++;
            }

            /* ^^^ MOUSE INPUT ^^^ */

            else final_replace_done = 1;
        }
        else
        {
            /* vvv KEYBOARD INPUT vvv */

            if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_ESCAPE%") && !input_mouse_cam_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_ESCAPE%", current_platform == PLATFORM_SWITCH ? conf_controllers_option_values_switch[config_get_d(CONFIG_JOYSTICK_BUTTON_START)] : conf_controllers_option_values_xbox[config_get_d(CONFIG_JOYSTICK_BUTTON_START)], final_lvl_message[1]);
                input_mouse_cam_enbind_done = 1;
                final_replace_count++;
            }

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_1%") && !input_mouse_cam_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_1%", current_platform == PLATFORM_SWITCH ? conf_controllers_option_values_switch[config_get_d(CONFIG_JOYSTICK_BUTTON_X)] : conf_controllers_option_values_xbox[config_get_d(CONFIG_JOYSTICK_BUTTON_X)], final_lvl_message[1]);
                input_mouse_cam_enbind_done = 1;
                final_replace_count++;
            }

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_2%") && !input_mouse_cam_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_2%", current_platform == PLATFORM_SWITCH ? conf_controllers_option_values_switch[config_get_d(CONFIG_JOYSTICK_BUTTON_X)] : conf_controllers_option_values_xbox[config_get_d(CONFIG_JOYSTICK_BUTTON_X)], final_lvl_message[1]);
                input_mouse_cam_enbind_done = 1;
                final_replace_count++;
            }

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_3%") && !input_mouse_cam_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%KEYBD_KEY_CAM_3%", current_platform == PLATFORM_SWITCH ? conf_controllers_option_values_switch[config_get_d(CONFIG_JOYSTICK_BUTTON_X)] : conf_controllers_option_values_xbox[config_get_d(CONFIG_JOYSTICK_BUTTON_X)], final_lvl_message[1]);
                input_mouse_cam_enbind_done = 1;
                final_replace_count++;
            }

            /* ^^^ KEYBOARD INPUT ^^^ */

            /* vvv MOUSE INPUT vvv */

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_MOVEMENT%") && !input_mouse_cam_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_MOVEMENT%", _("Use your Left Stick"), final_lvl_message[1]);
                input_mouse_cam_enbind_done = 1;
                final_replace_count++;
            }

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_LR%") && !input_mouse_cam_enbind_done)
            {
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_LR%", _("Right Stick"), final_lvl_message[1]);
                input_mouse_cam_enbind_done = 1;
                final_replace_count++;
            }
            
            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_PRESSTOSTART%") && !input_mouse_presstostart_enbind_done)
            {
                char ks_ps_presstostart[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(ks_ps_presstostart, MAXSTR,
#else
                sprintf(ks_ps_presstostart,
#endif
                        _("Press %s to begin"), current_platform == PLATFORM_SWITCH ? conf_controllers_option_values_switch[config_get_d(CONFIG_JOYSTICK_BUTTON_A)] : conf_controllers_option_values_xbox[config_get_d(CONFIG_JOYSTICK_BUTTON_A)]);
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_PRESSTOSTART%", ks_ps_presstostart, final_lvl_message[1]);
                input_mouse_presstostart_enbind_done = 1;
                final_replace_count++;
            }

            else if (strstr(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_PRESSTOCONTINUE%") && !input_mouse_presstocontinue_enbind_done)
            {
                char ks_ps_presstocontinue[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(ks_ps_presstocontinue, MAXSTR,
#else
                sprintf(ks_ps_presstocontinue,
#endif
                        _("Press %s to continue"), current_platform == PLATFORM_SWITCH ? conf_controllers_option_values_switch[config_get_d(CONFIG_JOYSTICK_BUTTON_A)] : conf_controllers_option_values_xbox[config_get_d(CONFIG_JOYSTICK_BUTTON_A)]);
                level_msg_search_and_replace(final_replace_count == 0 ? relay_lvl_message[0] : final_lvl_message[0], "%MOUSE_BUTTON_PRESSTOCONTINUE%", ks_ps_presstocontinue, final_lvl_message[1]);
                input_mouse_presstocontinue_enbind_done = 1;
                final_replace_count++;
            }

            /* ^^^ MOUSE INPUT ^^^ */

            else final_replace_done = 1;

            memcpy(final_lvl_message[0], final_lvl_message[1], strlen(final_lvl_message[1]));
        }
    }

    if (i >= 2048)
        log_errorf("String format iteration limit reached!\n");

    /* PHASE 3: FINALIZE */

    int count_newline = 0, count_newline_final = 0, index_nullterm_location = -1;

    for (int i = 0; msg[i] != '\0'; i++)
    {
        if (msg[i] == '\\' ||
            msg[i] == '\n')
            count_newline++;
    }

    if (prepare_replace_done && final_replace_done)
    {
        for (i = 0; final_lvl_message[0][i] != '\0' && count_newline != count_newline_final && i < MAXSTR; i++)
        {
            if (final_lvl_message[0][i] == '\\' ||
                final_lvl_message[0][i] == '\n')
            {
                count_newline_final++;
                index_nullterm_location = i + 1;
            }
        }

        if (index_nullterm_location != -1)
            final_lvl_message[0][index_nullterm_location] = '\0';
    }
    else if (prepare_replace_done)
    {
        for (i = 0; relay_lvl_message[0][i] != '\0' && count_newline != count_newline_final && i < MAXSTR; i++)
        {
            if (relay_lvl_message[0][i] == '\\' ||
                relay_lvl_message[0][i] == '\n')
            {
                count_newline_final++;
                index_nullterm_location = i + 1;
            }
        }

        if (index_nullterm_location != -1)
            relay_lvl_message[0][index_nullterm_location] = '\0';
    }

    free(ks_keybd_exit);
    free(ks_keybd_cam1);
    free(ks_keybd_cam2);
    free(ks_keybd_cam3);
    free(ks_keybd_rot_l);
    free(ks_keybd_rot_r);

    return prepare_replace_done && prepare_replace_count > 0 && final_replace_done && final_replace_count > 0 ? final_lvl_message[0] :
           prepare_replace_done && prepare_replace_count > 0 ? relay_lvl_message[0] : msg;
}
