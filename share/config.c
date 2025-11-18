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

#if _WIN32 && __MINGW32__
#include <SDL2/SDL.h>
#elif _WIN32 && _MSC_VER
#include <SDL.h>
#elif _WIN32
#error Security compilation error: No target include file in path for Windows specified!
#else
#include <SDL.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#ifndef NDEBUG
#include <assert.h>
#elif defined(_MSC_VER) && defined(_AFXDLL)
#include <afx.h>
/**
 * HACK: assert() for Microsoft Windows Apps in Release builds
 * will be replaced to VERIFY() - Ersohn Styne
 */
#define assert VERIFY
#else
#define assert(_x) (_x)
#endif

#if NB_HAVE_PB_BOTH==1
#include "account.h"
#include "networking.h"
#ifdef __EMSCRIPTEN__
#include "config_wgcl.h"
#endif
#endif
#include "glext.h"
#include "accessibility.h"
#include "base_config.h"
#include "config.h"
#include "common.h"
#include "fs.h"

#include "log.h"

/*
 * TODO: Uncomment, if you want to lock the player name
 * by the system OS settings.
 */
//#define CONFIG_PLAYER_LOCKEDBY_SYSTEMOS

#if defined(__WII__)
/* We're using SDL 1.2 on Wii, which has SDLKey instead of SDL_Keycode. */
typedef SDLKey SDL_Keycode;
#endif

/*---------------------------------------------------------------------------*/

int config_is_init = 0;
int config_busy = 0;

/* Integer options. */

int CONFIG_ACCOUNT_TUTORIAL;
int CONFIG_ACCOUNT_HINT;
int CONFIG_ACCOUNT_BEAM_STYLE;
int CONFIG_ACCOUNT_SAVE;
int CONFIG_ACCOUNT_LOAD;

int CONFIG_NOTIFICATION_CHKP;
int CONFIG_NOTIFICATION_REWARD;
int CONFIG_NOTIFICATION_SHOP;

int CONFIG_GRAPHIC_RESTORE_ID;
int CONFIG_GRAPHIC_RESTORE_VAL1;
int CONFIG_GRAPHIC_RESTORE_VAL2;

int CONFIG_MAINMENU_PANONLY;

int CONFIG_SCREEN_ANIMATIONS;
//int CONFIG_UI_HWACCEL;
int CONFIG_SMOOTH_FIX;
int CONFIG_FORCE_SMOOTH_FIX;
int CONFIG_WIDESCREEN;
int CONFIG_FULLSCREEN;
int CONFIG_MAXIMIZED;
int CONFIG_DISPLAY;
int CONFIG_WIDTH;
int CONFIG_HEIGHT;
int CONFIG_STEREO;
int CONFIG_CAMERA;
int CONFIG_CAMERA_ROTATE_MODE;
int CONFIG_TEXTURES;
int CONFIG_MOTIONBLUR;
int CONFIG_REFLECTION;
int CONFIG_MULTISAMPLE;
#ifdef GL_GENERATE_MIPMAP_SGIS
int CONFIG_MIPMAP;
#endif
#ifdef GL_TEXTURE_MAX_ANISOTROPY_EXT
int CONFIG_ANISO;
#endif
int CONFIG_BACKGROUND;
int CONFIG_SHADOW;
int CONFIG_AUDIO_BUFF;
int CONFIG_TILTING_FLOOR;
int CONFIG_MOUSE_SENSE;
int CONFIG_MOUSE_RESPONSE;
int CONFIG_MOUSE_INVERT;
int CONFIG_VSYNC;
int CONFIG_HMD;
int CONFIG_HIGHDPI;
int CONFIG_MOUSE_CAMERA_1;
int CONFIG_MOUSE_CAMERA_2;
int CONFIG_MOUSE_CAMERA_3;
int CONFIG_MOUSE_CAMERA_TOGGLE;
int CONFIG_MOUSE_CAMERA_L;
int CONFIG_MOUSE_CAMERA_R;
int CONFIG_MOUSE_CANCEL_MENU;
int CONFIG_NICE;
int CONFIG_FPS;
int CONFIG_MASTER_VOLUME;
int CONFIG_SOUND_VOLUME;
int CONFIG_MUSIC_VOLUME;
int CONFIG_NARRATOR_VOLUME;
int CONFIG_JOYSTICK;
int CONFIG_JOYSTICK_RESPONSE;
int CONFIG_JOYSTICK_AXIS_X0;
int CONFIG_JOYSTICK_AXIS_Y0;
int CONFIG_JOYSTICK_AXIS_X1;
int CONFIG_JOYSTICK_AXIS_Y1;
int CONFIG_JOYSTICK_AXIS_X0_INVERT;
int CONFIG_JOYSTICK_AXIS_Y0_INVERT;
int CONFIG_JOYSTICK_AXIS_X1_INVERT;
int CONFIG_JOYSTICK_AXIS_Y1_INVERT;

int CONFIG_JOYSTICK_BUTTON_A;
int CONFIG_JOYSTICK_BUTTON_B;
int CONFIG_JOYSTICK_BUTTON_X;
int CONFIG_JOYSTICK_BUTTON_Y;
int CONFIG_JOYSTICK_BUTTON_L1;
int CONFIG_JOYSTICK_BUTTON_R1;
int CONFIG_JOYSTICK_BUTTON_L2;
int CONFIG_JOYSTICK_BUTTON_R2;
int CONFIG_JOYSTICK_BUTTON_SELECT;
int CONFIG_JOYSTICK_BUTTON_START;
int CONFIG_JOYSTICK_DPAD_L;
int CONFIG_JOYSTICK_DPAD_R;
int CONFIG_JOYSTICK_DPAD_U;
int CONFIG_JOYSTICK_DPAD_D;

