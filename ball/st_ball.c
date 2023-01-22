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

#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
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

#include "game_server.h"
#include "game_proxy.h"
#include "game_client.h"
#include "game_common.h"

#include "st_ball.h"
#include "st_conf.h"
#include "st_shared.h"

#if NB_HAVE_PB_BOTH==1
int super_environment = 1;
#endif

int model_studio = 0;

static int switchball_useable(void)
{
    const SDL_Keycode k_auto = config_get_d(CONFIG_KEY_CAMERA_TOGGLE);
    const SDL_Keycode k_cam1 = config_get_d(CONFIG_KEY_CAMERA_1);
    const SDL_Keycode k_cam2 = config_get_d(CONFIG_KEY_CAMERA_2);
    const SDL_Keycode k_cam3 = config_get_d(CONFIG_KEY_CAMERA_3);
    const SDL_Keycode k_restart = config_get_d(CONFIG_KEY_RESTART);
    const SDL_Keycode k_caml = config_get_d(CONFIG_KEY_CAMERA_L);
    const SDL_Keycode k_camr = config_get_d(CONFIG_KEY_CAMERA_R);

    SDL_Keycode k_arrowkey[4];
    k_arrowkey[0] = config_get_d(CONFIG_KEY_FORWARD);
    k_arrowkey[1] = config_get_d(CONFIG_KEY_LEFT);
    k_arrowkey[2] = config_get_d(CONFIG_KEY_BACKWARD);
    k_arrowkey[3] = config_get_d(CONFIG_KEY_RIGHT);

    if (k_auto == SDLK_c && k_cam1 == SDLK_3 && k_cam2 == SDLK_1 && k_cam3 == SDLK_2
        && k_caml == SDLK_RIGHT && k_camr == SDLK_LEFT
        && k_arrowkey[0] == SDLK_w && k_arrowkey[1] == SDLK_a && k_arrowkey[2] == SDLK_s && k_arrowkey[3] == SDLK_d)
        return 1;
    else if (k_auto == SDLK_c && k_cam1 == SDLK_3 && k_cam2 == SDLK_1 && k_cam3 == SDLK_2
        && k_caml == SDLK_d && k_camr == SDLK_a
        && k_arrowkey[0] == SDLK_UP && k_arrowkey[1] == SDLK_LEFT && k_arrowkey[2] == SDLK_DOWN && k_arrowkey[3] == SDLK_RIGHT)
        return 1;

    /*
     * If the Switchball input preset is not detected,
     * Try it with the Neverball by default.
     */

    return 0;
}

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

    /*
     * Directory path name contains filename,
     * so they will splitted by workaround
     */

    /*if (str_ends_with(item->path, "-solid.sol") ||
        str_ends_with(item->path, "-inner.sol") ||
        str_ends_with(item->path, "-outer.sol"))
    {
        char *ptr = strtok(tmp_path, "-");
        char *next_str;
        
        char path_raw0[MAXSTR], path_raw1[MAXSTR], *path_final;
        SAFECPY(path_raw0, "ball");
        SAFECPY(path_raw1, "");

        int first = 1;

        while (ptr != 0)
        {
            if (ptr != 0)
            {
                next_str = strdup(ptr);
                ptr = strtok(0, "-");

                if (first)
                {
                    SAFECAT(path_raw0, path_last_sep(tmp_path));
                    SAFECAT(path_raw1, path_last_sep(tmp_path));
                    SAFECAT(path_raw1, "-");
                    first = 0;
                }
                else if(!str_ends_with(next_str, "solid.sol") &&
                    !str_ends_with(next_str, "inner.sol") &&
                    !str_ends_with(next_str, "outer.sol"))
                {
                    SAFECAT(path_raw0, next_str);
                    SAFECAT(path_raw0, "-");
                    SAFECAT(path_raw1, next_str);
                    SAFECAT(path_raw1, "-");
                }
            }
        }

        if (str_ends_with(item->path, "-solid.sol")) SAFECAT(path_raw1, "solid.sol");
        if (str_ends_with(item->path, "-inner.sol")) SAFECAT(path_raw1, "inner.sol");
        if (str_ends_with(item->path, "-outer.sol")) SAFECAT(path_raw1, "outer.sol");

        path_final = concat_string(path_raw0, path_raw1, 0);

        yes = fs_exists(path_final);

        return yes;
    }*/

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

    free(solid);
    free(inner);
    free(outer);

    free(tmp_path);

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

