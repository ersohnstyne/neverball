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

#ifndef CONFIG_H
#define CONFIG_H

/*
 * Statistics of the current frame time and frames-per-second,
 * averaged over one second, is not permitted, as of later than 1.6.
 */
#define LOG_NO_STATS

/*---------------------------------------------------------------------------*/

extern int config_busy;

/* Integer options. */

extern int CONFIG_ACCOUNT_TUTORIAL;
extern int CONFIG_ACCOUNT_HINT;
extern int CONFIG_ACCOUNT_BEAM_STYLE;
extern int CONFIG_ACCOUNT_SAVE;
extern int CONFIG_ACCOUNT_LOAD;

extern int CONFIG_NOTIFICATION_CHKP;
extern int CONFIG_NOTIFICATION_REWARD;
extern int CONFIG_NOTIFICATION_SHOP;

extern int CONFIG_GRAPHIC_RESTORE_ID;
extern int CONFIG_GRAPHIC_RESTORE_VAL1;
extern int CONFIG_GRAPHIC_RESTORE_VAL2;

extern int CONFIG_MAINMENU_PANONLY;

extern int CONFIG_SCREEN_ANIMATIONS;
extern int CONFIG_UI_HWACCEL;
extern int CONFIG_SMOOTH_FIX;
extern int CONFIG_FORCE_SMOOTH_FIX;
extern int CONFIG_WIDESCREEN;
extern int CONFIG_FULLSCREEN;
extern int CONFIG_MAXIMIZED;
extern int CONFIG_DISPLAY;
extern int CONFIG_WIDTH;
extern int CONFIG_HEIGHT;
extern int CONFIG_STEREO;
extern int CONFIG_CAMERA;
extern int CONFIG_CAMERA_ROTATE_MODE;
extern int CONFIG_TEXTURES;
extern int CONFIG_MOTIONBLUR;
extern int CONFIG_REFLECTION;
extern int CONFIG_MULTISAMPLE;
#ifdef GL_GENERATE_MIPMAP_SGIS
extern int CONFIG_MIPMAP;
#endif
#ifdef GL_TEXTURE_MAX_ANISOTROPY_EXT
extern int CONFIG_ANISO;
#endif
extern int CONFIG_BACKGROUND;
extern int CONFIG_SHADOW;
extern int CONFIG_AUDIO_BUFF;
extern int CONFIG_TILTING_FLOOR;
extern int CONFIG_MOUSE_SENSE;
extern int CONFIG_MOUSE_RESPONSE;
extern int CONFIG_MOUSE_INVERT;
extern int CONFIG_VSYNC;
extern int CONFIG_HMD;
extern int CONFIG_HIGHDPI;
extern int CONFIG_MOUSE_CAMERA_1;
extern int CONFIG_MOUSE_CAMERA_2;
extern int CONFIG_MOUSE_CAMERA_3;
extern int CONFIG_MOUSE_CAMERA_TOGGLE;
extern int CONFIG_MOUSE_CAMERA_L;
extern int CONFIG_MOUSE_CAMERA_R;
extern int CONFIG_MOUSE_CANCEL_MENU;
extern int CONFIG_NICE;
extern int CONFIG_FPS;
extern int CONFIG_MASTER_VOLUME;
extern int CONFIG_SOUND_VOLUME;
extern int CONFIG_MUSIC_VOLUME;
extern int CONFIG_NARRATOR_VOLUME;
extern int CONFIG_JOYSTICK;
extern int CONFIG_JOYSTICK_RESPONSE;
extern int CONFIG_JOYSTICK_AXIS_X0;
extern int CONFIG_JOYSTICK_AXIS_Y0;
extern int CONFIG_JOYSTICK_AXIS_X1;
extern int CONFIG_JOYSTICK_AXIS_Y1;
extern int CONFIG_JOYSTICK_AXIS_X0_INVERT;
extern int CONFIG_JOYSTICK_AXIS_Y0_INVERT;
extern int CONFIG_JOYSTICK_AXIS_X1_INVERT;
extern int CONFIG_JOYSTICK_AXIS_Y1_INVERT;

