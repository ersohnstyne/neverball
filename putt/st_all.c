/*
 * Copyright (C) 2023 Microsoft / Neverball authors
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

#include "console_control_gui.h"
#include "account.h"
#include "networking.h"

#include "joy.h"
#include "hud.h"
#include "geom.h"
#include "gui.h"
#include "vec3.h"
#include "game.h"
#include "hole.h"
#include "audio.h"
#include "course.h"
#include "config.h"
#include "video.h"

#include "st_all.h"
#include "st_conf.h"

#define MODERN_PUTT_HUD

/*---------------------------------------------------------------------------*/

struct state st_help;
struct state st_course;
struct state st_party;
struct state st_controltype;
struct state st_poser;
struct state st_flyby;
struct state st_stroke;
struct state st_roll;
struct state st_goal;
struct state st_stop;
struct state st_fall;
struct state st_retry;
struct state st_score;
struct state st_over;
struct state st_pause;

/*---------------------------------------------------------------------------*/

static int course_ranks;

static int hole_hilited;
static int stroke_type;
static int play_id;

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

                        if (curr_hole() == number(i) && hole_hilited)
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

                        if (curr_hole() == number(i) && hole_hilited)
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

static int player_ranks(const char  *title,
                        const GLubyte *c0,
                        const GLubyte *c1)
{
    int id, jd, kd;

    struct {
        char name      [4][MAXSTR];
        int  score     [4];
        int  score_high[4];
    } tmp_score_rank;

    int i;

    if ((id = gui_harray(0)))
    {
        gui_layout(id, 0, 0);
    }

    return id;
}

/*---------------------------------------------------------------------------*/

static void shared_leave(struct state *st, struct state *next, int id)
{
    gui_delete(id);

    if (next == &st_null)
        course_free();
}

static void shared_timer(int id, float dt)
{
    gui_timer(id, dt);
}

static void shared_point(int id, int x, int y, int dx, int dy)
{
    xbox_toggle_gui(0);
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
    xbox_toggle_gui(1);

    if (paused_indiv_ctrl_index != -1 || joy_get_active_cursor(0))
        shared_stick_basic(id, a, v, bump);
}

static void shared_fade(float alpha)
{
    xbox_control_gui_set_alpha(alpha);
}

static int shared_back_button(int pd)
{
    int id;

    if ((id = gui_hstack(pd)))
    {
        gui_label(id, GUI_CROSS, GUI_SML, gui_red, gui_red);
        gui_label(id, _("Back"), GUI_SML, gui_wht, gui_wht);

        gui_set_state(id, -1, 0);
        gui_set_rect(id, GUI_ALL);
    }

    return id;
}

/*---------------------------------------------------------------------------*/

enum
{
    TITLE_PLAY = 1,
    TITLE_HELP,
    TITLE_CONF,
    TITLE_EXIT
};

static int title_action(int i)
{
    PUTT_GAMEMENU_ACTION(-1);

    switch (i)
    {
        case TITLE_PLAY: return goto_state(&st_course);
        case TITLE_HELP: return goto_state(&st_help);
        case TITLE_CONF: return goto_state(&st_conf);
        case TITLE_EXIT:
            if (current_platform != PLATFORM_SWITCH)
            {
                /* bye! */
                course_free();
                return 0;
            }
    }
    return 1;
}

static int party_indiv_controllers = 0;

static int gamepadinfo_id = 0;
static int gamepadinfo_controller_ids[4];