#if defined(CONFIG_INCLUDES_ACCOUNT) && NB_HAVE_PB_BOTH==1
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

static void set_curr_ball(void)
{
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(ball_file, dstSize, "%s/%s",
        DIR_ITEM_GET(balls, curr_ball)->path,
        base_name(DIR_ITEM_GET(balls, curr_ball)->path));
#else
    sprintf(ball_file, "%s/%s",
            DIR_ITEM_GET(balls, curr_ball)->path,
            base_name(DIR_ITEM_GET(balls, curr_ball)->path));
#endif

#if defined(CONFIG_INCLUDES_ACCOUNT) && NB_HAVE_PB_BOTH==1
    account_set_s(ACCOUNT_BALL_FILE, ball_file);
#else
    config_set_s(CONFIG_BALL_FILE, ball_file);
#endif

    ball_free();
    ball_init();
    config_save();
#if defined(CONFIG_INCLUDES_ACCOUNT) && NB_HAVE_PB_BOTH==1
    account_save();
#endif
    gui_set_label(name_id, base_name(ball_file));
}

enum
{
    MODEL_ONLINE = GUI_LAST,
    MODEL_UPGRADE_EDITION
};

static int ball_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
    case MODEL_ONLINE:
#if defined(CONFIG_INCLUDES_ACCOUNT) && NB_HAVE_PB_BOTH==1
        if (account_get_d(ACCOUNT_PRODUCT_BALLS) == 1)
        {
#if _WIN32
            system("start msedge https://drive.google.com/drive/folders/1jBX7QtFcg3w7KUlSaH25xp-5qHItmVUT");
#elif __APPLE__
            system("open https://drive.google.com/drive/folders/1jBX7QtFcg3w7KUlSaH25xp-5qHItmVUT");
#else
            system("x-www-browser https://drive.google.com/drive/folders/1jBX7QtFcg3w7KUlSaH25xp-5qHItmVUT");
#endif
        }
#endif
        break;
    case MODEL_UPGRADE_EDITION:
#if _WIN32
        system("start msedge https://forms.gle/62iaMCNKan4z2SJs5");
#elif __APPLE__
        system("open https://forms.gle/62iaMCNKan4z2SJs5");
#else
        system("x-www-browser https://forms.gle/62iaMCNKan4z2SJs5");
#endif
        break;
    case GUI_NEXT:
        if (++curr_ball == array_len(balls))
            curr_ball = 0;

        set_curr_ball();
        break;

    case GUI_PREV:
        if (--curr_ball == -1)
            curr_ball = array_len(balls) - 1;

        set_curr_ball();
        break;

    case GUI_BACK:
        game_fade(+4.0);
#if NB_HAVE_PB_BOTH==1
        goto_state(&st_conf_account);
#else
        goto_state(&st_conf);
