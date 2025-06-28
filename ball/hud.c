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

/* Usable for campaign */
#define ENABLE_COMPASS 1

#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

#include <math.h>
#include <string.h>

#if NB_HAVE_PB_BOTH==1
#include "account.h"
#include "networking.h"
#include "boost_rush.h"
#endif

#include "glext.h"
#include "hud.h"
#include "gui.h"
#include "progress.h"
#include "config.h"
#include "video.h"
#include "audio.h"

#if ENABLE_DUALDISPLAY==1
#include "game_dualdisplay.h"
#endif
#include "game_common.h"
#include "game_client.h"

#include "st_pause.h"

/*---------------------------------------------------------------------------*/

static int speedbar_hud_id;
static int Lhud_id;
static int Rhud_id;

static int time_id;
static int Touch_id;

static int FSLhud_id;

static int speedometer_id;

static int speedup_logo_id;
static int speed_percent_id;

static int coin_id;
#if ENABLE_AFFECT==1
static int affect_id;
#endif
static int ball_id;
static int scor_id;
static int goal_id;
static int cam_id;
static int fps_id;
#if defined(LEVELGROUPS_INCLUDES_CAMPAIGN) && ENABLE_COMPASS==1
static int camcompass_id;
#endif
static int lvlname_id;

static int speed_id;
static int speed_ids[SPEED_MAX];

/* All of the HUD texts can be used for */
static int text_speedometer_id;
static int text_coins_id;
static int text_goal_id;
static int text_balls_id;
static int text_score_id;
static int text_speed_id;

static const char *speed_labels[SPEED_MAX] = {
    "", "128", "64", "32", "16", "8", "4", "2", "1",
    "2", "4", "8", "16", "32", "64", "128"
};

static float speedup_logo_timer;
static float cam_timer;
static float speed_timer_length;
static float touch_timer;

/* Visibility */

static int show_hud          = 1;
static int show_hud_expected = 0;

/* Screen animations */

static float show_hud_alpha;
static float show_hud_total_alpha;

static float replay_speed_alpha;
static float replay_speed_alpha_inverse;
static float cam_alpha;

void toggle_hud_visibility(int active)
{
    /* Cannot render in home room. */

    if (curr_mode() == MODE_NONE)
        return;

    show_hud = active;
}

int hud_visibility(void)
{
    return show_hud && show_hud_expected;
}

