/*
 * Copyright (C) 2026 Microsoft / Neverball authors / Jānis Rūcis
 *
 * NEVERPUTT is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */

#include <math.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

/*
 * HACK: Used with console version
 */
#include "console_control_gui.h"

#include "account.h"
#include "networking.h"

#include "joy.h"
#include "hud.h"
#include "geom.h"
#include "gui.h"
#include "transition.h"
#include "vec3.h"
#include "game.h"
#include "hole.h"
#include "audio.h"
#include "course.h"
#include "config.h"
#include "video.h"
#ifndef VERSION
#include "version.h"
#endif
#include "lang.h"
#include "key.h"
#include "package.h"

#include "st_all.h"
#include "st_conf.h"
#include "st_wgcl.h"

extern struct state st_ball_title;

/*
 * This variable name will be redirected to st_ball_title for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `st_ball_title` to `st_title`.
 */
#define st_title st_ball_title

#define MODERN_PUTT_HUD

extern struct state st_title;

/*---------------------------------------------------------------------------*/

struct state st_putt_help;
struct state st_putt_course;
//struct state st_putt_party;
struct state st_putt_controltype;
struct state st_putt_poser;
struct state st_putt_flyby;
struct state st_putt_stroke;
struct state st_putt_roll;
struct state st_putt_goal;
struct state st_putt_stop;
struct state st_putt_fall;
struct state st_putt_retry;
struct state st_putt_score;
//struct state st_putt_over;
struct state st_putt_pause;

/*---------------------------------------------------------------------------*/

static int course_ranks;

static int hole_hilited;
static int stroke_type;
static int play_id;
static int support_exit;

static char *number(int i)
{
    static char str[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(str, MAXSTR,
#else
    sprintf(str,
#endif
            "%02d", i);

    return str;
}

static int score_card(const char  *title,
                      const GLubyte *c0,
                      const GLubyte *c1)
{
    int id, jd, kd, ld;

    int p1 = (curr_party() >= 1) ? 1 : 0;
    int p2 = (curr_party() >= 2) ? 1 : 0;
    int p3 = (curr_party() >= 3) ? 1 : 0;
    int p4 = (curr_party() >= 4) ? 1 : 0;

    int i;
    int n = curr_count() - 1;
    int m = curr_count() / 2;

    int lid = 0,
        ljd = 0,
        lkd = 0,
        lld = 0,
        lmd = 0,
        lnd = 0;

#define SCORECARD_RESET_LIVE_HILITE \
    lid = 0; ljd = 0; lkd = 0; lld = 0; lmd = 0; lnd = 0 \

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, title, GUI_MED, c0, c1);
        gui_space(id);

        if ((jd = gui_hstack(id)))
        {
            if ((kd = gui_varray(jd)))
            {
                if (p1) gui_label(kd, _("O"),      GUI_SML, 0, 0);
                if (p1) gui_label(kd, hole_out(0), GUI_SML, GUI_COLOR_WHT);
                if (p1) gui_label(kd, hole_out(1), GUI_SML, gui_red, gui_wht);
                if (p2) gui_label(kd, hole_out(2), GUI_SML, gui_grn, gui_wht);
                if (p3) gui_label(kd, hole_out(3), GUI_SML, gui_blu, gui_wht);
                if (p4) gui_label(kd, hole_out(4), GUI_SML, gui_yel, gui_wht);

                gui_set_rect(kd, GUI_RGT);
            }

            if ((kd = gui_harray(jd)))
            {
                for (i = m; i > 0; i--)
                    if ((ld = gui_varray(kd)))
                    {
                        SCORECARD_RESET_LIVE_HILITE;

                        if (p1) lid = gui_label(ld, number    (i),    GUI_SML, 0, 0);
                        if (p1) ljd = gui_label(ld, hole_score(i, 0), GUI_SML, GUI_COLOR_WHT);
                        if (p1) lkd = gui_label(ld, hole_score(i, 1), GUI_SML, gui_red, gui_wht);
                        if (p2) lld = gui_label(ld, hole_score(i, 2), GUI_SML, gui_grn, gui_wht);
                        if (p3) lmd = gui_label(ld, hole_score(i, 3), GUI_SML, gui_blu, gui_wht);
                        if (p4) lnd = gui_label(ld, hole_score(i, 4), GUI_SML, gui_yel, gui_wht);

                        if (curr_hole() == atoi(number(i)) && hole_hilited)
                        {
                            if (lid) gui_set_hilite(lid, 1);
                            if (ljd) gui_set_hilite(ljd, 1);
                            if (lkd) gui_set_hilite(lkd, 1);
                            if (lld) gui_set_hilite(lld, 1);
                            if (lmd) gui_set_hilite(lmd, 1);
                            if (lnd) gui_set_hilite(lnd, 1);
                        }
                    }

                gui_set_rect(kd, GUI_LFT);
            }

            if ((kd = gui_vstack(jd)))
            {
                gui_space(kd);

                if ((ld = gui_varray(kd)))
                {
                    if (p1) gui_label(ld, _("Par"), GUI_SML, GUI_COLOR_WHT);
                    if (p1) gui_label(ld, _("P1"),  GUI_SML, gui_red, gui_wht);
                    if (p2) gui_label(ld, _("P2"),  GUI_SML, gui_grn, gui_wht);
                    if (p3) gui_label(ld, _("P3"),  GUI_SML, gui_blu, gui_wht);
                    if (p4) gui_label(ld, _("P4"),  GUI_SML, gui_yel, gui_wht);

                    gui_set_rect(ld, GUI_ALL);
                }
            }
        }

        gui_space(id);

        if ((jd = gui_hstack(id)))
        {
            if ((kd = gui_varray(jd)))
            {
                if (p1) gui_label(kd, _("Tot"),    GUI_SML, 0, 0);
                if (p1) gui_label(kd, hole_tot(0), GUI_SML, GUI_COLOR_WHT);
                if (p1) gui_label(kd, hole_tot(1), GUI_SML, gui_red, gui_wht);
                if (p2) gui_label(kd, hole_tot(2), GUI_SML, gui_grn, gui_wht);
                if (p3) gui_label(kd, hole_tot(3), GUI_SML, gui_blu, gui_wht);
                if (p4) gui_label(kd, hole_tot(4), GUI_SML, gui_yel, gui_wht);

                gui_set_rect(kd, GUI_ALL);
            }

            if ((kd = gui_varray(jd)))
            {
                if (p1) gui_label(kd, _("I"),     GUI_SML, 0, 0);
                if (p1) gui_label(kd, hole_in(0), GUI_SML, GUI_COLOR_WHT);
                if (p1) gui_label(kd, hole_in(1), GUI_SML, gui_red, gui_wht);
                if (p2) gui_label(kd, hole_in(2), GUI_SML, gui_grn, gui_wht);
                if (p3) gui_label(kd, hole_in(3), GUI_SML, gui_blu, gui_wht);
                if (p4) gui_label(kd, hole_in(4), GUI_SML, gui_yel, gui_wht);

                gui_set_rect(kd, GUI_RGT);
            }

            if ((kd = gui_harray(jd)))
            {
                for (i = n; i > m; i--)
                    if ((ld = gui_varray(kd)))
                    {
                        SCORECARD_RESET_LIVE_HILITE;

                        if (p1) lid = gui_label(ld, number    (i),    GUI_SML, 0, 0);
                        if (p1) ljd = gui_label(ld, hole_score(i, 0), GUI_SML, GUI_COLOR_WHT);
                        if (p1) lkd = gui_label(ld, hole_score(i, 1), GUI_SML, gui_red, gui_wht);
                        if (p2) lld = gui_label(ld, hole_score(i, 2), GUI_SML, gui_grn, gui_wht);
                        if (p3) lmd = gui_label(ld, hole_score(i, 3), GUI_SML, gui_blu, gui_wht);
                        if (p4) lnd = gui_label(ld, hole_score(i, 4), GUI_SML, gui_yel, gui_wht);

                        if (curr_hole() == atoi(number(i)) && hole_hilited)
                        {
                            if (lid) gui_set_hilite(lid, 1);
                            if (ljd) gui_set_hilite(ljd, 1);
                            if (lkd) gui_set_hilite(lkd, 1);
                            if (lld) gui_set_hilite(lld, 1);
                            if (lmd) gui_set_hilite(lmd, 1);
                            if (lnd) gui_set_hilite(lnd, 1);
                        }
                    }

                gui_set_rect(kd, GUI_LFT);
            }

            if ((kd = gui_vstack(jd)))
            {
                gui_space(kd);

                if ((ld = gui_varray(kd)))
                {
                    if (p1) gui_label(ld, _("Par"), GUI_SML, GUI_COLOR_WHT);
                    if (p1) gui_label(ld, _("P1"),  GUI_SML, gui_red, gui_wht);
                    if (p2) gui_label(ld, _("P2"),  GUI_SML, gui_grn, gui_wht);
                    if (p3) gui_label(ld, _("P3"),  GUI_SML, gui_blu, gui_wht);
                    if (p4) gui_label(ld, _("P4"),  GUI_SML, gui_yel, gui_wht);

                    gui_set_rect(ld, GUI_ALL);
                }
            }
        }

        gui_layout(id, 0, 0);
    }
#undef SCORECARD_RESET_LIVE_HILITE

    return id;
}

/*---------------------------------------------------------------------------*/

static int shared_leave(struct state *st, struct state *next, int id, int intent)
{
    if (next == &st_null)
        course_free();

    return transition_slide(id, 0, intent);
}

static void shared_timer(int id, float dt)
{
    gui_timer(id, dt);
}

static void shared_point(int id, int x, int y, int dx, int dy)
{
    gui_pulse(gui_point(id, x, y), 1.2f);
}

static int shared_click_basic(int b, int d)
{
    /* Activate on left click. */

    return (b == SDL_BUTTON_LEFT && d) ?
           st_buttn(config_get_d(CONFIG_JOYSTICK_BUTTON_A), 1) : 1;
}

static int shared_stick_basic(int id, int a, float v, int bump)
{
    int jd;

    if ((jd = gui_stick(id, a, v, bump)))
        gui_pulse(jd, 1.2f);

    return jd;
}

static int paused_indiv_ctrl_index = -1;

static void shared_stick(int id, int a, float v, int bump)
{
    if (paused_indiv_ctrl_index != -1 || joy_get_active_cursor(0))
        shared_stick_basic(id, a, v, bump);
}

static void shared_fade(float alpha)
{
    console_gui_set_alpha(alpha);
}

static int shared_back_button(int pd)
{
    int id;

    if ((id = gui_hstack(pd)))
    {
        const int icn_id = gui_label(id, GUI_CROSS, GUI_SML, GUI_COLOR_RED);
        gui_label(id, _("Back"), GUI_SML, GUI_COLOR_WHT);
        gui_set_font(icn_id, "ttf/DejaVuSans-Bold.ttf");

        gui_set_state(id, -1, 0);
        gui_set_rect(id, GUI_ALL);
    }

    return id;
}

