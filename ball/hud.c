/*
 * Copyright (C) 2022 Microsoft / Neverball authors
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

//#define ENABLE_AFFECT
//#define ENABLE_COMPASS

#include <assert.h>

#if _WIN32 && __GNUC__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
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

#include "game_common.h"
#include "game_client.h"

/*---------------------------------------------------------------------------*/

static int speedbar_hud_id;
static int Lhud_id;
static int Rhud_id;

static int FSLhud_id;

static int time_id;

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
#if ENABLE_COMPASS==1
static int camcompass_id;
#endif

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
    "", "128", "64", "32", "16", "8", "4", "2", "1", "2", "4", "8", "16", "32", "64", "128"
};

static float speedup_logo_timer;

static float cam_timer;
static float speed_timer_length;


/* Visibility */

int show_hud = 1;

/* Screen animations */

static float hud_animdt;
static float show_hud_alpha;
static float show_hud_total_alpha;

static float replay_speed_alpha;
static float replay_speed_alpha_inverse;
static float cam_alpha;

void toggle_hud_visibility(int active)
{
    if (curr_mode() == MODE_NONE) return; /* Cannot render in home room. */
    show_hud = active;
}

int hud_visibility(void)
{
    return show_hud;
}

static void hud_fps(void)
{
    gui_set_count(fps_id, video_perf());
}

void hud_init(void)
{
    int id;
    const char *str_cam;
    int v;

    if ((Rhud_id = gui_hstack(0)))
    {
        if ((id = gui_vstack(Rhud_id)))
        {
            text_coins_id = gui_label(id, _("Coins"), GUI_SML, gui_wht, gui_wht);
            text_goal_id = gui_label(id, _("Goal"),  GUI_SML, gui_wht, gui_wht);
        }
        if ((id = gui_vstack(Rhud_id)))
        {
            coin_id        = gui_count(id, 1000, GUI_SML);
            goal_id        = gui_count(id, 1000, GUI_SML);
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
            ball_id = gui_count(id, 1000, GUI_SML);
            scor_id = gui_count(id, 1000, GUI_SML);
        }
        if ((id = gui_vstack(Lhud_id)))
        {
            text_balls_id = gui_label(id, _("Balls"), GUI_SML, gui_wht, gui_wht);
            text_score_id = gui_label(id, _("Total"), GUI_SML, gui_wht, gui_wht);
        }
        gui_set_rect(Lhud_id, GUI_NE);
        gui_layout(Lhud_id, -1, -1);
    }

    if ((FSLhud_id = gui_hstack(0)))
    {
        if ((id = gui_vstack(FSLhud_id)))
            text_speedometer_id = gui_label(id, _("km/h"), GUI_SML, gui_wht, gui_wht);

        if ((id = gui_vstack(FSLhud_id)))
            speedometer_id = gui_count(id, 9999, GUI_SML);

        gui_set_rect(FSLhud_id, GUI_RGT);
        gui_layout(FSLhud_id, -1, 0);
    }

    /* Default is 59999 (24h = 360000 * 24) */
    if ((time_id = gui_clock(0, 8640000, GUI_MED)))
    {
        gui_set_clock(time_id, 0);
        gui_set_rect(time_id, GUI_TOP);
        gui_layout(time_id, 0, -1);
    }

    /* NEW: Boost Rush Mode */
    if ((speedbar_hud_id = gui_hstack(0)))
    {
        if ((id = gui_vstack(speedbar_hud_id)))
            speed_percent_id = gui_label(id, "--- %", GUI_SML, gui_yel, gui_yel);

        if ((id = gui_vstack(speedbar_hud_id)))
            text_speed_id = gui_label(id, _("Speed"), GUI_SML, gui_wht, gui_wht);

        gui_set_rect(speedbar_hud_id, GUI_BOT);
        gui_layout(speedbar_hud_id, 0, +1);
    }

    if ((speedup_logo_id = gui_label(0, _("Max acceleration!"), GUI_MED, gui_grn, gui_grn)))
    {
        gui_layout(speedup_logo_id, 0, 0);
    }

    /* Find the longest camera name. */

    for (str_cam = "", v = CAM_NONE + 1; v < CAM_MAX; v++)
        if (gui_measure(cam_to_str(v), GUI_SML).w > gui_measure(str_cam, GUI_SML).w)
            str_cam = cam_to_str(v);

    if ((cam_id = gui_label(0, str_cam, GUI_SML, gui_wht, gui_wht)))
    {
        gui_set_rect(cam_id, GUI_SW);
        gui_layout(cam_id, 1, 1);
    }

    //free(str_cam);

    /* This debug shows how it works */

    if ((fps_id = gui_count(0, 1000, GUI_SML)))
    {
        gui_set_rect(fps_id, GUI_SE);
        gui_layout(fps_id, -1, 1);
    }

#if ENABLE_COMPASS==1
    if ((camcompass_id = gui_label(0, "199 Deg (NONE)", GUI_SML, gui_yel, gui_red)))
    {
        gui_set_label(camcompass_id, "--- Deg");
        gui_set_rect(camcompass_id, GUI_BOT);
        gui_layout(camcompass_id, 0, 1);
    }
#endif

    if ((speed_id = gui_hstack(0)))
    {
        int i;

        for (i = SPEED_MAX - 1; i > SPEED_NONE; i--)
            speed_ids[i] = gui_label(speed_id, speed_labels[i], GUI_SML, 0, 0);

        gui_set_rect(speed_id, GUI_TOP);
        gui_layout(speed_id, 0, -1);
    }
}