static int title_enter(struct state *st, struct state *prev)
{
#if defined(__EMSCRIPTEN__)
    EM_ASM({
        Neverputt.isTitleScreen = true;
        });
#endif

    if (party_indiv_controllers)
    {
        joy_active_cursor(0, 1);
        joy_active_cursor(1, 0);
        joy_active_cursor(2, 0);
        joy_active_cursor(3, 0);
    }

    int id, jd, kd;

    /* Build the title GUI. */

    if ((id = gui_vstack(0)))
    {
        char os_env[MAXSTR];
#if ENABLE_HMD
        sprintf(os_env, _("%s Edition"), "OpenHMD");
#else
        if (current_platform == PLATFORM_PC)
        {
#ifdef __linux__
            sprintf(os_env, _("%s Edition"), TITLE_PLATFORM_CINMAMON);
#else
            sprintf(os_env, _("%s Edition"), TITLE_PLATFORM_WINDOWS);
#endif
        }
        else if (current_platform == PLATFORM_XBOX)
        {
#if NEVERBALL_FAMILY_API == NEVERBALL_XBOX_360_FAMILY_API
            sprintf(os_env, _("%s Edition"), TITLE_PLATFORM_XBOX_360);
#else
            sprintf(os_env, _("%s Edition"), TITLE_PLATFORM_XBOX_ONE);
#endif
        }
        else if (current_platform == PLATFORM_PS)
            sprintf(os_env, _("%s Edition"), TITLE_PLATFORM_PS);
        else if (current_platform == PLATFORM_SWITCH)
            sprintf(os_env, _("%s Edition"), TITLE_PLATFORM_SWITCH);
#endif

        if ((jd = gui_vstack(id)))
        {
            gui_title_header(jd, "  Neverputt  ", GUI_LRG, 0, 0);
#if NB_STEAM_API==1
            gui_label(jd, _("Steam Valve Edition"), GUI_SML, GUI_COLOR_WHT);
#else
            gui_label(jd, os_env, GUI_SML, GUI_COLOR_WHT);
#endif
            gui_set_rect(jd, GUI_ALL);
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

                gui_state(kd, gt_prefix("menu^Help"),    GUI_MED, TITLE_HELP, 0);
                gui_state(kd, gt_prefix("menu^Options"), GUI_MED, TITLE_CONF, 0);

                /* Comment it, if you avoid quit the game */
                //gui_state(kd, gt_prefix("menu^Exit"),    GUI_MED, TITLE_EXIT, 0);

                /* Hilight the start button. */

                gui_set_hilite(play_id, 1);
            }

            gui_filler(jd);
        }
        gui_layout(id, 0, 0);
    }

    /* Build the gamepad GUI. */

    if (gamepadinfo_id = gui_hstack(0))
    {
        gamepadinfo_controller_ids[3] = gui_label(gamepadinfo_id, _("P4 XXX"), GUI_SML, gui_gry, gui_gry);
        gamepadinfo_controller_ids[2] = gui_label(gamepadinfo_id, _("P3 XXX"), GUI_SML, gui_gry, gui_gry);
        gamepadinfo_controller_ids[1] = gui_label(gamepadinfo_id, _("P2 XXX"), GUI_SML, gui_gry, gui_gry);
        gamepadinfo_controller_ids[0] = gui_label(gamepadinfo_id, _("P1 XXX"), GUI_SML, gui_wht, gui_red);

        gui_layout(gamepadinfo_id, 0, 1);
        gui_set_rect(gamepadinfo_id, GUI_S);
    }

    course_init();
    course_rand();

    return id;
}

static void title_leave(struct state *st, struct state *next, int id)
{
#if defined(__EMSCRIPTEN__)
    EM_ASM({
        Neverputt.isTitleScreen = false;
        });
#endif

    gui_delete(gamepadinfo_id);
    gui_delete(id);

    if (next == &st_conf)
    {
        /*
         * This is ugly, but better than stupidly deleting stuff using
         * object names from a previous GL context.
         */
        course_free();
    }
}

static void title_paint(int id, float t)
{
    game_draw(0, t);
    gui_paint(id);
    gui_paint(gamepadinfo_id);

    xbox_control_title_gui_paint();
}

