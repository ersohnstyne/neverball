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

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
#include "console_control_gui.h"
#endif

#if NB_HAVE_PB_BOTH==1
#include "account.h"
#include "networking.h"
#endif

#include "gui.h"
#include "state.h"
#include "array.h"
#include "dir.h"
#include "config.h"
#include "fs.h"
#include "common.h"
#include "ball.h"
#include "cmd.h"
#include "audio.h"
#include "geom.h"
#include "video.h"
#include "demo.h"
#include "progress.h"

#include "game_server.h"
#include "game_proxy.h"
#include "game_client.h"
#include "game_common.h"

#include "st_ball.h"
#include "st_conf.h"
#include "st_shared.h"
#include "st_setup.h"

#if _DEBUG && _MSC_VER
#ifndef _CRTDBG_MAP_ALLOC
#pragma message(__FILE__": Missing CRT-Debugger include header, recreate: crtdbg.h")
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

#if NB_HAVE_PB_BOTH==1
int super_environment = 1;
#endif

int model_studio = 0;

/*---------------------------------------------------------------------------*/

#if NB_HAVE_PB_BOTH==1
struct state *setup_finish_state;

int goto_ball_setup(struct state *finish)
{
    setup_finish_state = finish;

    return goto_state(&st_ball);
}
#endif

/*---------------------------------------------------------------------------*/

static int ball_manual_hotreload = 0;

static Array balls;
static int   curr_ball;
static char  ball_file[64];

static int   name_id = 0;

static int has_ball_sols(struct dir_item *item)
{
    char *tmp_path = strdup(item->path);
    char *solid, *inner, *outer;
    int yes = 0;

    solid = concat_string(tmp_path,
                          "/",
                          base_name(tmp_path),
                          "-solid.sol",
                          NULL);
    inner = concat_string(tmp_path,
                          "/",
                          base_name(tmp_path),
                          "-inner.sol",
                          NULL);
    outer = concat_string(tmp_path,
                          "/",
                          base_name(tmp_path),
                          "-outer.sol",
                          NULL);

    yes = (fs_exists(solid) || fs_exists(inner) || fs_exists(outer));

    free(solid); solid = NULL;
    free(inner); inner = NULL;
    free(outer); outer = NULL;

    free(tmp_path); tmp_path = NULL;

    return yes;
}

static int cmp_dir_items(const void *A, const void *B)
{
    const struct dir_item *a = A, *b = B;
    return strcmp(a->path, b->path);
}

static void scan_balls(void)
{
    int i;

#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT) && defined(CONFIG_INCLUDES_MULTIBALLS)
    switch (ball_multi_curr())
    {
        case 0:  SAFECPY(ball_file, account_get_s(ACCOUNT_BALL_FILE_LL)); break;
        case 1:  SAFECPY(ball_file, account_get_s(ACCOUNT_BALL_FILE_L));  break;
        case 2:  SAFECPY(ball_file, account_get_s(ACCOUNT_BALL_FILE_C));  break;
        case 3:  SAFECPY(ball_file, account_get_s(ACCOUNT_BALL_FILE_R));  break;
        case 4:  SAFECPY(ball_file, account_get_s(ACCOUNT_BALL_FILE_RR)); break;
        default: SAFECPY(ball_file, account_get_s(ACCOUNT_BALL_FILE_C));
    }

    account_set_s(ACCOUNT_BALL_FILE, ball_file);
#elif NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT)
    SAFECPY(ball_file, account_get_s(ACCOUNT_BALL_FILE));
#else
    SAFECPY(ball_file, config_get_s(CONFIG_BALL_FILE));
#endif

    if ((balls = fs_dir_scan("ball", has_ball_sols)))
    {
        array_sort(balls, cmp_dir_items);

        for (i = 0; i < array_len(balls); i++)
        {
            const char *path = DIR_ITEM_GET(balls, i)->path;

            if (strncmp(ball_file, path, strlen(path)) == 0)
            {
                curr_ball = i;
                break;
            }
        }
    }
}

static void free_balls(void)
{
    fs_dir_free(balls);
    balls = NULL;
}