#endif
        break;
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
            ball_action(GUI_BACK, 0);
            return;
        }
    }
    else
    {
        int g;

        /* "g" is a stupid hack to keep the goal locked. */

        if (!demo_replay_init("gui/ball.nbr", &g, NULL, NULL, NULL, NULL, NULL))
        {
            ball_action(GUI_BACK, 0);
            return;
        }
#if NB_HAVE_PB_BOTH==1
        demo_replay_speed(super_environment ? SPEED_SLOWER : SPEED_NORMAL);
#endif
    }

    audio_music_fade_to(0.0f, switchball_useable() ? "bgm/title-switchball.ogg" : "bgm/title.ogg");
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
    int id, jd;
    int i;

    if ((id = gui_vstack(0)))
    {
        if ((jd = gui_hstack(id)))
        {
            gui_label(jd, _("Ball Model"), GUI_SML, 0, 0);
            gui_filler(jd);
            gui_space(jd);

#if NB_HAVE_PB_BOTH==1
#ifndef __EMSCRIPTEN__
            if (current_platform == PLATFORM_PC)
                gui_start(jd, _("Back"), GUI_SML, GUI_BACK, 0);
            else
#endif
                gui_filler(jd);
#else
            gui_start(jd, _("Back"), GUI_SML, GUI_BACK, 0);
#endif
        }

#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
        if (!xbox_show_gui())
#endif
            gui_space(id);

#if NB_HAVE_PB_BOTH==1
#if NB_STEAM_API==1
        const char *more_balls_text = server_policy_get_d(SERVER_POLICY_EDITION) > -1 ? N_("Open Steam Workshop!")
#else
        const char *more_balls_text = server_policy_get_d(SERVER_POLICY_EDITION) > -1 ? N_("Get more Balls!")
#endif
            : N_("Upgrade to Home Edition!");

        if ((account_get_d(ACCOUNT_PRODUCT_BALLS) == 1
            || server_policy_get_d(SERVER_POLICY_EDITION) < 0)
#ifndef __EMSCRIPTEN__
            && !xbox_show_gui()
#endif
            )
        {
            int online_id;
            if (online_id = gui_label(id, _(more_balls_text), GUI_SML, gui_wht, gui_grn))
            {
                if (server_policy_get_d(SERVER_POLICY_EDITION) == -1)
                    gui_set_state(online_id, MODEL_UPGRADE_EDITION, 0);
                else
                    gui_set_state(online_id, MODEL_ONLINE, 0);
            }
        }
#ifndef __EMSCRIPTEN__
        else if (!xbox_show_gui())
            gui_label(id, _(more_balls_text), GUI_SML, gui_gry, gui_gry);
#endif
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
                gui_maybe_img(jd, "gui/navig/arrow_right_disabled.png", "gui/navig/arrow_right.png", GUI_NEXT, GUI_NONE, 1);
#else
                gui_maybe(jd, GUI_ARROW_RGHT, GUI_NEXT, GUI_NONE, 1);
#endif

            name_id = gui_label(jd, "very-long-ball-name", GUI_SML,
                                gui_wht, gui_wht);

            gui_set_trunc(name_id, TRUNC_TAIL);
            gui_set_fill(name_id);

#ifndef __EMSCRIPTEN__
            if (!xbox_show_gui() && total_ball_name > 1)
#else
            if (total_ball_name > 1)
#endif
#ifdef SWITCHBALL_GUI
                gui_maybe_img(jd, "gui/navig/arrow_left_disabled.png", "gui/navig/arrow_left.png", GUI_PREV, GUI_NONE, 1);
#else
                gui_maybe(jd, GUI_ARROW_LFT, GUI_PREV, GUI_NONE, 1);
#endif
        }

        for (i = 0; i < 12; i++)
            gui_space(id);

        gui_layout(id, 0, 0);

        gui_set_label(name_id, base_name(ball_file));
    }

    return id;
}

static int ball_enter(struct state *st, struct state *prev)
{
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
    game_client_draw(POSE_BALL);
#endif

    gui_paint(id);
#ifndef __EMSCRIPTEN__
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
    if (d)
    {
        if (c == KEY_EXIT)
            return ball_action(GUI_BACK, 0);
        if (c == KEY_LOOKAROUND)
        {
            ball_manual_hotreload = 1;
            return goto_state(&st_ball);
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

#if defined(CONFIG_INCLUDES_ACCOUNT) && NB_HAVE_PB_BOTH==1
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_Y, b))
            return ball_action(server_policy_get_d(SERVER_POLICY_EDITION) == -1 ? MODEL_UPGRADE_EDITION : MODEL_ONLINE, 0);
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