int CONFIG_WIIMOTE_INVERT_PITCH;
int CONFIG_WIIMOTE_INVERT_ROLL;
int CONFIG_WIIMOTE_PITCH_SENSITIVITY;
int CONFIG_WIIMOTE_ROLL_SENSITIVITY;
int CONFIG_WIIMOTE_SMOOTH_ALPHA;
int CONFIG_WIIMOTE_HOLD_SIDEWAYS;

int CONFIG_KEY_CAMERA_AUTO;
int CONFIG_KEY_CAMERA_1;
int CONFIG_KEY_CAMERA_2;
int CONFIG_KEY_CAMERA_3;
int CONFIG_KEY_CAMERA_TOGGLE;
int CONFIG_KEY_CAMERA_R;
int CONFIG_KEY_CAMERA_L;
int CONFIG_KEY_FORWARD;
int CONFIG_KEY_BACKWARD;
int CONFIG_KEY_LEFT;
int CONFIG_KEY_RIGHT;
int CONFIG_KEY_RESTART;
int CONFIG_KEY_SCORE_NEXT;
int CONFIG_KEY_ROTATE_FAST;
int CONFIG_VIEW_FOV;
int CONFIG_VIEW_DP;
int CONFIG_VIEW_DC;
int CONFIG_VIEW_DZ;
int CONFIG_ROTATE_FAST;
int CONFIG_ROTATE_SLOW;
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
int CONFIG_CHEAT;
#if !defined(LOG_NO_STATS)
int CONFIG_STATS;
#endif
#endif
int CONFIG_SCREENSHOT;
int CONFIG_LOCK_GOALS;
int CONFIG_UNITS_METRIC;
int CONFIG_CAMERA_1_SPEED;
int CONFIG_CAMERA_2_SPEED;
int CONFIG_CAMERA_3_SPEED;

int CONFIG_TOUCH_ROTATE;

int CONFIG_ONLINE;

/* String options. */

int CONFIG_PLAYER;
#if !defined(CONFIG_INCLUDES_ACCOUNT)
int CONFIG_BALL_FILE;
#endif
int CONFIG_WIIMOTE_ADDR;
int CONFIG_REPLAY_NAME;
int CONFIG_LANGUAGE;
int CONFIG_THEME;
int CONFIG_DEDICATED_IPADDRESS;
int CONFIG_DEDICATED_IPPORT;

/*---------------------------------------------------------------------------*/

float axis_offset[4];

/*---------------------------------------------------------------------------*/

