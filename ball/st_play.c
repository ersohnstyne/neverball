/*
 * Copyright (C) 2023 Microsoft / Neverball authors
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

#if NB_HAVE_PB_BOTH==1
#ifndef __EMSCRIPTEN__
#include "console_control_gui.h"
#endif
#include "powerup.h"
#include "account.h"
#include "solid_chkp.h"
#endif

#ifdef MAPC_INCLUDES_CHKP
#include "checkpoints.h" // New: Checkpoints
#endif

#include "gui.h"
//#include "hud.h"
//#include "demo.h"
//#include "progress.h"
//#include "audio.h"
//#include "config.h"
//#include "video.h"
//#include "cmd.h"

//#include "geom.h"
//#include "vec3.h"

#include "st_play_sync.h"

//#include "game_draw.h"
//#include "game_common.h"
//#include "game_server.h"
//#include "game_proxy.h"
//#include "game_client.h"

#include "st_play.h"
#include "st_goal.h"
#include "st_fail.h"
#include "st_pause.h"
#include "st_level.h"
#include "st_shared.h"
#include "st_tutorial.h"

/*---------------------------------------------------------------------------*/

#define COUNTDOWN_PREPARATION_CROSS_LEN 20.f * (video.device_h / 1920.f)

typedef struct __countdown_preparation
{
    int id;
    float positions[2];
    float position_scaled;
    int isgreen;
    float alpha;
    float colorfill[3];
} countdown_preparation;

/* Have modern preparation start timer countdown */
countdown_preparation local_countdown_preparation[12];

static void __countdown_preparation_init()
{
    for (int i = 0; i < 12; i++)
    {
        memset(&local_countdown_preparation[i], 0, sizeof (countdown_preparation));
        local_countdown_preparation[i].id = i;
        local_countdown_preparation[i].isgreen = 0;

        if (video.device_w <= video.device_h)
        {
            local_countdown_preparation[i].positions[0] = fsinf(V_PI * (i / 6.0f)) * (video.device_h * 0.125f);
            local_countdown_preparation[i].positions[1] = -fcosf(V_PI * (i / 6.0f)) * (video.device_h * 0.125f);
        }
        else
        {
            local_countdown_preparation[i].positions[0] = fsinf(V_PI * (i / 6.0f)) * (video.device_w * 0.125f);
            local_countdown_preparation[i].positions[1] = -fcosf(V_PI * (i / 6.0f)) * (video.device_w * 0.125f);
        }
    }
}

static void __countdown_preparation_draw()
{
    for (int i = 0; i < 12; i++)
    {
        video_push_ortho();

        if (local_countdown_preparation[i].isgreen)
        {
            local_countdown_preparation[i].alpha = 3.0f - (4 * time_state());
            local_countdown_preparation[i].position_scaled = 1.0f - fcosf(V_PI * time_state()) * (time_state() * 2);

            local_countdown_preparation[i].colorfill[0] = 0.0f;
            local_countdown_preparation[i].colorfill[1] = 1.0f;
            local_countdown_preparation[i].colorfill[2] = 0.0f;
        }
        else
        {
            float time_state_offset = time_state() + (local_countdown_preparation[i].id / 12.0f);

            float alpha_result = fcosf(V_PI * time_state_offset);
            if (alpha_result < 0) alpha_result = -alpha_result;

            if (i == local_countdown_preparation[i].id)
            {
                local_countdown_preparation[i].alpha = alpha_result;
                local_countdown_preparation[i].position_scaled = 1.0f;

                local_countdown_preparation[i].colorfill[0] = 1.0f;
                local_countdown_preparation[i].colorfill[1] = 0.0f;
                local_countdown_preparation[i].colorfill[2] = 0.25f;
            }
        }

        glTranslatef(
            local_countdown_preparation[i].positions[0] * local_countdown_preparation[i].position_scaled,
            local_countdown_preparation[i].positions[1] * local_countdown_preparation[i].position_scaled,
            0.0f
        );
        glTranslatef(video.device_w / 2, video.device_h / 2, 0.0f);

        glDisable(GL_LIGHTING);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_TEXTURE_2D);
        glBegin(GL_TRIANGLE_STRIP);
        {
            glColor4ub(ROUND(local_countdown_preparation[i].colorfill[0] * 255),
                       ROUND(local_countdown_preparation[i].colorfill[1] * 255),
                       ROUND(local_countdown_preparation[i].colorfill[2] * 255),
                       ROUND(local_countdown_preparation[i].colorfill[3] * 255));
            glVertex2f(-COUNTDOWN_PREPARATION_CROSS_LEN, 0.0f);
            glVertex2f(0.0f, -COUNTDOWN_PREPARATION_CROSS_LEN);
            glVertex2f(0.0f, +COUNTDOWN_PREPARATION_CROSS_LEN);
            glVertex2f(+COUNTDOWN_PREPARATION_CROSS_LEN, 0.0f);
        }
        glEnd();
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_LIGHTING);

        video_pop_matrix();
    }
}