static void title_timer(int id, float dt)
{
    float g[3] = { 0.f, 0.f, 0.f };

    game_step(g, dt);
    game_set_fly(fcosf(V_PI * time_state() / 10.f)); // Default is 10 seconds; Default methods: fcosf(time_state() / 2.5f)

    int battery_level, gamepad_wired; 

    if (joy_connected(0, &battery_level, &gamepad_wired) &&
        gamepadinfo_controller_ids[0])
    {
        if (gamepad_wired)
            gui_set_label(gamepadinfo_controller_ids[0], _("P1 " GUI_GAMEPAD));
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

    gui_timer(id, dt);
}

static int title_click(int b, int d)
{
    return gui_click(b, d) ? title_action(gui_token(gui_active())) : 1;
}

static int title_keybd(int c, int d)
{
    if (d && c == KEY_EXIT)
        return title_action(TITLE_EXIT);

    return 1;
}

static int title_buttn(int b, int d)
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

static void title_fade(float alpha)
{
    xbox_control_gui_set_alpha(alpha);
    gui_set_alpha(gamepadinfo_id, alpha, GUI_ANIMATION_N_CURVE);
}

/*---------------------------------------------------------------------------*/

#define HELP_BACK -1

static int help_action(int i)
{
    PUTT_GAMEMENU_ACTION(HELP_BACK);

    switch (i)
    {
        case HELP_BACK: return goto_state(&st_title);
    }
    return 1;
}

static int help_enter(struct state *st, struct state *prev)
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
            //gui_start(jd, _("Back"), GUI_SML, HELP_BACK, 0);
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

    return id;
}

static void help_paint(int id, float t)
{
    game_draw(0, t);
    gui_paint(id);

    xbox_control_list_gui_paint();
}

static void help_point(int id, int x, int y, int dx, int dy)
{
    xbox_toggle_gui(0);
    int jd;

    if ((jd = gui_point(id, x, y)))
    {
        int i = gui_token(jd);

        gui_pulse(jd, 1.2f);
    }
}

static void help_stick(int id, int a, float v, int bump)
{
    xbox_toggle_gui(1);
    int jd;

    if (joy_get_cursor_actions(0))
    {
        if ((jd = shared_stick_basic(id, a, v, bump)))
        {
            int i = gui_token(jd);

            gui_pulse(jd, 1.2f);
        }
    }
}

static int help_click(int b, int d)
{
    return gui_click(b, d) ? help_action(gui_token(gui_active())) : 1;
}

static int help_keybd(int c, int d)
{
    if (d && c == KEY_EXIT)
        return help_action(HELP_BACK);

    return 1;
}

static int help_buttn(int b, int d)
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

/*---------------------------------------------------------------------------*/

static int desc_id;
static int shot_id;

#define COURSE_GETONLINE -2
#define COURSE_BACK -1

static int course_action(int i)
{
    PUTT_GAMEMENU_ACTION(COURSE_BACK);

    if (i == COURSE_GETONLINE)
    {
#if NB_STEAM_API==1
#elif defined(__EMSCRIPTEN__)
        EM_ASM({ window.open("https://drive.google.com/drive/folders/1YJzHckEBn15rNemvFy56Ig5skY3KX3Rp"); }, 0);
#elif _WIN32
        system("start msedge https://drive.google.com/drive/folders/1YJzHckEBn15rNemvFy56Ig5skY3KX3Rp");
#elif defined(__APPLE__)
        system("open https://drive.google.com/drive/folders/1YJzHckEBn15rNemvFy56Ig5skY3KX3Rp");
#elif defined(__linux__)
        system("x-www-browser https://drive.google.com/drive/folders/1YJzHckEBn15rNemvFy56Ig5skY3KX3Rp");
#endif
    }
    if (course_exists(i))
    {
        course_goto(i);
        goto_state(&st_party);
    }
    if (i == COURSE_BACK)
        goto_state(&st_title);

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

static int course_enter(struct state *st, struct state *prev)
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
#if !defined(__EMSCRIPTEN__)
            if (current_platform == PLATFORM_PC)
#endif
                shared_back_button(jd);

            if (server_policy_get_d(SERVER_POLICY_EDITION) >= 0)
            {
#if NB_STEAM_API==1
                gui_state(id, _("Get Courses from Steam Workshop!"), GUI_SML, COURSE_GETONLINE, 0);
#else
                gui_state(id, _("Get Courses from Website!"), GUI_SML, COURSE_GETONLINE, 0);
#endif
            }

            gui_layout(id, 0, 0);
        }
        return id;
    }

    r = comp_rows(n);
    c = comp_cols(n);

    if ((id = gui_vstack(0)))
    {
        // This one is out of date
        /*gui_label(id, _("Select Course"), GUI_MED, 0, 0);*/

        // This one is now syncronized
        if ((jd = gui_hstack(id)))
        {
            gui_label(jd, _("Select Course"), GUI_SML, 0, 0);
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
            {
                for (i = 0; i < r; i++)
                {
                    if ((ld = gui_harray(kd)))
                    {
                        for (j = c - 1; j >= 0; j--)
                        {
                            int k = i * c + j;

                            if (k < n)
                            {
                                md = gui_image(ld, course_shot(k), ww / c, hh / c);
                                gui_set_state(md, k, 0);

                                if (k == 0)
                                    gui_focus(md);
                            }
                            else
                                gui_space(ld);
                        }
                    }
                }
            }
        }

        gui_space(id);
        desc_id = gui_multi(id, _(course_desc(0)), GUI_SML, gui_yel, gui_wht);
        gui_space(id);

        // This one is out of date
        /*if ((jd = gui_hstack(id)))
        {
            gui_filler(jd);
            shared_back_button(jd);
        }*/

        gui_layout(id, 0, 0);
    }

    audio_music_fade_to(0.5f, _("bgm/title.ogg"));

    return id;
}