/*---------------------------------------------------------------------------*/

<<<<<<< HEAD
enum
=======
#define TITLE_PLAY 1
#define TITLE_CONF 2
#define TITLE_EXIT 3
#define TITLE_BALL 4

static int title_action(int i)
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
{
    TITLE_PLAY = 1,
    TITLE_HELP,
    TITLE_CONF,
    TITLE_EXIT,
    TITLE_BALL
};

static int putt_title_action(int i)
{
    PUTT_GAMEMENU_ACTION(-1);

    switch (i)
    {
<<<<<<< HEAD
        case TITLE_PLAY: return goto_state(&st_putt_course);
        case TITLE_HELP: return goto_state(&st_putt_help);
        case TITLE_CONF:
#if NB_HAVE_PB_BOTH == 1 && defined(CONFIG_WGCL_GAME_OPTIONS)
            WGCL_InitGameOptions(config_ball() ? &st_ball_title : (config_putt() ? &st_putt_title : 0));
            return goto_state(&st_null);
#else
            return goto_conf(config_ball() ? &st_ball_title : (config_putt() ? &st_putt_title : 0), 0, 0);
#endif
        case TITLE_EXIT:
            if (current_platform != PLATFORM_SWITCH)
            {
                /* bye! */
                course_free();
                return 0;
            }
        case TITLE_BALL: 
            config_set_d(CONFIG_GAME, GAME_BALL);
            return exit_state(&st_ball_title);
=======
    case TITLE_PLAY: return goto_state(&st_putt_course);
    case TITLE_CONF: return goto_state(&st_conf);
    case TITLE_EXIT: return 0;
    case TITLE_BALL:
        config_set_d(CONFIG_GAME, GAME_BALL);
        return goto_state(&st_title);
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
    }
    return 1;
}

/*
 * This function name will be redirected to putt_title_action() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `putt_title_action()` to `title_action()`.
 */
#define title_action putt_title_action

static int party_indiv_controllers = 0;

static int gamepadinfo_controller_ids[4];

static int putt_title_enter(struct state *st, struct state *prev, int intent)
{
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    support_exit = (current_platform != PLATFORM_PS &&
                    current_platform != PLATFORM_STEAMDECK &&
                    current_platform != PLATFORM_SWITCH) &&
                   (current_platform == PLATFORM_PC);
#endif

    if (party_indiv_controllers)
    {
        joy_active_cursor(0, 1);
        joy_active_cursor(1, 0);
        joy_active_cursor(2, 0);
        joy_active_cursor(3, 0);
    }

    int root_id, id, jd, kd;

    /* Build the title GUI. */

    if ((root_id = gui_root()))
    {
        if ((id = gui_vstack(root_id)))
        {
            if ((jd = gui_vstack(id)))
            {
                const int title_id = gui_title_header(jd, "  Neverputt  ", GUI_LRG, 0, 0);
                gui_set_font(title_id, "ttf/DejaVuSans-Bold.ttf");
#if NB_STEAM_API==1
                gui_label(jd, _("Steam Valve Edition"), GUI_SML, GUI_COLOR_WHT);
#endif
                gui_set_rect(jd, GUI_ALL);
                gui_set_slide(jd, GUI_N | GUI_FLING | GUI_EASE_ELASTIC, 0, 1.6f, 0);
            }

            gui_space(id);

            if ((jd = gui_harray(id)))
            {
                gui_filler(jd);

                if ((kd = gui_varray(jd)))
                {
                    if (config_cheat())
                        play_id = gui_start(kd, gt_prefix("menu^Cheat"),
                                                GUI_MED, TITLE_PLAY, 1);
                    else
                        play_id = gui_start(kd, gt_prefix("menu^Play"),
                                                GUI_MED, TITLE_PLAY, 1);

                    gui_state(kd, gt_prefix("menu^Help"), GUI_MED, TITLE_HELP, 0);
                    gui_state(kd, gt_prefix("menu^Options"), GUI_MED, TITLE_CONF, 0);

                    if (support_exit && config_get_d(CONFIG_FULLSCREEN))
                        gui_state(kd, gt_prefix("menu^Exit"), GUI_MED, TITLE_EXIT, 0);

                    /* Hilight the start button. */

                    gui_set_hilite(play_id, 1);
                    gui_set_slide(kd, GUI_N | GUI_EASE_ELASTIC, 0.8f, 0.8f, 0.05f);
                }

                gui_filler(jd);
            }
            gui_layout(id, 0, 0);
        }

#if ENABLE_VERSION
        if ((id = gui_label(root_id, "Neverputt " VERSION, GUI_TNY, GUI_COLOR_WHT2)))
        {
            gui_clr_rect(id);
            gui_layout(id, 0, -1);
        }
#endif

        if ((id = gui_vstack(root_id)))
        {
            const int w = video.device_w;
            const int h = video.device_h;

<<<<<<< HEAD
            const int ww = MIN(w, h) / flerp(14, 16, 0.5f);
=======
            const int ww = MIN(w, h) / 14;
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
            const int hh = ww;

            if ((jd = gui_hstack(id)))
            {
                kd = gui_image(jd, "gui/neverball-modern-256.png", ww, hh);
<<<<<<< HEAD
                gui_set_state(kd, TITLE_BALL, 0);
=======
                
                gui_set_state(kd, TITLE_BALL, 0);

>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
                gui_space(jd);
            }

            gui_space(id);
<<<<<<< HEAD
            gui_layout(id, -1, -1);
        }

        /* Build the gamepad GUI. */

        if ((id = gui_hstack(root_id)))
        {
            gamepadinfo_controller_ids[3] = gui_label(id, _("P4 XXXXX"), GUI_SML, GUI_COLOR_GRY);
            gamepadinfo_controller_ids[2] = gui_label(id, _("P3 XXXXX"), GUI_SML, GUI_COLOR_GRY);
            gamepadinfo_controller_ids[1] = gui_label(id, _("P2 XXXXX"), GUI_SML, GUI_COLOR_GRY);
            gamepadinfo_controller_ids[0] = gui_label(id, _("P1 XXXXX"), GUI_SML, gui_wht, gui_red);

            for (int i = 0; i < 4; i++)
                gui_set_font(gamepadinfo_controller_ids[i], "ttf/seguiemj.ttf");

            gui_layout(id, 0, 1);
            gui_set_rect(id, GUI_S);
            gui_set_slide(id, GUI_N | GUI_EASE_ELASTIC, 1.6f, 0.8f, 0.05f);
        }
=======

            gui_layout(id, -1, -1);
        }
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
    }

    if (prev == config_ball() ? &st_ball_title : (config_putt() ? &st_putt_title : 0))
        return root_id;

    course_init();
    course_rand();

    return transition_slide(root_id, 1, intent);
}

/*
 * This function name will be redirected to putt_title_enter() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `putt_title_enter()` to `title_enter()`.
 */
#define title_enter putt_title_enter

static int putt_title_leave(struct state *st, struct state *next, int id, int intent)
{
    if (next == &st_conf ||
        next == &st_null)
    {
        /*
         * This is ugly, but better than stupidly deleting stuff using
         * object names from a previous GL context.
         */
        course_free();
    }

    if (next == config_ball() ? &st_ball_title : (config_putt() ? &st_putt_title : 0))
    {
        gui_delete(id);
        return 0;
    }

    return transition_slide(id, 0, intent);
}

/*
 * This function name will be redirected to putt_title_leave() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `putt_title_leave()` to `title_leave()`.
 */
#define title_leave putt_title_leave

static void putt_title_paint(int id, float t)
{
    putt_draw(0, t);
    gui_paint(id);

    console_gui_title_paint();
}

/*
 * This function name will be redirected to putt_title_leave() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `putt_title_leave()` to `title_paint()`.
 */
#define title_paint putt_title_paint

static void putt_title_timer(int id, float dt)
{
    float g[3] = { 0.f, 0.f, 0.f };

<<<<<<< HEAD
    putt_step_fade(dt);
    putt_step(g, dt);
    putt_set_fly(fcosf(V_PI * time_state() / 10.f));

    int battery_level, gamepad_wired;

    if (joy_connected(0, &battery_level, &gamepad_wired) &&
        gamepadinfo_controller_ids[0])
    {
        if (gamepad_wired)
            gui_set_label(gamepadinfo_controller_ids[0], _("P1 " GUI_GAMEPAD));
        else if (battery_level < 2)
            gui_set_label(gamepadinfo_controller_ids[0], _("P1 " GUI_GAMEPAD " " GUI_BATTERY_LOW));
        else
            gui_set_label(gamepadinfo_controller_ids[0], _("P1 " GUI_GAMEPAD " " GUI_BATTERY));

        gui_set_color(gamepadinfo_controller_ids[0],
                      battery_level < 2 && !gamepad_wired && fcosf(V_PI * time_state() * 2) > 0 ? gui_red : gui_wht,
                      gui_red);
    }
    else
    {
        gui_set_label(gamepadinfo_controller_ids[0], _("P1"));
        gui_set_color(gamepadinfo_controller_ids[0], gui_wht, gui_red);
    }

    if (joy_connected(1, &battery_level, &gamepad_wired) &&
        gamepadinfo_controller_ids[1])
    {
        if (gamepad_wired)
            gui_set_label(gamepadinfo_controller_ids[1], _("P2 " GUI_GAMEPAD));
        else if (battery_level < 2)
            gui_set_label(gamepadinfo_controller_ids[1], _("P2 " GUI_GAMEPAD " " GUI_BATTERY_LOW));
        else
            gui_set_label(gamepadinfo_controller_ids[1], _("P2 " GUI_GAMEPAD " " GUI_BATTERY));

        gui_set_color(gamepadinfo_controller_ids[1],
                      battery_level < 2 && !gamepad_wired && fcosf(V_PI * time_state() * 2) > 0 ? gui_red : gui_wht,
                      battery_level < 2 && !gamepad_wired && fcosf(V_PI * time_state() * 2) > 0 ? gui_red : gui_grn);
    }
    else
    {
        gui_set_label(gamepadinfo_controller_ids[1], _("P2"));
        gui_set_color(gamepadinfo_controller_ids[1], gui_gry, gui_gry);
    }

    if (joy_connected(2, &battery_level, &gamepad_wired) &&
        gamepadinfo_controller_ids[2])
    {
        if (gamepad_wired)
            gui_set_label(gamepadinfo_controller_ids[2], _("P3 " GUI_GAMEPAD));
        else if (battery_level < 2)
            gui_set_label(gamepadinfo_controller_ids[2], _("P3 " GUI_GAMEPAD " " GUI_BATTERY_LOW));
        else
            gui_set_label(gamepadinfo_controller_ids[2], _("P3 " GUI_GAMEPAD " " GUI_BATTERY));

        gui_set_color(gamepadinfo_controller_ids[2],
                      battery_level < 2 && !gamepad_wired && fcosf(V_PI * time_state() * 2) > 0 ? gui_red : gui_wht,
                      battery_level < 2 && !gamepad_wired && fcosf(V_PI * time_state() * 2) > 0 ? gui_red : gui_blu);
    }
    else
    {
        gui_set_label(gamepadinfo_controller_ids[2], _("P3"));
        gui_set_color(gamepadinfo_controller_ids[2], gui_gry, gui_gry);
    }

    if (joy_connected(3, &battery_level, &gamepad_wired) &&
        gamepadinfo_controller_ids[3])
    {
        if (gamepad_wired)
            gui_set_label(gamepadinfo_controller_ids[3], _("P4 " GUI_GAMEPAD));
        else if (battery_level < 2)
            gui_set_label(gamepadinfo_controller_ids[3], _("P4 " GUI_GAMEPAD " " GUI_BATTERY_LOW));
        else
            gui_set_label(gamepadinfo_controller_ids[3], _("P4 " GUI_GAMEPAD " " GUI_BATTERY));

        gui_set_color(gamepadinfo_controller_ids[3],
                      battery_level < 2 && !gamepad_wired && fcosf(V_PI * time_state() * 2) > 0 ? gui_red : gui_wht,
                      battery_level < 2 && !gamepad_wired && fcosf(V_PI * time_state() * 2) > 0 ? gui_red : gui_yel);
    }
    else
    {
        gui_set_label(gamepadinfo_controller_ids[3], _("P4"));
        gui_set_color(gamepadinfo_controller_ids[3], gui_gry, gui_gry);
    }
=======
    putt_step(g, dt);
    putt_set_fly(fcosf(time_state() / 10.f));
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b

    gui_timer(id, dt);
}