static void __countdown_preparation_setgreen()
{
    for (int i = 0; i < 12; i++)
        local_countdown_preparation[i].isgreen = 1;
}

#undef COUNTDOWN_PREPARATION_CROSS_LEN

/*---------------------------------------------------------------------------*/

static void set_lvlinfo(void)
{
    if (curr_mode() != MODE_STANDALONE
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        && !campaign_used()
#endif
        )
    {
        char setname[MAXSTR];
        char lvlname[MAXSTR];

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        if (curr_mode() == MODE_CAMPAIGN)
            SAFECPY(setname, _("Campaign"));
        else
#endif
            SAFECPY(setname, set_name(curr_set()));

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(lvlname, dstSize,
#else
        sprintf(lvlname,
#endif
                level_bonus(curr_level()) ? "%s - Bonus %s" : "%s - %s",
                setname, level_name(curr_level()));
        hud_lvlname_set(lvlname, level_bonus(curr_level()));
    }
}

static void set_camera(int c)
{
    config_set_d(CONFIG_CAMERA, c);
    hud_cam_pulse(c);
}

static void toggle_camera(void)
{
    int cam = (config_tst_d(CONFIG_CAMERA, CAM_AUTO) ?
               CAM_3 : CAM_AUTO);

    set_camera(cam);
}

static void next_camera(void)
{
    int cam = config_get_d(CONFIG_CAMERA) + 1;

    if (cam <= CAM_NONE || cam >= CAM_MAX)
        cam = CAM_1;

    set_camera(cam);
}

static void keybd_camera(int c)
{
    if (config_tst_d(CONFIG_KEY_CAMERA_1, c))
        set_camera(CAM_1);
    if (config_tst_d(CONFIG_KEY_CAMERA_2, c))
        set_camera(CAM_2);
    if (config_tst_d(CONFIG_KEY_CAMERA_3, c))
        set_camera(CAM_3);

    if (config_tst_d(CONFIG_KEY_CAMERA_TOGGLE, c))
        toggle_camera();
}

static void click_camera(int b)
{
    if (config_tst_d(CONFIG_MOUSE_CAMERA_1, b))
        set_camera(CAM_1);
    if (config_tst_d(CONFIG_MOUSE_CAMERA_2, b))
        set_camera(CAM_2);
    if (config_tst_d(CONFIG_MOUSE_CAMERA_3, b))
        set_camera(CAM_3);

    if (config_tst_d(CONFIG_MOUSE_CAMERA_TOGGLE, b))
        toggle_camera();
}

static void buttn_camera(int b)
{
    if (config_tst_d(CONFIG_JOYSTICK_BUTTON_X, b))
        next_camera();
}

/*---------------------------------------------------------------------------*/

static void play_shared_fade(float alpha)
{
    hud_set_alpha(alpha);
}

static void play_shared_exit(int id)
{
    progress_stat(GAME_NONE);
    progress_stop();
    progress_exit();
}

/*---------------------------------------------------------------------------*/

#ifdef MAPC_INCLUDES_CHKP
static int restart_cancel_allchkp;
#endif
static int play_freeze_all;
static int use_mouse;
static int use_keyboard;

static int play_ready_gui(void)
{
    int id;

    if ((id = gui_title_header(0, _("Ready?"), GUI_LRG, gui_blu, gui_red)))
    {
        gui_layout(id, 0, 0);
        gui_pulse(id, 1.2f);
    }

    return id;
}

static int play_ready_enter(struct state *st, struct state *prev)
{
    __countdown_preparation_init();
#ifdef MAPC_INCLUDES_CHKP
    restart_cancel_allchkp = 0;
#endif
    play_freeze_all = 0;
    if (curr_mode() == MODE_NONE)
    {
        video_set_grab(1);

        hud_cam_pulse(config_get_d(CONFIG_CAMERA));
        toggle_hud_visibility(1);

        /* Cannot run traffic in home room. */

        return 0;
    }
    audio_narrator_play(AUD_READY);
    video_set_grab(1);
    hud_speedup_reset();

    game_client_sync(!campaign_hardcore_norecordings() && curr_mode() != MODE_NONE ? demo_fp : NULL);
    hud_update(0, 0.0f);
    hud_update_camera_direction(curr_viewangle());

    hud_cam_pulse(config_get_d(CONFIG_CAMERA));
    toggle_hud_visibility(1);

    return play_ready_gui();
}

static void play_ready_paint(int id, float t)
{
    game_client_draw(0, t);

    __countdown_preparation_draw();
    gui_paint(id);
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
    if (xbox_show_gui() && current_platform != PLATFORM_PC)
    {
        hud_cam_paint();
        xbox_control_preparation_gui_paint();

        if (config_get_d(CONFIG_SCREEN_ANIMATIONS))
        {
            hud_paint();
            hud_lvlname_paint();
        }
    }
    else if (hud_visibility())
#endif
    {
        hud_paint();
        hud_lvlname_paint();
    }
}

static void play_ready_timer(int id, float dt)
{
    game_lerp_pose_point_tick(dt);
    geom_step(dt);
    float t = time_state();

    game_client_fly(1.0f - 0.5f * t);
    
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
    if (current_platform != PLATFORM_PC)
    {
        xbox_control_gui_set_alpha(1.0f);
        hud_set_alpha(0.0f);
        hud_timer(dt);
    }
#endif

    if (dt > 0.0f && t > 1.0f)
        goto_state_full(&st_play_set, 0, 0, 1);

    set_lvlinfo();

    game_step_fade(dt);
    game_client_blend(game_server_blend());
    game_client_sync(!campaign_hardcore_norecordings() && curr_mode() != MODE_NONE ? demo_fp : NULL);

    gui_timer(id, dt);

#ifndef __EMSCRIPTEN__
    if (xbox_show_gui())
        hud_cam_timer(dt);
    else if (hud_visibility())
#endif
        hud_timer(dt);
}

static void play_ready_stick(int id, int a, float v, int bump)
{
    use_mouse = 0; use_keyboard = 1;
}

static int play_ready_click(int b, int d)
{
    if (d)
    {
        use_mouse = 1; use_keyboard = 0;
        click_camera(b);

        if (b == SDL_BUTTON_LEFT)
            goto_state_full(&st_play_loop, 0, 0, 1);
    }
    return 1;
}

static int play_ready_keybd(int c, int d)
{
    if (d)
    {
        keybd_camera(c);

        if (c == KEY_EXIT
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            && current_platform == PLATFORM_PC
#endif
            )
        {
            hud_speedup_reset();
            goto_pause(curr_state());
        }
    }
    return 1;
}

static int play_ready_buttn(int b, int d)
{
    if (d)
    {
        buttn_camera(b);

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return goto_state_full(&st_play_loop, 0, 0, 1);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
        {
            hud_speedup_reset();
            return goto_pause(curr_state());
        }
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int play_set_gui(void)
{
    int id;

    if ((id = gui_title_header(0, _("Set?"), GUI_LRG, gui_blu, gui_yel)))
    {
        gui_layout(id, 0, 0);
        gui_pulse(id, 1.2f);
    }

    return id;
}

static int play_set_enter(struct state *st, struct state *prev)
{
#ifdef MAPC_INCLUDES_CHKP
    restart_cancel_allchkp = 0;
#endif
    play_freeze_all = 0;
    if (curr_mode() == MODE_NONE) return 0; /* Cannot run traffic in home room. */
    audio_narrator_play(AUD_SET);

    toggle_hud_visibility(1);

    return play_set_gui();
}

static void play_set_paint(int id, float t)
{
    game_client_draw(0, t);

    __countdown_preparation_draw();
    gui_paint(id);
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
    if (xbox_show_gui() || current_platform != PLATFORM_PC)
    {
        hud_cam_paint();
        xbox_control_preparation_gui_paint();

        if (config_get_d(CONFIG_SCREEN_ANIMATIONS))
        {
            hud_paint();
            hud_lvlname_paint();
        }
    }
    else if (hud_visibility())
#endif
    {
        hud_paint();
        hud_lvlname_paint();
    }
}

static void play_set_timer(int id, float dt)
{
    game_lerp_pose_point_tick(dt);
    geom_step(dt);
    float t = time_state();

    game_client_fly(0.5f - 0.5f * t);
    
    if (dt > 0.0f && t > 1.0f)
    {
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        if (current_platform != PLATFORM_PC) hud_set_alpha(1.0f);
#endif
        goto_state_full(&st_play_loop, 0, 0, 1);
    }

    game_step_fade(dt);
    game_client_blend(game_server_blend());
    game_client_sync(!campaign_hardcore_norecordings() && curr_mode() != MODE_NONE ? demo_fp : NULL);

    gui_timer(id, dt);

#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
    if (current_platform != PLATFORM_PC)
    {
        xbox_control_gui_set_alpha(CLAMP(0.0f, flerp(6.0f, 0.0f, t), 1.0f));
        hud_set_alpha(CLAMP(0.0f, flerp(-5.0f, 1.0f, t), 1.0f));
    }

    if (xbox_show_gui() || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        hud_cam_timer(dt);
    else if (hud_visibility() || config_get_d(CONFIG_SCREEN_ANIMATIONS))
#endif
        hud_timer(dt);
}

static void play_set_stick(int id, int a, float v, int bump)
{
    use_mouse = 0; use_keyboard = 1;
}

static int play_set_click(int b, int d)
{
    if (d)
    {
        use_mouse = 1; use_keyboard = 0;
        click_camera(b);

        if (b == SDL_BUTTON_LEFT)
            goto_state_full(&st_play_loop, 0, 0, 1);
    }
    return 1;
}

static int play_set_keybd(int c, int d)
{
    if (d)
    {
        keybd_camera(c);

        if (c == KEY_EXIT
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            && current_platform == PLATFORM_PC
#endif
            )
        {
            hud_speedup_reset();
            goto_pause(curr_state());
        }
    }
    return 1;
}

static int play_set_buttn(int b, int d)
{
    if (d)
    {
        buttn_camera(b);

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return goto_state_full(&st_play_loop, 0, 0, 1);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
        {
            hud_speedup_reset();
            return goto_pause(curr_state());
        }
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

enum
{
    ROT_NONE = 0,
    ROT_ROTATE,
    ROT_HOLD
};

#define DIR_R 0x1
#define DIR_L 0x2

static int   rot_dir;
static float rot_val;

static void rot_init(void)
{
    rot_val = 0.0f;
    rot_dir = 0;
}

static void rot_set(int dir, float value, int exclusive)
{
    rot_val = value;

    if (exclusive)
        rot_dir  = dir;
    else
        rot_dir |= dir;
}

static void rot_clr(int dir)
{
    rot_dir &= ~dir;
}

static int rot_get(float *v)
{
    *v = 0.0f;

    if ((rot_dir & DIR_R) && (rot_dir & DIR_L))
    {
        return ROT_HOLD;
    }
    else if (rot_dir & DIR_R)
    {
        *v = +rot_val;
        return ROT_ROTATE;
    }
    else if (rot_dir & DIR_L)
    {
        *v = -rot_val;
        return ROT_ROTATE;
    }
    return ROT_NONE;
}

/*---------------------------------------------------------------------------*/

static int play_block_state;

static int evalue;
static int fvalue;
static int svalue;

static int lmb_holded;
static int rmb_holded;
static float lmb_hold_time;
static float rmb_hold_time;

static int fast_rotate;
static int max_speed;
static int man_rot;
static float rotation_offset;

static float tilt_x;
static float tilt_y;

static int play_loop_gui(void)
{
    int id;

    if ((id = gui_title_header(0, _("GO!"), GUI_LRG, gui_blu, gui_grn)))
    {
        gui_layout(id, 0, 0);
        gui_pulse(id, 1.2f);
    }

    return id;
}

struct state *global_prev;
static float smoothfix_slowdown_time;

static int play_loop_enter(struct state *st, struct state *prev)
{
    smoothfix_slowdown_time = 0;
#ifdef MAPC_INCLUDES_CHKP
    restart_cancel_allchkp = 0;
#endif
    play_freeze_all = 0;
    play_block_state = 0;
    rot_init();

    lmb_holded = 0;
    lmb_hold_time = -0.01f;

    fast_rotate = 0;
    max_speed = 0;
    man_rot = 0;
    rotation_offset = 0;

    global_prev = prev;
    video_set_grab(0);

    tilt_x = 0; tilt_y = 0;

    if (curr_mode() == MODE_NONE) return 0; /* Cannot run traffic in home room. */

    if ((prev != &st_play_ready && prev != &st_play_set && prev != &st_tutorial) || prev == &st_play_loop)
        return 0;

    __countdown_preparation_setgreen();

#if defined(ENABLE_POWERUP) && defined(CONFIG_INCLUDES_ACCOUNT)
    evalue = account_get_d(ACCOUNT_CONSUMEABLE_EARNINATOR);
    fvalue = account_get_d(ACCOUNT_CONSUMEABLE_FLOATIFIER);
    svalue = account_get_d(ACCOUNT_CONSUMEABLE_SPEEDIFIER);

    if (get_coin_multiply() == 2)
    {
        evalue -= 1;
        account_set_d(ACCOUNT_CONSUMEABLE_EARNINATOR, evalue);
    }
    if (get_gravity_multiply() <= 0.51f)
    {
        fvalue -= 1;
        account_set_d(ACCOUNT_CONSUMEABLE_FLOATIFIER, fvalue);
    }
    if (get_tilt_multiply() == 2)
    {
        svalue -= 1;
        account_set_d(ACCOUNT_CONSUMEABLE_SPEEDIFIER, svalue);
    }
    account_save();
#endif

    audio_narrator_play(AUD_GO);

    game_client_fly(0.0f);

    toggle_hud_visibility(1);

    return play_loop_gui();
}

static void play_loop_paint(int id, float t)
{
    game_client_draw(0, t);

    if (hud_visibility() || config_get_d(CONFIG_SCREEN_ANIMATIONS))
    {
        hud_paint();
        hud_lvlname_paint();
    }

    if (time_state() < 1.f && id)
    {
        __countdown_preparation_draw();
        gui_paint(id);
    }
}

static void play_loop_timer(int id, float dt)
{
    if (config_get_d(CONFIG_SMOOTH_FIX) && video_perf() < 25)
    {
        smoothfix_slowdown_time += dt;

        if (smoothfix_slowdown_time >= 30)
        {
            config_set_d(CONFIG_SMOOTH_FIX, config_get_d(CONFIG_FORCE_SMOOTH_FIX));
            smoothfix_slowdown_time = 0;
        }
    }
    else
        smoothfix_slowdown_time = 0;

    game_lerp_pose_point_tick(dt);

    if (!game_client_get_jump_b())
        geom_step(dt);

    /* Boost rush uses auto forward */
    if (curr_mode() == MODE_BOOST_RUSH)
        game_set_x(curr_speed_percent() / 100.f * -0.875f + (time_state() < 1.0f && global_prev != &st_pause ? -0.5f : 0));

    float k = (fast_rotate ?
               (float) config_get_d(CONFIG_ROTATE_FAST) :
               (float) config_get_d(CONFIG_ROTATE_SLOW)) / 100.f;

    float r = 0.0f;

    hud_update_camera_direction(curr_viewangle());
    gui_timer(id, dt);
    hud_timer(dt);

    /* Switchball works on max speed and manual rotation */

    if (!max_speed && use_mouse && !use_keyboard)
    {
        tilt_x = flerp(0, tilt_x, 0.5f);
        tilt_y = flerp(0, tilt_y, 0.5f);
    }

    if (!lmb_holded && use_mouse && !use_keyboard)
    {
        game_set_pos(tilt_x, tilt_y);
        max_speed = 0;
        lmb_hold_time = -0.01f;
    }
    else if (use_mouse && !use_keyboard)
    {
        lmb_hold_time += dt;
        if (lmb_hold_time > 0.5f) max_speed = 1;
    }

    if (!rmb_holded && use_mouse && !use_keyboard)
    {
        rotation_offset = 0;
        man_rot = 0;
        rmb_hold_time = -0.01f;
    }
    else if (use_mouse && !use_keyboard)
    {
        rmb_hold_time += dt;
        if (rmb_hold_time > 0.5f)
        {
            game_set_pos(0, 0);
            man_rot = 1;
        }
    }

    switch (rot_get(&r))
    {
    case ROT_HOLD:
        /*
         * Cam 3 could be anything. But let's assume it's a manual cam
         * and holding down both rotation buttons freezes the camera
         * rotation.
         */
         game_set_rot(0.0f);
         game_set_cam(CAM_3);
        break;

    case ROT_ROTATE:
    case ROT_NONE:
        game_set_rot(r * k);
        game_set_cam(config_get_d(CONFIG_CAMERA));
        break;
    }

    if (man_rot)
    {
        game_set_rot(rotation_offset);
        rotation_offset = 0;
    }

    game_step_fade(dt);

    if (!play_freeze_all)
        game_server_step(dt);

    game_client_blend(game_server_blend());
    game_client_sync(curr_mode() != MODE_NONE
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                  && !campaign_hardcore_norecordings()
#endif
                         ? demo_fp : NULL);

    /* Cannot update state in home room. */

    if (curr_mode() == MODE_NONE) return;

    if (curr_status() == GAME_NONE && !play_freeze_all)
        progress_step();
    else if (!play_freeze_all && !play_block_state)
    {
        play_block_state = 1;
        progress_stat(curr_status());
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        goto_state_full(
            curr_status() == GAME_GOAL ?
                (curr_mode() == MODE_HARDCORE && !progress_done() ? &st_goal_hardcore : &st_goal)
                : &st_fail,
            0, GUI_ANIMATION_E_CURVE, 0);
#else
        goto_state_full(curr_status() == GAME_GOAL ?
                            &st_goal : &st_fail,
                        0, GUI_ANIMATION_E_CURVE, 0);
#endif
    }
}

static void play_loop_point(int id, int x, int y, int dx, int dy)
{
#if NDEBUG
    /* Good news: This isn't controlled with mouse while debug mode. */
#ifndef __EMSCRIPTEN__
    if (current_platform == PLATFORM_PC)
#endif
    {
        if (use_mouse && !use_keyboard)
        {
            use_mouse = 1; use_keyboard = 0;
            if (max_speed)
            {
                tilt_x = 0;
                tilt_y = 0;
                game_set_pos_max_speed(dx * get_tilt_multiply(),
                                       curr_mode() == MODE_BOOST_RUSH ? 0 : dy * get_tilt_multiply());
            }
            else if (man_rot)
            {
                /* Reset tilt to horizontally */
                game_set_pos(0, 0);

                float k = (fast_rotate ?
                    (float) config_get_d(CONFIG_ROTATE_FAST) / 100.0f :
                    (float) config_get_d(CONFIG_ROTATE_SLOW) / 100.0f);

                rotation_offset = (-50 * dx) / config_get_d(CONFIG_MOUSE_SENSE);
            }
            else if (!max_speed && !man_rot)
            {
                tilt_x += (dx * 10);
                tilt_y += (dy * 10);
                game_set_pos((tilt_x * get_tilt_multiply()) * 10,
                             curr_mode() == MODE_BOOST_RUSH ? 0 : (tilt_y * get_tilt_multiply()) * 10);
            }
        }
        else
            use_mouse = 1; use_keyboard = 0;
    }
#endif
}

static void play_loop_stick(int id, int a, float v, int bump)
{
    if (!use_mouse && use_keyboard)
    {
        use_mouse = 0; use_keyboard = 1;
        /* Camera movement */
        if (config_tst_d(CONFIG_JOYSTICK_AXIS_X1, a))
        {
            if (v + axis_offset[2] > 0.0f)
                rot_set(DIR_R, -v + axis_offset[2], 1); /* Previously used: +v */
            else if (v + axis_offset[2] < 0.0f)
                rot_set(DIR_L, +v + axis_offset[2], 1); /* Previously used: -v */
            else
                rot_clr(DIR_R | DIR_L);
        }
        if (config_tst_d(CONFIG_JOYSTICK_AXIS_Y1, a))
            game_set_zoom(v + +axis_offset[3]);

        if (config_tst_d(CONFIG_JOYSTICK_AXIS_X0, a))
            tilt_x = v * get_tilt_multiply();
        if (config_tst_d(CONFIG_JOYSTICK_AXIS_Y0, a))
            tilt_y = (curr_mode() == MODE_BOOST_RUSH ? 0 : v * get_tilt_multiply());

        game_set_z(tilt_x);
        game_set_x(tilt_y);
    }
    else
        use_mouse = 0; use_keyboard = 1;
}

static void play_loop_angle(int id, float x, float z)
{
    game_set_ang(x, z);

    /* TODO: Throttle the camera rotation */
}

static int play_loop_click(int b, int d)
{
    if (d && use_mouse && !use_keyboard)
    {
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        if (config_tst_d(CONFIG_MOUSE_CAMERA_L, b) && current_platform == PLATFORM_PC)
            lmb_holded = 1;

        if (config_tst_d(CONFIG_MOUSE_CAMERA_R, b) && current_platform == PLATFORM_PC)
            rmb_holded = 1;
#else
        if (config_tst_d(CONFIG_MOUSE_CAMERA_L, b))
            lmb_holded = 1;

        if (config_tst_d(CONFIG_MOUSE_CAMERA_R, b))
            rmb_holded = 1;
#endif

        click_camera(b);
    }
    else if (use_mouse && !use_keyboard
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
          && current_platform == PLATFORM_PC
#endif
        )
    {
        /*if (config_tst_d(CONFIG_MOUSE_CAMERA_R, b))
            rot_clr(DIR_L);
        if (config_tst_d(CONFIG_MOUSE_CAMERA_L, b))
            rot_clr(DIR_R);*/

        if (config_tst_d(CONFIG_MOUSE_CAMERA_L, b))
            lmb_holded = 0;
        if (config_tst_d(CONFIG_MOUSE_CAMERA_R, b))
            rmb_holded = 0;
    }
    else
    {
        use_mouse = 1; use_keyboard = 0;
    }

    return 1;
}

static int play_loop_keybd(int c, int d)
{
    if (d)
    {
#ifdef MAPC_INCLUDES_CHKP
        if (c == SDLK_LCTRL || c == SDLK_LGUI)
            restart_cancel_allchkp = 1;
#endif

        if (config_tst_d(CONFIG_KEY_CAMERA_R, c)
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            && current_platform == PLATFORM_PC
#endif
            )
            rot_set(DIR_R, 1.0f, 0);
        if (config_tst_d(CONFIG_KEY_CAMERA_L, c)
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            && current_platform == PLATFORM_PC
#endif
            )
            rot_set(DIR_L, 1.0f, 0);
        if (config_tst_d(CONFIG_KEY_ROTATE_FAST, c)
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            && current_platform == PLATFORM_PC
#endif
            )
            fast_rotate = 1;

        keybd_camera(c);

        if (config_tst_d(CONFIG_KEY_RESTART, c)
            && progress_same_avail()
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            && current_platform == PLATFORM_PC
#endif
            )
        {
            play_freeze_all = 1;

#ifdef MAPC_INCLUDES_CHKP
            if (restart_cancel_allchkp)
                checkpoints_stop();
#endif
            if (progress_same())
                goto_state(&st_play_ready);
        }
        if (c == KEY_EXIT
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            && current_platform == PLATFORM_PC
#endif
            )
        {
            play_freeze_all = 1;
            hud_speedup_reset();
            goto_pause(curr_state());
        }
    }
    else
    {
#ifdef MAPC_INCLUDES_CHKP
        if (c == SDLK_LCTRL || c == SDLK_LGUI)
            restart_cancel_allchkp = 0;
#endif
        if (config_tst_d(CONFIG_KEY_CAMERA_R, c))
            rot_clr(DIR_R);
        if (config_tst_d(CONFIG_KEY_CAMERA_L, c))
            rot_clr(DIR_L);
        if (config_tst_d(CONFIG_KEY_ROTATE_FAST, c))
            fast_rotate = 0;
    }

    /*
     * In Switchball, there is a timed mode in a speedrun.
     * During the game with no time limits and no required coins,
     * you can disable at any time.
     */
    if (d && c == KEY_POSE)
        toggle_hud_visibility(!hud_visibility());

#if NB_STEAM_API==0 && NB_EOS_SDK==0
    /* Used centered player or special camera orientation */
    if (d && c == KEY_LOOKAROUND && config_cheat())
        return goto_state(&st_look);
#endif

    return 1;
}

static int play_loop_buttn(int b, int d)
{
    if (d == 1)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
        {
            hud_speedup_reset();
            goto_pause(curr_state());
        }
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_R1, b))
            rot_set(DIR_R, 1.0f, 0);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L1, b))
            rot_set(DIR_L, 1.0f, 0);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L2, b))
            fast_rotate = 1;

        buttn_camera(b);
    }
    else
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_R1, b))
            rot_clr(DIR_R);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L1, b))
            rot_clr(DIR_L);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L2, b))
            fast_rotate = 0;
    }
    return 1;
}