static void course_paint(int id, float t)
{
    game_draw(0, t);
    gui_paint(id);

    xbox_control_list_gui_paint();
}

static void course_point(int id, int x, int y, int dx, int dy)
{
    xbox_toggle_gui(0);
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
    xbox_toggle_gui(1);
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
        case PARTY_1:
            if (hole_goto(1, 1))
                goto_state(&st_next);
            break;
        case PARTY_2:
            holdage_player_count = 2;
            goto_state(&st_controltype);
            break;
        case PARTY_3:
            holdage_player_count = 3;
            goto_state(&st_controltype);
            break;
        case PARTY_4:
            holdage_player_count = 4;
            goto_state(&st_controltype);
            break;
        case PARTY_B:
            goto_state(&st_course);
            break;
    }
    return 1;
}

static int party_enter(struct state *st, struct state *prev)
{
    int id, jd;

    holdage_player_count = 1;
    party_indiv_controllers = 0;

    if ((id = gui_vstack(0)))
    {
        // This one is out of date
        /*gui_label(id, _("Players?"), GUI_MED, 0, 0);*/

        // This one is now syncronized
        if ((jd = gui_hstack(id)))
        {
            gui_label(jd, _("Players?"), GUI_SML, 0, 0);
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

        // This one is out of date
        /*if ((jd = gui_hstack(id)))
        {
            gui_filler(jd);
            shared_back_button(jd);
        }*/

        gui_layout(id, 0, 0);
    }

    return id;
}

static void party_paint(int id, float t)
{
    game_draw(0, t);
    gui_paint(id);

    xbox_control_list_gui_paint();
}

static int party_click(int b, int d)
{
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
                return goto_state(&st_next);
            break;
        case CONTROLTYPE_B:
            return goto_state(&st_course);
        default: return 1;
    }

    return 1;
}

static int controltype_enter(struct state *st, struct state *prev)
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

    return id;
}