static void set_curr_ball(int ball_index)
{
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(ball_file, 64,
#else
    sprintf(ball_file,
#endif
            "%s/%s",
            DIR_ITEM_GET(balls, ball_index)->path,
            base_name(DIR_ITEM_GET(balls, ball_index)->path));

#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT) && defined(CONFIG_INCLUDES_MULTIBALLS)
    account_set_s(ACCOUNT_BALL_FILE, ball_file);

    switch (ball_multi_curr())
    {
        case 0:  account_set_s(ACCOUNT_BALL_FILE_LL, ball_file); break;
        case 1:  account_set_s(ACCOUNT_BALL_FILE_L,  ball_file); break;
        case 2:  account_set_s(ACCOUNT_BALL_FILE_C,  ball_file); break;
        case 3:  account_set_s(ACCOUNT_BALL_FILE_R,  ball_file); break;
        case 4:  account_set_s(ACCOUNT_BALL_FILE_RR, ball_file); break;
        default: account_set_s(ACCOUNT_BALL_FILE_C,  ball_file);
    }
#elif NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT)
    account_set_s(ACCOUNT_BALL_FILE, ball_file);
#else
    config_set_s(CONFIG_BALL_FILE, ball_file);
#endif

#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT) && defined(CONFIG_INCLUDES_MULTIBALLS)
    ball_multi_free();
    ball_multi_init();
#else
    ball_free();
    ball_init();
#endif
    config_save();
#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT)
    account_save();
#endif
    gui_set_label(name_id, _(base_name(ball_file)));
}

enum
{
    MODEL_ONLINE = GUI_LAST,
    MODEL_UPGRADE_EDITION,
    MODEL_SETUP_FINISH
};

static int ball_action(int tok, int val)
{
    if (game_setup_process() && tok == GUI_BACK)
    {
        audio_play(AUD_DISABLED, 1.0f);
        return 1;
    }

    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
#if !defined(__EMSCRIPTEN__)
        case MODEL_ONLINE:
#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT)
            if (account_get_d(ACCOUNT_PRODUCT_BALLS) == 1)
            {
#if defined(__EMSCRIPTEN__)
                EM_ASM({ window.open("https://drive.google.com/drive/folders/1jBX7QtFcg3w7KUlSaH25xp-5qHItmVUT");}, 0);
#elif _WIN32
                system("start msedge https://drive.google.com/drive/folders/1jBX7QtFcg3w7KUlSaH25xp-5qHItmVUT");
#elif defined(__APPLE__)
                system("open https://drive.google.com/drive/folders/1jBX7QtFcg3w7KUlSaH25xp-5qHItmVUT");
#elif defined(__linux__)
                system("x-www-browser https://drive.google.com/drive/folders/1jBX7QtFcg3w7KUlSaH25xp-5qHItmVUT");
#endif
            }
#endif
            break;

        case MODEL_UPGRADE_EDITION:
#if defined(__EMSCRIPTEN__)
            EM_ASM({ window.open("https://forms.office.com/r/upfWqaVVtA"); }, 0);
#elif _WIN32
            system("start msedge https://forms.office.com/r/upfWqaVVtA");
#elif defined(__APPLE__)
            system("open https://forms.office.com/r/upfWqaVVtA");
#elif defined(__linux__)
            system("x-www-browser https://forms.office.com/r/upfWqaVVtA");
#endif
            break;
#endif
        case GUI_NEXT:
            if (++curr_ball == array_len(balls))
                curr_ball = 0;

            set_curr_ball(curr_ball);
            break;

        case GUI_PREV:
            if (--curr_ball == -1)
                curr_ball = array_len(balls) - 1;

            set_curr_ball(curr_ball);
            break;

        case GUI_BACK:
            game_fade(+4.0);
#if NB_HAVE_PB_BOTH==1
            goto_state(&st_conf_account);
#else
            goto_state(&st_conf);
#endif
            break;

#if NB_HAVE_PB_BOTH==1
        case MODEL_SETUP_FINISH:
            game_fade(+4.0);
            goto_game_setup_finish(setup_finish_state);

            break;
#endif
    }

    return 1;
}

static int filter_cmd(const union cmd *cmd)
{
    return (cmd ? cmd->type != CMD_MAKE_ITEM : 1);
}

static void load_ball_demo(void)
{
    if (model_studio)
    {
        if (!game_client_init("gui/model-studio.sol"))
        {
            assert(!game_setup_process());

            ball_action(GUI_BACK, 0);
            return;
        }
        else
            game_client_toggle_show_balls(1);
    }
    else if (!progress_replay_full("gui/ball.nbr", 0, 0, 0, 0, 0, 0))
    {
        assert(!game_setup_process());

        ball_action(GUI_BACK, 0);
        return;
    }
    else
        game_client_toggle_show_balls(1);

#if NB_HAVE_PB_BOTH==1
    demo_replay_speed(super_environment ? SPEED_SLOWER : SPEED_NORMAL);
#endif
    game_client_fly(0);

    game_fade(-4.0);
    if (!config_get_d(CONFIG_SCREEN_ANIMATIONS))
        game_kill_fade();

#if NB_HAVE_PB_BOTH==1
    if (super_environment == 0)
#endif
        back_init("back/premium.png");
}