static int play_loop_touch(const SDL_TouchFingerEvent *event)
{
    if (event->type == SDL_FINGERMOTION)
    {
        int dx = (int) ((float) video.device_w *  event->dx);
        int dy = (int) ((float) video.device_h * -event->dy);

        game_set_pos(dx, dy);
    }

    /* TODO: rotate camera, change camera, etc. */

    return 1;
}

static void play_loop_wheel(int x, int y)
{
    if (config_get_d(CONFIG_VIEW_DP) == 75 && config_get_d(CONFIG_VIEW_DC) == 25 && config_get_d(CONFIG_VIEW_DZ) == 200)
    {
        /* For some reasons, this may not work on zoom functions. */
        /* if (y > 0) game_set_zoom(-1.0f); */
        /* if (y < 0) game_set_zoom(1.0f); */
    }
    return;
}

/*---------------------------------------------------------------------------*/

void play_shared_leave(struct state *st, struct state *next, int id)
{
    if (curr_mode() == MODE_NONE)
        game_set_pos(0, 0);

    gui_delete(id);
}

/*---------------------------------------------------------------------------*/

static float phi, theta;

static int   look_panning;
static float look_stick_x[2],
             look_stick_y[2],
             look_stick_z;

static int look_enter(struct state *st, struct state *prev)
{
    look_panning = 0;
    look_stick_x[0] = 0;
    look_stick_y[0] = 0;
    look_stick_x[1] = 0;
    look_stick_y[1] = 0;
    look_stick_z = 0;
    phi   = 0;
    theta = 0;
    return 0;
}