void hud_free(void)
{
    int i;

    gui_delete(text_coins_id);
    gui_delete(text_goal_id);
    gui_delete(text_balls_id);
    gui_delete(text_score_id);
    gui_delete(text_speed_id);

    gui_delete(speedbar_hud_id);
    gui_delete(speedup_logo_id);

    gui_delete(Rhud_id);
    gui_delete(Lhud_id);
    gui_delete(FSLhud_id);
    gui_delete(time_id);
    gui_delete(cam_id);
    gui_delete(fps_id);

#if ENABLE_COMPASS==1
    gui_delete(camcompass_id);
#endif

    gui_delete(speed_id);

    for (i = SPEED_NONE + 1; i < SPEED_MAX; i++)
        gui_delete(speed_ids[i]);
}

static void hud_update_alpha(void)
{
    replay_speed_alpha_inverse = 1.0f - replay_speed_alpha;
    float standard_hud_alpha = (replay_speed_alpha_inverse * show_hud_alpha) * show_hud_total_alpha;
    float cam_hud_alpha = (cam_alpha * show_hud_alpha) * show_hud_total_alpha;
    float replay_hud_alpha = (replay_speed_alpha * show_hud_alpha) * show_hud_total_alpha;

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

    gui_set_alpha(fps_id, standard_hud_alpha, GUI_ANIMATION_N_CURVE | GUI_ANIMATION_W_CURVE);
    gui_set_alpha(speed_id, replay_hud_alpha, GUI_ANIMATION_S_CURVE);
    gui_set_alpha(speedbar_hud_id, standard_hud_alpha, GUI_ANIMATION_N_CURVE);
    gui_set_alpha(cam_id, cam_hud_alpha, GUI_ANIMATION_N_CURVE | GUI_ANIMATION_E_CURVE);
    gui_set_alpha(time_id, standard_hud_alpha, GUI_ANIMATION_S_CURVE);
    gui_set_alpha(FSLhud_id, standard_hud_alpha, GUI_ANIMATION_W_CURVE);
    gui_set_alpha(Lhud_id, standard_hud_alpha, GUI_ANIMATION_S_CURVE | GUI_ANIMATION_W_CURVE);
    gui_set_alpha(Rhud_id, standard_hud_alpha, GUI_ANIMATION_S_CURVE | GUI_ANIMATION_E_CURVE);
}

void hud_set_alpha(float alpha)
{
    show_hud_total_alpha = alpha;
    hud_update_alpha();
}