static void hud_fps(void)
{
    char  perf_attr[MAXSTR];
    float ms_latence = 1.f / video_perf();

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(perf_attr, MAXSTR,
#else
    sprintf(perf_attr,
#endif
            "FPS: %d (%.3f ms)", video_perf(), ms_latence);

    gui_set_label(fps_id, perf_attr);
}

void hud_init(void)
{
    int         id;
    const char *str_cam;
    int         v;

    if ((Rhud_id = gui_hstack(0)))
    {
        if ((id = gui_vstack(Rhud_id)))
        {
            text_coins_id = gui_label(id, _("Coins"),
                                          GUI_SML, GUI_COLOR_WHT);
            text_goal_id  = gui_label(id, _("Goal"),
                                          GUI_SML, GUI_COLOR_WHT);
        }
        if ((id = gui_vstack(Rhud_id)))
        {
            coin_id       = gui_count(id, 1000, GUI_SML);
            goal_id       = gui_count(id, 1000, GUI_SML);
        }

        gui_set_rect(Rhud_id, GUI_NW);
        gui_layout(Rhud_id, +1, -1);
    }

    if ((Lhud_id = gui_hstack(0)))
    {
#if ENABLE_AFFECT==1
        if ((id = gui_vstack(Lhud_id)))
        {
            affect_id = gui_count(id, 0, GUI_SML);
            gui_filler(id);
        }
#endif
        if ((id = gui_vstack(Lhud_id)))
        {
            ball_id       = gui_count(id, 1000, GUI_SML);
            scor_id       = gui_count(id, 1000, GUI_SML);
        }
        if ((id = gui_vstack(Lhud_id)))
        {
            text_balls_id = gui_label(id, _("Balls"),
                                          GUI_SML, GUI_COLOR_WHT);
            text_score_id = gui_label(id, _("Total"),
                                          GUI_SML, GUI_COLOR_WHT);
        }

        gui_set_rect(Lhud_id, GUI_NE);
        gui_layout(Lhud_id, -1, -1);
    }

    /* Let Mojang done these */
    if ((Touch_id = gui_vstack(0)))
    {
        gui_space(Touch_id);

        if ((id = gui_hstack(Touch_id)))
        {
            gui_space(id);

            gui_state(id, GUI_ROMAN_2, GUI_TCH, GUI_BACK, 0);

            gui_space(id);

            const int cambtn_id = gui_state(id, GUI_FISHEYE,
                                                GUI_TCH, GUI_CAMERA, 0);
            gui_set_font(cambtn_id, "ttf/DejaVuSans-Bold.ttf");
        }

        // HACK: hide by default.
        gui_set_slide(Touch_id, GUI_N, 0, 0, 0);

        gui_layout(Touch_id, +1, +1);
    }

    /* Default is 59999 (24h = 360000 * 24) */
    if ((time_id = gui_clock(0, 8640000, GUI_MED)))
    {
        gui_set_clock(time_id, 0);
        gui_set_rect(time_id, GUI_TOP);
        gui_layout(time_id, 0, -1);
    }

    if ((FSLhud_id = gui_hstack(0)))
    {
        if ((id = gui_vstack(FSLhud_id)))
            text_speedometer_id = gui_label(id, _("km/h"),
                GUI_SML, GUI_COLOR_WHT);

        if ((id = gui_vstack(FSLhud_id)))
            speedometer_id = gui_count(id, 9999, GUI_SML);

        gui_set_rect(FSLhud_id, GUI_RGT);
        gui_layout(FSLhud_id, -1, 0);
    }

    /* NEW: Boost Rush Mode */
    if ((speedbar_hud_id = gui_hstack(0)))
    {
        if ((id = gui_vstack(speedbar_hud_id)))
            speed_percent_id = gui_label(id, "--- %", GUI_SML, GUI_COLOR_YEL);

        if ((id = gui_vstack(speedbar_hud_id)))
            text_speed_id = gui_label(id, _("Speed"), GUI_SML, GUI_COLOR_WHT);

        gui_set_rect(speedbar_hud_id, GUI_BOT);
        gui_layout(speedbar_hud_id, 0, +1);
    }

    if ((speedup_logo_id = gui_label(0, _("Max acceleration!"),
                                        GUI_MED, GUI_COLOR_GRN)))
        gui_layout(speedup_logo_id, 0, 0);

    /* Find the longest camera name. */

    for (str_cam = "", v = CAM_NONE + 1; v < CAM_MAX; v++)
        if (gui_measure(cam_to_str(v), GUI_SML).w > gui_measure(str_cam, GUI_SML).w)
            str_cam = cam_to_str(v);

    if ((cam_id = gui_label(0, str_cam, GUI_SML, GUI_COLOR_WHT)))
    {
        gui_set_rect(cam_id, GUI_SE);
        gui_layout(cam_id, -1, 1);
    }

    /* This debug shows how it works */

    if ((fps_id = gui_label(0, "XXXXXXXXXXXXXXXXXXXXXXXXX", GUI_SML, GUI_COLOR_DEFAULT)))
    {
        gui_set_trunc(fps_id, TRUNC_TAIL);
        gui_set_rect(fps_id, GUI_BOT);
        gui_layout(fps_id, 0, 1);
    }

    if ((lvlname_id = gui_label(0, "XXXXXXXXXXXXXXXXXXXXXXXXX",
                                   GUI_SML, gui_yel, gui_wht)))
    {
        gui_set_rect(lvlname_id, GUI_BOT);
        gui_set_label(lvlname_id, "");
        gui_set_trunc(lvlname_id, TRUNC_TAIL);
        gui_layout(lvlname_id, 0, 1);
    }

#if defined(LEVELGROUPS_INCLUDES_CAMPAIGN) && ENABLE_COMPASS==1
    if ((camcompass_id = gui_label(0, "199 Deg (NONE)",
                                      GUI_SML, gui_wht, gui_cya)))
    {
        gui_set_label(camcompass_id, "--- Deg (-)");
        gui_set_rect(camcompass_id, GUI_BOT);
        gui_layout(camcompass_id, 0, 1);
    }
#endif

    if ((speed_id = gui_hstack(0)))
    {
        for (int i = SPEED_MAX - 1; i > SPEED_NONE; i--)
            speed_ids[i] = gui_label(speed_id, speed_labels[i], GUI_SML, 0, 0);

        gui_set_rect(speed_id, GUI_TOP);
        gui_layout(speed_id, 0, -1);
    }
}

void hud_free(void)
{
    gui_delete(text_coins_id);
    gui_delete(text_goal_id);
    gui_delete(text_balls_id);
    gui_delete(text_score_id);
    gui_delete(text_speed_id);

    gui_delete(speedbar_hud_id);
    gui_delete(speedup_logo_id);

    gui_delete(FSLhud_id);
    gui_delete(Rhud_id);
    gui_delete(Lhud_id);
    gui_delete(Touch_id);
    gui_delete(time_id);
    gui_delete(cam_id);
    gui_delete(fps_id);
    gui_delete(lvlname_id);

#if defined(LEVELGROUPS_INCLUDES_CAMPAIGN) && ENABLE_COMPASS==1
    gui_delete(camcompass_id);
#endif

    gui_delete(speed_id);

    for (int i = SPEED_NONE + 1; i < SPEED_MAX; i++)
        gui_delete(speed_ids[i]);
}

static void hud_update_alpha(void)
{
    replay_speed_alpha_inverse = 1.0f - replay_speed_alpha;
    float standard_hud_alpha   = (replay_speed_alpha_inverse * show_hud_alpha) *
                                 show_hud_total_alpha;
    float cam_hud_alpha        = (cam_alpha * show_hud_alpha) *
                                 show_hud_total_alpha;
    float replay_hud_alpha     = (replay_speed_alpha * show_hud_alpha) *
                                 show_hud_total_alpha;

    if (!config_get_d(CONFIG_SCREEN_ANIMATIONS))
    {
        if (speed_timer_length >= 0.1f)
        {
            standard_hud_alpha = 0;
            cam_hud_alpha = 0;
            replay_hud_alpha = 1;
        }
        else
        {
            standard_hud_alpha = 1;
            cam_hud_alpha = 1;
            replay_hud_alpha = 0;
        }
    }

    gui_set_alpha(FSLhud_id,       standard_hud_alpha,
                                   GUI_ANIMATION_W_CURVE);
    gui_set_alpha(fps_id,          standard_hud_alpha,
                                   GUI_ANIMATION_N_CURVE);
    gui_set_alpha(speed_id,        replay_hud_alpha,
                                   GUI_ANIMATION_S_CURVE);
    gui_set_alpha(lvlname_id,      standard_hud_alpha,
                                   GUI_ANIMATION_N_CURVE);
#if NB_HAVE_PB_BOTH==1 && defined(LEVELGROUPS_INCLUDES_CAMPAIGN)
    gui_set_alpha(camcompass_id,   standard_hud_alpha,
                                   GUI_ANIMATION_N_CURVE);
#endif
    gui_set_alpha(speedbar_hud_id, standard_hud_alpha,
                                   GUI_ANIMATION_N_CURVE);
    gui_set_alpha(cam_id,          cam_hud_alpha * standard_hud_alpha,
                                   GUI_ANIMATION_N_CURVE | GUI_ANIMATION_W_CURVE);
    gui_set_alpha(time_id,         standard_hud_alpha, GUI_ANIMATION_S_CURVE);
    gui_set_alpha(Touch_id,        standard_hud_alpha,
                                   GUI_ANIMATION_N_CURVE | GUI_ANIMATION_E_CURVE);
    gui_set_alpha(Lhud_id,         standard_hud_alpha,
                                   GUI_ANIMATION_S_CURVE | GUI_ANIMATION_W_CURVE);
    gui_set_alpha(Rhud_id,         standard_hud_alpha,
                                   GUI_ANIMATION_S_CURVE | GUI_ANIMATION_E_CURVE);
}

void hud_set_alpha(float alpha)
{
    show_hud_total_alpha = alpha;
    hud_update_alpha();
}

void hud_paint(void)
{
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    const int wgcl_newhud = EM_ASM_INT({
        return systemsettings_conf_hudstyle_value != undefined &&
               systemsettings_conf_hudstyle_value != null &&
               systemsettings_conf_hudstyle_value == 0
    });
#else
    const int wgcl_newhud = 0;
#endif

    if (wgcl_newhud)
    {
        if (config_get_d(CONFIG_FPS))
            gui_paint(fps_id);

        hud_cam_paint();
        return;
    }

    if (curr_mode() == MODE_NONE) return; /* Cannot render in home room. */

    if (speed_timer_length < 0.0f || config_get_d(CONFIG_SCREEN_ANIMATIONS))
    {
        gui_paint(FSLhud_id);

        if (curr_mode() == MODE_CHALLENGE ||
            curr_mode() == MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
         || curr_mode() == MODE_HARDCORE
#endif
            )
            gui_paint(Lhud_id);

        if (curr_coins() > 0 || curr_goal() > 0)
            gui_paint(Rhud_id);

        gui_paint(time_id);
    }

    if (config_get_d(CONFIG_FPS))
        gui_paint(fps_id);

    hud_cam_paint();
    hud_speed_paint();
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__)
    hud_touch_paint();
#endif
}