static void controltype_paint(int id, float t)
{
    game_draw(0, t);
    gui_paint(id);

    xbox_control_list_gui_paint();
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
    return gui_click(b, d) ? party_action(gui_token(gui_active())) : 1;
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

enum
{
    PAUSE_CONTINUE = 1,
    PAUSE_QUIT,
    PAUSE_SKIP,
    PAUSE_RESHOT
};

int goto_pause(int use_keybd)
{
    if (curr_state() == &st_pause)
        return 1;

    if (use_keybd)
        paused_indiv_ctrl_index = 0;
    else if (party_indiv_controllers)
        paused_indiv_ctrl_index = joy_get_gamepad_action_index();

    st_continue = curr_state();
    st_quit = &st_over;
    paused = 1;

    return goto_state(&st_pause);
}

static int pause_action(int i)
{
    PUTT_GAMEMENU_ACTION(-1);

    switch(i)
    {
    case PAUSE_CONTINUE:
        return goto_state(st_continue ? st_continue : &st_title);

    case PAUSE_RESHOT:
        return goto_state(&st_retry);

    case PAUSE_SKIP:
        hole_skip();
        return goto_state(&st_score);

    case PAUSE_QUIT:
        stroke_type = 0;
        stroke_set_type(0);
        return goto_state(st_quit);
    }
    return 1;
}

static int stroke_allowed = 0;

static int pause_enter(struct state *st, struct state *prev)
{
    int pdid;
    int id, jd, td;

    audio_music_fade_out(0.2f);

    if ((id = gui_vstack(0)))
    {
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
            gui_state(jd, _("Quit"), GUI_SML, PAUSE_QUIT, 0);
            gui_state(jd, _("Skip"), GUI_SML, PAUSE_SKIP, 0);

            if (!stroke_allowed && hole_retry_avail(stroke_allowed))
                gui_state(jd, _("Retry"), GUI_SML, PAUSE_RESHOT, 0);

            gui_start(jd, _("Continue"), GUI_SML, PAUSE_CONTINUE, 1);
        }

        gui_pulse(td, 1.2f);
        gui_layout(id, 0, 0);
    }
    
    hud_init();
    return id;
}

static void pause_leave(struct state *st, struct state *next, int id)
{
    paused_indiv_ctrl_index = -1;
    gui_delete(id);
    hud_free();
    audio_music_fade_in(0.5f);
}

static void pause_paint(int id, float t)
{
    game_draw(0, t);
    gui_paint(id);
    hud_paint();
}

static int pause_click(int b, int d)
{
    return gui_click(b, d) ? pause_action(gui_token(gui_active())) : 1;
}

static int pause_keybd(int c, int d)
{
    if (d && c == KEY_EXIT)
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
    if (d && (c == KEY_EXIT))
        return goto_pause(1);

    return 1;
}

/*---------------------------------------------------------------------------*/

static int num = 0;
static int next_show_scorecard = 0;

static int next_enter(struct state *st, struct state *prev)
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
        gui_title_header(id, str, GUI_MED, 0, 0);

        if (curr_party() > 1)
        {
            audio_play(AUD_NEXTTURN, 1.f);

            gui_space(id);

            if ((jd = gui_vstack(id)))
            {
                gui_label(jd, _("Player"), GUI_SML, 0, 0);

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
    
    if (paused)
        paused = 0;

    game_set_fly(1.f);

    return id;
}

static void next_paint(int id, float t)
{
    game_draw(0, t);
    hud_paint();
    gui_paint(id);
}

static int next_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_POSE)
            return goto_state(&st_poser);
        if (c == KEY_EXIT)
            return goto_pause(1);

        if (config_tst_d(CONFIG_KEY_SCORE_NEXT, c))
        {
            next_show_scorecard = 1;
            return goto_state(&st_poser);
        }

#ifndef NDEBUG
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
                    return goto_state(&st_next);
                }
                else
                {
                    num = 0;
                    return 1;
                }
            }
#endif
            return goto_state(&st_flyby);
        }

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_X, b))
        {
            next_show_scorecard = 1;
            return goto_state(&st_poser);
        }
    }

    if (d && config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
        return goto_pause(0);

    return 1;
}

/*---------------------------------------------------------------------------*/

static int poser_enter(struct state *st, struct state *prev)
{
    if (!next_show_scorecard)
    {
        game_set_fly(-1.f);
        return 0;
    }

    return score_card(_("Scores"), GUI_COLOR_DEFAULT);
}

static void poser_paint(int id, float t)
{
    game_draw(1, t);
}

static int poser_keybd(int c, int d)
{
    if (d && c == KEY_EXIT)
        return goto_state(&st_next);

    return 1;
}

static int poser_buttn(int b, int d)
{
    if (d &&
        (joy_get_cursor_actions(curr_player() - 1) || !party_indiv_controllers))
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b)
         || config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return goto_state(&st_next);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int flyby_done = 0;

static int flyby_enter(struct state *st, struct state *prev)
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

    video_hide_cursor();

    flyby_done = 0;

    return 0;
}