static void look_leave(struct state *st, struct state *next, int id)
{
}

static void look_timer(int id, float dt)
{
    theta += look_stick_x[1] *  2 * (dt * 4000);
    phi   += look_stick_y[1] * -2 * (dt * 4000);

    if (phi > +90.0f)    phi    = +90.0f;
    if (phi < -90.0f)    phi    = -90.0f;

    if (theta > +180.0f) theta -= 360.0f;
    if (theta < -180.0f) theta += 360.0f;

    float look_moves[2];
    look_moves[0] = (fcosf((V_PI * theta) / 180) * look_stick_x[0])
                  + (fsinf((V_PI * theta) / 180) * -look_stick_y[0]);
    look_moves[1] = (fcosf((V_PI * theta) / 180) * look_stick_y[0])
                  + (fsinf((V_PI * theta) / 180) * look_stick_x[0]);

    game_look_v2(look_moves[0] * (dt * 5),
                 look_stick_z  * (dt * 5),
                 look_moves[1] * (dt * 5),
                 phi, theta);
}

static void look_paint(int id, float t)
{
    game_client_draw(0, t);
}

static void look_point(int id, int x, int y, int dx, int dy)
{
    if (look_panning)
    {
        phi   += 90.0f  * dy / video.device_h;
        theta += 180.0f * dx / video.device_w;

        if (phi > +90.0f)    phi    = +90.0f;
        if (phi < -90.0f)    phi    = -90.0f;

        if (theta > +180.0f) theta -= 360.0f;
        if (theta < -180.0f) theta += 360.0f;

        game_look(phi, theta);
    }
}