static int ball_gui(void)
{
    int root_id, id, jd;
    int i;

    if ((root_id = gui_root()))
    {
        if ((id = gui_vstack(root_id)))
        {
            gui_space(id);

            if ((jd = gui_hstack(id)))
            {
                gui_label (jd, _("Ball Model"), GUI_SML, 0, 0);
                gui_filler(jd);
                gui_space (jd);

#if NB_HAVE_PB_BOTH==1
#ifndef __EMSCRIPTEN__
                if (current_platform != PLATFORM_PC)
                    gui_filler(jd);
                else
#endif
                if (!game_setup_process())
                    gui_back_button(jd);
#else
                gui_back_button(jd);
#endif
            }

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            if (!xbox_show_gui())
#endif
                gui_space(id);

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            const char *more_balls_text = server_policy_get_d(SERVER_POLICY_EDITION) > -1 ?
#if NB_STEAM_API==1
                                          N_("Open Steam Workshop!") :
#else
                                          N_("Get more Balls!") :
#endif
                                          N_("Upgrade to Home Edition!");

            if ((account_get_d(ACCOUNT_PRODUCT_BALLS) == 1 ||
                 server_policy_get_d(SERVER_POLICY_EDITION) < 0) &&
                !xbox_show_gui() &&
                !game_setup_process())
            {
                int online_id;
                if (online_id = gui_label(id, _(more_balls_text),
                                              GUI_SML, gui_wht, gui_grn))
                {
                    if (server_policy_get_d(SERVER_POLICY_EDITION) == -1)
                        gui_set_state(online_id, MODEL_UPGRADE_EDITION, 0);
                    else
                        gui_set_state(online_id, MODEL_ONLINE, 0);
                }
            }

            else if (!xbox_show_gui() && !game_setup_process())
                gui_label(id, _(more_balls_text), GUI_SML, GUI_COLOR_GRY);

            gui_space(id);
#endif

            if ((jd = gui_hstack(id)))
            {
                int total_ball_name = array_len(balls);

#ifndef __EMSCRIPTEN__
                if (!xbox_show_gui() && total_ball_name > 1)
#else
                if (total_ball_name > 1)
#endif
#ifdef SWITCHBALL_GUI
                    gui_maybe_img(jd, "gui/navig/arrow_right_disabled.png",
                                      "gui/navig/arrow_right.png",
                                      GUI_NEXT, GUI_NONE, 1);
#else
                    gui_maybe(jd, GUI_TRIANGLE_RIGHT, GUI_NEXT, GUI_NONE, 1);
#endif

                name_id = gui_label(jd, "very-long-ball-name", GUI_SML,
                                        GUI_COLOR_WHT);

                gui_set_trunc(name_id, TRUNC_TAIL);
                gui_set_fill (name_id);

#ifndef __EMSCRIPTEN__
                if (!xbox_show_gui() && total_ball_name > 1)
#else
                if (total_ball_name > 1)
#endif
#ifdef SWITCHBALL_GUI
                    gui_maybe_img(jd, "gui/navig/arrow_left_disabled.png",
                                      "gui/navig/arrow_left.png",
                                      GUI_PREV, GUI_NONE, 1);
#else
                    gui_maybe(jd, GUI_TRIANGLE_LEFT, GUI_PREV, GUI_NONE, 1);
#endif
            }

            gui_layout(id, 0, +1);

            gui_set_label(name_id, _(base_name(ball_file)));
        }

        if (game_setup_process())
        {
            if ((id = gui_vstack(root_id)))
            {
                if ((jd = gui_hstack(id)))
                {
                    gui_label(jd, GUI_CHECKMARK, GUI_SML, GUI_COLOR_GRN);
                    gui_label(jd, _("Finish"), GUI_SML, GUI_COLOR_WHT);

                    gui_set_state(jd, MODEL_SETUP_FINISH, 0);
                    gui_set_rect(jd, GUI_ALL);
                    gui_focus(jd);
                }

                gui_space(id);

                gui_layout(id, 0, -1);
            }
        }
    }

    return root_id;
}

static int ball_enter(struct state *st, struct state *prev)
{
    if (prev != &st_ball || ball_manual_hotreload)
        scan_balls();

    if (prev != &st_ball)
    {
        game_proxy_filter(filter_cmd);
        load_ball_demo();
    }

    ball_manual_hotreload = 0;

    return ball_gui();
}

