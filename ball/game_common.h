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

#ifndef GAME_COMMON_H
#define GAME_COMMON_H

#include "lang.h"
#include "solid_vary.h"

/*---------------------------------------------------------------------------*/

#define AUD_INTRO_THROW   "snd/intro-throw.ogg"
#define AUD_INTRO_SHATTER "snd/intro-shatter.ogg"

#define AUD_DISABLED "snd/disabled.ogg"
#define AUD_FOCUS    "snd/focus.ogg"
#define AUD_MENU     "snd/menu.ogg"
#define AUD_START  _("snd/select.ogg")
#define AUD_READY  _("snd/ready.ogg")
#define AUD_SET    _("snd/set.ogg")
#define AUD_GO     _("snd/go.ogg")
#define AUD_BALL     "snd/ball.ogg"
#define AUD_BUMPS    "snd/bumplil.ogg"
#define AUD_BUMPM    "snd/bump.ogg"
#define AUD_BUMPL    "snd/bumpbig.ogg"
#define AUD_COIN     "snd/coin.ogg"
#define AUD_TICK     "snd/tick.ogg"
#define AUD_TOCK     "snd/tock.ogg"
#define AUD_SWITCH   "snd/switch.ogg"
#define AUD_JUMP     "snd/jump.ogg"
#define AUD_GOAL     "snd/goal.ogg"
#define AUD_SCORE  _("snd/record.ogg")
#define AUD_FALL   _("snd/fall.ogg")
#define AUD_TIME   _("snd/time.ogg")
#define AUD_OVER   _("snd/over.ogg")
#define AUD_GROW     "snd/grow.ogg"
#define AUD_SHRINK   "snd/shrink.ogg"
#define AUD_CLOCK    "snd/clock.ogg"

/* And with Switchball features? */

#define AUD_STARTGAME "snd/startgame.ogg"
#define AUD_QUITGAME  "snd/quitgame.ogg"
#define AUD_BACK      "snd/back.ogg"
#define AUD_CHKP      "snd/checkpoint.ogg"
#define AUD_RESPAWN   "snd/respawn.ogg"
#define AUD_GOAL_N    "snd/goal_noninvert.ogg"

/* And with Geometry Dash features? */

#define AUD_STARS     "snd/stars.ogg"

/* OK, title music locales, but test. */

#define BGM_TITLE_CONF_LANGUAGE \
    str_starts_with(config_get_s(CONFIG_LANGUAGE), "de") ? (fs_exists("bgm/de/title.ogg") ? "bgm/de/title.ogg" : "bgm/title.ogg") : \
    str_starts_with(config_get_s(CONFIG_LANGUAGE), "fr") ? (fs_exists("bgm/fr/title.ogg") ? "bgm/fr/title.ogg" : "bgm/title.ogg") : \
    str_starts_with(config_get_s(CONFIG_LANGUAGE), "ja") ? (fs_exists("bgm/jp/title.ogg") ? "bgm/jp/title.ogg" : "bgm/title.ogg") : \
    str_starts_with(config_get_s(CONFIG_LANGUAGE), "jp") ? (fs_exists("bgm/jp/title.ogg") ? "bgm/jp/title.ogg" : "bgm/title.ogg") : \
    str_starts_with(config_get_s(CONFIG_LANGUAGE), "th") ? (fs_exists("bgm/th/title.ogg") ? "bgm/th/title.ogg" : "bgm/title.ogg") : \
    str_starts_with(config_get_s(CONFIG_LANGUAGE), "zh") ? (fs_exists("bgm/zh/title.ogg") ? "bgm/zh/title.ogg" : "bgm/title.ogg") : \
    "bgm/title.ogg"

#define BGM_TITLE_MAP(bgm_path) \
    str_starts_with(bgm_path, "bgm/title.ogg") && str_starts_with(config_get_s(CONFIG_LANGUAGE), "de") ? (fs_exists("bgm/de/title.ogg") ? "bgm/de/title.ogg" : "bgm/title.ogg") : \
    str_starts_with(bgm_path, "bgm/title.ogg") && str_starts_with(config_get_s(CONFIG_LANGUAGE), "fr") ? (fs_exists("bgm/fr/title.ogg") ? "bgm/fr/title.ogg" : "bgm/title.ogg") : \
    str_starts_with(bgm_path, "bgm/title.ogg") && str_starts_with(config_get_s(CONFIG_LANGUAGE), "ja") ? (fs_exists("bgm/jp/title.ogg") ? "bgm/jp/title.ogg" : "bgm/title.ogg") : \
    str_starts_with(bgm_path, "bgm/title.ogg") && str_starts_with(config_get_s(CONFIG_LANGUAGE), "jp") ? (fs_exists("bgm/jp/title.ogg") ? "bgm/jp/title.ogg" : "bgm/title.ogg") : \
    str_starts_with(bgm_path, "bgm/title.ogg") && str_starts_with(config_get_s(CONFIG_LANGUAGE), "th") ? (fs_exists("bgm/th/title.ogg") ? "bgm/th/title.ogg" : "bgm/title.ogg") : \
    str_starts_with(bgm_path, "bgm/title.ogg") && str_starts_with(config_get_s(CONFIG_LANGUAGE), "zh") ? (fs_exists("bgm/zh/title.ogg") ? "bgm/zh/title.ogg" : "bgm/title.ogg") : \
    bgm_path