static struct
{
    int        *sym;
    const char *name;
    const int   def;
    int         cur;
} option_d[] = {
    { &CONFIG_ACCOUNT_TUTORIAL,        "tutorial",               1 },
    { &CONFIG_ACCOUNT_HINT,            "hint",                   1 },
    { &CONFIG_ACCOUNT_BEAM_STYLE,      "beam_style",             0 },
    { &CONFIG_ACCOUNT_SAVE,            "save_replay",            0 },
    { &CONFIG_ACCOUNT_LOAD,            "load_filter",            2 },

    { &CONFIG_NOTIFICATION_CHKP,   "notification_chkp",   1 },
    { &CONFIG_NOTIFICATION_REWARD, "notification_reward", 1 },
    { &CONFIG_NOTIFICATION_SHOP,   "notification_shop",   1 },

    { &CONFIG_GRAPHIC_RESTORE_ID,   "graphic_restore_id",   -1 },
    { &CONFIG_GRAPHIC_RESTORE_VAL1, "graphic_restore_val1",  0 },
    { &CONFIG_GRAPHIC_RESTORE_VAL2, "graphic_restore_val2",  0 },

    { &CONFIG_MAINMENU_PANONLY,"mainmenu_panonly",0 },

    { &CONFIG_SCREEN_ANIMATIONS,"screen_animation",1 },
//    { &CONFIG_UI_HWACCEL,     "ui_hwaccel",   1 },
    { &CONFIG_SMOOTH_FIX,     "smooth_fix",   1 },
    { &CONFIG_FORCE_SMOOTH_FIX, "force_smooth_fix", 0 },
    { &CONFIG_WIDESCREEN,     "widescreen",   0 },
    { &CONFIG_FULLSCREEN,     "fullscreen",   0 },
    { &CONFIG_MAXIMIZED,      "maximized",    0 },
    { &CONFIG_DISPLAY,        "display",      0 },
#ifdef __EMSCRIPTEN__
    { &CONFIG_WIDTH,          "width",        1280 },
    { &CONFIG_HEIGHT,         "height",       720 },
#else
    { &CONFIG_WIDTH,          "width",        1200 },
    { &CONFIG_HEIGHT,         "height",       900 },
#endif
    { &CONFIG_STEREO,         "stereo",       0 },
    { &CONFIG_CAMERA,         "camera",       0 },
    { &CONFIG_CAMERA_ROTATE_MODE, "camera_rotate_mode",       1 },
    { &CONFIG_TEXTURES,       "textures",     1 },
#ifdef __EMSCRIPTEN__
    { &CONFIG_MOTIONBLUR,     "motion_blur",  0 },
    { &CONFIG_REFLECTION,     "reflection",   0 },
#else
    { &CONFIG_MOTIONBLUR,     "motion_blur",  1 },
    { &CONFIG_REFLECTION,     "reflection",   1 },
#endif
    { &CONFIG_MULTISAMPLE,    "multisample",  0 },
#ifdef GL_GENERATE_MIPMAP_SGIS
    { &CONFIG_MIPMAP,         "mipmap",       1 },
#endif
#ifdef GL_TEXTURE_MAX_ANISOTROPY_EXT
    { &CONFIG_ANISO,          "aniso",        0 },
#endif
    { &CONFIG_BACKGROUND,     "background",   1 },
    { &CONFIG_SHADOW,         "shadow",       1 },
    { &CONFIG_AUDIO_BUFF,     "audio_buff",   AUDIO_BUFF_HI },
    { &CONFIG_TILTING_FLOOR,  "tilting_floor", 1 },
    { &CONFIG_MOUSE_SENSE,    "mouse_sense",  300 },
    { &CONFIG_MOUSE_RESPONSE, "mouse_response", 50 },
    { &CONFIG_MOUSE_INVERT,   "mouse_invert", 0 },
    { &CONFIG_VSYNC,          "vsync",        1 },
    { &CONFIG_HMD,            "hmd",          0 },
    { &CONFIG_HIGHDPI,        "highdpi",      1 },

    { &CONFIG_MOUSE_CAMERA_1,      "mouse_camera_1",      0 },
    { &CONFIG_MOUSE_CAMERA_2,      "mouse_camera_2",      0 },
    { &CONFIG_MOUSE_CAMERA_3,      "mouse_camera_3",      0 },
    { &CONFIG_MOUSE_CAMERA_TOGGLE, "mouse_camera_toggle", SDL_BUTTON_MIDDLE },
    { &CONFIG_MOUSE_CAMERA_L,      "mouse_camera_l",      SDL_BUTTON_LEFT },
    { &CONFIG_MOUSE_CAMERA_R,      "mouse_camera_r",      SDL_BUTTON_RIGHT },
    { &CONFIG_MOUSE_CANCEL_MENU,   "mouse_cancel_menu",   SDL_BUTTON_X1 },

    { &CONFIG_NICE,             "nice",            0 },
    { &CONFIG_FPS,              "fps",             0 },
    { &CONFIG_MASTER_VOLUME,    "master_volume",   10 },
    { &CONFIG_SOUND_VOLUME,     "sound_volume",    10 },
    { &CONFIG_MUSIC_VOLUME,     "music_volume",    4 },  /* NB default config value: 6 */
    { &CONFIG_NARRATOR_VOLUME,  "narrator_volume", 10 }, /* NB default config value: 6 */

    { &CONFIG_JOYSTICK,                "joystick",                1 },
    { &CONFIG_JOYSTICK_RESPONSE,       "joystick_response",       250 },
    { &CONFIG_JOYSTICK_AXIS_X0,        "joystick_axis_x0",        0 },
    { &CONFIG_JOYSTICK_AXIS_Y0,        "joystick_axis_y0",        1 },
    { &CONFIG_JOYSTICK_AXIS_X1,        "joystick_axis_x1",        3 },
    { &CONFIG_JOYSTICK_AXIS_Y1,        "joystick_axis_y1",        4 },
    { &CONFIG_JOYSTICK_AXIS_X0_INVERT, "joystick_axis_x0_invert", 0 },
    { &CONFIG_JOYSTICK_AXIS_Y0_INVERT, "joystick_axis_y0_invert", 0 },
    { &CONFIG_JOYSTICK_AXIS_X1_INVERT, "joystick_axis_x1_invert", 0 },
    { &CONFIG_JOYSTICK_AXIS_Y1_INVERT, "joystick_axis_y1_invert", 0 },

    { &CONFIG_JOYSTICK_BUTTON_A,       "joystick_button_a",      0 },
    { &CONFIG_JOYSTICK_BUTTON_B,       "joystick_button_b",      1 },
    { &CONFIG_JOYSTICK_BUTTON_X,       "joystick_button_x",      2 },
    { &CONFIG_JOYSTICK_BUTTON_Y,       "joystick_button_y",      3 },
    { &CONFIG_JOYSTICK_BUTTON_L1,      "joystick_button_l1",     4 },
    { &CONFIG_JOYSTICK_BUTTON_R1,      "joystick_button_r1",     6 },
    { &CONFIG_JOYSTICK_BUTTON_L2,      "joystick_button_l2",     5 },
    { &CONFIG_JOYSTICK_BUTTON_R2,      "joystick_button_r2",     7 },
    { &CONFIG_JOYSTICK_BUTTON_SELECT,  "joystick_button_select", 8 },
    { &CONFIG_JOYSTICK_BUTTON_START,   "joystick_button_start",  9 },
    { &CONFIG_JOYSTICK_DPAD_L,         "joystick_dpad_l",        8 },
    { &CONFIG_JOYSTICK_DPAD_R,         "joystick_dpad_r",        9 },
    { &CONFIG_JOYSTICK_DPAD_U,         "joystick_dpad_u",       10 },
    { &CONFIG_JOYSTICK_DPAD_D,         "joystick_dpad_d",       11 },

    { &CONFIG_WIIMOTE_INVERT_PITCH,      "wiimote_invert_pitch"     , 0 },
    { &CONFIG_WIIMOTE_INVERT_ROLL,       "wiimote_invert_roll"      , 0 },
    { &CONFIG_WIIMOTE_PITCH_SENSITIVITY, "wiimote_pitch_sensitivity", 100 },
    { &CONFIG_WIIMOTE_ROLL_SENSITIVITY,  "wiimote_roll_sensitivity" , 100 },
    { &CONFIG_WIIMOTE_SMOOTH_ALPHA,      "wiimote_smooth_alpha"     , 50 },
    { &CONFIG_WIIMOTE_HOLD_SIDEWAYS,     "wiimote_hold_sideways"    , 0 },

    { &CONFIG_KEY_CAMERA_1,      "key_camera_1",      SDLK_1 },
    { &CONFIG_KEY_CAMERA_2,      "key_camera_2",      SDLK_2 },
    { &CONFIG_KEY_CAMERA_3,      "key_camera_3",      SDLK_3 },
    { &CONFIG_KEY_CAMERA_TOGGLE, "key_camera_toggle", SDLK_e },
    { &CONFIG_KEY_CAMERA_R,      "key_camera_r",      SDLK_d },
    { &CONFIG_KEY_CAMERA_L,      "key_camera_l",      SDLK_s },
    { &CONFIG_KEY_FORWARD,       "key_forward",       SDLK_UP },
    { &CONFIG_KEY_BACKWARD,      "key_backward",      SDLK_DOWN },
    { &CONFIG_KEY_LEFT,          "key_left",          SDLK_LEFT },
    { &CONFIG_KEY_RIGHT,         "key_right",         SDLK_RIGHT },
    { &CONFIG_KEY_RESTART,       "key_restart",       SDLK_r },
    { &CONFIG_KEY_SCORE_NEXT,    "key_score_next",    SDLK_TAB },
    { &CONFIG_KEY_ROTATE_FAST,   "key_rotate_fast",   SDLK_LSHIFT },

    { &CONFIG_VIEW_FOV,    "view_fov",    50 },
    { &CONFIG_VIEW_DP,     "view_dp",     75 },
    { &CONFIG_VIEW_DC,     "view_dc",     25 },
    { &CONFIG_VIEW_DZ,     "view_dz",     200 },
    { &CONFIG_ROTATE_FAST, "rotate_fast", 300 },
    { &CONFIG_ROTATE_SLOW, "rotate_slow", 150 },
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
    { &CONFIG_CHEAT,       "cheat",       0 },
#if !defined(LOG_NO_STATS)
    { &CONFIG_STATS,       "stats",       0 },
#endif
#endif
    { &CONFIG_SCREENSHOT,  "screenshot",  0 },
    { &CONFIG_LOCK_GOALS,  "lock_goals",  0 },
    { &CONFIG_UNITS_METRIC, "units_metric", 0 },

    { &CONFIG_CAMERA_1_SPEED, "camera_1_speed", 250 },
    { &CONFIG_CAMERA_2_SPEED, "camera_2_speed", 0 },
    { &CONFIG_CAMERA_3_SPEED, "camera_3_speed", -1 },

    { &CONFIG_TOUCH_ROTATE, "touch_rotate", 16 },

    { &CONFIG_ONLINE, "online", 1 },
};

