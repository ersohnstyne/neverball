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

#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

/*
 * HACK: Used with console version
 */
#include "console_control_gui.h"

#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

#if NB_HAVE_PB_BOTH==1
#include "powerup.h"
#include "account.h"
#include "account_wgcl.h"
#include "solid_chkp.h"
#endif

#ifdef MAPC_INCLUDES_CHKP
#include "checkpoints.h" /* New: Checkpoints */
#endif

#include "gui.h"
#include "transition.h"
//#include "hud.h"
//#include "demo.h"
//#include "progress.h"
//#include "audio.h"
//#include "config.h"
//#include "video.h"
//#include "cmd.h"
//#include "key.h"

//#include "geom.h"
//#include "vec3.h"

//#include "game_draw.h"
//#include "game_common.h"
//#include "game_server.h"
//#include "game_proxy.h"
//#include "game_client.h"

#include "st_play_sync.h"

#include "st_common.h"
#include "st_play.h"
#include "st_goal.h"
#include "st_fail.h"
#include "st_pause.h"
#include "st_level.h"
#include "st_shared.h"
#include "st_tutorial.h"

/*---------------------------------------------------------------------------*/

#define COUNTDOWN_PREPARATION_CROSS_LEN 20.0f * (video.device_h / 1920.0f)

typedef struct __countdown_preparation
{
    int   id;
    int   isgreen;

    float positions[2];
    float position_scaled;
    float alpha;
    float colorfill[4];
} countdown_preparation;

/* Have modern preparation start timer countdown */
static countdown_preparation local_countdown_preparation[12];

static void __countdown_preparation_init()
{
    for (int i = 0; i < 12; i++)
    {
        memset(&local_countdown_preparation[i], 0,
               sizeof (countdown_preparation));
        local_countdown_preparation[i].id = i;
        local_countdown_preparation[i].isgreen = 0;

        if (video.device_w <= video.device_h)
        {
            local_countdown_preparation[i].positions[0] =
                fsinf(V_PI * (i / 6.0f)) * (video.device_h * 0.125f);
            local_countdown_preparation[i].positions[1] =
                -fcosf(V_PI * (i / 6.0f)) * (video.device_h * 0.125f);
        }
        else
        {
            local_countdown_preparation[i].positions[0] =
                fsinf(V_PI * (i / 6.0f)) * (video.device_w * 0.125f);
            local_countdown_preparation[i].positions[1] =
                -fcosf(V_PI * (i / 6.0f)) * (video.device_w * 0.125f);
        }
    }
}