static void flyby_leave(struct state *st, struct state *next, int id)
{
    video_show_cursor();

    flyby_done = 1;
}

static void flyby_paint(int id, float t)
{
    game_draw(0, t);
}

static void flyby_timer(int id, float dt)
{
    float t = time_state();

    if (dt > 0.f && t > 1.f && !flyby_done)
    {
        flyby_done = 1;
        goto_state(&st_stroke);
    }
    else if (!flyby_done)
        game_set_fly(1.f - t);

    gui_timer(id, dt);
}

static int flyby_buttn(int b, int d)
{
    if (d &&
        (joy_get_cursor_actions(curr_player() - 1) || !party_indiv_controllers))
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
        {
            game_set_fly(0.f);
            return goto_state(&st_stroke);
        }
    }

    if (d && config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
        return goto_pause(0);

    return 1;
}

/*---------------------------------------------------------------------------*/

static int stroke_rotate = 0;
static int stroke_rotate_alt = 0;
static int stroke_mag = 0;
static int stroke_type = 0;

static int stroke_enter(struct state *st, struct state *prev)
{
    if (party_indiv_controllers)
    {
        joy_active_cursor(curr_player() - 1, curr_player() == 1);
        joy_active_cursor(curr_player() - 1, curr_player() == 2);
        joy_active_cursor(curr_player() - 1, curr_player() == 3);
        joy_active_cursor(curr_player() - 1, curr_player() == 4);
    }

    stroke_allowed = 1;
    hud_init();
    game_clr_mag();
    config_set_d(CONFIG_CAMERA, 2);
    video_set_grab(1);

    if (paused)
        paused = 0;

    audio_music_fade_in(0.5f);

    return 0;
}

static void stroke_leave(struct state *st, struct state *next, int id)
{
    hud_free();
    video_clr_grab();
    config_set_d(CONFIG_CAMERA, 0);
    stroke_rotate = 0.0f;
}

static void stroke_paint(int id, float t)
{
    game_draw(0, t);
    hud_paint();

    xbox_control_putt_stroke_gui_paint();
}

static void stroke_timer(int id, float dt)
{
    /* HACK: Used with realtime gravity? Yes, it's real! */

    float g[3] = { 0.f, -9.8f, 0.f };

    float k;

    if (SDL_GetModState() & KMOD_SHIFT || stroke_rotate_alt)
        k = 0.25;
    else
        k = 1.0;

    if (dt <= 0.f) return;

    game_set_rot(stroke_rotate * k);
    game_set_mag(stroke_mag * k);

    game_update_view(dt);

    switch (game_step(g, dt))
    {
        case GAME_FALL: goto_state(&st_fall); break;
    }
}

static void stroke_point(int id, int x, int y, int dx, int dy)
{
    xbox_toggle_gui(0);
    game_set_rot(dx);
    game_set_mag(dy);
}

static void stroke_stick(int id, int a, float v, int bump)
{
    xbox_toggle_gui(1);

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
    if (d)
    {
        if (c == KEY_EXIT)
            return goto_pause(1);
        if (c == KEY_PUTT_UPGRADE && stroke_type < 3)
        {
            stroke_type++;
            stroke_set_type(stroke_type);
            game_set_mag(0);
            return goto_state_full(&st_stroke, 0, 0, 1);
        }
        if (c == KEY_PUTT_DNGRADE && stroke_type > 0)
        {
            stroke_type--;
            stroke_set_type(stroke_type);
            game_set_mag(0);
            return goto_state_full(&st_stroke, 0, 0, 1);
        }
    }
    return 1;
}

static int stroke_buttn(int b, int d)
{
    if (d &&
        (joy_get_cursor_actions(curr_player() - 1) || !party_indiv_controllers))
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_X, b))
            stroke_rotate_alt = 1;
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
        {
            game_putt();
            return goto_state(&st_roll);
        }

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L1, b) && stroke_type < 3)
        {
            stroke_type++;
            stroke_set_type(stroke_type);
            game_set_mag(0);
            return goto_state_full(&st_stroke, 0, 0, 1);
        }
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L2, b) && stroke_type > 0)
        {
            stroke_type--;
            stroke_set_type(stroke_type);
            game_set_mag(0);
            return goto_state_full(&st_stroke, 0, 0, 1);
        }
    }
    else if (joy_get_cursor_actions(curr_player() - 1) || !party_indiv_controllers)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_X, b))
            stroke_rotate_alt = 0;
    }

    if (d && config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
        return goto_pause(0);

    return 1;
}