/*
 * This function name will be redirected to putt_title_timer() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `putt_title_timer()` to `title_timer()`.
 */
#define title_timer putt_title_timer

static int putt_title_click(int b, int d)
{
    if (config_tst_d(CONFIG_MOUSE_CANCEL_MENU, b))
        return title_action(TITLE_EXIT);

    return gui_click(b, d) ? title_action(gui_token(gui_active())) : 1;
}

/*
 * This function name will be redirected to putt_title_click() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `putt_title_click()` to `title_click()`.
 */
#define title_click putt_title_click

static int putt_title_keybd(int c, int d)
{
    if (d && c == KEY_EXIT)
        return title_action(TITLE_EXIT);

    return 1;
}

/*
 * This function name will be redirected to putt_title_keybd() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `putt_title_keybd()` to `title_keybd()`.
 */
#define title_keybd putt_title_keybd

static int putt_title_buttn(int b, int d)
{
    if (d && joy_get_cursor_actions(0))
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return title_action(gui_token(gui_active()));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return title_action(TITLE_EXIT);
    }
    return 1;
}

/*
 * This function name will be redirected to putt_title_buttn() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `putt_title_buttn()` to `title_buttn()`.
 */
#define title_buttn putt_title_buttn

/*---------------------------------------------------------------------------*/

#define HELP_BACK -1

static int putt_help_action(int i)
{
    PUTT_GAMEMENU_ACTION(HELP_BACK);

    switch (i)
    {
        case HELP_BACK: return exit_state(config_ball() ? &st_ball_title : (config_putt() ? &st_putt_title : 0));
    }
    return 1;
}

/*
 * This function name will be redirected to putt_help_action() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `putt_help_action()` to `help_action()`.
 */
#define help_action putt_help_action

static int putt_help_enter(struct state *st, struct state *prev, int intent)
{
    int id, jd;

    /* Build the help GUI. */

    if ((id = gui_vstack(0)))
    {
        if ((jd = gui_hstack(id)))
        {
            gui_label(jd, gt_prefix("menu^Help"), GUI_SML, 0, 0);
            gui_filler(jd);
            shared_back_button(jd);
        }

        gui_space(id);

        gui_multi(id, _("Move the mouse from the direction you wish to shoot.\n"
                        "A power indicator will show you which direction\n"
                        "is going to roll. The longer line is, the more powerful\n"
                        "your shot will be. Once you have your shot aimed\n"
                        "click LMB on your mouse to shoot."),
                        GUI_SML, GUI_COLOR_WHT);
    }

    gui_layout(id, 0, 0);

    return transition_slide(id, 1, intent);
}

/*
 * This function name will be redirected to putt_help_enter() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `putt_help_enter()` to `help_enter()`.
 */
#define help_enter putt_help_enter

static void putt_help_paint(int id, float t)
{
    putt_draw(0, t);
    gui_paint(id);

    console_gui_list_paint();
}

/*
 * This function name will be redirected to putt_help_paint() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `putt_help_paint()` to `help_paint()`.
 */
#define help_paint putt_help_paint

static void putt_help_point(int id, int x, int y, int dx, int dy)
{
    if (joy_get_cursor_actions(0))
        gui_pulse(gui_point(id, x, y), 1.2f);
}

/*
 * This function name will be redirected to putt_help_point() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `putt_help_point()` to `help_point()`.
 */
#define help_point putt_help_point

static void putt_help_stick(int id, int a, float v, int bump)
{
    if (joy_get_cursor_actions(0))
        gui_pulse(shared_stick_basic(id, a, v, bump), 1.2f);
}

/*
 * This function name will be redirected to putt_help_stick() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `putt_help_stick()` to `help_stick()`.
 */
#define help_stick putt_help_stick

static int putt_help_click(int b, int d)
{
    if (config_tst_d(CONFIG_MOUSE_CANCEL_MENU, b))
        return help_action(HELP_BACK);

    return gui_click(b, d) ? help_action(gui_token(gui_active())) : 1;
}

/*
 * This function name will be redirected to putt_help_click() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `putt_help_click()` to `help_click()`.
 */
#define help_click putt_help_click

static int putt_help_keybd(int c, int d)
{
    if (d && c == KEY_EXIT)
        return help_action(HELP_BACK);

    return 1;
}

/*
 * This function name will be redirected to putt_help_keybd() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `putt_help_keybd()` to `help_keybd()`.
 */
#define help_keybd putt_help_keybd

static int putt_help_buttn(int b, int d)
{
    if (d && joy_get_cursor_actions(0))
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return help_action(gui_token(gui_active()));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return help_action(HELP_BACK);
    }
    return 1;
}

/*
 * This function name will be redirected to putt_help_buttn() for modern WGCL source project.
 * To continue with legacy source project Neverball,
 * please change from `putt_help_buttn()` to `help_buttn()`.
 */
#define help_buttn putt_help_buttn

/*---------------------------------------------------------------------------*/

static int desc_id;
static int shot_id;

#define COURSE_GETONLINE -2
#define COURSE_BACK -1

static void course_refresh_packages_done(void* data1, void* data2)
{
    struct fetch_done *dn = data2;

    if (dn->success)
    {
#if NB_HAVE_PB_BOTH == 1
        goto_wgcl_addons_login(0, &st_putt_course, 0);
#else
        goto_package(0, &st_putt_course);
#endif
    }
    else audio_play("snd/uierror.ogg", 1.0f);
}

static unsigned int course_refresh_packages(void)
{
    package_change_category(PACKAGE_CATEGORY_COURSE);

    struct fetch_callback callback = { 0 };

    callback.data = NULL;
    callback.done = course_refresh_packages_done;

    return package_refresh(callback, course_cmp_available_packages);
}

static int course_action(int i)
{
    PUTT_GAMEMENU_ACTION(COURSE_BACK);

    if (i == COURSE_GETONLINE)
        course_refresh_packages();

    if (course_exists(i))
    {
        course_goto(i);
        goto_state(&st_putt_party);
    }

    if (i == COURSE_BACK)
<<<<<<< HEAD
        exit_state(config_ball() ? &st_ball_title : (config_putt() ? &st_putt_title : 0));
=======
        exit_state(&st_putt_title);
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b

    return 1;
}

static int comp_size(int n, int s)
{
    return n <= s * s ? s : comp_size(n, s + 1);
}

static int comp_cols(int n)
{
    return comp_size(n, 1);
}

static int comp_rows(int n)
{
    int s = comp_size(n, 1);

    return n <= s * (s - 1) ? s - 1 : s;
}

static int course_enter(struct state *st, struct state *prev, int intent)
{
    int w = video.device_w;
    int h = video.device_h;

    int id, jd, kd, ld, md;

    int i, j, r, c, n;

    n = course_count();

    if (n == 0)
    {
        if ((id = gui_vstack(0)))
        {
            gui_title_header(id, _("No Courses"), GUI_MED, GUI_COLOR_DEFAULT);
            gui_space(id);

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
#if NB_STEAM_API==1
            gui_state(id, _("Get Courses from Steam Workshop!"),
                          GUI_SML, COURSE_GETONLINE, 0);
#elif NB_EOS_SDK==1
            gui_state(id, _("Get Courses from Epic Games Store!"),
                          GUI_SML, COURSE_GETONLINE, 0);
#else
            gui_state(id, _("Get Courses from Addons!"),
                          GUI_SML, COURSE_GETONLINE, 0);
#endif
#endif
#if !defined(__EMSCRIPTEN__)
            if (current_platform == PLATFORM_PC)
#endif
                shared_back_button(id);

            gui_layout(id, 0, 0);
        }

        return transition_slide(id, 1, intent);
    }

    r = comp_rows(n);
    c = comp_cols(n);

    if ((id = gui_vstack(0)))
    {
        /* This one is out of date */
        /*gui_label(id, _("Select Course"), GUI_MED, GUI_COLOR_DEFAULT);*/

        /* This one is now syncronized */
        if ((jd = gui_hstack(id)))
        {
            gui_label(jd, _("Select Course"), GUI_SML, GUI_COLOR_DEFAULT);
            gui_filler(jd);
            shared_back_button(jd);
        }

        gui_space(id);

        if ((jd = gui_hstack(id)))
        {
            const int ww = MIN(w, h) * 7 / 12;
            const int hh = ww / 4 * 3;

            shot_id = gui_image(jd, course_shot(0), ww, hh);

            gui_filler(jd);

            if ((kd = gui_varray(jd)))
                for (i = 0; i < r; i++)
                    if ((ld = gui_harray(kd)))
                        for (j = c - 1; j >= 0; j--)
                        {
                            int k = i * c + j;

                            if (k < n)
                            {
                                const int iw = (c > 0) ? w / 3 / c : 0;
                                const int ih = (r > 0) ? h / 3 / r : 0;

                                md = gui_image(ld, course_shot(k), iw, ih);
                                gui_set_state(md, k, 0);

                                if (k == 0)
                                    gui_focus(md);
                            }
                            else
                                gui_space(ld);
                        }
        }

        gui_space(id);
        desc_id = gui_multi(id, _(course_desc(0)), GUI_SML, gui_yel, gui_wht);
        gui_space(id);

        /* This one is out of date */
        /*if ((jd = gui_hstack(id)))
        {
            gui_filler(jd);
            shared_back_button(jd);
        }*/

        gui_layout(id, 0, 0);
    }

    audio_music_fade_to(0.5f, _("bgm/title.ogg"), 1);

    return transition_slide(id, 1, intent);
}