static void __countdown_preparation_draw()
{
    for (int i = 0; i < 12; i++)
    {
        video_set_ortho();

        if (local_countdown_preparation[i].isgreen)
        {
            local_countdown_preparation[i].alpha = 3.0f - (4 * time_state());
            local_countdown_preparation[i].position_scaled =
                1.0f - fcosf(V_PI * time_state()) * (time_state() * 2);

            local_countdown_preparation[i].colorfill[0] = 0.0f;
            local_countdown_preparation[i].colorfill[1] = 1.0f;
            local_countdown_preparation[i].colorfill[2] = 0.0f;
        }
        else
        {
            float time_state_offset = time_state() +
                                      (local_countdown_preparation[i].id / 12.0f);

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

#if ENABLE_MOTIONBLUR!=0
        local_countdown_preparation[i].colorfill[3] =
            config_get_d(CONFIG_MOTIONBLUR) ? (1.0f * video_motionblur_alpha_get()) : 1.0f;
#else
        local_countdown_preparation[i].colorfill[3] = 1.f;
#endif

        glTranslatef(
            local_countdown_preparation[i].positions[0] *
            local_countdown_preparation[i].position_scaled,
            local_countdown_preparation[i].positions[1] *
            local_countdown_preparation[i].position_scaled,
            0.0f
        );
        glTranslatef(video.device_w / 2.0f, video.device_h / 2.0f, 0.0f);

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
        {
            const char *curr_setname = set_name(curr_set());

            if (!curr_setname)
            {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(setname, MAXSTR,
#else
                sprintf(setname,
#endif
                        _("Untitled set name (%d)"), curr_set());
            }
            else
                 SAFECPY(setname, curr_setname);
        }

        const char *curr_lvlname = level_name(curr_level());

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(lvlname, MAXSTR,
#else
        sprintf(lvlname,
#endif
                level_bonus(curr_level()) ? "%s - Bonus %s" : "%s - %s",
                setname, curr_lvlname ? curr_lvlname : "---");


        const char *curr_setid = set_id(curr_set());
        char curr_setid_final[MAXSTR];

        if (!curr_setid)
        {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(curr_setid_final, MAXSTR,
#else
            sprintf(curr_setid_final,
#endif
                    _("none_%d"), curr_set());
        }
        else
            SAFECPY(curr_setid_final, curr_setid);


#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        if (str_starts_with(curr_setid_final, "SB") ||
            str_starts_with(curr_setid_final, "sb") ||
            str_starts_with(curr_setid_final, "Sb") ||
            str_starts_with(curr_setid_final, "sB"))
            hud_lvlname_campaign(lvlname, level_bonus(curr_level()));
        else
#endif
        if (str_starts_with(curr_setid_final, "anime"))
            hud_lvlname_set_ana(lvlname, level_bonus(curr_level()));
        else
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

#if !defined(__WII__)
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
#endif

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

/*---------------------------------------------------------------------------*/

static const float time_in   = 0.5f;
static const float time_out  = 0.6f;
static const int   flags_in  = GUI_E | GUI_FLING | GUI_EASE_BACK;
static const int   flags_out = GUI_W | GUI_FLING | GUI_EASE_BACK | GUI_BACKWARD;

/*---------------------------------------------------------------------------*/

#ifdef MAPC_INCLUDES_CHKP
static int restart_cancel_allchkp;
#endif

static float prep_tilt_x;
static float prep_tilt_y;

static int lmb_holded;
static int rmb_holded;

static float lmb_hold_time;
static float rmb_hold_time;

static int play_freeze_all;

static int play_update_server = 0;
static int play_update_client = 1;

static int use_mouse;
static int use_keyboard;

static int ready_transition = 0;

static float devicemotion_timer_tilt;
static int   devicemotion_tilt_can_autocalibrate;
static int   devicemotion_tilt_init_x;
static int   devicemotion_tilt_init_y;

int play_pause_goto(struct state *returnable)
{
    play_freeze_all = 1;

    return goto_pause(returnable);
}

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

static int play_ready_enter(struct state *st, struct state *prev, int intent)
{
    devicemotion_tilt_can_autocalibrate = 1;

    prep_tilt_x = 0;
    prep_tilt_y = 0;

    lmb_holded = 0;
    rmb_holded = 0;

    lmb_hold_time = -0.01f;
    rmb_hold_time = -0.01f;

#ifdef MAPC_INCLUDES_CHKP
    restart_cancel_allchkp = 0;
#endif
    play_freeze_all = 0;

    video_set_grab(1);

    if (curr_mode() == MODE_NONE)
    {
        hud_cam_pulse(config_get_d(CONFIG_CAMERA));

        /* Cannot run traffic lights in home room. */

        return 0;
    }

    audio_play("snd/2.2/game_countdown_prep.ogg", 1.0f);
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    /* FIXME: WGCL Narrator can do it! */

    /*EM_ASM({
        if (navigator.language.startsWith("ja") || navigator.language.startsWith("jp") || gameoptions_debug_locale_japanese)
            CoreLauncherOptions_GameOptions_PlayNarratorAudio("ja-JP/corelauncher_narrator_trafficlight_ready.mp3");
    });*/
#elif NB_HAVE_PB_BOTH!=1 || !defined(__EMSCRIPTEN__)
    audio_narrator_play(AUD_READY);
#endif
    hud_speedup_reset();

    if (play_update_client)
    {
        game_client_sync(
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                         !campaign_hardcore_norecordings() &&
#endif
                         curr_mode() != MODE_NONE ? demo_fp : NULL);
        game_client_blend(game_server_blend());

        play_update_client = 0;
        play_update_server = 1;
    }

    hud_update(0, 0.0f);
    hud_update_camera_direction(curr_viewangle());

    if (!console_gui_shown())
    {
        hud_show(0.0f);
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
        EM_ASM({ Neverball.WGCLshowGameHUD(); });
        if (curr_mode() == MODE_CHALLENGE || curr_mode() == MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
         || curr_mode() == MODE_HARDCORE
#endif
            )
            EM_ASM({ Neverball.WGCLshowChallengeHUD(); });
#endif
    }

    hud_cam_pulse(config_get_d(CONFIG_CAMERA));

    int id = play_ready_gui();
    gui_slide(id, flags_in, 0, time_in, 0);
    return id;
}

static void play_ready_timer(int id, float dt)
{
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    /* HACK: Do not attempt, when the level is loading. */

    if (EM_ASM_INT({ return Neverball.wgclIsLevelLoading; })) return;
#endif

    game_lerp_pose_point_tick(dt);
    geom_step(dt);

    float t = time_state();

    game_client_fly(1.0f - 0.5f * t);

    if (dt > 0.0f && t > 1.0f && !st_global_animating())
        goto_state(&st_play_set);

    set_lvlinfo();

    game_step_fade(dt);

    if (play_update_client)
    {
        game_client_sync(
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                         !campaign_hardcore_norecordings() &&
#endif
                         curr_mode() != MODE_NONE ? demo_fp : NULL);
        game_client_blend(game_server_blend());

        play_update_client = 0;
        play_update_server = 1;
    }

    gui_timer(id, dt);

    if (t >= 1.0f && !ready_transition)
    {
        gui_slide(id, flags_out, 0, time_out, 0);
        ready_transition = 1;
    }

    /* Powerful screen animations! */

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    if (console_gui_shown() && !config_get_d(CONFIG_SCREEN_ANIMATIONS))
        hud_cam_timer(dt);
    else
#endif
        hud_timer(dt);
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

static int play_set_enter(struct state *st, struct state *prev, int intent)
{
#ifdef MAPC_INCLUDES_CHKP
    restart_cancel_allchkp = 0;
#endif
    play_freeze_all = 0;

    /* Cannot run traffic lights in home room. */

    if (curr_mode() == MODE_NONE) return 0;

    audio_play("snd/2.2/game_countdown_prep.ogg", 1.0f);
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    /* FIXME: WGCL Narrator can do it! */

    /*EM_ASM({
        if (navigator.language.startsWith("ja") || navigator.language.startsWith("jp") || gameoptions_debug_locale_japanese)
            CoreLauncherOptions_GameOptions_PlayNarratorAudio("ja-JP/corelauncher_narrator_trafficlight_set.mp3");
    });*/
#elif NB_HAVE_PB_BOTH!=1 || !defined(__EMSCRIPTEN__)
    audio_narrator_play(AUD_SET);
#endif

    if (!console_gui_shown())
    {
        hud_show(0.0f);
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
        EM_ASM({ Neverball.WGCLshowGameHUD(); });
        if (curr_mode() == MODE_CHALLENGE || curr_mode() == MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
         || curr_mode() == MODE_HARDCORE
#endif
            )
            EM_ASM({ Neverball.WGCLshowChallengeHUD(); });
#endif
    }

    int id = play_set_gui();
    gui_slide(id, flags_in, 0, time_in, 0);
    return id;
}

static void play_set_timer(int id, float dt)
{
    game_lerp_pose_point_tick(dt);
    geom_step(dt);
    float t = time_state();

    game_client_fly(0.5f - 0.5f * t);

    if (dt > 0.0f && t > 1.0f && !st_global_animating())
        goto_state(&st_play_loop);

    set_lvlinfo();

    game_step_fade(dt);

    if (play_update_client)
    {
        game_client_sync(
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                         !campaign_hardcore_norecordings() &&
#endif
                         curr_mode() != MODE_NONE ? demo_fp : NULL);
        game_client_blend(game_server_blend());

        play_update_client = 0;
        play_update_server = 1;
    }

    gui_timer(id, dt);

    if (t >= 1.0f && !ready_transition)
    {
        gui_slide(id, flags_out, 0, time_out, 0);
        ready_transition = 1;
    }

    /* Powerful screen animations! */

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    if (console_gui_shown() && !config_get_d(CONFIG_SCREEN_ANIMATIONS))
        hud_cam_timer(dt);
    else
#endif
        hud_timer(dt);
}

/*---------------------------------------------------------------------------*/

static int play_prep_leave(struct state *st, struct state *next, int id, int intent)
{
    if (next == &st_null)
    {
        progress_stat(GAME_NONE);
        progress_exit();

        campaign_quit();
        set_quit();

        game_server_free(NULL);
        game_client_free(NULL);
    }

    if (next == &st_pause)
    {
        hud_hide();
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
        EM_ASM({ Neverball.WGCLhideGameHUD(); });
#endif
    }

    gui_slide(id, flags_out | GUI_REMOVE, 0, time_out, 0);
    transition_add(id);
    return id;
}

static void play_prep_paint(int id, float t)
{
    game_client_draw(0, t);

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    if (current_platform != PLATFORM_PC || console_gui_shown())
    {
        console_gui_preparation_paint();

        if (config_get_d(CONFIG_SCREEN_ANIMATIONS))
        {
            hud_paint();
            hud_lvlname_paint();
        }
    }
    else
#endif
    if (hud_visibility() || config_get_d(CONFIG_SCREEN_ANIMATIONS))
    {
        hud_paint();
        hud_lvlname_paint();
    }

    gui_paint(id);
}

static void play_prep_point(int id, int x, int y, int dx, int dy)
{
    hud_show(0.0f);
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    EM_ASM({
        Neverball.WGCLshowGameHUD();
        Neverball.WGCLhideGameHUDTouch();
    });

    if (curr_mode() == MODE_CHALLENGE || curr_mode() == MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
     || curr_mode() == MODE_HARDCORE
#endif
        )
        EM_ASM({ Neverball.WGCLshowChallengeHUD(); });
#endif
}

static void play_prep_stick(int id, int a, float v, int bump)
{
    if (!use_mouse && use_keyboard)
    {
        if (config_tst_d(CONFIG_JOYSTICK_AXIS_X0, a))
            prep_tilt_x = v * powerup_get_tilt_multiply();
        if (config_tst_d(CONFIG_JOYSTICK_AXIS_Y0, a))
            prep_tilt_y = (curr_mode() == MODE_BOOST_RUSH ? 0 :
                                                            v * powerup_get_tilt_multiply());

        game_set_z(prep_tilt_x);
        game_set_x(prep_tilt_y);
    }
    else
    {
        use_mouse = 0;
        use_keyboard = 1;
    }
}

static int play_prep_click(int b, int d)
{
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    /* HACK: Do not attempt, when the level is loading. */

    if (EM_ASM_INT({ return Neverball.wgclIsLevelLoading; })) return 1;
#endif

    hud_show(0.0f);
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    EM_ASM({
        Neverball.WGCLshowGameHUD();
        Neverball.WGCLhideGameHUDTouch();
    });

    if (curr_mode() == MODE_CHALLENGE || curr_mode() == MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
     || curr_mode() == MODE_HARDCORE
#endif
        )
        EM_ASM({ Neverball.WGCLshowChallengeHUD(); });
#endif

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    if (current_platform == PLATFORM_PC)
#endif
    {
        use_mouse = 1; use_keyboard = 0;

        if (config_tst_d(CONFIG_MOUSE_CAMERA_L, b))
            lmb_holded = d;

        if (config_tst_d(CONFIG_MOUSE_CAMERA_R, b))
            rmb_holded = d;
    }

    if (d)
    {
#if !defined(__WII__)
        click_camera(b);
#endif

        if (b == SDL_BUTTON_LEFT && !opt_touch)
            goto_state(&st_play_loop);
    }
    return 1;
}

static int play_prep_keybd(int c, int d)
{
    hud_show(0.0f);
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    EM_ASM({ Neverball.WGCLshowGameHUD(); });

    if (curr_mode() == MODE_CHALLENGE || curr_mode() == MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
     || curr_mode() == MODE_HARDCORE
#endif
        )
        EM_ASM({ Neverball.WGCLshowChallengeHUD(); });
#endif

    if (d)
    {
#if !defined(__WII__)
        keybd_camera(c);
#endif

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

static int play_prep_buttn(int b, int d)
{
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    /* HACK: Do not attempt, when the level is loading. */

    if (EM_ASM_INT({ return Neverball.wgclIsLevelLoading; })) return 1;
#endif

    if (d)
    {
        buttn_camera(b);

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return goto_state(&st_play_loop);
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

static int fast_rotate;
static int loop_transition;
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

static int play_loop_enter(struct state *st, struct state *prev, int intent)
{
    devicemotion_tilt_can_autocalibrate = 1;

    smoothfix_slowdown_time = 0;
#ifdef MAPC_INCLUDES_CHKP
    restart_cancel_allchkp  = 0;
#endif
    play_freeze_all         = 0;
    play_block_state        = 0;
    rot_init();

    if (opt_touch)
    {
        prep_tilt_x = 0;
        prep_tilt_y = 0;

        lmb_holded = 0;
        rmb_holded = 0;
    }

    lmb_hold_time = -0.01f;
    rmb_hold_time = -0.01f;

    fast_rotate     = 0;
    max_speed       = 0;
    man_rot         = 0;
    rotation_offset = 0;

    global_prev = prev;
    video_set_grab(1);

    devicemotion_timer_tilt = 0;
    tilt_x = prep_tilt_x; tilt_y = prep_tilt_y;

    prep_tilt_x = 0;
    prep_tilt_y = 0;

    /* Cannot run traffic lights in home room. */

    if (curr_mode() == MODE_NONE) return 0;

    devicemotion_timer_tilt = 10.0f;

    hud_update(0, 0.0f);
    hud_show(0.0f);
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    EM_ASM({ Neverball.WGCLshowGameHUD(); });

    if (curr_mode() == MODE_CHALLENGE || curr_mode() == MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
     || curr_mode() == MODE_HARDCORE
#endif
        )
        EM_ASM({ Neverball.WGCLshowChallengeHUD(); });
#endif

    if ((prev != &st_play_ready &&
         prev != &st_play_set &&
         prev != &st_tutorial) ||
        prev == &st_play_loop)
        return 0;

    audio_play("snd/2.2/game_countdown_go.ogg", 1.0f);
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    /* FIXME: WGCL Narrator can do it! */

    /*EM_ASM({
        if (navigator.language.startsWith("ja") || navigator.language.startsWith("jp") || gameoptions_debug_locale_japanese)
            CoreLauncherOptions_GameOptions_PlayNarratorAudio("ja-JP/corelauncher_narrator_trafficlight_start.mp3");
    });*/
#elif NB_HAVE_PB_BOTH!=1 || !defined(__EMSCRIPTEN__)
    audio_narrator_play(AUD_GO);
#endif

#if defined(ENABLE_POWERUP) && defined(CONFIG_INCLUDES_ACCOUNT)
    if (powerup_get_coin_multiply() == 2)
    {
#if ENABLE_RFD==1
        if (!progress_rfd_take_powerup(0))
#endif
            account_wgcl_do_add(0, 0, 0, -1, 0, 0);
    }
    if (powerup_get_grav_multiply() <= 0.51f)
    {
#if ENABLE_RFD==1
        if (!progress_rfd_take_powerup(1))
#endif
            account_wgcl_do_add(0, 0, 0, 0, -1, 0);
    }
    if (powerup_get_tilt_multiply() == 2)
    {
#if ENABLE_RFD==1
        if (!progress_rfd_take_powerup(2))
#endif
            account_wgcl_do_add(0, 0, 0, 0, 0, -1);
    }
    account_wgcl_save();
#endif

    game_client_fly(0.0f);

    loop_transition = 0;

    int id = play_loop_gui();
    gui_slide(id, flags_in, 0, time_in, 0);
    return id;
}

static int play_loop_leave(struct state *st, struct state *next, int id, int intent)
{
    game_client_maxspeed(0.0f, 0);

    if (next == &st_null)
    {
        progress_stat(GAME_NONE);
        progress_exit();

        campaign_quit();
        set_quit();

        game_server_free(NULL);
        game_client_free(NULL);
    }

    hud_hide();
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    EM_ASM({ Neverball.WGCLhideGameHUD(); });
#endif

    gui_delete(id);
    return 0;
}

static void play_loop_paint(int id, float t)
{
    game_client_draw(0, t);

    if (hud_visibility() || config_get_d(CONFIG_SCREEN_ANIMATIONS))
    {
        hud_paint();
        hud_lvlname_paint();
    }

    if (config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_set_alpha(id, CLAMP(0, 1.5f - time_state(), 1), GUI_ANIMATION_NONE);

    if (!st_global_animating() && id &&
        (time_state() < 1.0f || config_get_d(CONFIG_SCREEN_ANIMATIONS)))
        gui_paint(id);
}

static void play_loop_timer(int id, float dt)
{
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    /* HACK: Do not attempt, when the level is loading. */

    if (EM_ASM_INT({ return Neverball.wgclIsLevelLoading; })) return;
#endif

    ST_PLAY_SYNC_SMOOTH_FIX_TIMER(smoothfix_slowdown_time);

    game_lerp_pose_point_tick(dt);

    if (!game_client_get_jump_b() && !play_freeze_all)
        geom_step(dt);

    /* Boost rush uses auto forward */
    if (curr_mode() == MODE_BOOST_RUSH)
        game_set_x(curr_speed_percent() / 100.0f * -0.875f +
                   (time_state() < 1.0f && global_prev != &st_pause ? -0.5f :
                                                                       0));

    float k = (fast_rotate ?
               (float) config_get_d(CONFIG_ROTATE_FAST) :
               (float) config_get_d(CONFIG_ROTATE_SLOW)) / 100.0f;

    float r = 0.0f;

    hud_update_camera_direction(curr_viewangle());
    gui_timer(id, dt);
    hud_timer(dt);

    if (time_state() >= 1.0f && !loop_transition)
    {
        gui_slide(id, flags_out, 0, 0.6f, 0);
        loop_transition = 1;
    }

    devicemotion_timer_tilt += dt;

    /* Switchball works on max speed and manual rotation */

    if (!max_speed && use_mouse && !use_keyboard)
    {
        tilt_x = flerp(0, tilt_x, 0.5f);
        tilt_y = flerp(0, tilt_y, 0.5f);

        game_client_maxspeed(0.0f, 0);
    }
    else if (max_speed)
        game_client_maxspeed(V_DEG(fatan2f(tilt_y, tilt_x)), 1);

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

    if (!play_freeze_all &&
        play_update_server && !play_update_client)
    {
        game_server_step(dt);

        play_update_server = 0;
        play_update_client = 1;
    }

    if (play_update_client && !play_update_server)
    {
        game_client_sync(
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                         !campaign_hardcore_norecordings() &&
#endif
                         curr_mode() != MODE_NONE ? demo_fp : NULL);
        game_client_blend(game_server_blend());

        play_update_client = 0;
        play_update_server = 1;
    }

    /* Cannot update state in home room. */

    if (curr_mode() == MODE_NONE) return;

    if (curr_status() == GAME_NONE && !play_freeze_all)
    {
#if ENABLE_LIVESPLIT!=0
        progress_livesplit_pause(0);
        progress_livesplit_step(dt);
#endif
        progress_step();
    }
    else if (!play_freeze_all && !play_block_state)
    {
        play_block_state = 1;
        progress_stat(curr_status());
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        goto_state(curr_status() == GAME_GOAL ?
                   (campaign_used() && curr_mode() == MODE_HARDCORE &&
                    !progress_done() ? &st_goal_hardcore :
                                       &st_goal) :
                   &st_fail);
#else
        goto_state(curr_status() == GAME_GOAL ?
                   &st_goal : &st_fail);
#endif
    }
}

static void play_loop_point(int id, int x, int y, int dx, int dy)
{
#if (NDEBUG || _MSC_VER) && !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__)
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    if (current_platform == PLATFORM_PC)
#endif
    {
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
        EM_ASM({ Neverball.WGCLhideGameHUDTouch(); });
#endif

        if (use_mouse && !use_keyboard)
        {
            use_mouse = 1; use_keyboard = 0;

            if (max_speed)
            {
                tilt_x = CLAMP((-ANGLE_BOUND * 20) * powerup_get_tilt_multiply(),
                               tilt_x + dx,
                               ( ANGLE_BOUND * 20) * powerup_get_tilt_multiply());
                tilt_y = CLAMP((-ANGLE_BOUND * 20) * powerup_get_tilt_multiply(),
                               tilt_y + dy,
                               ( ANGLE_BOUND * 20) * powerup_get_tilt_multiply());

                game_set_pos_max_speed(tilt_x * powerup_get_tilt_multiply(),
                                       curr_mode() == MODE_BOOST_RUSH ? 0 : tilt_y * powerup_get_tilt_multiply());
            }
            else if (man_rot)
            {
                /* Reset tilt to horizontal */

                game_set_pos(0, 0);

                rotation_offset = (-50.f * dx) / config_get_d(CONFIG_MOUSE_SENSE);
            }
            else if (!max_speed && !man_rot)
            {
                tilt_x += opt_touch ? 0 : dx * 10;
                tilt_y += opt_touch ? 0 : dy * 10;

                game_set_pos((tilt_x * powerup_get_tilt_multiply()) * 10,
                             curr_mode() == MODE_BOOST_RUSH ? 0 : (tilt_y * powerup_get_tilt_multiply()) * 10);
            }
        }
        else use_mouse = 1; use_keyboard = 0;
    }
#endif
}

static void play_loop_stick(int id, int a, float v, int bump)
{
    if (!use_mouse && use_keyboard)
    {
        use_mouse = 0; use_keyboard = 1;

        if (config_tst_d(CONFIG_JOYSTICK_AXIS_X0, a))
            tilt_x = v * powerup_get_tilt_multiply();
        if (config_tst_d(CONFIG_JOYSTICK_AXIS_Y0, a))
            tilt_y = (curr_mode() == MODE_BOOST_RUSH ? 0 :
                                                       v * powerup_get_tilt_multiply());

        /* Camera movement */

        if (config_tst_d(CONFIG_JOYSTICK_AXIS_X1, a))
        {
            if (v + axis_offset[2] > 0.0f)
                rot_set(DIR_R, -v + axis_offset[2], 1);
            else if (v + axis_offset[2] < 0.0f)
                rot_set(DIR_L, +v + axis_offset[2], 1);
            else
                rot_clr(DIR_R | DIR_L);
        }
        if (config_tst_d(CONFIG_JOYSTICK_AXIS_Y1, a))
            game_set_zoom_rate(v);

        game_set_z(tilt_x);
        game_set_x(tilt_y);
    }
    else use_mouse = 0; use_keyboard = 1;
}

static void play_loop_angle(int id, float x, float z)
{
    game_set_ang(x, z);

    /* TODO: Throttle the camera rotation */
}

static int play_loop_click(int b, int d)
{
    if (opt_touch)
    {
        use_mouse = 1; use_keyboard = 0;

        if (config_tst_d(CONFIG_MOUSE_CAMERA_R, b))
        {
            rmb_holded    = d;
            rmb_hold_time = d ? 1000.0f : -0.01f;
            man_rot       = d;
        }

#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
        EM_ASM({ Neverball.WGCLshowGameHUDTouch(); });
#endif

        return 1;
    }

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    if (current_platform == PLATFORM_PC)
#endif
    {
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
        EM_ASM({ Neverball.WGCLhideGameHUDTouch(); });
#endif

        use_mouse = 1; use_keyboard = 0;

        if (config_tst_d(CONFIG_MOUSE_CAMERA_L, b))
            lmb_holded = d;

        if (config_tst_d(CONFIG_MOUSE_CAMERA_R, b))
            rmb_holded = d;
    }

    if (d && use_mouse && !use_keyboard)
    {
        /*if (config_tst_d(CONFIG_MOUSE_CAMERA_R, b))
            rot_set(DIR_R, 1.0f, 0);
        if (config_tst_d(CONFIG_MOUSE_CAMERA_L, b))
            rot_set(DIR_L, 1.0f, 0);*/

#if !defined(__WII__)
        click_camera(b);
#endif
    }
    else if (use_mouse && !use_keyboard
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
          && current_platform == PLATFORM_PC
#endif
        )
    {
        /*if (config_tst_d(CONFIG_MOUSE_CAMERA_R, b))
            rot_clr(DIR_R);
        if (config_tst_d(CONFIG_MOUSE_CAMERA_L, b))
            rot_clr(DIR_L);*/
    }
#endif

    return 1;
}

static int play_loop_keybd(int c, int d)
{
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
    if (d)
    {

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        if (current_platform == PLATFORM_PC)
#endif
        {
#ifdef MAPC_INCLUDES_CHKP
            if (c == SDLK_LCTRL || c == SDLK_LGUI)
                restart_cancel_allchkp = 1;
#endif
            if (config_tst_d(CONFIG_KEY_CAMERA_R, c))
                rot_set(DIR_R, 1.0f, 0);
            if (config_tst_d(CONFIG_KEY_CAMERA_L, c))
                rot_set(DIR_L, 1.0f, 0);
            if (config_tst_d(CONFIG_KEY_ROTATE_FAST, c))
                fast_rotate = 1;

            keybd_camera(c);

            /*if (config_tst_d(CONFIG_KEY_RESTART, c) &&
                progress_same_avail())
            {
                play_freeze_all = 1;

#ifdef MAPC_INCLUDES_CHKP
                if (restart_cancel_allchkp)
                    checkpoints_stop();

                if (last_active)
                    if (!checkpoints_load())
                        checkpoints_stop();
#endif
                if (progress_same())
                    goto_state(&st_play_ready);
            }*/
            if (c == KEY_EXIT)
            {
                play_freeze_all = 1;
                hud_speedup_reset();
                goto_pause(curr_state());
            }
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
    if (d && (c == KEY_POSE || c == KEY_TOGGLESHOWHUD))
        toggle_hud_visibility(!hud_visibility());

#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
    /* Used centered player or special camera orientation */
    if (d && c == KEY_LOOKAROUND && config_cheat())
        return goto_state(&st_look);
#endif
#endif

    return 1;
}

static int play_loop_buttn(int b, int d)
{
    if (d)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
        {
            play_freeze_all = 1;
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

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__)
static int touch_arrow_enabled;

static int play_loop_touch(const SDL_TouchFingerEvent *e)
{
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    EM_ASM({ Neverball.WGCLshowGameHUDTouch(); });
#endif

    devicemotion_timer_tilt = 0;

    static SDL_FingerID rotate_finger = -1;

    static float rotate = 0.0f; /* Filtered input. */

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
        {
            touch_arrow_enabled = 0;
            game_client_maxspeed(0.0f, touch_arrow_enabled);
            play_pause_goto(curr_state());
        }
        else if (token == GUI_CAMERA)
            next_camera();

        gui_focus(0);
    }
    else if (e->type == SDL_FINGERDOWN)
    {
        SDL_Finger *camrot_finger = SDL_GetTouchFinger(e->touchId, 1); /* Second finger. */

        if (camrot_finger && e->fingerId == camrot_finger->id)
        {
            rotate_finger = camrot_finger->id;
            rotate = 0.0f;
        }
        else
        {
            touch_arrow_enabled = 1;
            lmb_holded    = 1;
            lmb_hold_time = 1000.0f;
            max_speed     = 1;
        }
    }
    else if (e->type == SDL_FINGERUP)
    {
        if (e->fingerId == rotate_finger)
        {
            rotate_finger = -1;
            rot_clr(DIR_R | DIR_L);
            rotate = 0.0f;
        }
        else
        {
            touch_arrow_enabled = 0;
            lmb_holded    = 0;
            lmb_hold_time = -0.01f;
            max_speed     = 0;
            tilt_x = 0; tilt_y = 0;

            game_client_maxspeed(0.0f, 0);
            game_set_pos(0, 0);
        }
    }
    else if (e->type == SDL_FINGERMOTION)
    {
        if (e->fingerId == rotate_finger)
        {
            /* Discard accumulated input when moving in the opposite direction. */

            if ((rotate < 0.0f && e->dx > 0.0f) || (e->dx < 0.0f && rotate > 0.0f))
                rotate = 0.0f;

            /* Filter the input for a smoother experience. */

            rotate += e->dx * 0.6f;

            /*
             * touch_rotate gives the fraction of the screen that you need to swipe
             * across to reach rotate_slow rotation speed. E.g., a value of 32
             * is 1/32 of screen.
             *
             * To rotate slower, swipe a smaller distance than that.
             * To rotate faster, swipe farther.
             */

            if (rotate != 0.0f)
            {
                const float scaled_rotate = (float) config_get_d(CONFIG_TOUCH_ROTATE) * rotate;
                rot_set(DIR_L, CLAMP(-rmax, scaled_rotate, +rmax), 1);
            }
        }
        else
        {
            int dx = (int) ((float) video.device_w * +e->dx);
            int dy = (int) ((float) video.device_h * -e->dy);

            game_client_maxspeed(V_DEG(fatan2f(dy, dx)), touch_arrow_enabled);

            tilt_x = CLAMP((-ANGLE_BOUND * 20) * powerup_get_tilt_multiply(),
                           tilt_x + dx,
                           ( ANGLE_BOUND * 20) * powerup_get_tilt_multiply());
            tilt_y = CLAMP((-ANGLE_BOUND * 20) * powerup_get_tilt_multiply(),
                           tilt_y + dy,
                           ( ANGLE_BOUND * 20) * powerup_get_tilt_multiply());

            game_set_pos_max_speed(tilt_x * powerup_get_tilt_multiply(),
                                   curr_mode() == MODE_BOOST_RUSH ? 0 : tilt_y * powerup_get_tilt_multiply());
            /* game_set_pos(dx, dy); */
        }
    }

    return 1;
}
#endif

static void play_loop_wheel(int x, int y)
{
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
    /*
     * Hardcoded camera view position settings on 2.2 and later:
     * DP: 75
     * DC: 25
     * DZ: 200 (adds 20)
     */

    if (y > 0) game_set_zoom(-0.05f);
    if (y < 0) game_set_zoom(+0.05f);
#endif
}

/*---------------------------------------------------------------------------*/

void wgcl_play_touch_pause(void)
{
    goto_pause(curr_state());
}

void wgcl_play_touch_cammode(void)
{
    next_camera();
}

void wgcl_play_touch_rotate_camera(int rot_l, int rot_r)
{
    if      (rot_l && rot_r) rot_clr(DIR_L | DIR_R);
    else if (rot_l)          rot_set(DIR_R, 1.0f, 0);
    else if (rot_r)          rot_set(DIR_L, 1.0f, 0);
    else                     rot_clr(DIR_L | DIR_R);
}

void wgcl_play_touch_zoom_camera(int v)
{
    if      (v < 0) game_set_zoom_rate(+1.0f);
    else if (v > 0) game_set_zoom_rate(-1.0f);
    else            game_set_zoom_rate(0.0f);
}

void wgcl_play_devicemotion_tilt(int x, int y)
{
    const int parsed_x = x > 180 ? x - 360 : x;
    const int parsed_y = y > 180 ? y - 360 : y;

    if (devicemotion_tilt_can_autocalibrate == 1)
    {
        devicemotion_tilt_can_autocalibrate = 0;

        devicemotion_tilt_init_x = x;
        devicemotion_tilt_init_y = y;
    }

    if (devicemotion_timer_tilt >= 10.0f)
    {
        tilt_x = (parsed_x - devicemotion_tilt_init_x) / ANGLE_BOUND;
        tilt_y = (parsed_y - devicemotion_tilt_init_y) / ANGLE_BOUND;

        game_set_pos(tilt_x * 2, tilt_y * 2);
    }
}

/*---------------------------------------------------------------------------*/

static float phi, theta;

static int   look_panning;
static float look_stick_x[2],
             look_stick_y[2],
             look_stick_z;

static int look_enter(struct state *st, struct state *prev, int intent)
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

static int look_leave(struct state *st, struct state *next, int id, int intent)
{
    if (next == &st_null)
    {
        progress_stat(GAME_NONE);
        progress_exit();

        campaign_quit();
        set_quit();

        game_server_free(NULL);
        game_client_free(NULL);
    }

    return 0;
}

static void look_timer(int id, float dt)
{
    theta += look_stick_x[1] *  2 * (dt * 4000);
    phi   += look_stick_y[1] * -2 * (dt * 4000);

    if (phi > +90.0f)    phi    = +90.0f;
    if (phi < -90.0f)    phi    = -90.0f;

    if (theta > +180.0f) theta -= 360.0f;
    if (theta < -180.0f) theta += 360.0f;

    float look_moves[2] = {
        (fcosf((V_PI * theta) / 180) * +look_stick_x[0]) +
        (fsinf((V_PI * theta) / 180) * -look_stick_y[0]),
        (fcosf((V_PI * theta) / 180) * +look_stick_y[0]) +
        (fsinf((V_PI * theta) / 180) * +look_stick_x[0])
    };

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
    play_prep_leave,
    play_prep_paint,
    play_ready_timer,
    play_prep_point,
    play_prep_stick,
    NULL,
    play_prep_click,
    play_prep_keybd,
    play_prep_buttn,
    NULL,
    NULL,
    play_shared_fade
};

struct state st_play_set = {
    play_set_enter,
    play_prep_leave,
    play_prep_paint,
    play_set_timer,
    play_prep_point,
    play_prep_stick,
    NULL,
    play_prep_click,
    play_prep_keybd,
    play_prep_buttn,
    NULL,
    NULL,
    play_shared_fade
};

struct state st_play_loop = {
    play_loop_enter,
    play_loop_leave,
    play_loop_paint,
    play_loop_timer,
    play_loop_point,
    play_loop_stick,
    play_loop_angle,
    play_loop_click,
    play_loop_keybd,
    play_loop_buttn,
    play_loop_wheel,
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__)
    play_loop_touch,
#else
    NULL,
#endif
    play_shared_fade
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
    play_shared_fade
};