static void stroke_fade(float alpha)
{
    xbox_control_gui_set_alpha(alpha);
    hud_set_alpha(alpha);
}

static void stroke_wheel(int x, int y)
{
    if (joy_get_cursor_actions(curr_player() - 1) || !party_indiv_controllers)
    {
        if (y > 0 && stroke_type < 3)
        {
            stroke_type++;
            stroke_set_type(stroke_type);
            game_set_mag(0);
            return goto_state_full(&st_stroke, 0, 0, 1);
        }
        if (y < 0 && stroke_type > 0)
        {
            stroke_type--;
            stroke_set_type(stroke_type);
            game_set_mag(0);
            return goto_state_full(&st_stroke, 0, 0, 1);
        }
    }
}

/*---------------------------------------------------------------------------*/

static int roll_enter(struct state *st, struct state *prev)
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

    if (paused)
        paused = 0;

    return 0;
}

static void roll_leave(struct state *st, struct state *next, int id)
{
    video_show_cursor();
}

static void roll_paint(int id, float t)
{
    game_draw(0, t);
}

static void roll_timer(int id, float dt)
{
    float g[3] = { 0.0f, -9.8f, 0.0f };

    if (dt <= 0.f) return;

    switch (game_step(g, dt))
    {
        case GAME_STOP: goto_state(&st_stop); break;
        case GAME_GOAL: goto_state(&st_goal); break;
        case GAME_FALL: goto_state(&st_fall); break;
    }
}

static int roll_buttn(int b, int d)
{
    if (d && config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
        return goto_pause(0);

    return 1;
}

/*---------------------------------------------------------------------------*/

static int goal_enter(struct state *st, struct state *prev)
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
        sprintf(hole_statname, "+%d", scr_v - par_v);
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

    return id;
}

static void goal_paint(int id, float t)
{
    game_draw(0, t);
    gui_paint(id);

    xbox_control_putt_stop_gui_paint();
}

static void goal_timer(int id, float dt)
{
    if (time_state() > 3 && !st_global_animating())
    {
        if (hole_next())
            goto_state(&st_next);
        else
            goto_state(&st_score);
    }
}

static int goal_buttn(int b, int d)
{
    if (d &&
        (joy_get_cursor_actions(curr_player() - 1) || !party_indiv_controllers))
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
        {
            if (hole_next())
                goto_state(&st_next);
            else
                goto_state(&st_score);
        }
    }

    if (d && config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
        return goto_pause(0);

    return 1;
}

/*---------------------------------------------------------------------------*/

static int stop_enter(struct state *st, struct state *prev)
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

    return 0;
}

static void stop_leave(struct state *st, struct state *next, int id)
{
}

static void stop_paint(int id, float t)
{
    game_draw(0, t);

    xbox_control_putt_stop_gui_paint();
}

static void stop_timer(int id, float dt)
{
    float g[3] = { 0.f, -9.8f, 0.f };

    if (dt <= 0) return;

    game_update_view(dt);
    game_step(g, dt);

    if (time_state() > 1 && !st_global_animating())
    {
        if (hole_next())
            goto_state(curr_party() > 1 ? &st_next : &st_stroke);
        else
            goto_state(&st_score);
    }
}

static int stop_buttn(int b, int d)
{
    if (d &&
        (joy_get_cursor_actions(curr_player() - 1) || !party_indiv_controllers))
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
        {
            if (hole_next())
                goto_state(curr_party() > 1 ? &st_next : &st_stroke);
            else
                goto_state(&st_score);
        }
    }

    if (d && config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
        return goto_pause(0);

    return 1;
}

/*---------------------------------------------------------------------------*/