static void course_paint(int id, float t)
{
    putt_draw(0, t);
    gui_paint(id);

    console_gui_list_paint();
}

static void course_point(int id, int x, int y, int dx, int dy)
{
    int jd;

    if ((jd = gui_point(id, x, y)))
    {
        int i = gui_token(jd);

        if (course_exists(i))
        {
            gui_set_image(shot_id, course_shot(i));
            gui_set_multi(desc_id, _(course_desc(i)));
        }
        gui_pulse(jd, 1.2f);
    }
}

static void course_stick(int id, int a, float v, int bump)
{
    int jd;

    if ((jd = shared_stick_basic(id, a, v, bump)))
    {
        int i = gui_token(jd);

        if (course_exists(i))
        {
            gui_set_image(shot_id, course_shot(i));
            gui_set_multi(desc_id, _(course_desc(i)));
        }
        gui_pulse(jd, 1.2f);
    }
}

static int course_click(int b, int d)
{
    if (config_tst_d(CONFIG_MOUSE_CANCEL_MENU, b))
        return course_action(HELP_BACK);

    return gui_click(b, d) ? course_action(gui_token(gui_active())) : 1;
}

static int course_keybd(int c, int d)
{
    if (d && c == KEY_EXIT)
        return course_action(COURSE_BACK);

    return 1;
}

static int course_buttn(int b, int d)
{
    if (d && joy_get_cursor_actions(0))
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return course_action(gui_token(gui_active()));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return course_action(COURSE_BACK);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

#define PARTY_T 0
#define PARTY_1 1
#define PARTY_2 2
#define PARTY_3 3
#define PARTY_4 4
#define PARTY_B -1

static int holdage_player_count = 1;

static int party_action(int i)
{
    PUTT_GAMEMENU_ACTION(PARTY_B);

    switch (i)
    {
<<<<<<< HEAD
        case PARTY_1:
            if (hole_goto(1, 1))
                goto_state(&st_putt_next);
            break;
        case PARTY_2:
            holdage_player_count = 2;
            goto_state(&st_putt_controltype);
            break;
        case PARTY_3:
            holdage_player_count = 3;
            goto_state(&st_putt_controltype);
            break;
        case PARTY_4:
            holdage_player_count = 4;
            goto_state(&st_putt_controltype);
            break;
        case PARTY_B:
            exit_state(&st_putt_course);
            break;
=======
    case PARTY_1:
        audio_play(AUD_MENU, 1.f);
        if (hole_goto(1, 1))
            goto_state(&st_putt_next);
        break;
    case PARTY_2:
        audio_play(AUD_MENU, 1.f);
        if (hole_goto(1, 2))
            goto_state(&st_putt_next);
        break;
    case PARTY_3:
        audio_play(AUD_MENU, 1.f);
        if (hole_goto(1, 3))
            goto_state(&st_putt_next);
        break;
    case PARTY_4:
        audio_play(AUD_MENU, 1.f);
        if (hole_goto(1, 4))
            goto_state(&st_putt_next);
        break;
    case PARTY_B:
        audio_play(AUD_MENU, 1.f);
        exit_state(&st_putt_course);
        break;
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
    }
    return 1;
}

static int party_enter(struct state *st, struct state *prev, int intent)
{
    int id, jd;

    holdage_player_count = 1;
    party_indiv_controllers = 0;

    if ((id = gui_vstack(0)))
    {
        /* This one is out of date */
        /*gui_label(id, _("Players?"), GUI_MED, GUI_COLOR_DEFAULT);*/

        /* This one is now syncronized */
        if ((jd = gui_hstack(id)))
        {
            gui_label(jd, _("Players?"), GUI_SML, GUI_COLOR_DEFAULT);
            gui_filler(jd);
            shared_back_button(jd);
        }

        gui_space(id);

        if ((jd = gui_harray(id)))
        {
            int p4 = gui_state(jd, "4", GUI_LRG, PARTY_4, 0);
            int p3 = gui_state(jd, "3", GUI_LRG, PARTY_3, 0);
            int p2 = gui_state(jd, "2", GUI_LRG, PARTY_2, 0);
            int p1 = gui_state(jd, "1", GUI_LRG, PARTY_1, 0);

            gui_set_color(p1, gui_red, gui_wht);
            gui_set_color(p2, gui_grn, gui_wht);
            gui_set_color(p3, gui_blu, gui_wht);
            gui_set_color(p4, gui_yel, gui_wht);

            gui_focus(p1);
        }

        gui_space(id);

        /* This one is out of date */
        /*if ((jd = gui_hstack(id)))
        {
            gui_filler(jd);
            shared_back_button(jd);
        }*/

        gui_layout(id, 0, 0);
    }

    return transition_slide(id, 1, intent);
}

static void party_paint(int id, float t)
{
    putt_draw(0, t);
    gui_paint(id);

    console_gui_list_paint();
}

static int party_click(int b, int d)
{
    if (config_tst_d(CONFIG_MOUSE_CANCEL_MENU, b))
        return party_action(PARTY_B);

    return gui_click(b, d) ? party_action(gui_token(gui_active())) : 1;
}

static int party_keybd(int c, int d)
{
    if (d && c == KEY_EXIT)
        return party_action(PARTY_B);

    return 1;
}

static int party_buttn(int b, int d)
{
    if (d && joy_get_cursor_actions(0))
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return party_action(gui_token(gui_active()));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return party_action(PARTY_B);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

#define CONTROLTYPE_T 0
#define CONTROLTYPE_M 1
#define CONTROLTYPE_1 2
#define CONTROLTYPE_B -1

static int ctrltype_name_id = 0, ctrltype_desc_id = 0;

static int controltype_action(int i)
{
    int ji, indiv_ctrltype_ready = 1;
    PUTT_GAMEMENU_ACTION(CONTROLTYPE_B);

    switch (i)
    {
        case CONTROLTYPE_M:
            for (ji = 0; ji < holdage_player_count && indiv_ctrltype_ready; ji++)
            {
                if (!joy_connected(ji, 0, 0))
                    return 1;
            }
            party_indiv_controllers = 1;
        case CONTROLTYPE_1:
            if (hole_goto(1, holdage_player_count))
                return goto_state(&st_putt_next);
            break;
        case CONTROLTYPE_B:
            return exit_state(&st_putt_course);
        default: return 1;
    }

    return 1;
}

static int controltype_enter(struct state *st, struct state *prev, int intent)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        if ((jd = gui_hstack(id)))
        {
            gui_label(jd, _("Control type"), GUI_SML, 0, 0);
            gui_filler(jd);
            shared_back_button(jd);
        }

        gui_space(id);

        if ((jd = gui_vstack(id)))
        {
            ctrltype_name_id = gui_label(jd, _("1 Controller/Player"), GUI_SML, 0, 0);
            ctrltype_desc_id = gui_multi(jd,
                                         "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
                                         GUI_SML, GUI_COLOR_WHT);
            gui_set_rect(jd, GUI_ALL);
            gui_set_state(jd, CONTROLTYPE_M, 0);
        }

        gui_space(id);

        if ((jd = gui_vstack(id)))
        {
            gui_label(jd, _("1 Controller for all"), GUI_SML, 0, 0);
            gui_label(jd, _("Each Player uses the same Controller"), GUI_SML, GUI_COLOR_WHT);
            gui_set_rect(jd, GUI_ALL);
            gui_set_state(jd, CONTROLTYPE_1, 0);
        }

        gui_layout(id, 0, 0);
    }

    return transition_slide(id, 1, intent);
}

static void controltype_paint(int id, float t)
{
    putt_draw(0, t);
    gui_paint(id);

    console_gui_list_paint();
}