extern int CONFIG_JOYSTICK_BUTTON_A;
extern int CONFIG_JOYSTICK_BUTTON_B;
extern int CONFIG_JOYSTICK_BUTTON_X;
extern int CONFIG_JOYSTICK_BUTTON_Y;
extern int CONFIG_JOYSTICK_BUTTON_L1;
extern int CONFIG_JOYSTICK_BUTTON_R1;
extern int CONFIG_JOYSTICK_BUTTON_L2;
extern int CONFIG_JOYSTICK_BUTTON_R2;
extern int CONFIG_JOYSTICK_BUTTON_SELECT;
extern int CONFIG_JOYSTICK_BUTTON_START;
extern int CONFIG_JOYSTICK_DPAD_L;
extern int CONFIG_JOYSTICK_DPAD_R;
extern int CONFIG_JOYSTICK_DPAD_U;
extern int CONFIG_JOYSTICK_DPAD_D;

extern int CONFIG_WIIMOTE_INVERT_PITCH;
extern int CONFIG_WIIMOTE_INVERT_ROLL;
extern int CONFIG_WIIMOTE_PITCH_SENSITIVITY;
extern int CONFIG_WIIMOTE_ROLL_SENSITIVITY;
extern int CONFIG_WIIMOTE_SMOOTH_ALPHA;
extern int CONFIG_WIIMOTE_HOLD_SIDEWAYS;

extern int CONFIG_KEY_CAMERA_1;
extern int CONFIG_KEY_CAMERA_2;
extern int CONFIG_KEY_CAMERA_3;
extern int CONFIG_KEY_CAMERA_TOGGLE;
extern int CONFIG_KEY_CAMERA_R;
extern int CONFIG_KEY_CAMERA_L;
extern int CONFIG_KEY_FORWARD;
extern int CONFIG_KEY_BACKWARD;
extern int CONFIG_KEY_LEFT;
extern int CONFIG_KEY_RIGHT;
extern int CONFIG_KEY_RESTART;
extern int CONFIG_KEY_SCORE_NEXT;
extern int CONFIG_KEY_ROTATE_FAST;
extern int CONFIG_VIEW_FOV;
extern int CONFIG_VIEW_DP;
extern int CONFIG_VIEW_DC;
extern int CONFIG_VIEW_DZ;
extern int CONFIG_ROTATE_FAST;
extern int CONFIG_ROTATE_SLOW;
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
extern int CONFIG_CHEAT;
#if !defined(LOG_NO_STATS)
extern int CONFIG_STATS;
#endif
#endif
extern int CONFIG_SCREENSHOT;
extern int CONFIG_LOCK_GOALS;
extern int CONFIG_UNITS_METRIC;
extern int CONFIG_CAMERA_1_SPEED;
extern int CONFIG_CAMERA_2_SPEED;
extern int CONFIG_CAMERA_3_SPEED;

extern int CONFIG_TOUCH_ROTATE;

extern int CONFIG_ONLINE;

/* String options. */

extern int CONFIG_PLAYER;
#if !defined(CONFIG_INCLUDES_ACCOUNT)
extern int CONFIG_BALL_FILE;
#endif
extern int CONFIG_WIIMOTE_ADDR;
extern int CONFIG_REPLAY_NAME;
extern int CONFIG_LANGUAGE;
extern int CONFIG_THEME;
extern int CONFIG_DEDICATED_IPADDRESS;
extern int CONFIG_DEDICATED_IPPORT;

/*---------------------------------------------------------------------------*/

extern float axis_offset[4];

/*---------------------------------------------------------------------------*/

void config_init(void);
void config_quit(void);
void config_load(void);
void config_save(void);

/*---------------------------------------------------------------------------*/

void config_set_d(int, int);
void config_tgl_d(int);
int  config_tst_d(int, int);
int  config_get_d(int);

void        config_set_s(int, const char *);
const char *config_get_s(int);

void config_set(const char *, const char *);

/*---------------------------------------------------------------------------*/

#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
int  config_cheat(void);
#else
/* This keys is not available for cheat */
#define config_cheat() ((int) 0)
#endif
void config_set_cheat(void);
void config_clr_cheat(void);

/*---------------------------------------------------------------------------*/

int config_screenshot(void);

/*---------------------------------------------------------------------------*/

void config_lock_local_username(void);
int  config_playername_locked(void);

/*---------------------------------------------------------------------------*/

#endif