void hud_paint(void)
{
    if (curr_mode() == MODE_NONE) return; /* Cannot render in home room. */

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    if (((curr_mode() == MODE_CHALLENGE || curr_mode() == MODE_BOOST_RUSH || curr_mode() == MODE_HARDCORE) && (speed_timer_length < 0.0f || config_get_d(CONFIG_SCREEN_ANIMATIONS))))
        gui_paint(Lhud_id);
    else if ((curr_mode() == MODE_CHALLENGE || curr_mode() == MODE_BOOST_RUSH || curr_mode() == MODE_HARDCORE) && !config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_paint(Lhud_id);
#else
    if (((curr_mode() == MODE_CHALLENGE || curr_mode() == MODE_BOOST_RUSH) && (speed_timer_length < 0.0f || config_get_d(CONFIG_SCREEN_ANIMATIONS))))
        gui_paint(Lhud_id);
    else if ((curr_mode() == MODE_CHALLENGE || curr_mode() == MODE_BOOST_RUSH) && !config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_paint(Lhud_id);
#endif

    if (curr_mode() == MODE_BOOST_RUSH && speed_timer_length < 0.0f)
        gui_paint(speedbar_hud_id);

    if ((speed_timer_length < 0.0f || config_get_d(CONFIG_SCREEN_ANIMATIONS)))
    {
        if (curr_coins() > 0 || curr_goal() > 0)
            gui_paint(Rhud_id);

        gui_paint(FSLhud_id);
#if ENABLE_COMPASS==1
        gui_paint(camcompass_id);
#endif

        gui_paint(time_id);
    }
    else if (!config_get_d(CONFIG_SCREEN_ANIMATIONS))
    {
        if (curr_coins() > 0 || curr_goal() > 0)
            gui_paint(Rhud_id);

        gui_paint(FSLhud_id);
#if ENABLE_COMPASS==1
        gui_paint(camcompass_id);
#endif

        gui_paint(time_id);
    }
    else
        assert(0 && "Unknown methods!");

    if (config_get_d(CONFIG_FPS))
    {
        gui_paint(fps_id);
    }

    hud_cam_paint();
    hud_speed_paint();
}

void hud_update(int pulse, float animdt)
{
    if (curr_mode() == MODE_NONE) return;

    float speedpercent = curr_speed_percent();
    char speedattr[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(speedattr, MAXSTR, "%d %%", ((int) roundf(speedpercent)));
#else
    sprintf(speedattr, "%d %%", ((int) roundf(speedpercent)));
#endif

    int clock = curr_clock();
    int coins = curr_coins();
    int goal  = curr_goal();
    int balls = curr_balls();
    int score = curr_score();
    float ballspeed = curr_speedometer();

    int c_id;
    int last;

    if (!pulse)
    {
        /* reset the hud */
        gui_set_color(text_speedometer_id, gui_wht, gui_wht);
        gui_set_color(text_coins_id, gui_wht, gui_wht);
        gui_set_color(text_goal_id, gui_wht, gui_wht);
        gui_set_color(text_balls_id, gui_wht, gui_wht);
        gui_set_color(text_score_id, gui_wht, gui_wht);
        gui_set_color(text_speed_id, gui_wht, gui_wht);

        gui_pulse(ball_id, 0.f);
        gui_pulse(time_id, 0.f);
        gui_pulse(coin_id, 0.f);

        speed_timer_length = 0.0f;
    }

    /* screen animations */

    if (config_get_d(CONFIG_SCREEN_ANIMATIONS))
    {
        show_hud_alpha += show_hud ? animdt : -animdt; show_hud_alpha = CLAMP(0.0f, show_hud_alpha, 1.0f);
        cam_alpha += cam_timer > 0.25f ? animdt : -animdt; cam_alpha = CLAMP(0.0f, cam_alpha, 1.0f);
    }
    else
    {
        show_hud_alpha = 1.0f;
        cam_alpha = 1.0f;
    }

    replay_speed_alpha += speed_timer_length > 0.25f ? animdt : -animdt; replay_speed_alpha = CLAMP(0.0f, replay_speed_alpha, 1.0f);

    hud_set_alpha(show_hud_total_alpha);

    /* time and tick-tock */

    if (clock != (last = gui_value(time_id)))
    {
        gui_set_clock(time_id, clock);

        if (pulse)
        {
            if (last > clock && clock > 5)
            {
                if (clock <= 1000 && (last / 100) > (clock / 100))
                {
                    gui_set_color(text_speedometer_id, gui_red, gui_red);
                    gui_set_color(text_coins_id, gui_red, gui_red);
                    gui_set_color(text_goal_id, gui_red, gui_red);
                    gui_set_color(text_balls_id, gui_red, gui_red);
                    gui_set_color(text_score_id, gui_red, gui_red);
                    gui_set_color(text_speed_id, gui_red, gui_red);

                    audio_play(AUD_TICK, 1.f);
                    gui_pulse(time_id, 1.50);
                }
                else if (clock < 500 && (last / 50) >(clock / 50))
                {
                    gui_set_color(text_speedometer_id, gui_vio, gui_vio);
                    gui_set_color(text_coins_id, gui_vio, gui_vio);
                    gui_set_color(text_goal_id, gui_vio, gui_vio);
                    gui_set_color(text_balls_id, gui_vio, gui_vio);
                    gui_set_color(text_score_id, gui_vio, gui_vio);
                    gui_set_color(text_speed_id, gui_vio, gui_vio);

                    audio_play(AUD_TOCK, 1.f);
                    gui_pulse(time_id, 1.25);
                }
                else if ((last / 25) > (clock / 25))
                {
                    gui_set_color(text_speedometer_id, gui_wht, gui_wht);
                    gui_set_color(text_coins_id, gui_wht, gui_wht);
                    gui_set_color(text_goal_id, gui_wht, gui_wht);
                    gui_set_color(text_balls_id, gui_wht, gui_wht);
                    gui_set_color(text_score_id, gui_wht, gui_wht);
                    gui_set_color(text_speed_id, gui_wht, gui_wht);
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
    case MODE_CHALLENGE:
        if (gui_value(ball_id) != balls) gui_set_count(ball_id, balls);
        if (gui_value(scor_id) != livecoins) gui_set_count(scor_id, livecoins);

        c_id = coin_id;
        break;

    case MODE_BOOST_RUSH:
        gui_set_label(speed_percent_id, speedattr);
        gui_set_color(speed_percent_id, gui_yel, speedpercent >= 100.f ? gui_red : gui_yel);

        if (gui_value(ball_id) != balls) gui_set_count(ball_id, balls);
        if (gui_value(scor_id) != livecoins) gui_set_count(scor_id, livecoins);

        c_id = coin_id;
        break;

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    case MODE_HARDCORE:
        if (gui_value(ball_id) != balls) gui_set_count(ball_id, 0);
        if (gui_value(scor_id) != livecoins) gui_set_count(scor_id, livecoins);

        c_id = coin_id;
        break;
#endif

    default:
        c_id = coin_id;
        break;
    }

    /* New: Speedometer */
    ballspeed = ballspeed * UPS;

    if (config_get_d(CONFIG_UNITS_METRIC))
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
            if (curr_mode() == MODE_CHALLENGE || curr_mode() == MODE_BOOST_RUSH || curr_mode() == MODE_HARDCORE)
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

    int hundred_score = (int)round(score / 100);

#if ENABLE_AFFECT==1
    if (hundred_score != (last = (int)round(gui_value(scor_id)) / 100))
    {
        /* GOT MY AFFECTS!!! */

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        if (curr_mode() == MODE_CHALLENGE || curr_mode() == MODE_BOOST_RUSH || curr_mode() == MODE_HARDCORE)
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
                gui_set_color(affect_id, gui_yel, gui_red);
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

    if (config_get_d(CONFIG_FPS))
        hud_fps();
}

void hud_timer(float dt)
{
    hud_animdt = ((config_get_d(CONFIG_SMOOTH_FIX) ? MIN(.01666666f, dt) : dt) * 6.0f);

    hud_update(1, hud_animdt);

    gui_timer(speedbar_hud_id, dt);
    gui_timer(speedup_logo_id, dt);
    hud_speedup_timer(dt);

    gui_timer(Rhud_id, dt);
    gui_timer(Lhud_id, dt);
    gui_timer(time_id, dt);

    hud_cam_timer(dt);
    hud_speed_timer(dt);
}

void hud_update_camera_direction(float rot_direction)
{
    float hdg_area = 22.5f;
    float hdg_num = rot_direction - (hdg_area / 2);

    char camdirref[MAXSTR];
    char *hdg_name = "";

    if (0 >= hdg_num && hdg_num <= hdg_area * 1)
        hdg_name = "N";
    else if (hdg_area * 1 >= hdg_num && hdg_num <= hdg_area * 2)
        hdg_name = "NNW";
    else if (hdg_area * 2 >= hdg_num && hdg_num <= hdg_area * 3)
        hdg_name = "NW";
    else if (hdg_area * 3 >= hdg_num && hdg_num <= hdg_area * 4)
        hdg_name = "WNW";
    else if (hdg_area * 4 >= hdg_num && hdg_num <= hdg_area * 5)
        hdg_name = "W";
    else if (hdg_area * 5 >= hdg_num && hdg_num <= hdg_area * 6)
        hdg_name = "WSW";
    else if (hdg_area * 6 >= hdg_num && hdg_num <= hdg_area * 7)
        hdg_name = "SW";
    else if (hdg_area * 7 >= hdg_num && hdg_num <= hdg_area * 8)
        hdg_name = "SSW";
    else if (hdg_area * 8 >= hdg_num && hdg_num <= hdg_area * 9 || hdg_area * -8 >= hdg_num && hdg_num <= hdg_area * -9)
        hdg_name = "S";
    else if (hdg_area * -7 >= hdg_num && hdg_num <= hdg_area * -8)
        hdg_name = "SSE";
    else if (hdg_area * -6 >= hdg_num && hdg_num <= hdg_area * -7)
        hdg_name = "SE";
    else if (hdg_area * -5 >= hdg_num && hdg_num <= hdg_area * -6)
        hdg_name = "ESE";
    else if (hdg_area * -4 >= hdg_num && hdg_num <= hdg_area * -5)
        hdg_name = "E";
    else if (hdg_area * -3 >= hdg_num && hdg_num <= hdg_area * -4)
        hdg_name = "ENE";
    else if (hdg_area * -2 >= hdg_num && hdg_num <= hdg_area * -3)
        hdg_name = "NE";
    else if (hdg_area * -1 >= hdg_num && hdg_num <= hdg_area * -2)
        hdg_name = "NNE";
    else if (hdg_area * 0 >= hdg_num && hdg_num <= hdg_area * -1)
        hdg_name = "N";

    hdg_num += (hdg_area / 2);

    if (hdg_num < -180) hdg_num += 360;
    if (hdg_num > 180) hdg_num -= 360;

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(camdirref, dstSize, "%i Deg (%s)", (int) round(hdg_num), hdg_name);
#else
    sprintf(camdirref, "%i Deg (%s)", (int) round(hdg_num), hdg_name);
#endif
#if ENABLE_COMPASS==1
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
    gui_set_label(speedup_logo_id, get_speed_indicator() >= 100.f ? _("Max acceleration!") : _("Speed up!"));
    gui_pulse(speedup_logo_id, 1.2f);
    speedup_logo_timer = 1.0f;
}

void hud_speedup_timer(float dt)
{
    speedup_logo_timer -= dt;
    gui_timer(speedup_logo_id, dt);
}

void hud_speedup_paint(void)
{
    if (speedup_logo_timer > 0.0f)
        gui_paint(speedup_logo_id);
}

/*---------------------------------------------------------------------------*/

void hud_cam_pulse(int c)
{
    gui_set_label(cam_id, cam_to_str(c));
    gui_pulse(cam_id, 1.2f);
    cam_timer = 2.0f;
}

void hud_cam_timer(float dt)
{
    cam_timer -= dt;
    gui_timer(cam_id, dt);
}

void hud_cam_paint(void)
{
    if (cam_timer > 0.0f)
        gui_paint(cam_id);
}

/*---------------------------------------------------------------------------*/

void hud_speed_pulse(int speed)
{
    int i;

    for (i = SPEED_NONE + 1; i < SPEED_MAX; i++)
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

    speed_timer_length = 2.0f;
}

void hud_speed_timer(float dt)
{
    speed_timer_length -= dt;
    gui_timer(speed_id, dt);
}

void hud_speed_paint(void)
{
    if (speed_timer_length > 0.0f || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_paint(speed_id);
}

/*---------------------------------------------------------------------------*/