static void controltype_timer(int id, float dt)
{
    int indiv_ctrltype_ready = 1;

    for (int i = 0; i < holdage_player_count && indiv_ctrltype_ready; i++)
    {
        if (!joy_connected(i, 0, 0))
        {
            char descattr[64];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf(descattr, 64
#else
            sprintf(descattr,
#endif
                    _("Connect %d more Controller to play this!"), i - holdage_player_count);

            gui_set_color(ctrltype_name_id, gui_gry, gui_red);
            gui_set_multi(ctrltype_desc_id, descattr);

            indiv_ctrltype_ready = 0;
            break;
        }
    }

    if (indiv_ctrltype_ready)
    {
        gui_set_color(ctrltype_name_id, GUI_COLOR_DEFAULT);
        gui_set_label(ctrltype_desc_id, _("Each Player has one Controller"));
    }

    gui_timer(id, dt);
}

static int controltype_click(int b, int d)
{
    if (config_tst_d(CONFIG_MOUSE_CANCEL_MENU, b))
        return controltype_action(PARTY_B);

    return gui_click(b, d) ? controltype_action(gui_token(gui_active())) : 1;
}

static int controltype_keybd(int c, int d)
{
    if (d && c == KEY_EXIT && joy_get_cursor_actions(0))
        return controltype_action(CONTROLTYPE_B);

    return 1;
}

static int controltype_buttn(int b, int d)
{
    if (d && joy_get_cursor_actions(0))
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return controltype_action(gui_token(gui_active()));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return controltype_action(CONTROLTYPE_B);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int paused = 0;

static struct state *st_continue;
static struct state *st_quit;

<<<<<<< HEAD
enum
{
    PAUSE_CONTINUE = 1,
    PAUSE_QUIT,
    PAUSE_OPTIONS,
    PAUSE_SKIP,
    PAUSE_RESHOT
};

int goto_putt_pause(int use_keybd)
{
    audio_play("snd/2.2/game_pause.ogg", 1.0f);

=======
#define PAUSE_CONTINUE 1
#define PAUSE_QUIT     2
#define PAUSE_OPTIONS  3

int goto_putt_pause(struct state *s)
{
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
    if (curr_state() == &st_putt_pause)
        return 1;

    if (use_keybd)
        paused_indiv_ctrl_index = 0;
    else if (party_indiv_controllers)
        paused_indiv_ctrl_index = joy_get_gamepad_action_index();

    st_continue = curr_state();
    st_quit = &st_putt_over;
    paused = 1;

    return goto_state(&st_putt_pause);
}

static int pause_action(int i)
{
    PUTT_GAMEMENU_ACTION(-1);

    switch(i)
    {
    case PAUSE_OPTIONS:
        return goto_state(&st_conf);

    case PAUSE_CONTINUE:
<<<<<<< HEAD
        return exit_state(st_continue ? st_continue : config_ball() ? &st_ball_title : (config_putt() ? &st_putt_title : 0));

    case PAUSE_RESHOT:
        return exit_state(&st_putt_retry);

    case PAUSE_SKIP:
        hole_skip();
        return goto_state(&st_putt_score);
=======
        return goto_state(st_continue ? st_continue : &st_putt_title);
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b

    case PAUSE_QUIT:
        stroke_type = 0;
        stroke_set_type(0);
        return goto_state(st_quit);
    }
    return 1;
}

static int stroke_allowed = 0;

static int pause_enter(struct state *st, struct state *prev, int intent)
{
    int pdid;
    int id, jd, kd, ld, md, td;

    audio_music_fade_out(0.2f);

    if ((id = gui_vstack(0)))
    {
        if (paused_indiv_ctrl_index == 0)
        {
            if ((jd = gui_hstack(id)))
            {
                if ((kd = gui_hstack(jd)))
                {
                    gui_label(kd, GUI_GEAR, GUI_SML, GUI_COLOR_WHT);
                    gui_label(kd, _("Options"), GUI_SML, GUI_COLOR_WHT);

                    gui_set_state(kd, PAUSE_OPTIONS, 0);
                    gui_set_rect(kd, GUI_ALL);
                }
                gui_filler(jd);
            }

            gui_space(id);
        }

        td = gui_title_header(id, _("Paused"), GUI_LRG, gui_gry, gui_red);
        gui_space(id);

        if (curr_party() > 1)
        {
            if ((pdid = gui_harray(id)))
            {
                if (curr_party() > 3)
                {
                    gui_label(pdid, curr_scr_profile(4), GUI_SML, gui_yel, gui_wht);
                    gui_label(pdid, _("P4"), GUI_SML, gui_yel, gui_wht);
                }
                if (curr_party() > 2)
                {
                    gui_label(pdid, curr_scr_profile(3), GUI_SML, gui_blu, gui_wht);
                    gui_label(pdid, _("P3"), GUI_SML, gui_blu, gui_wht);
                }

                gui_label(pdid, curr_scr_profile(2), GUI_SML, gui_grn, gui_wht);
                gui_label(pdid, _("P2"), GUI_SML, gui_grn, gui_wht);

                gui_label(pdid, curr_scr_profile(1), GUI_SML, gui_red, gui_wht);
                gui_label(pdid, _("P1"), GUI_SML, gui_red, gui_wht);
            }
            gui_set_rect(pdid, GUI_ALL);
            gui_space(id);
        }

        if ((jd = gui_harray(id)))
        {
            if ((kd = gui_hstack(jd)))
            {
                if ((ld = gui_hstack(kd)))
                {
                    gui_label(ld, GUI_CROSS, GUI_SML, GUI_COLOR_RED);

                    md = gui_label(ld, _("Quit"), GUI_SML, GUI_COLOR_WHT);
                    gui_set_fill(md);

                    gui_set_state(ld, PAUSE_QUIT, 0);
                    gui_set_rect(ld, GUI_ALL);
                }
            }

            if ((kd = gui_hstack(jd)))
            {
                if ((ld = gui_hstack(kd)))
                {
                    gui_label(ld, GUI_CROSS, GUI_SML, GUI_COLOR_RED);

                    md = gui_label(ld, _("Skip"), GUI_SML, GUI_COLOR_WHT);
                    gui_set_fill(md);

                    gui_set_state(ld, PAUSE_QUIT, 0);
                    gui_set_rect(ld, GUI_ALL);
                }
            }

            if (!stroke_allowed && hole_retry_avail(stroke_allowed))
                if ((kd = gui_hstack(jd)))
                {
                    if ((ld = gui_hstack(kd)))
                    {
                        gui_label(ld, GUI_CIRCLE_ARROW, GUI_SML, GUI_COLOR_RED);

                        md = gui_label(ld, _("Retry"), GUI_SML, GUI_COLOR_WHT);
                        gui_set_fill(md);

                        gui_set_state(ld, PAUSE_RESHOT, 0);
                        gui_set_rect(ld, GUI_ALL);
                    }
                }

            if ((kd = gui_hstack(jd)))
            {
                gui_label(kd, GUI_TRIANGLE_RIGHT, GUI_SML, GUI_COLOR_GRN);

                ld = gui_label(kd, _("Continue"), GUI_SML, GUI_COLOR_WHT);
                gui_set_fill(ld);

                gui_set_state(kd, PAUSE_CONTINUE, 1);
                gui_set_rect(kd, GUI_ALL);

                gui_focus(kd);
            }
        }

        gui_pulse(td, 1.2f);
        gui_layout(id, 0, 0);
    }

<<<<<<< HEAD
    putt_hud_hide();
=======
    putt_hud_init();
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
    return transition_slide(id, 1, intent);
}

static int pause_leave(struct state *st, struct state *next, int id, int intent)
{
<<<<<<< HEAD
    paused_indiv_ctrl_index = -1;
=======
    putt_hud_free();
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b

    if (next == &st_null)
        course_free();
    else if (next != &st_conf)
        audio_music_fade_in(0.5f);

    return transition_slide(id, 0, intent);
}

static void pause_paint(int id, float t)
{
    putt_draw(0, t);
<<<<<<< HEAD

    if (putt_hud_visibility() ||
        config_get_d(CONFIG_SCREEN_ANIMATIONS))
        putt_hud_paint();

    gui_paint(id);
=======
    gui_paint(id);
    putt_hud_paint();
}

static void pause_timer(int id, float dt)
{
    gui_timer(id, dt);
}

static void pause_point(int id, int x, int y, int dx, int dy)
{
    gui_pulse(gui_point(id, x, y), 1.2f);
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
}

static int pause_click(int b, int d)
{
    if (config_tst_d(CONFIG_MOUSE_CANCEL_MENU, b))
        return pause_action(PAUSE_CONTINUE);

    return gui_click(b, d) ? pause_action(gui_token(gui_active())) : 1;
}

static int pause_keybd(int c, int d)
{
    if (config_tst_d(CONFIG_KEY_ROTATE_FAST, c))
    {
        putt_set_fast(d);
        return 1;
    }

    if (d && KEY_IS_PAUSE(c))
        return pause_action(PAUSE_CONTINUE);

    return 1;
}

static int pause_buttn(int b, int d)
{
    if (d &&
        (joy_get_cursor_actions(paused_indiv_ctrl_index) || !party_indiv_controllers))
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return pause_action(gui_token(gui_active()));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b) ||
            config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
            return pause_action(PAUSE_CONTINUE);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int shared_keybd(int c, int d)
{
    if (config_tst_d(CONFIG_KEY_ROTATE_FAST, c))
    {
<<<<<<< HEAD
        putt_set_fast(d);
        return 1;
=======
        if (c == KEY_EXIT)
            return goto_putt_pause(&st_putt_over);
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
    }

    if (d && KEY_IS_PAUSE(c))
        return goto_putt_pause(1);

    return 1;
}

/*---------------------------------------------------------------------------*/

static int num = 0;
static int next_show_scorecard = 0;

static int next_enter(struct state *st, struct state *prev, int intent)
{
    int id, jd;
    char str[MAXSTR];

    hole_hilited        = 1;
    next_show_scorecard = 0;
    stroke_type         = 0;

    stroke_set_type(0);

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(str, MAXSTR
#else
    sprintf(str,
#endif
            _("Hole %02d"), curr_hole());

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, str, GUI_MED, GUI_COLOR_DEFAULT);

        if (curr_party() > 1)
        {
            audio_play(AUD_NEXTTURN, 1.f);

            gui_space(id);

            if ((jd = gui_vstack(id)))
            {
                gui_label(jd, _("Player"), GUI_SML, GUI_COLOR_DEFAULT);

                switch (curr_player())
                {
                    case 1:
                        gui_label(jd, "1", GUI_LRG, gui_red, gui_wht);
                        audio_narrator_play(AUD_PLAYER1);
                        break;
                    case 2:
                        gui_label(jd, "2", GUI_LRG, gui_grn, gui_wht);
                        audio_narrator_play(AUD_PLAYER2);
                        break;
                    case 3:
                        gui_label(jd, "3", GUI_LRG, gui_blu, gui_wht);
                        audio_narrator_play(AUD_PLAYER3);
                        break;
                    case 4:
                        gui_label(jd, "4", GUI_LRG, gui_yel, gui_wht);
                        audio_narrator_play(AUD_PLAYER4);
                        break;
                }

                gui_set_rect(jd, GUI_ALL);
            }

            if (!party_indiv_controllers)
                gui_label(jd, _("Pass the controller to another player"), GUI_SML, 0, 0);
            else
            {
                joy_active_cursor(curr_player() - 1, curr_player() == 1);
                joy_active_cursor(curr_player() - 1, curr_player() == 2);
                joy_active_cursor(curr_player() - 1, curr_player() == 3);
                joy_active_cursor(curr_player() - 1, curr_player() == 4);
            }
        }
        gui_layout(id, 0, 0);
    }

<<<<<<< HEAD
=======
    putt_hud_init();
    putt_set_fly(1.f);

>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
    if (paused)
        paused = 0;

    putt_set_fly(1.f);

    return transition_slide(id, 1, intent);
}

<<<<<<< HEAD
static void next_paint(int id, float t)
{
    putt_draw(0, t);
=======
static int next_leave(struct state *st, struct state *next, int id, int intent)
{
    putt_hud_free();
    return transition_slide(id, 0, intent);
}

static void next_paint(int id, float t)
{
    putt_draw(0, t);
    putt_hud_paint();
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
    gui_paint(id);
}

static void next_timer(int id, float dt)
{
    putt_step_fade(dt);

    gui_timer(id, dt);
}

<<<<<<< HEAD
=======
static void next_point(int id, int x, int y, int dx, int dy)
{
    gui_pulse(gui_point(id, x, y), 1.2f);
}

static int next_click(int b, int d)
{
    return (d && b == SDL_BUTTON_LEFT) ? goto_state(&st_putt_flyby) : 1;
}

>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
static int next_keybd(int c, int d)
{
    if (config_tst_d(CONFIG_KEY_ROTATE_FAST, c))
    {
        putt_set_fast(d);
        return 1;
    }

    if (d)
    {
        if (c == KEY_POSE)
            return goto_state(&st_putt_poser);
<<<<<<< HEAD
        if (KEY_IS_PAUSE(c))
            return goto_putt_pause(1);

        if (config_tst_d(CONFIG_KEY_SCORE_NEXT, c))
        {
            next_show_scorecard = 1;
            return goto_state(&st_putt_poser);
        }

#ifndef NDEBUG
=======
        if (c == KEY_EXIT)
            return goto_putt_pause(&st_putt_over);
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
        if ('0' <= c && c <= '9')
            num = num * 10 + c - '0';
#endif
    }
    return 1;
}

static int next_buttn(int b, int d)
{
    if (d &&
        (joy_get_cursor_actions(curr_player() - 1) || !party_indiv_controllers))
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
        {
#ifndef NDEBUG
            if (num > 0)
            {
                if (hole_goto(num, -1))
                {
                    num = 0;
                    return goto_state(&st_putt_next);
                }
                else
                {
                    num = 0;
                    return 1;
                }
            }
<<<<<<< HEAD
#endif
            return goto_state(&st_putt_flyby);
        }

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_X, b))
        {
            next_show_scorecard = 1;
            return goto_state(&st_putt_poser);
        }
=======
            return goto_state(&st_putt_flyby);
        }
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return goto_putt_pause(&st_putt_over);
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
    }

    if (d && config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
        return goto_putt_pause(0);

    return 1;
}