void hud_update(int pulse, float animdt)
{
    if (curr_mode() == MODE_NONE) return;

    /* Let WGCL system choose, how to use these speed units. */

#ifdef __EMSCRIPTEN__
    const int autoconf_units_imperial = EM_ASM_INT({
        var userLanguage = navigator.language || navigator.userLanguage;
        return userLanguage == "en-US";
    });
#else
    const int autoconf_units_imperial = 0;
#endif

    float speedpercent = curr_speed_percent();
    char  speedattr[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(speedattr, MAXSTR,
#else
    sprintf(speedattr,
#endif
            "%d %%", ROUND(speedpercent));

    int clock       = curr_clock();
    int coins       = curr_coins();
    int goal        = curr_goal();
    int balls       = curr_balls();
    int score       = curr_score();
    float ballspeed = curr_speedometer();

    int c_id;
    int last;

    if (!pulse)
    {
        /* reset the hud */
        gui_set_color(text_speedometer_id, GUI_COLOR_WHT);
        gui_set_color(text_coins_id,       GUI_COLOR_WHT);
        gui_set_color(text_goal_id,        GUI_COLOR_WHT);
        gui_set_color(text_balls_id,       GUI_COLOR_WHT);
        gui_set_color(text_score_id,       GUI_COLOR_WHT);
        gui_set_color(text_speed_id,       GUI_COLOR_WHT);
        gui_set_color(Touch_id,            GUI_COLOR_WHT);

        gui_pulse(ball_id, 0.0f);
        gui_pulse(time_id, 0.0f);
        gui_pulse(coin_id, 0.0f);

        speed_timer_length = 0.0f;
    }

    /* screen animations */

    if (config_get_d(CONFIG_SCREEN_ANIMATIONS))
    {
        show_hud_alpha += (show_hud && show_hud_expected ? animdt : -animdt) * 6;
        show_hud_alpha  = CLAMP(0.0f, show_hud_alpha, 1.0f);
        cam_alpha      += (cam_timer > 0.0f ? animdt : -animdt) * 6;
        cam_alpha       = CLAMP(0.0f, cam_alpha, 1.0f);
    }
    else
    {
        show_hud_alpha = 1.0f;
        cam_alpha      = 1.0f;
    }

    replay_speed_alpha += (speed_timer_length > 0.75f ? animdt : -animdt) * 6;
    replay_speed_alpha  = CLAMP(0.0f, replay_speed_alpha, 1.0f);

    hud_set_alpha(show_hud_total_alpha);

    /* time and tick-tock */

    if (clock != (last = gui_value(time_id)))
    {
        gui_set_clock(time_id, clock);

#if ENABLE_DUALDISPLAY==1
        game_dualdisplay_set_time(clock);
#endif

        if (pulse)
        {
            if (last > clock && clock > 5)
            {
                if (clock <= 1000 && (last / 100) > (clock / 100))
                {
                    gui_set_color(text_speedometer_id, GUI_COLOR_RED);
                    gui_set_color(text_coins_id,       GUI_COLOR_RED);
                    gui_set_color(text_goal_id,        GUI_COLOR_RED);
                    gui_set_color(text_balls_id,       GUI_COLOR_RED);
                    gui_set_color(text_score_id,       GUI_COLOR_RED);
                    gui_set_color(text_speed_id,       GUI_COLOR_RED);
                    gui_set_color(Touch_id,            GUI_COLOR_RED);

                    audio_play(AUD_TICK, 1.0f);
                    gui_pulse(time_id, 1.50);
                }
                else if (clock < 500 && (last / 50) >(clock / 50))
                {
                    gui_set_color(text_speedometer_id, GUI_COLOR_VIO);
                    gui_set_color(text_coins_id,       GUI_COLOR_VIO);
                    gui_set_color(text_goal_id,        GUI_COLOR_VIO);
                    gui_set_color(text_balls_id,       GUI_COLOR_VIO);
                    gui_set_color(text_score_id,       GUI_COLOR_VIO);
                    gui_set_color(text_speed_id,       GUI_COLOR_VIO);
                    gui_set_color(Touch_id,            GUI_COLOR_VIO);

                    audio_play(AUD_TOCK, 1.0f);
                    gui_pulse(time_id, 1.25);
                }
                else if ((last / 25) > (clock / 25))
                {
                    gui_set_color(text_speedometer_id, GUI_COLOR_WHT);
                    gui_set_color(text_coins_id,       GUI_COLOR_WHT);
                    gui_set_color(text_goal_id,        GUI_COLOR_WHT);
                    gui_set_color(text_balls_id,       GUI_COLOR_WHT);
                    gui_set_color(text_score_id,       GUI_COLOR_WHT);
                    gui_set_color(text_speed_id,       GUI_COLOR_WHT);
                    gui_set_color(Touch_id,            GUI_COLOR_WHT);
                }
            }
            else
            {
                if (clock > last + 2950)
                    gui_pulse(time_id, 2.00);
                else if (clock > last + 1450)
                    gui_pulse(time_id, 1.50);
                else if (clock > last + 450)
                    gui_pulse(time_id, 1.25);
            }
        }
    }

    /* balls and score + select coin widget */

    int livecoins = score + coins;

    switch (curr_mode())
    {
        case MODE_BOOST_RUSH:
            gui_set_label(speed_percent_id, speedattr);
            gui_set_color(speed_percent_id, gui_yel,
                                            speedpercent >= 100.0f ? gui_red : gui_yel);

        case MODE_CHALLENGE:
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        case MODE_HARDCORE:
#endif
            if (gui_value(ball_id) != balls)     gui_set_count(ball_id, 0);
            if (gui_value(scor_id) != livecoins) gui_set_count(scor_id, livecoins);

            c_id = coin_id;
            break;

        default:
            c_id = coin_id;
            break;
    }

    /* New: Speedometer */
    ballspeed = ballspeed * UPS;

    if (config_get_d(CONFIG_UNITS_METRIC) || !autoconf_units_imperial)
        ballspeed = ballspeed * 3600.0f / 1000.0f / 64.0f;  /* convert to km/h */
    else
        ballspeed = ballspeed * 3600.0f / 1609.34f / 64.0f;  /* convert to mph */

    if ((int) ballspeed != gui_value(speedometer_id))
        gui_set_count(speedometer_id, (int) ballspeed);

    /* coins and pulse */

    if (coins != (last = gui_value(c_id)))
    {
        last = coins - last;

        gui_set_count(c_id, coins);

        if (pulse && last > 0)
        {
            /* Standard gamemode */
            if      (last >= 10) gui_pulse(coin_id, 2.00f);
            else if (last >=  5) gui_pulse(coin_id, 1.50f);
            else                 gui_pulse(coin_id, 1.25f);

            if (goal > 0)
            {
                if      (last >= 10) gui_pulse(goal_id, 2.00f);
                else if (last >=  5) gui_pulse(goal_id, 1.50f);
                else                 gui_pulse(goal_id, 1.25f);
            }

            /* Broadband gamemode */
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            if (curr_mode() == MODE_CHALLENGE  ||
                curr_mode() == MODE_BOOST_RUSH ||
                curr_mode() == MODE_HARDCORE)
#else
            if (curr_mode() == MODE_CHALLENGE || curr_mode() == MODE_BOOST_RUSH)
#endif
            {
                if      (last >= 10) gui_pulse(scor_id, 2.00f);
                else if (last >=  5) gui_pulse(scor_id, 1.50f);
                else                 gui_pulse(scor_id, 1.25f);
            }
        }

        gui_set_count(scor_id, livecoins);
    }

    int hundred_score = ROUND(score / 100);

#if ENABLE_AFFECT==1
    if (hundred_score != (last = ROUND(gui_value(scor_id)) / 100))
    {
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        if (curr_mode() == MODE_CHALLENGE  ||
            curr_mode() == MODE_BOOST_RUSH ||
            curr_mode() == MODE_HARDCORE)
#else
        if (curr_mode() == MODE_CHALLENGE || curr_mode() == MODE_BOOST_RUSH)
#endif
        {
            if (hundred_score + last)
            {
                gui_set_count(affect_id, last - hundred_score);
                gui_set_color(affect_id, gui_wht, gui_grn);
            }
            else
            {
                gui_set_count(affect_id, 0);
                gui_set_color(affect_id, GUI_COLOR_DEFAULT);
            }
        }
    }
#endif

    /* goal and pulse */

    if (goal != (last = gui_value(goal_id)))
    {
        gui_set_count(goal_id, goal);

        if (pulse && goal == 0 && last > 0)
            gui_pulse(goal_id, 2.00f);
    }

#ifdef __EMSCRIPTEN__
    if (config_get_d(CONFIG_UNITS_METRIC) || !autoconf_units_imperial)
        EM_ASM({
            Neverball.WGCLupdateGameHUD($0, $1, $2, $3, $4, $5);
        }, clock, coins, goal, balls, score, ROUND(ballspeed));
    else
        EM_ASM({
            Neverball.WGCLupdateGameHUD_imperial($0, $1, $2, $3, $4, $5);
        }, clock, coins, goal, balls, score, ROUND(ballspeed));
#endif

    if (config_get_d(CONFIG_FPS))
        hud_fps();
}

void hud_timer(float dt)
{
    hud_update(1, dt);

    gui_timer(speedbar_hud_id, dt);
    gui_timer(speedup_logo_id, dt);

    gui_timer(FSLhud_id, dt);
    gui_timer(Rhud_id, dt);
    gui_timer(Lhud_id, dt);
    gui_timer(Touch_id, dt);
    gui_timer(time_id, dt);

    gui_timer(camcompass_id, dt);
    gui_timer(lvlname_id, dt);
    gui_timer(fps_id, dt);
    gui_timer(speed_percent_id, dt);

    hud_speedup_timer(dt);
    hud_cam_timer(dt);
    hud_speed_timer(dt);
    hud_touch_timer(dt);
}

void hud_show(float delay)
{
    if (show_hud_expected == 1) return;

    show_hud_expected = 1;

    gui_slide(FSLhud_id,        GUI_W  | GUI_EASE_BACK, delay + 0.0f, 0.3f, 0);
    gui_slide(Lhud_id,          GUI_SW | GUI_EASE_BACK, delay + 0.0f, 0.3f, 0);
    gui_slide(time_id,          GUI_S  | GUI_EASE_BACK, delay + 0.1f, 0.3f, 0);
    gui_slide(camcompass_id,    GUI_N  | GUI_EASE_BACK, delay + 0.1f, 0.3f, 0);
    gui_slide(lvlname_id,       GUI_N  | GUI_EASE_BACK, delay + 0.1f, 0.3f, 0);
    gui_slide(fps_id,           GUI_N  | GUI_EASE_BACK, delay + 0.1f, 0.3f, 0);
    gui_slide(speed_percent_id, GUI_N  | GUI_EASE_BACK, delay + 0.1f, 0.3f, 0);
    gui_slide(Rhud_id,          GUI_SE | GUI_EASE_BACK, delay + 0.2f, 0.3f, 0);
}

void hud_hide(void)
{
    if (show_hud_expected == 0) return;

    show_hud_expected = 0;

    gui_slide(FSLhud_id,        GUI_W  | GUI_EASE_BACK | GUI_BACKWARD, 0, 0.3f, 0);
    gui_slide(Lhud_id,          GUI_SW | GUI_EASE_BACK | GUI_BACKWARD, 0, 0.3f, 0);
    gui_slide(time_id,          GUI_S  | GUI_EASE_BACK | GUI_BACKWARD, 0, 0.3f, 0);
    gui_slide(camcompass_id,    GUI_N  | GUI_EASE_BACK | GUI_BACKWARD, 0, 0.3f, 0);
    gui_slide(lvlname_id,       GUI_N  | GUI_EASE_BACK | GUI_BACKWARD, 0, 0.3f, 0);
    gui_slide(fps_id,           GUI_N  | GUI_EASE_BACK | GUI_BACKWARD, 0, 0.3f, 0);
    gui_slide(speed_percent_id, GUI_N  | GUI_EASE_BACK | GUI_BACKWARD, 0, 0.3f, 0);
    gui_slide(Rhud_id,          GUI_SE | GUI_EASE_BACK | GUI_BACKWARD, 0, 0.3f, 0);

    if (touch_timer < 0.0f)
    {
        touch_timer = 0.0f;
        gui_slide(Touch_id, GUI_N | GUI_EASE_BACK | GUI_BACKWARD, 0, 0.3f, 0);
    }

    gui_slide(speed_id, GUI_S | GUI_EASE_BACK | GUI_BACKWARD, 0, 0.3f, 0);
}

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__)
int hud_touch(const SDL_TouchFingerEvent *e)
{
    if (touch_timer == 0.0f)
        gui_slide(Touch_id, GUI_N | GUI_EASE_BACK, 0, 0.3f, 0);

    touch_timer = 5.0f;

    if (e->type == SDL_FINGERUP)
    {
        const int x = (int) ((float) video.device_w * e->x);
        const int y = (int) ((float) video.device_h * (1.0f - e->y));

        return gui_point(Touch_id, x, y);
    }

    return 0;
}
#endif

void hud_update_camera_direction(float rot_direction)
{
    float hdg_area = 22.5f;
    float hdg_num = (hdg_area / 2) - (rot_direction + (hdg_area / 2));
    hdg_num -= (hdg_area / 4);

    if (hdg_num < 0)    do hdg_num += 360; while (hdg_num < 0);
    if (hdg_num >= 360) do hdg_num -= 360; while (hdg_num >= 360);

    char *hdg_name = "N";

    if      (hdg_area * 1.5f  >= hdg_num && hdg_num <= hdg_area * 2.5f)
        hdg_name = "NNE";
    else if (hdg_area * 2.5f  >= hdg_num && hdg_num <= hdg_area * 3.5f)
        hdg_name = "NE";
    else if (hdg_area * 3.5f  >= hdg_num && hdg_num <= hdg_area * 4.5f)
        hdg_name = "ENE";
    else if (hdg_area * 4.5f  >= hdg_num && hdg_num <= hdg_area * 5.5f)
        hdg_name = "E";
    else if (hdg_area * 5.5f  >= hdg_num && hdg_num <= hdg_area * 6.5f)
        hdg_name = "ESE";
    else if (hdg_area * 6.5f  >= hdg_num && hdg_num <= hdg_area * 7.5f)
        hdg_name = "SE";
    else if (hdg_area * 7.5f  >= hdg_num && hdg_num <= hdg_area * 8.5f)
        hdg_name = "SSE";
    else if (hdg_area * 8.5f  >= hdg_num && hdg_num <= hdg_area * 9.5f)
        hdg_name = "S";
    else if (hdg_area * 9.5f  >= hdg_num && hdg_num <= hdg_area * 10.5f)
        hdg_name = "SSW";
    else if (hdg_area * 10.5f >= hdg_num && hdg_num <= hdg_area * 11.5f)
        hdg_name = "SW";
    else if (hdg_area * 11.5f >= hdg_num && hdg_num <= hdg_area * 12.5f)
        hdg_name = "WSW";
    else if (hdg_area * 12.5f >= hdg_num && hdg_num <= hdg_area * 13.5f)
        hdg_name = "W";
    else if (hdg_area * 13.5f >= hdg_num && hdg_num <= hdg_area * 14.5f)
        hdg_name = "WNW";
    else if (hdg_area * 14.5f >= hdg_num && hdg_num <= hdg_area * 15.5f)
        hdg_name = "NW";
    else if (hdg_area * 15.5f >= hdg_num && hdg_num <= hdg_area * 16.5f)
        hdg_name = "NNW";

    hdg_num += (hdg_area / 4);
    if (hdg_num < 0)    do hdg_num += 360; while (hdg_num < 0);
    if (hdg_num >= 360) do hdg_num -= 360; while (hdg_num >= 360);

    char camdirref[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(camdirref, MAXSTR,
#else
    sprintf(camdirref,
#endif
            "%i Deg (%s)", ROUND(hdg_num), hdg_name);

#if defined(LEVELGROUPS_INCLUDES_CAMPAIGN) && ENABLE_COMPASS==1
    gui_set_label(camcompass_id, camdirref);
#endif
}

/*---------------------------------------------------------------------------*/

void hud_speedup_reset(void)
{
    speedup_logo_timer = 0.0f;
}

void hud_speedup_pulse(void)
{
    const char *speedup_text = get_speed_indicator() < 100.0f ? N_("Speed up!") :
                                                                N_("Max acceleration!");

    gui_set_label(speedup_logo_id, _(speedup_text));
    gui_pulse(speedup_logo_id, 1.2f);
    speedup_logo_timer = 1.0f;

    gui_slide(speedup_logo_id, GUI_W | GUI_EASE_BACK, 0, 0.5f, 0);
}

void hud_speedup_timer(float dt)
{
    if (speedup_logo_timer > 0.0f)
    {
        speedup_logo_timer -= dt;

        if (speedup_logo_timer <= 0.0f)
        {
            speedup_logo_timer = 0.0f;
            gui_slide(speedup_logo_id, GUI_E | GUI_EASE_BACK | GUI_BACKWARD, 0, 0.5f, 0);
        }
    }

    gui_timer(speedup_logo_id, dt);
}

void hud_speedup_paint(void)
{
    if (speedup_logo_timer < 0.0f || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_paint(speedup_logo_id);
}

/*---------------------------------------------------------------------------*/

void hud_lvlname_campaign(const char *name, int b)
{
    gui_set_label(lvlname_id, name);
    gui_set_color(lvlname_id, b ? gui_grn : gui_cya, b ? gui_wht : gui_cya);
}

void hud_lvlname_set(const char *name, int b)
{
    gui_set_label(lvlname_id, name);
    gui_set_color(lvlname_id, b ? gui_grn : gui_yel, gui_wht);
}

void hud_lvlname_set_ana(const char *name, int b)
{
    gui_set_label(lvlname_id, name);
    gui_set_color(lvlname_id, b ? gui_grn : gui_cya, b ? gui_wht : gui_blu);
}

void hud_lvlname_paint(void)
{
    if (config_get_d(CONFIG_FPS)) return;

    if (speed_timer_length < 0.0f || config_get_d(CONFIG_SCREEN_ANIMATIONS))
    {
#if NB_HAVE_PB_BOTH==1
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        if (campaign_used())
        {
#if ENABLE_COMPASS==1
            gui_paint(camcompass_id);
#endif
        }
#endif
        else if (curr_mode() == MODE_BOOST_RUSH)
            gui_paint(speedbar_hud_id);
        else
#endif
        if (curr_mode() != MODE_STANDALONE)
            gui_paint(lvlname_id);
    }
}

/*---------------------------------------------------------------------------*/

void hud_cam_pulse(int c)
{
    gui_set_label(cam_id, cam_to_str(c));
    gui_pulse(cam_id, 1.2f);

    if (cam_timer <= 0.0f)
        gui_slide(cam_id, GUI_NW | GUI_EASE_BACK, 0, 0.2f, 0);

    cam_timer = 1.0f;
}

void hud_cam_timer(float dt)
{
    gui_timer(cam_id, dt);

    if (cam_timer > 0.0f)
    {
        cam_timer -= dt;

        if (cam_timer <= 0.0f)
        {
            cam_timer = 0.0f;
            gui_slide(cam_id, GUI_NW | GUI_EASE_BACK | GUI_BACKWARD, 0, 0.2f, 0);
        }
    }
}

void hud_cam_paint(void)
{
#if NB_HAVE_PB_BOTH!=1 || !defined(__EMSCRIPTEN__)
    if (cam_timer < 2.0f || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_paint(cam_id);
#endif
}

/*---------------------------------------------------------------------------*/

void hud_speed_pulse(int speed)
{
    for (int i = SPEED_NONE + 1; i < SPEED_MAX; i++)
    {
        const GLubyte *c = gui_gry;

        if (speed != SPEED_NONE)
        {
            if      (i > SPEED_NORMAL && i <= speed)
                c = gui_grn;
            else if (i < SPEED_NORMAL && i >= speed)
                c = gui_red;
            else if (i == SPEED_NORMAL)
                c = gui_wht;
        }

        gui_set_color(speed_ids[i], c, c);

        if (i == speed)
            gui_pulse(speed_ids[i], 1.2f);
    }

    if (speed_timer_length <= 0.0f)
        gui_slide(speed_id, GUI_S | GUI_EASE_BACK, 0, 0.3f, 0);

    speed_timer_length = 2.0f;
}

void hud_speed_timer(float dt)
{
    if (speed_timer_length > 0.0f)
    {
        speed_timer_length -= dt;

        if (speed_timer_length <= 0.0f)
        {
            speed_timer_length = 0.0f;
            gui_slide(speed_id, GUI_S | GUI_EASE_BACK | GUI_BACKWARD, 0, 0.3f, 0);
        }
    }

    gui_timer(speed_id, dt);
}

void hud_speed_paint(void)
{
#if NB_HAVE_PB_BOTH!=1 || !defined(__EMSCRIPTEN__)
    if (speed_timer_length > 0.0f || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_paint(speed_id);
#endif
}

/*---------------------------------------------------------------------------*/

void hud_touch_timer(float dt)
{
    if (touch_timer > 0.0f)
    {
        touch_timer -= dt;

        if (touch_timer <= 0.0f)
        {
            touch_timer = 0.0f;
            gui_slide(Touch_id, GUI_N | GUI_EASE_BACK | GUI_BACKWARD, 0, 0.3f, 0);
        }
    }

    gui_timer(Touch_id, dt);
}

void hud_touch_paint(void)
{
#if NB_HAVE_PB_BOTH!=1 || !defined(__EMSCRIPTEN__)
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__)
    gui_paint(Touch_id);
#endif
#endif
}

/*---------------------------------------------------------------------------*/