static struct
{
    int        *sym;
    const char *name;
    const char *def;
    char       *cur;
} option_s[] = {
    { &CONFIG_PLAYER,                  "player",          "" },
#if !defined(CONFIG_INCLUDES_ACCOUNT)
    { &CONFIG_BALL_FILE,               "ball_file",       "ball/basic-ball/basic-ball" },
#endif
    { &CONFIG_WIIMOTE_ADDR,            "wiimote_addr",    "" },
    { &CONFIG_REPLAY_NAME,             "replay_name",     "%s-%l-%r" },
    { &CONFIG_LANGUAGE,                "language",        "" },
    { &CONFIG_THEME,                   "theme",           "classic" },
    { &CONFIG_DEDICATED_IPADDRESS,     "dedicated_ipaddress", "play.neverball.org" },
    { &CONFIG_DEDICATED_IPPORT,        "dedicated_ipport", "5000" }
};

static int dirty = 0;

/*---------------------------------------------------------------------------*/

static void config_key(const char *s, int i)
{
#if !defined(__GAMECUBE__) && !defined(__WII__)
    SDL_Keycode c = SDL_GetKeyFromName(s);
#else
    SDL_Keycode c = 0;
#endif

    if (c == SDLK_UNKNOWN)
        config_set_d(i, option_d[i].def);
    else
        config_set_d(i, c);
}

/*---------------------------------------------------------------------------*/

static void config_mouse(const char *s, int i)
{
    if      (strcmp(s, "none") == 0)
        config_set_d(i, 0);
    else if (strcmp(s, "left") == 0)
        config_set_d(i, SDL_BUTTON_LEFT);
    else if (strcmp(s, "right") == 0)
        config_set_d(i, SDL_BUTTON_RIGHT);
    else if (strcmp(s, "middle") == 0)
        config_set_d(i, SDL_BUTTON_MIDDLE);
    else
        config_set_d(i, atoi(s));
}