/*---------------------------------------------------------------------------*/

static int poser_enter(struct state *st, struct state *prev, int intent)
{
<<<<<<< HEAD
    if (!next_show_scorecard)
    {
        putt_set_fly(-1.f);
        return 0;
    }

    return score_card(_("Scores"), GUI_COLOR_DEFAULT);
=======
    putt_set_fly(-1.f);
    return 0;
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
}

static void poser_paint(int id, float t)
{
    putt_draw(1, t);
<<<<<<< HEAD
}

static int poser_keybd(int c, int d)
{
    if (d && c == KEY_EXIT)
        return goto_state(&st_putt_next);

    return 1;
=======
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
}

static int poser_buttn(int b, int d)
{
    if (d &&
        (joy_get_cursor_actions(curr_player() - 1) || !party_indiv_controllers))
    {
<<<<<<< HEAD
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b)
         || config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
=======
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return goto_state(&st_putt_next);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
            return goto_state(&st_putt_next);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int flyby_done = 0;

static int flyby_enter(struct state *st, struct state *prev, int intent)
{
    if (party_indiv_controllers)
    {
        joy_active_cursor(curr_player() - 1, curr_player() == 1);
        joy_active_cursor(curr_player() - 1, curr_player() == 2);
        joy_active_cursor(curr_player() - 1, curr_player() == 3);
        joy_active_cursor(curr_player() - 1, curr_player() == 4);
    }

    if (paused)
        paused = 0;
<<<<<<< HEAD

    video_hide_cursor();

    flyby_done = 0;
=======
    else
        putt_hud_init();
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b

    return 0;
}

static int flyby_leave(struct state *st, struct state *next, int id, int intent)
{
<<<<<<< HEAD
    if (next == &st_null)
        course_free();
    else
    {
        video_show_cursor();

        flyby_done = 1;
    }

=======
    video_show_cursor();
    putt_hud_free();
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
    return 0;
}

static void flyby_paint(int id, float t)
{
    putt_draw(0, t);
<<<<<<< HEAD
=======
    putt_hud_paint();
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
}

static void flyby_timer(int id, float dt)
{
    float t = time_state();

<<<<<<< HEAD
    if (dt > 0.f && t > 1.f && !flyby_done)
    {
        flyby_done = 1;
        goto_state(&st_putt_stroke);
=======
    if (dt > 0.f && t > 1.f)
        goto_state(&st_putt_stroke);
    else
        putt_set_fly(1.f - t);

    gui_timer(id, dt);
}

static int flyby_click(int b, int d)
{
    if (d && b == SDL_BUTTON_LEFT)
    {
        putt_set_fly(0.f);
        return goto_state(&st_putt_stroke);
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
    }
    else if (!flyby_done)
        putt_set_fly(1.f - t);

    putt_step_fade(dt);
    putt_hud_timer(dt);
    gui_timer(id, dt);
}

static int flyby_buttn(int b, int d)
{
    if (d &&
        (joy_get_cursor_actions(curr_player() - 1) || !party_indiv_controllers))
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
        {
            putt_set_fly(0.f);
            return goto_state(&st_putt_stroke);
        }
<<<<<<< HEAD
=======
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b) ||
            config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
            return goto_putt_pause(&st_putt_over);
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
    }

    if (d && config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
        return goto_putt_pause(0);

    return 1;
}

/*---------------------------------------------------------------------------*/

static int stroke_rotate     = 0;
static int stroke_rotate_alt = 0;
static int stroke_mag        = 0;
static int stroke_type       = 0;

static int stroke_enter(struct state *st, struct state *prev, int intent)
{
<<<<<<< HEAD
    if (party_indiv_controllers)
    {
        joy_active_cursor(curr_player() - 1, curr_player() == 1);
        joy_active_cursor(curr_player() - 1, curr_player() == 2);
        joy_active_cursor(curr_player() - 1, curr_player() == 3);
        joy_active_cursor(curr_player() - 1, curr_player() == 4);
    }

    stroke_allowed = 1;
=======
    putt_hud_init();
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
    putt_clr_mag();
    config_set_d(CONFIG_CAMERA, 2);
    video_set_grab(1);
    putt_hud_show(prev == &st_putt_flyby ? 0.5f : 0.0f);

    if (paused) paused = 0;
    else        stroke_type = 0;

    audio_music_fade_in(0.5f);

    return 0;
}

static int stroke_leave(struct state *st, struct state *next, int id, int intent)
{
<<<<<<< HEAD
=======
    putt_hud_free();
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
    video_clr_grab();
    config_set_d(CONFIG_CAMERA, 0);
    stroke_rotate = 0.0f;
    stroke_mag = 0.0f;

    if (next == &st_null)
    {
        course_free();
        video_show_cursor();
    }

    return 0;
}

static void stroke_paint(int id, float t)
{
    putt_draw(0, t);
<<<<<<< HEAD

    if (putt_hud_visibility() ||
        config_get_d(CONFIG_SCREEN_ANIMATIONS))
        putt_hud_paint();

    console_gui_putt_stroke_paint();
=======
    putt_hud_paint();
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
}

static void stroke_timer(int id, float dt)
{
    /* HACK: Used with realtime gravity? Yes, it's real! */

    float g[3] = { 0.f, -9.8f, 0.f };

    float k;

    if (SDL_GetModState() & KMOD_SHIFT || stroke_rotate_alt)
        k = (dt * (1.0f / 60.0f)) / 4.0f;
    else
        k = dt * (1.0f / 60.0f);

<<<<<<< HEAD
    if (dt <= 0.f) return;

    putt_set_rot(stroke_rotate * k);
    putt_set_mag(stroke_mag * k);

    putt_update_view(dt);

    switch (putt_step(g, dt))
    {
        case PUTT_FALL: goto_state(&st_putt_fall); break;
    }

    putt_step_fade(dt);
    putt_hud_timer(dt);
    gui_timer(id, dt);
=======
    putt_set_rot(stroke_rotate * k);
    putt_set_mag(stroke_mag * k);

    putt_update_view(dt);
    putt_step(g, dt);
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
}

static void stroke_point(int id, int x, int y, int dx, int dy)
{
    putt_set_rot(dx);
    putt_set_mag(dy);
}

static void stroke_stick(int id, int a, float v, int bump)
{
    if (joy_get_cursor_actions(curr_player() - 1) || !party_indiv_controllers)
    {
        if (config_tst_d(CONFIG_JOYSTICK_AXIS_X0, a))
            stroke_rotate = 6 * v;
        else if (config_tst_d(CONFIG_JOYSTICK_AXIS_Y0, a))
            stroke_mag = -6 * v;
    }
}

static int stroke_keybd(int c, int d)
{
<<<<<<< HEAD
    if (d)
    {
        if (KEY_IS_PAUSE(c))
            return goto_putt_pause(1);
        if (c == KEY_PUTT_UPGRADE && stroke_type < 3)
        {
            stroke_type++;
            stroke_set_type(stroke_type);
            putt_set_mag(0);
            return goto_state(&st_putt_stroke);
        }
        if (c == KEY_PUTT_DNGRADE && stroke_type > 0)
        {
            stroke_type--;
            stroke_set_type(stroke_type);
            putt_set_mag(0);
            return goto_state(&st_putt_stroke);
        }
    }
    return 1;
=======
    return (d && b == SDL_BUTTON_LEFT) ? goto_state(&st_putt_roll) : 1;
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
}

static int stroke_buttn(int b, int d)
{
    if (d &&
        (joy_get_cursor_actions(curr_player() - 1) || !party_indiv_controllers))
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_X, b))
            stroke_rotate_alt = 1;
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
<<<<<<< HEAD
        {
            putt_putt();
            return goto_state(&st_putt_roll);
        }

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L1, b) && stroke_type < 3)
        {
            stroke_type++;
            stroke_set_type(stroke_type);
            putt_set_mag(0);
            return goto_state(&st_putt_stroke);
        }
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L2, b) && stroke_type > 0)
        {
            stroke_type--;
            stroke_set_type(stroke_type);
            putt_set_mag(0);
            return goto_state(&st_putt_stroke);
        }
=======
            return goto_state(&st_putt_roll);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
            return goto_putt_pause(&st_putt_over);
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
    }
    else if (joy_get_cursor_actions(curr_player() - 1) || !party_indiv_controllers)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_X, b))
            stroke_rotate_alt = 0;
    }

    if (d && config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
        return goto_putt_pause(0);

    return 1;
}

static void stroke_fade(float alpha)
{
    console_gui_set_alpha(alpha);
    putt_hud_set_alpha(alpha);
}

static void stroke_wheel(int x, int y)
{
    if (joy_get_cursor_actions(curr_player() - 1) || !party_indiv_controllers)
    {
        if (y > 0 && stroke_type < 3)
        {
            stroke_type++;
            stroke_set_type(stroke_type);
            putt_set_mag(0);
            goto_state(&st_putt_stroke);
        }
        if (y < 0 && stroke_type > 0)
        {
            stroke_type--;
            stroke_set_type(stroke_type);
            putt_set_mag(0);
            goto_state(&st_putt_stroke);
        }
    }
}

static int stroke_tap_shot;