static void look_stick(int id, int a, float v, int bump)
{
    if (config_tst_d(CONFIG_JOYSTICK_AXIS_X0, a))
        look_stick_x[0] = v;
    if (config_tst_d(CONFIG_JOYSTICK_AXIS_Y0, a))
        look_stick_y[0] = v;
    if (config_tst_d(CONFIG_JOYSTICK_AXIS_X1, a))
        look_stick_x[1] = v;
    if (config_tst_d(CONFIG_JOYSTICK_AXIS_Y1, a))
        look_stick_y[1] = v;
}

static int look_click(int b, int d)
{
    if (d && config_tst_d(CONFIG_MOUSE_CAMERA_R, b))
        look_panning = 1;
    else if (config_tst_d(CONFIG_MOUSE_CAMERA_R, b))
        look_panning = 0;

    return 1;
}

static int look_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT || c == KEY_LOOKAROUND)
            return goto_state(&st_play_loop);
        if (c == SDLK_LSHIFT)
            look_stick_z = -1;
        if (c == SDLK_SPACE)
            look_stick_z = 1;
    }
    else if (!d)
    {
        if (c == SDLK_LSHIFT || c == SDLK_SPACE)
            look_stick_z = 0;
    }

    return 1;
}

static int look_buttn(int b, int d)
{
    if (d && (config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b)))
        return goto_state(&st_play_loop);

    return 1;
}