static int fall_enter(struct state *st, struct state *prev)
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

    stroke_allowed = 1;

    return id;
}

static void fall_leave(struct state *st, struct state *next, int id)
{
    gui_delete(id);
}

static void fall_paint(int id, float t)
{
    game_draw(0, t);
    gui_paint(id);

    xbox_control_putt_stop_gui_paint();
}

static void fall_timer(int id, float dt)
{
    if (time_state() > (curr_party() > 1 ? 3 : 1) && !st_global_animating())
    {
        if (hole_next())
            goto_state(curr_party() > 1 ? &st_next : &st_stroke);
        else
            goto_state(&st_score);
    }

    if (curr_party() == 1)
        game_set_fly(1.f - time_state());

    gui_timer(id, dt);
}

static int fall_buttn(int b, int d)
{
    if (d &&
        (joy_get_cursor_actions(curr_player() - 1) || !party_indiv_controllers))
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
        {
            if (hole_next())
                goto_state(curr_party() > 1 ? &st_next : &st_stroke);
            else
                goto_state(&st_score);
        }
    }

    if (d && config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
        return goto_pause(0);

    return 1;
}

/*---------------------------------------------------------------------------*/

static int retry_enter(struct state *st, struct state *prev)
{
    int id;

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

    return id;
}

static void retry_leave(struct state *st, struct state *next, int id)
{
    gui_delete(id);
}

static void retry_paint(int id, float t)
{
    game_draw(0, t);
    gui_paint(id);

    xbox_control_putt_stop_gui_paint();
}

/*---------------------------------------------------------------------------*/

static int score_enter(struct state *st, struct state *prev)
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

    return score_card(_("Scores"), GUI_COLOR_DEFAULT);
}

static void score_paint(int id, float t)
{
    game_draw(0, t);
    gui_paint(id);

    xbox_control_putt_scores_gui_paint();
}

static int score_keybd(int c, int d)
{
    if (d &&
        (joy_get_cursor_actions(0) || !party_indiv_controllers))
    {
        if (c == KEY_EXIT)
            return goto_pause(1);
    }
    return 1;
}

static int score_buttn(int b, int d)
{
    if (d &&
        (joy_get_cursor_actions(0) || !party_indiv_controllers))
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            goto_state(hole_move() ? &st_next : &st_over);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return goto_pause(0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int over_enter(struct state *st, struct state *prev)
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
    return score_card(_("Final Scores"), GUI_COLOR_DEFAULT);
}

static void over_leave(struct state *st, struct state *next, int id)
{
    party_indiv_controllers = 0;

    gui_delete(id);
}

static void over_paint(int id, float t)
{
    game_draw(0, t);
    gui_paint(id);

    xbox_control_putt_scores_gui_paint();
}

static int over_keybd(int c, int d)
{
    if (d &&
        (joy_get_cursor_actions(0) || !party_indiv_controllers))
    {
        if (c == KEY_EXIT)
            return goto_state(&st_title);
    }
    return 1;
}

static int over_buttn(int b, int d)
{
    if (d &&
        (joy_get_cursor_actions(0) || !party_indiv_controllers))
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return goto_state(&st_title);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return goto_state(&st_title);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

struct state st_title = {
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
    title_fade
};

struct state st_help = {
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

struct state st_course = {
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

struct state st_party = {
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

struct state st_controltype = {
    controltype_enter,
    shared_leave,
    controltype_paint,
    controltype_timer,
    shared_point,
    shared_stick,
    NULL,
    controltype_click,
    controltype_keybd,
    controltype_buttn
};

struct state st_next = {
    next_enter,
    shared_leave,
    next_paint,
    shared_timer,
    shared_point,
    shared_stick,
    NULL,
    shared_click_basic,
    next_keybd,
    next_buttn
};

struct state st_poser = {
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

struct state st_flyby = {
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

struct state st_stroke = {
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
    NULL,
    stroke_fade
};

struct state st_roll = {
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

struct state st_goal = {
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

struct state st_stop = {
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

struct state st_fall = {
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

struct state st_retry = {
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

struct state st_score = {
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

struct state st_over = {
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

struct state st_pause = {
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