static int stroke_touch(const SDL_TouchFingerEvent *e)
{
#if defined(__ANDROID__) || defined(__IOS__) || defined(__EMSCRIPTEN__)
    static SDL_FingerID rotate_finger = -1;

    static float rotate = 0.0f, mag = 0.0f; /* Filtered input. */

    /*
     * Make sure not to exceed rotate_fast rotation speed.
     *
     * Find the coefficient needed to reach rotate_fast speed and
     * clamp the scaled input at that value.
     *
     * Derivation:
     * Default rotate_slow is 150 or 1.5
     * Default rotate_fast is 300 or 3.0
     *
     *   x * slow = 1.0 * fast
     *   x = (1.0 * fast) / slow
     *   x = fast / slow
     */
    const float rs = config_get_d(CONFIG_ROTATE_SLOW) / 100.0f;
    const float rf = config_get_d(CONFIG_ROTATE_FAST) / 100.0f;
    const float rmax = rf / rs;

    int id;

    if ((id = hud_touch(e)))
    {
        int token = gui_token(id);

        gui_pulse(id, 1.2f);

        if (token == GUI_BACK)
            goto_state(&st_putt_pause);

        gui_focus(0);
    }
    else if (e->type == SDL_FINGERMOTION)
    {
        stroke_tap_shot = 0;

        /* Discard accumulated input when moving in the opposite direction. */

        if ((rotate < 0.0f && e->dx > 0.0f) || (e->dx < 0.0f && rotate > 0.0f))
            rotate = 0.0f;

        /* Filter the input for a smoother experience. */

        rotate += e->dx * 0.6f;
        mag    += e->dy * 0.6f;

        stroke_rotate += CLAMP(-rmax, rotate, +rmax);
        stroke_mag    += CLAMP(-rmax, mag, +rmax);
    }
    else if (e->type == SDL_FINGERDOWN)
        stroke_tap_shot = 1;
    else if (e->type == SDL_FINGERUP)
    {
        rotate = 0.0f;
        mag    = 0.0f;

        if (stroke_tap_shot)
        {
            stroke_tap_shot = 0;

            putt_putt();
            return goto_state(&st_putt_roll);
        }
    }
#endif

    return 1;
}

/*---------------------------------------------------------------------------*/

static int roll_enter(struct state *st, struct state *prev, int intent)
{
    if (party_indiv_controllers)
    {
        joy_active_cursor(curr_player() - 1, curr_player() == 1);
        joy_active_cursor(curr_player() - 1, curr_player() == 2);
        joy_active_cursor(curr_player() - 1, curr_player() == 3);
        joy_active_cursor(curr_player() - 1, curr_player() == 4);
    }

    stroke_allowed = 0;
    video_hide_cursor();
<<<<<<< HEAD

    if (paused)
        paused = 0;
=======
    putt_hud_init();

    if (paused)
        paused = 0;
    else
        putt_putt();
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b

    putt_hud_hide();
    return 0;
}

static int roll_leave(struct state *st, struct state *next, int id, int intent)
{
    if (next == &st_null)
        course_free();

    video_show_cursor();
<<<<<<< HEAD

=======
    putt_hud_free();
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
    return 0;
}

static void roll_paint(int id, float t)
{
    putt_draw(0, t);
<<<<<<< HEAD

    if (putt_hud_visibility() ||
        config_get_d(CONFIG_SCREEN_ANIMATIONS))
        putt_hud_paint();
=======
    putt_hud_paint();
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
}

static void roll_timer(int id, float dt)
{
    float g[3] = { 0.0f, -9.8f, 0.0f };

<<<<<<< HEAD
    if (dt <= 0.f) return;

    switch (putt_step(g, dt))
    {
        case PUTT_STOP: goto_state(&st_putt_stop); break;
        case PUTT_GOAL: goto_state(&st_putt_goal); break;
        case PUTT_FALL: goto_state(&st_putt_fall); break;
=======
    switch (putt_step(g, dt))
    {
    case PUTT_STOP: goto_state(&st_putt_stop); break;
    case PUTT_GOAL: goto_state(&st_putt_goal); break;
    case PUTT_FALL: goto_state(&st_putt_fall); break;
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
    }

    putt_step_fade(dt);
    putt_hud_timer(dt);
    gui_timer(id, dt);
}

static int roll_buttn(int b, int d)
{
<<<<<<< HEAD
    if (d && config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
        return goto_putt_pause(0);

=======
    if (d)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b) ||
            config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
            return goto_putt_pause(&st_putt_over);
    }
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
    return 1;
}

/*---------------------------------------------------------------------------*/

static int goal_enter(struct state *st, struct state *prev, int intent)
{
    if (party_indiv_controllers)
    {
        joy_active_cursor(curr_player() - 1, curr_player() == 1);
        joy_active_cursor(curr_player() - 1, curr_player() == 2);
        joy_active_cursor(curr_player() - 1, curr_player() == 3);
        joy_active_cursor(curr_player() - 1, curr_player() == 4);
    }

    int id;

    int scr_v = atoi(curr_scr());
    int par_v = atoi(curr_par());
    char hole_statname[64];

    GLubyte *c0, *c1;

    if (scr_v == 1)
    {
        SAFECPY(hole_statname, _("Hole in one"));
        c0 = gui_wht;
        c1 = gui_yel;
    }
    else if (scr_v < par_v)
    {
        c0 = gui_wht;
        c1 = gui_yel;

        if (scr_v == par_v - 3)
            SAFECPY(hole_statname, _("Albatross"));
        else if (scr_v == par_v - 2)
            SAFECPY(hole_statname, _("Eagle"));
        else if (scr_v == par_v - 1)
            SAFECPY(hole_statname, _("Birdie"));
        else
        {
            SAFECPY(hole_statname, _("It's In!"));
            c0 = c1 = gui_grn;
        }
    }
    else if (scr_v > par_v)
    {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(hole_statname, 64,
#else
        sprintf(hole_statname,
#endif
                "+%d", scr_v - par_v);

        c0 = gui_blu;
        c1 = gui_wht;
    }
    else
    {
        SAFECPY(hole_statname, _("On Par"));
        c0 = gui_wht;
        c1 = gui_wht;
    }

    if ((id = gui_title_header(0, hole_statname, GUI_MED, c0, c1)))
    {
        gui_layout(id, 0, 0);
        gui_pulse(id, 1.2f);
    }

    if (paused)
        paused = 0;
    else
        hole_goal();

<<<<<<< HEAD
    putt_hud_hide();
    return transition_slide(id, 1, intent);
}

=======
    putt_hud_init();

    return transition_slide(id, 1, intent);
}

static int goal_leave(struct state *st, struct state *next, int id, int intent)
{
    putt_hud_free();
    return transition_slide(id, 0, intent);
}

>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
static void goal_paint(int id, float t)
{
    putt_draw(0, t);
    gui_paint(id);
<<<<<<< HEAD

    console_gui_putt_stop_paint();
=======
    putt_hud_paint();
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
}

static void goal_timer(int id, float dt)
{
    gui_timer(id, dt);

    if (time_state() > 3 && !st_global_animating())
    {
        if (hole_next())
            goto_state(&st_putt_next);
        else
            goto_state(&st_putt_score);
    }
}

<<<<<<< HEAD
=======
static int goal_click(int b, int d)
{
    if (b == SDL_BUTTON_LEFT && d == 1)
    {
        if (hole_next())
            goto_state(&st_putt_next);
        else
            goto_state(&st_putt_score);
    }
    return 1;
}

>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
static int goal_buttn(int b, int d)
{
    if (d &&
        (joy_get_cursor_actions(curr_player() - 1) || !party_indiv_controllers))
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
        {
            if (hole_next())
                goto_state(&st_putt_next);
            else
                goto_state(&st_putt_score);
        }
<<<<<<< HEAD
=======
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return goto_putt_pause(&st_putt_over);
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
    }

    if (d && config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
        return goto_putt_pause(0);

    return 1;
}

/*---------------------------------------------------------------------------*/

static int stop_enter(struct state *st, struct state *prev, int intent)
{
    if (party_indiv_controllers)
    {
        joy_active_cursor(curr_player() - 1, curr_player() == 1);
        joy_active_cursor(curr_player() - 1, curr_player() == 2);
        joy_active_cursor(curr_player() - 1, curr_player() == 3);
        joy_active_cursor(curr_player() - 1, curr_player() == 4);
    }

    if (paused)
        paused = 0;
    else
        hole_stop();

<<<<<<< HEAD
    putt_hud_hide();
=======
    putt_hud_init();

>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
    return 0;
}

static int stop_leave(struct state *st, struct state *next, int id, int intent)
{
<<<<<<< HEAD
    if (next == &st_null)
        course_free();

=======
    putt_hud_free();
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
    return 0;
}

static void stop_paint(int id, float t)
{
    putt_draw(0, t);
<<<<<<< HEAD

    console_gui_putt_stop_paint();
=======
    putt_hud_paint();
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
}

static void stop_timer(int id, float dt)
{
    float g[3] = { 0.f, -9.8f, 0.f };

<<<<<<< HEAD
    if (dt <= 0) return;
=======
    putt_update_view(dt);
    putt_step(g, dt);
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b

    putt_update_view(dt);
    putt_step(g, dt);
    putt_step_fade(dt);

    if (time_state() > 1 && !st_global_animating())
    {
        if (hole_next())
<<<<<<< HEAD
            goto_state(curr_party() > 1 ? &st_putt_next : &st_putt_stroke);
=======
            goto_state(&st_putt_next);
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
        else
            goto_state(&st_putt_score);
    }
}

<<<<<<< HEAD
=======
static int stop_click(int b, int d)
{
    if (b == SDL_BUTTON_LEFT && d == 1)
    {
        if (hole_next())
            goto_state(&st_putt_next);
        else
            goto_state(&st_putt_score);
    }
    return 1;
}

>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
static int stop_buttn(int b, int d)
{
    if (d &&
        (joy_get_cursor_actions(curr_player() - 1) || !party_indiv_controllers))
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
        {
            if (hole_next())
<<<<<<< HEAD
                goto_state(curr_party() > 1 ? &st_putt_next : &st_putt_stroke);
            else
                goto_state(&st_putt_score);
        }
=======
                goto_state(&st_putt_next);
            else
                goto_state(&st_putt_score);
        }
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b) ||
            config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
            return goto_putt_pause(&st_putt_over);
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
    }

    if (d && config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
        return goto_putt_pause(0);

    return 1;
}

/*---------------------------------------------------------------------------*/

static int fall_enter(struct state *st, struct state *prev, int intent)
{
    if (party_indiv_controllers)
    {
        joy_active_cursor(curr_player() - 1, curr_player() == 1);
        joy_active_cursor(curr_player() - 1, curr_player() == 2);
        joy_active_cursor(curr_player() - 1, curr_player() == 3);
        joy_active_cursor(curr_player() - 1, curr_player() == 4);
    }

    int id;

    if ((id = gui_title_header(0, _("1 Stroke Penalty"), GUI_MED, gui_blk, gui_red)))
    {
        audio_music_fade_out(0); audio_play("snd/intro-shatter.ogg", 1.0f);
        gui_pulse(id, 1.2f);
        gui_layout(id, 0, 0);
    }

    if (paused)
        paused = 0;
    else
        hole_fall(stroke_allowed);

<<<<<<< HEAD
    stroke_allowed = 1;
=======
    putt_hud_init();
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b

    putt_hud_hide();
    return transition_slide(id, 1, intent);
}