/*---------------------------------------------------------------------------*/

struct state st_play_ready = {
    play_ready_enter,
    play_shared_leave,
    play_ready_paint,
    play_ready_timer,
    NULL,
    play_ready_stick,
    NULL,
    play_ready_click,
    play_ready_keybd,
    play_ready_buttn,
    NULL,
    NULL,
    play_shared_fade,
    play_shared_exit
};

struct state st_play_set = {
    play_set_enter,
    play_shared_leave,
    play_set_paint,
    play_set_timer,
    NULL,
    play_set_stick,
    NULL,
    play_set_click,
    play_set_keybd,
    play_set_buttn,
    NULL,
    NULL,
    play_shared_fade,
    play_shared_exit
};

struct state st_play_loop = {
    play_loop_enter,
    play_shared_leave,
    play_loop_paint,
    play_loop_timer,
    play_loop_point,
    play_loop_stick,
    play_loop_angle,
    play_loop_click,
    play_loop_keybd,
    play_loop_buttn,
    play_loop_wheel,
    play_loop_touch,
    play_shared_fade,
    play_shared_exit
};

struct state st_look = {
    look_enter,
    look_leave,
    look_paint,
    look_timer,
    look_point,
    look_stick,
    NULL,
    look_click,
    look_keybd,
    look_buttn,
    NULL,
    NULL,
    play_shared_fade,
    play_shared_exit
};