static void ball_leave(struct state *st, struct state *next, int id)
{
    if (next != &st_ball)
    {
        gui_delete(id);
        back_free();

        if (next == &st_null)
            game_client_free(NULL);

        demo_replay_stop(0);
    }

    free_balls();
}

static void ball_paint(int id, float t)
{
    video_push_persp((float) config_get_d(CONFIG_VIEW_FOV), 0.1f, FAR_DIST);

#if NB_HAVE_PB_BOTH==1
    if (super_environment == 0)
#endif
        back_draw_easy();

    video_pop_matrix();

#if NB_HAVE_PB_BOTH==1
    game_client_draw(super_environment ? 0 : POSE_BALL, t);
#else
    game_client_draw(POSE_BALL, t);
#endif

    gui_paint(id);
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
    xbox_control_model_gui_paint();
#endif
}

static void ball_timer(int id, float dt)
{
    gui_timer(id, dt);
    game_step_fade(dt);

    if (model_studio)
        game_client_fly(1.0f);
    else if (!demo_replay_step(dt))
    {
        demo_replay_stop(0);
        load_ball_demo();
#if NB_HAVE_PB_BOTH==1
        demo_replay_speed(super_environment ? SPEED_SLOWER : SPEED_NORMAL);
#endif
        game_client_blend(demo_replay_blend());
    }
}

static int ball_keybd(int c, int d)
{
    int initial_fov = config_get_d(CONFIG_VIEW_FOV);
    int initial_dc  = config_get_d(CONFIG_VIEW_DC);
    int initial_dp  = config_get_d(CONFIG_VIEW_DP);
    int initial_w   = config_get_d(CONFIG_WIDTH);
    int initial_h   = config_get_d(CONFIG_HEIGHT);

    int i;

    if (d)
    {
        switch (c)
        {
            case KEY_LOOKAROUND:
                ball_manual_hotreload = 1;
                return goto_state(&st_ball);

            case KEY_EXIT:
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
                if (current_platform == PLATFORM_PC)
#endif
                    return ball_action(GUI_BACK, 0);

            case KEY_LEVELSHOTS:
                video_set_window_size(800 / video.device_scale, 600 / video.device_scale);
                video_resize         (800 / video.device_scale, 600 / video.device_scale);

                // Zoom in on the ball.

                config_set_d(CONFIG_VIEW_DC,  0);
                config_set_d(CONFIG_VIEW_DP,  50);
                config_set_d(CONFIG_VIEW_FOV, 20);

                game_client_fly(0.0f);

                // Take screenshots.

                for (i = 0; balls && i < array_len(balls); ++i)
                {
                    static char filename[64];

                    sprintf(filename, "Screenshots/ball-%s.png",
                                      base_name(DIR_ITEM_GET(balls, i)->path));

                    set_curr_ball(i);

                    video_clear();
                    video_push_persp((float) initial_fov, 0.1f, FAR_DIST);
                    back_draw_easy();
                    video_pop_matrix();

                    game_client_draw(POSE_BALL, 0);
                    video_snap(filename);
                    video_swap();
                }

                // Restore config.

                config_set_d(CONFIG_VIEW_FOV, initial_fov);
                config_set_d(CONFIG_VIEW_DC,  initial_dc);
                config_set_d(CONFIG_VIEW_DP,  initial_dp);

                video_set_window_size(initial_w, initial_h);
                video_resize         (initial_w, initial_h);

                set_curr_ball(curr_ball);

                break;
        }
    }
    return 1;
}

static int ball_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return ball_action(gui_token(active), gui_value(active));

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return ball_action(GUI_BACK, 0);

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L1, b))
            return ball_action(GUI_PREV, 0);

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_R1, b))
            return ball_action(GUI_NEXT, 0);

#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT) && !defined(__EMSCRIPTEN__)
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_Y, b) &&
            (account_get_d(ACCOUNT_PRODUCT_BALLS) == 1 ||
             server_policy_get_d(SERVER_POLICY_EDITION) < 0) &&
            !game_setup_process())
            return ball_action(server_policy_get_d(SERVER_POLICY_EDITION) < 0 ?
                               MODEL_UPGRADE_EDITION : MODEL_ONLINE, 0);
#endif
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

struct state st_ball = {
    ball_enter,
    ball_leave,
    ball_paint,
    ball_timer,
    shared_point,
    shared_stick,
    NULL,
    shared_click,
    ball_keybd,
    ball_buttn
};