static int fall_leave(struct state *st, struct state *next, int id, int intent)
{
<<<<<<< HEAD
    if (next == &st_null)
        course_free();

    gui_delete(id);

=======
    putt_hud_free();
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
    return transition_slide(id, 0, intent);
}

static void fall_paint(int id, float t)
{
    putt_draw(0, t);
    gui_paint(id);
<<<<<<< HEAD

    console_gui_putt_stop_paint();
=======
    putt_hud_paint();
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
}

static void fall_timer(int id, float dt)
{
    gui_timer(id, dt);

    if (time_state() > (curr_party() > 1 ? 3 : 1) && !st_global_animating())
    {
        if (hole_next())
<<<<<<< HEAD
            goto_state(curr_party() > 1 ? &st_putt_next : &st_putt_stroke);
=======
            goto_state(&st_putt_next);
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
        else
            goto_state(&st_putt_score);
    }

<<<<<<< HEAD
    if (curr_party() == 1)
        putt_set_fly(1.f - time_state());
=======
static int fall_click(int b, int d)
{
    if (b == SDL_BUTTON_LEFT && d == 1)
    {
        if (hole_next())
            goto_state(&st_putt_next);
        else
            goto_state(&st_putt_score);
    }
    return 1;
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
}

static int fall_buttn(int b, int d)
{
    if (d &&
        (joy_get_cursor_actions(curr_player() - 1) || !party_indiv_controllers))
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
        {
            if (hole_next())
<<<<<<< HEAD
                goto_state(curr_party() > 1 ? &st_putt_next : &st_putt_stroke);
            else
                goto_state(&st_putt_score);
        }
=======
                goto_state(&st_putt_next);
            else
                goto_state(&st_putt_score);
        }
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return goto_putt_pause(&st_putt_over);
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
    }

    if (d && config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
        return goto_putt_pause(0);

    return 1;
}

/*---------------------------------------------------------------------------*/

static int retry_enter(struct state *st, struct state *prev, int intent)
{
    int id;

    if (party_indiv_controllers)
    {
        joy_active_cursor(curr_player() - 1, curr_player() == 1);
        joy_active_cursor(curr_player() - 1, curr_player() == 2);
        joy_active_cursor(curr_player() - 1, curr_player() == 3);
        joy_active_cursor(curr_player() - 1, curr_player() == 4);
    }

    if ((id = gui_title_header(0, _("Retry"), GUI_MED, gui_blk, gui_red)))
    {
        gui_pulse(id, 1.2f);
        gui_layout(id, 0, 0);
    }

    if (paused)
        paused = 0;
    else
        hole_retry(stroke_allowed);

    stroke_allowed = 1;

    putt_hud_hide();
    return transition_slide(id, 1, intent);
}

static int retry_leave(struct state *st, struct state *next, int id, int intent)
{
    if (next == &st_null)
        course_free();

    gui_delete(id);

    return transition_slide(id, 0, intent);
}

static void retry_paint(int id, float t)
{
    putt_draw(0, t);
    gui_paint(id);

    console_gui_putt_stop_paint();
}

/*---------------------------------------------------------------------------*/

static int score_enter(struct state *st, struct state *prev, int intent)
{
    if (party_indiv_controllers)
    {
        joy_active_cursor(0, 1);
        joy_active_cursor(1, 0);
        joy_active_cursor(2, 0);
        joy_active_cursor(3, 0);
    }

    audio_music_fade_out(2.f);

    if (paused)
        paused = 0;

    putt_hud_hide();
    return transition_slide(score_card(_("Scores"), GUI_COLOR_DEFAULT), 1, intent);
}

static void score_paint(int id, float t)
{
    putt_draw(0, t);
    gui_paint(id);

    console_gui_putt_scores_paint();
}

static int score_keybd(int c, int d)
{
    if (d &&
        (joy_get_cursor_actions(0) || !party_indiv_controllers))
    {
<<<<<<< HEAD
        if (KEY_IS_PAUSE(c))
            return goto_putt_pause(1);
=======
        if (hole_move())
            return goto_state(&st_putt_next);
        else
            return goto_state(&st_putt_title);
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
    }
    return 1;
}

static int score_buttn(int b, int d)
{
    if (d &&
        (joy_get_cursor_actions(0) || !party_indiv_controllers))
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
<<<<<<< HEAD
            goto_state(hole_move() ? &st_putt_next : &st_putt_over);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return goto_putt_pause(0);
=======
        {
            if (hole_move())
                goto_state(&st_putt_next);
            else
                goto_state(&st_putt_title);
        }
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return goto_putt_pause(&st_putt_over);
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int over_enter(struct state *st, struct state *prev, int intent)
{
    hole_hilited = 0;

    if (party_indiv_controllers)
    {
        joy_active_cursor(0, 1);
        joy_active_cursor(1, 0);
        joy_active_cursor(2, 0);
        joy_active_cursor(3, 0);
    }

    audio_music_fade_out(2.f);
    return transition_slide(score_card(_("Final Scores"), GUI_COLOR_DEFAULT), 1, intent);
}

static int over_leave(struct state *st, struct state *next, int id, int intent)
{
    if (next == &st_null)
        course_free();

    party_indiv_controllers = 0;

    gui_delete(id);
    putt_hud_free();

    return transition_slide(id, 0, intent);
}

static void over_paint(int id, float t)
{
    putt_draw(0, t);
    gui_paint(id);

    console_gui_putt_scores_paint();
}

static int over_keybd(int c, int d)
{
<<<<<<< HEAD
    if (d &&
        (joy_get_cursor_actions(0) || !party_indiv_controllers))
    {
        if (c == KEY_EXIT)
            return exit_state(config_ball() ? &st_ball_title : (config_putt() ? &st_putt_title : 0));
    }
    return 1;
=======
    gui_timer(id, dt);
}

static int over_click(int b, int d)
{
    return (d && b == SDL_BUTTON_LEFT) ? goto_state(&st_putt_title) : 1;
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
}

static int over_buttn(int b, int d)
{
    if (d &&
        (joy_get_cursor_actions(0) || !party_indiv_controllers))
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
<<<<<<< HEAD
            return exit_state(config_ball() ? &st_ball_title : (config_putt() ? &st_putt_title : 0));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return exit_state(config_ball() ? &st_ball_title : (config_putt() ? &st_putt_title : 0));
=======
            return goto_state(&st_putt_title);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return goto_state(&st_putt_title);
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

struct state st_putt_title = {
    title_enter,
    title_leave,
    title_paint,
    title_timer,
    shared_point,
    shared_stick,
    NULL,
    title_click,
    title_keybd,
    title_buttn,
    NULL,
    NULL,
    shared_fade
};

<<<<<<< HEAD
struct state st_putt_help = {
    help_enter,
    shared_leave,
    help_paint,
    shared_timer,
    help_point,
    shared_stick,
    NULL,
    help_click,
    help_keybd,
    help_buttn
};

=======
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
struct state st_putt_course = {
    course_enter,
    shared_leave,
    course_paint,
    shared_timer,
    course_point,
    course_stick,
    NULL,
    course_click,
    course_keybd,
    course_buttn
};

struct state st_putt_party = {
    party_enter,
    shared_leave,
    party_paint,
    shared_timer,
    shared_point,
    shared_stick,
    NULL,
    party_click,
    party_keybd,
    party_buttn
};

<<<<<<< HEAD
struct state st_putt_controltype = {
    controltype_enter,
    shared_leave,
    controltype_paint,
    controltype_timer,
    shared_point,
=======
struct state st_putt_next = {
    next_enter,
    next_leave,
    next_paint,
    next_timer,
    next_point,
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
    shared_stick,
    NULL,
    controltype_click,
    controltype_keybd,
    controltype_buttn
};

struct state st_putt_next = {
    next_enter,
    shared_leave,
    next_paint,
    next_timer,
    shared_point,
    shared_stick,
    NULL,
    shared_click_basic,
    next_keybd,
    next_buttn
};

struct state st_putt_poser = {
    poser_enter,
    NULL,
    poser_paint,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    poser_keybd,
    poser_buttn
};

struct state st_putt_flyby = {
    flyby_enter,
    flyby_leave,
    flyby_paint,
    flyby_timer,
    NULL,
    NULL,
    NULL,
    shared_click_basic,
    shared_keybd,
    flyby_buttn
};

struct state st_putt_stroke = {
    stroke_enter,
    stroke_leave,
    stroke_paint,
    stroke_timer,
    stroke_point,
    stroke_stick,
    NULL,
    shared_click_basic,
    stroke_keybd,
    stroke_buttn,
    stroke_wheel,
    stroke_touch,
    stroke_fade
};

struct state st_putt_roll = {
    roll_enter,
    roll_leave,
    roll_paint,
    roll_timer,
    NULL,
    NULL,
    NULL,
    NULL,
    shared_keybd,
    roll_buttn,
    NULL,
    NULL,
    shared_fade
};

struct state st_putt_goal = {
    goal_enter,
    shared_leave,
    goal_paint,
    goal_timer,
    shared_point,
    NULL,
    NULL,
    shared_click_basic,
    shared_keybd,
    goal_buttn,
    NULL,
    NULL,
    shared_fade
};

struct state st_putt_stop = {
    stop_enter,
    stop_leave,
    stop_paint,
    stop_timer,
    shared_point,
    NULL,
    NULL,
    shared_click_basic,
    shared_keybd,
    stop_buttn,
    NULL,
    NULL,
    shared_fade
};

struct state st_putt_fall = {
    fall_enter,
    fall_leave,
    fall_paint,
    fall_timer,
    shared_point,
    NULL,
    NULL,
    shared_click_basic,
    shared_keybd,
    fall_buttn,
    NULL,
    NULL,
    shared_fade
};

<<<<<<< HEAD
struct state st_putt_retry = {
    retry_enter,
    retry_leave,
    retry_paint,
    fall_timer,
    shared_point,
    NULL,
    NULL,
    shared_click_basic,
    shared_keybd,
    fall_buttn,
    NULL,
    NULL,
    shared_fade
};

=======
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
struct state st_putt_score = {
    score_enter,
    shared_leave,
    score_paint,
    shared_timer,
    shared_point,
    NULL,
    NULL,
    shared_click_basic,
    score_keybd,
    score_buttn,
    NULL,
    NULL,
    shared_fade
};

struct state st_putt_over = {
    over_enter,
    over_leave,
    over_paint,
    shared_timer,
    shared_point,
    NULL,
    NULL,
    shared_click_basic,
    over_keybd,
    over_buttn,
    NULL,
    NULL,
    shared_fade
};

struct state st_putt_pause = {
    pause_enter,
    pause_leave,
    pause_paint,
    shared_timer,
    shared_point,
    shared_stick,
    NULL,
    pause_click,
    pause_keybd,
    pause_buttn,
    NULL,
    NULL,
    shared_fade
};