static const char *config_mouse_name(int b)
{
    static char buff[4];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(buff, 4,
#else
    sprintf(buff,
#endif
           "%d", b);

    switch (b)
    {
        case 0:                    return "none";      break;
        case SDL_BUTTON_LEFT:      return "left";      break;
        case SDL_BUTTON_RIGHT:     return "right";     break;
        case SDL_BUTTON_MIDDLE:    return "middle";    break;
        default:                   return buff;        break;
    }
}

/*---------------------------------------------------------------------------*/

void config_init(void)
{
    int i;

    config_busy = 1;

    /*
     * Store index of each option in its associated config symbol and
     * initialise current values with defaults.
     */

    for (i = 0; i < ARRAYSIZE(option_d); i++)
    {
        *option_d[i].sym = i;
        config_set_d(i, option_d[i].def);
    }

    for (i = 0; i < ARRAYSIZE(option_s); i++)
    {
        *option_s[i].sym = i;
        config_set_s(i, option_s[i].def);
    }

    config_busy = 0;

    config_is_init = 1;
}

void config_quit(void)
{
    if (!config_is_init) return;

    int i;

    for (i = 0; i < ARRAYSIZE(option_s); i++)
    {
        if (option_s[i].cur)
        {
            free(option_s[i].cur);
            option_s[i].cur = NULL;
        }
    }
    config_is_init = 0;
}

/*
 * Scan an option string and store pointers to the start of key and
 * value at the passed-in locations.  No memory is allocated to store
 * the strings; instead, the option string is modified in-place as
 * needed.  Return 1 on success, 0 on error.
 */
static int scan_key_and_value(char **dst_key, char **dst_val, char *line)
{
    if (line)
    {
        int valid_num, ks, ke, vs;

        ks = -1;
        ke = -1;
        vs = -1;

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        valid_num = sscanf_s(line,
#else
        valid_num = sscanf(line,
#endif
                           " %n%*s%n %n", &ks, &ke, &vs);

        if (ks < 0 || ke < 0 || vs < 0)
            return 0;

        if (vs - ke < 1)
            return 0;

        line[ke] = 0;

        *dst_key = line + ks;
        *dst_val = line + vs;

        return 1;
    }

    return 0;
}

/*---------------------------------------------------------------------------*/

/*
 * HACK: These config file has lots of mapped text file to pretend,
 * but let's do it with macros as renamers, same as replay commands.
 * - Ersohn Styne
 */

#define CONFIG_PARSE_FILENAME(_src)                                      \
    do {                                                                 \
        if (fs_exists(_src)) {                                           \
            if (parse_ok) fs_remove(_src);                               \
            else if (fs_rename(_src, filename) == 0) {                   \
                log_errorf("Can't replace filename!: %s\n", fs_error()); \
                fs_remove(_src);                                         \
            } else {                                                     \
                log_printf("Config filename replaced!: %s -> %s\n",      \
                           _src, filename);                              \
                parse_ok = 1;                                            \
            }                                                            \
        } else if (fs_exists("." _src)) {                                \
            if (parse_ok) fs_remove("." _src);                           \
            else if (fs_rename("." _src, filename) == 0) {               \
                log_errorf("Can't replace filename!: %s\n", fs_error()); \
                fs_remove("." _src);                                     \
            } else {                                                     \
                log_printf("Config filename replaced!: %s -> %s\n",      \
                           "." _src, filename);                          \
                parse_ok = 1;                                            \
            }                                                            \
        } else if (fs_exists(_src ".txt")) {                             \
            if (parse_ok) fs_remove(_src ".txt");                        \
            else if (fs_rename(_src ".txt", filename) == 0) {            \
                log_errorf("Can't replace filename!: %s\n", fs_error()); \
                fs_remove(_src ".txt");                                  \
            } else {                                                     \
                log_printf("Config filename replaced!: %s -> %s\n",      \
                           _src ".txt", filename);                       \
                parse_ok = 1;                                            \
            }                                                            \
        } else if (fs_exists("." _src ".txt")) {                         \
            if (parse_ok) fs_remove("." _src ".txt");                    \
            else if (fs_rename("." _src ".txt", filename) == 0) {        \
                log_errorf("Can't replace filename!: %s\n", fs_error()); \
                fs_remove("." _src ".txt");                              \
            } else {                                                     \
                log_printf("Config filename replaced!: %s -> %s\n",      \
                           "." _src ".txt", filename);                   \
                parse_ok = 1;                                            \
            }                                                            \
        }                                                                \
    } while (0)                                                          \

#define CONFIG_PARSE_FILETYPE(_ext)                                         \
    do {                                                                    \
        if (fs_exists(USER_CONFIG_FILE _ext)) {                             \
            if (parse_ok) fs_remove(USER_CONFIG_FILE _ext);                 \
            else if (fs_rename(USER_CONFIG_FILE _ext, filename) == 0) {     \
                log_errorf("Can't take out file types!: %s\n", fs_error()); \
                fs_remove(USER_CONFIG_FILE _ext);                           \
            } else {                                                        \
                log_printf("File type taken out!: %s -> %s\n",              \
                           USER_CONFIG_FILE _ext, filename);                \
                parse_ok = 1;                                               \
            }                                                               \
        }                                                                   \
    } while (0)                                                             \

/*
 * Scan all config files to parse the game settings. Only one game config file
 * is allowed. The other config file will be deleted after renamed.
 *
 * Return 1 on success, 0 on error.
 *
 * NOTE: Backups are not affected while parsing.
 */
static int config_parse_file(const char *filename)
{
    int parse_ok = 0;

    /* Replace specified file name to *ballrc with optional file extensions. */

    CONFIG_PARSE_FILENAME("conf");
    CONFIG_PARSE_FILENAME("config");
    CONFIG_PARSE_FILENAME("options");
    CONFIG_PARSE_FILENAME("preferences");
    CONFIG_PARSE_FILENAME("settings");
    CONFIG_PARSE_FILENAME("user_settings");
    CONFIG_PARSE_FILENAME("user-settings");
    CONFIG_PARSE_FILENAME("usersettings");

    CONFIG_PARSE_FILENAME("game_conf");
    CONFIG_PARSE_FILENAME("game-conf");
    CONFIG_PARSE_FILENAME("game_config");
    CONFIG_PARSE_FILENAME("game-config");
    CONFIG_PARSE_FILENAME("game_options");
    CONFIG_PARSE_FILENAME("game-options");
    CONFIG_PARSE_FILENAME("game_preferences");
    CONFIG_PARSE_FILENAME("game-preferences");
    CONFIG_PARSE_FILENAME("game_settings");
    CONFIG_PARSE_FILENAME("game-settings");
    CONFIG_PARSE_FILENAME("game_user_settings");
    CONFIG_PARSE_FILENAME("game-user-settings");
    CONFIG_PARSE_FILENAME("game_usersettings");
    CONFIG_PARSE_FILENAME("game-usersettings");

    CONFIG_PARSE_FILENAME("gameconf");
    CONFIG_PARSE_FILENAME("gameconfig");
    CONFIG_PARSE_FILENAME("gameoptions");
    CONFIG_PARSE_FILENAME("gamepreferences");
    CONFIG_PARSE_FILENAME("gamesettings");
    CONFIG_PARSE_FILENAME("gameusersettings");
    CONFIG_PARSE_FILENAME("gameusersettings");

    CONFIG_PARSE_FILENAME("global_conf");
    CONFIG_PARSE_FILENAME("global-conf");
    CONFIG_PARSE_FILENAME("global_conf");
    CONFIG_PARSE_FILENAME("global-config");
    CONFIG_PARSE_FILENAME("global_options");
    CONFIG_PARSE_FILENAME("global-options");
    CONFIG_PARSE_FILENAME("global_preferences");
    CONFIG_PARSE_FILENAME("global-preferences");
    CONFIG_PARSE_FILENAME("global_settings");
    CONFIG_PARSE_FILENAME("global-settings");
    CONFIG_PARSE_FILENAME("global_user_settings");
    CONFIG_PARSE_FILENAME("global-user-settings");
    CONFIG_PARSE_FILENAME("global_usersettings");
    CONFIG_PARSE_FILENAME("global-usersettings");

    CONFIG_PARSE_FILENAME("globalconf");
    CONFIG_PARSE_FILENAME("globalconfig");
    CONFIG_PARSE_FILENAME("globaloptions");
    CONFIG_PARSE_FILENAME("globalpreferences");
    CONFIG_PARSE_FILENAME("globalsettings");
    CONFIG_PARSE_FILENAME("globalusersettings");
    CONFIG_PARSE_FILENAME("globalusersettings");

    /* Take out the file type extensions for global settings. */

    CONFIG_PARSE_FILETYPE(".txt");

#if NB_HAVE_PB_BOTH==1
    /* Replace legacy config filename. */

    CONFIG_PARSE_FILENAME("neverballrc");
#endif

    return parse_ok || fs_exists(USER_CONFIG_FILE);
}

#undef CONFIG_PARSE_FILENAME_REPLACE
#undef CONFIG_PARSE_FILENAME_TAKEOUT

/*---------------------------------------------------------------------------*/

void config_load(void)
{
    fs_file fh;
#ifndef NDEBUG
#if NB_HAVE_PB_BOTH==1
    assert(!networking_busy && !accessibility_busy && !account_busy &&
           "This networking, accessibility or account data is busy and cannot be edit there!");
#else
    assert(!accessibility_busy &&
           "This accessibility data is busy and cannot be edit there!");
#endif
    SDL_assert(SDL_WasInit(SDL_INIT_VIDEO));
#endif

    const char *filename = USER_CONFIG_FILE;

    if (!config_is_init)
    {
        log_errorf("Failure to load configuration file! Configurations must be initialized!\n");
#if _DEBUG
        SDL_TriggerBreakpoint();
#endif

        return;
    }

    if (!config_parse_file(filename)) return;

    if ((fh = fs_open_read(filename)))
    {
        config_busy = 1;
        char *line, *key, *val;

        while (read_line(&line, fh))
        {
            if (scan_key_and_value(&key, &val, line))
            {
                int i;

                /* Look up an integer option by that name. */

                for (i = 0; i < ARRAYSIZE(option_d); i++)
                {
                    if (strcmp(key, option_d[i].name) == 0)
                    {
                        /* Translate some strings to integers. */

                        if (i == CONFIG_MOUSE_CAMERA_1      ||
                            i == CONFIG_MOUSE_CAMERA_2      ||
                            i == CONFIG_MOUSE_CAMERA_3      ||
                            i == CONFIG_MOUSE_CAMERA_TOGGLE ||
                            i == CONFIG_MOUSE_CAMERA_L      ||
                            i == CONFIG_MOUSE_CAMERA_R      ||
                            i == CONFIG_MOUSE_CANCEL_MENU)
                            config_mouse(val, i);
                        else if (i == CONFIG_KEY_FORWARD       ||
                                 i == CONFIG_KEY_BACKWARD      ||
                                 i == CONFIG_KEY_LEFT          ||
                                 i == CONFIG_KEY_RIGHT         ||
                                 i == CONFIG_KEY_CAMERA_1      ||
                                 i == CONFIG_KEY_CAMERA_2      ||
                                 i == CONFIG_KEY_CAMERA_3      ||
                                 i == CONFIG_KEY_CAMERA_TOGGLE ||
                                 i == CONFIG_KEY_CAMERA_R      ||
                                 i == CONFIG_KEY_CAMERA_L      ||
                                 i == CONFIG_KEY_RESTART       ||
                                 i == CONFIG_KEY_SCORE_NEXT    ||
                                 i == CONFIG_KEY_ROTATE_FAST)
                            config_key(val, i);
                        else if (
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
                                 i != CONFIG_CHEAT   &&
#endif
                                 i != CONFIG_VIEW_DP &&
                                 i != CONFIG_VIEW_DC &&
                                 i != CONFIG_VIEW_DZ)
                            config_set_d(i, atoi(val));

                        /* Stop looking. */

                        break;
                    }
                }

                /* Look up a string option by that name.*/

                for (i = 0; i < ARRAYSIZE(option_s); i++)
                {
                    if (strcmp(key, option_s[i].name) == 0)
                    {
                        config_set_s(i, val);
                        break;
                    }
                }
            }
            free(line);
            line = NULL;
        }
        fs_close(fh);

#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
        WGCL_LoadGameSystemSettings();
#endif

        dirty = 0;
        config_busy = 0;
    }
}

void config_save(void)
{
    fs_file fh;

#ifndef NDEBUG
#if NB_HAVE_PB_BOTH==1
    assert(!networking_busy && !accessibility_busy && !account_busy &&
           "This networking, accessibility or account data is busy and cannot be edit there!");
#else
    assert(!accessibility_busy &&
           "This accessibility data is busy and cannot be edit there!");
#endif
    SDL_assert(SDL_WasInit(SDL_INIT_VIDEO));
#endif

    const char *filename = USER_CONFIG_FILE;

    if (dirty && !config_is_init)
    {
        log_errorf("Failure to save configuration file! Configurations must be initialized!\n");
#if _DEBUG
        SDL_TriggerBreakpoint();
#endif

        if (fs_exists(filename))
            fs_remove(filename);

        return;
    }

    if (dirty && (fh = fs_open_write(filename)))
    {
        config_busy = 1;
        int i;

        /* Write out integer options. */

        for (i = 0; i < ARRAYSIZE(option_d); i++)
        {
            const char *s = NULL;

            /* Switchball Classic / HD made me do it. */

            if (i == CONFIG_VIEW_DP ||
                i == CONFIG_VIEW_DC ||
                i == CONFIG_VIEW_DZ)
                continue;

            /* Translate some integers to strings. */

            if (i == CONFIG_MOUSE_CAMERA_1      ||
                i == CONFIG_MOUSE_CAMERA_2      ||
                i == CONFIG_MOUSE_CAMERA_3      ||
                i == CONFIG_MOUSE_CAMERA_TOGGLE ||
                i == CONFIG_MOUSE_CAMERA_L      ||
                i == CONFIG_MOUSE_CAMERA_R      ||
                i == CONFIG_MOUSE_CANCEL_MENU)
                s = config_mouse_name(option_d[i].cur);
            else if (i == CONFIG_KEY_FORWARD       ||
                     i == CONFIG_KEY_BACKWARD      ||
                     i == CONFIG_KEY_LEFT          ||
                     i == CONFIG_KEY_RIGHT         ||
                     i == CONFIG_KEY_CAMERA_1      ||
                     i == CONFIG_KEY_CAMERA_2      ||
                     i == CONFIG_KEY_CAMERA_3      ||
                     i == CONFIG_KEY_CAMERA_TOGGLE ||
                     i == CONFIG_KEY_CAMERA_R      ||
                     i == CONFIG_KEY_CAMERA_L      ||
                     i == CONFIG_KEY_RESTART       ||
                     i == CONFIG_KEY_SCORE_NEXT    ||
                     i == CONFIG_KEY_ROTATE_FAST)
                s = SDL_GetKeyName((SDL_Keycode) option_d[i].cur);
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
            else if (i == CONFIG_CHEAT)
            {
                if (!config_cheat())
                    continue;
            }
#endif

            if (s)
                fs_printf(fh, "%-25s %s\n", option_d[i].name, s);
            else
                fs_printf(fh, "%-25s %d\n", option_d[i].name, option_d[i].cur);
        }

        /* Write out string options. */

        for (i = 0; i < ARRAYSIZE(option_s); i++)
            fs_printf(fh, "%-25s %s\n", option_s[i].name, option_s[i].cur);

        fs_close(fh);

#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
        WGCL_SaveGameSystemSettings();
#endif
    }
    else if (dirty)
        log_errorf("Failure to save configuration file!: %s / %s\n",
                   filename, fs_error());

    dirty = 0;
    config_busy = 0;
}

/*---------------------------------------------------------------------------*/

void config_set_d(int i, int d)
{
#ifndef NDEBUG
#if NB_HAVE_PB_BOTH==1
    assert(!networking_busy && !accessibility_busy && !account_busy &&
           "This networking, accessibility or account data is busy and cannot be edit there!");
    if (!networking_busy && !accessibility_busy && !account_busy)
#else
    assert(!accessibility_busy &&
           "This networking or accessibility data is busy and cannot be edit there!");
    if (!accessibility_busy)
#endif
#endif
    {
        config_busy = 1;
        option_d[i].cur = d;
        dirty = 1;
        config_busy = 0;
    }
}

void config_tgl_d(int i)
{
#ifndef NDEBUG
#if NB_HAVE_PB_BOTH==1
    assert(!networking_busy && !accessibility_busy && !account_busy &&
           "This networking, accessibility or account data is busy and cannot be edit there!");
    if (!networking_busy && !accessibility_busy && !account_busy)
#else
    assert(!accessibility_busy &&
           "This networking or accessibility data is busy and cannot be edit there!");
    if (!accessibility_busy)
#endif
#endif
    {
        config_busy = 1;
        option_d[i].cur = (option_d[i].cur ? 0 : 1);
        dirty = 1;
        config_busy = 0;
    }
}

int config_tst_d(int i, int d)
{
    return (option_d[i].cur == d) ? 1 : 0;
}

int config_get_d(int i)
{
    return option_d[i].cur;
}

/*---------------------------------------------------------------------------*/

void config_set_s(int i, const char *src)
{
#ifndef NDEBUG
#if NB_HAVE_PB_BOTH==1
    assert(!networking_busy && !accessibility_busy && !account_busy &&
           "This networking, accessibility or account data is busy and cannot be edit there!");
    if (!networking_busy && !accessibility_busy && !account_busy)
#else
    assert(!accessibility_busy &&
           "This accessibility data is busy and cannot be edit there!");
    if (!accessibility_busy)
#endif
#endif
    {
        config_busy = 1;

        if (option_s[i].cur)
        {
            free(option_s[i].cur);
            option_s[i].cur = NULL;
        }

        option_s[i].cur = strdup(src);

        dirty = 1;
        config_busy = 0;
    }
}

const char *config_get_s(int i)
{
    return option_s[i].cur;
}

/*---------------------------------------------------------------------------*/

/*
 * Set an option from a string value.
 *
 * Works for both int and string options. Don't use this if you already have an int.
 */
void config_set(const char *key, const char *value)
{
    if (key && *key)
    {
        int i;

        for (i = 0; i < ARRAYSIZE(option_s); ++i)
        {
            if (strcmp(option_s[i].name, key) == 0)
                config_set_s(i, value);
        }

        for (i = 0; i < ARRAYSIZE(option_d); ++i)
        {
            if (strcmp(option_d[i].name, key) == 0)
                config_set_d(i, atoi(value));
        }
    }
}

/*---------------------------------------------------------------------------*/

#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
int config_cheat(void)
{
    return config_get_d(CONFIG_CHEAT);
}

void config_set_cheat(void)
{
#ifndef NDEBUG
#if NB_HAVE_PB_BOTH==1
    assert(!networking_busy && !accessibility_busy && !account_busy &&
           "This networking, accessibility or account data is busy and cannot be edit there!");
    if (!networking_busy && !accessibility_busy && !account_busy)
#else
    assert(!accessibility_busy &&
           "This networking, accessibility data is busy and cannot be edit there!");
    if (!accessibility_busy)
#endif
#endif
    {
        config_busy = 1;
        config_set_d(CONFIG_CHEAT, 1);
        config_busy = 0;
    }
}

void config_clr_cheat(void)
{
#ifndef NDEBUG
#if NB_HAVE_PB_BOTH==1
    assert(!networking_busy && !accessibility_busy && !account_busy &&
           "This networking, accessibility or account data is busy and cannot be edit there!");
    if (!networking_busy && !accessibility_busy && !account_busy)
#else
    assert(!accessibility_busy &&
           "This networking, accessibility data is busy and cannot be edit there!");
    if (!accessibility_busy)
#endif
#endif
    {
        config_busy = 1;
        config_set_d(CONFIG_CHEAT, 0);
        config_busy = 0;
    }
}
#endif

/*---------------------------------------------------------------------------*/

int config_screenshot(void)
{
#ifndef NDEBUG
#if NB_HAVE_PB_BOTH==1
    assert(!networking_busy && !accessibility_busy && !account_busy &&
           "This networking, accessibility or account data is busy and cannot be edit there!");
#else
    assert(!accessibility_busy &&
           "This networking, accessibility data is busy and cannot be edit there!");
#endif
#endif
    return ++option_d[CONFIG_SCREENSHOT].cur;
}

/*---------------------------------------------------------------------------*/

static int local_username_locked;

void config_lock_local_username(void)
{
#if !defined(__EMSCRIPTEN__) && defined(CONFIG_PLAYER_LOCKEDBY_SYSTEMOS)
    char username[MAXSTR];

#if _MSC_VER
    /* Microsoft Windows Username Display */

    DWORD username_len = MAXSTR;

    if (GetUserNameA(username, &username_len))
    {
        log_errorf("This player name was locked to \"%s\" by the system os!\n", username);
        config_set_s(CONFIG_PLAYER, username);
        local_username_locked = 1;
    }
#else
    /* POSIX Username Display */

    uid_t          uid = geteuid();
    struct passwd *pw  = getpwuid(uid);

    if (pw)
    {
        SAFECPY(username, pw->pw_name);
        log_errorf("This player name was locked to \"%s\" by the system os!\n", username);
        config_set_s(CONFIG_PLAYER, username);
        local_username_locked = 1;
    }
#endif
    else local_username_locked = 0;
#endif
}

int config_playername_locked(void)
{
#ifndef __EMSCRIPTEN__
    return local_username_locked;
#else
    return 0;
#endif
}

/*---------------------------------------------------------------------------*/