/*---------------------------------------------------------------------------*/

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

/*---------------------------------------------------------------------------*/

enum
{
    GAME_NONE = 0,

    GAME_TIME,
    GAME_GOAL,
    GAME_FALL,

    GAME_MAX
};

const char *status_to_str(int);

/*---------------------------------------------------------------------------*/

enum
{
    CAM_NONE = -1,

    CAM_1, /* Chase camera */
    CAM_2, /* Static / Lazy Camera */
    CAM_3, /* Manual / Free camera */

    CAM_AUTO, /* Switchball uses an automatic camera */

    CAM_MAX
};

const char *cam_to_str(int);

int cam_speed(int);

/*---------------------------------------------------------------------------*/

#if ENABLE_EARTHQUAKE==1
void   game_randomize_earthquake_shake(void);
float *game_get_earthquake_shake(void);
#endif

/*---------------------------------------------------------------------------*/

extern float zoom_diff;

extern const float GRAVITY_UP[];
extern const float GRAVITY_DN[];
extern const float GRAVITY_BUSY[];

struct game_tilt
{
    float x[3], rx;
    float z[3], rz;

    float q[4];
};

void game_tilt_init(struct game_tilt *);
void game_tilt_calc(struct game_tilt *, float view_e[3][3]);
void game_tilt_grav(float h[3], const float g[3], const struct game_tilt *);

/*---------------------------------------------------------------------------*/

struct game_view
{
    float dc;                           /* Ideal view center distance above ball */
    float dp;                           /* Ideal view position distance above ball */
    float dz;                           /* Ideal view position distance behind ball */

    float c[3];                         /* Current view center               */
    float p[3];                         /* Current view position             */
    float e[3][3];                      /* Current view reference frame      */

    float a;                            /* Ideal view rotation about Y axis  */
};

void game_view_set_static_cam_view(int, float pos[3]);
void game_view_init(struct game_view *);
void game_view_zoom(struct game_view *, float);
void game_view_fly(struct game_view *, const struct s_vary *, int, float);
void game_view_set_pos_and_target(struct game_view *,
                                  const struct s_vary *,
                                  float pos[3], float center[3]);

/*---------------------------------------------------------------------------*/

/*
 * Either 90 Hz or 144 Hz.
 */
#define UPS 90
#define DT  (1.0f / (float) UPS)

/*
 * Simple fixed time step scheme.
 */

struct lockstep
{
    void (*step)(float);

    float dt;                           /* Time step length                  */
    float at;                           /* Accumulator                       */
    float ts;                           /* Time scale factor                 */
};

void lockstep_clr(struct lockstep *);
void lockstep_run(struct lockstep *, float);
void lockstep_scl(struct lockstep *, float);

#define lockstep_blend(ls) ((ls)->at / (ls)->dt)

/*---------------------------------------------------------------------------*/

extern struct s_base game_base;

int  game_base_load(const char *);
void game_base_free(const char *);

/*---------------------------------------------------------------------------*/

enum
{
    SPEED_NONE = 0,

    SPEED_SLOWESTESTEST,
    SPEED_SLOWESTESTER,
    SPEED_SLOWESTEST,
    SPEED_SLOWESTER,
    SPEED_SLOWEST,
    SPEED_SLOWER,
    SPEED_SLOW,
    SPEED_NORMAL,
    SPEED_FAST,
    SPEED_FASTER,
    SPEED_FASTEST,
    SPEED_FASTESTER,
    SPEED_FASTESTEST,
    SPEED_FASTESTESTER,
    SPEED_FASTESTESTEST,

    SPEED_MAX
};

extern float SPEED_FACTORS[];

#define SPEED_UP(s) MIN((s) + 1, SPEED_MAX - 1)
#define SPEED_DN(s) MAX((s) - 1, SPEED_NONE)

/*---------------------------------------------------------------------------*/

#endif
