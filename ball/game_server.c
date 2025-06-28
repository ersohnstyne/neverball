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

#include <math.h>
#ifndef NDEBUG
#include <assert.h>
#elif defined(_MSC_VER) && defined(_AFXDLL)
#include <afx.h>
/**
 * HACK: assert() for Microsoft Windows Apps in Release builds
 * will be replaced to VERIFY() - Ersohn Styne
 */
#define assert VERIFY
#else
#define assert(_x) (_x)
#endif

#if NB_HAVE_PB_BOTH==1
#include "solid_chkp.h"
#include "campaign.h"
#include "powerup.h"
#include "mediation.h"
#include "networking.h"
#endif

#if ENABLE_MOON_TASKLOADER!=0
#define SKIP_MOON_TASKLOADER
#include "moon_taskloader.h"
#endif

#ifdef MAPC_INCLUDES_CHKP
#include "checkpoints.h" /* New: Checkpoints */
#endif
#include "progress.h"

#include "vec3.h"
#include "geom.h"
#include "config.h"
#include "binary.h"
#include "common.h"
#include "ease.h"
#include "state.h"

#include "solid_sim.h"
#include "solid_all.h"

#include "game_client.h"
#include "game_common.h"
#include "game_draw.h"
#include "game_server.h"
#include "game_proxy.h"

#include "cmd.h"

#if NB_HAVE_PB_BOTH==1 && !defined(MAPC_INCLUDES_CHKP)
#error Security compilation error: Please enable checkpoints after joined PB+NB Discord Server!
#endif

/*---------------------------------------------------------------------------*/

#define MAX_PLAYERS 8
#define CURR_PLAYER 0

/*---------------------------------------------------------------------------*/

static int server_state = 0;

static struct s_vary vary;

static int   timer_hold   = 0;          /* Hold timer                        */
#if NB_HAVE_PB_BOTH==1
static float time_travel  = 0;          /* Time travel                       */
#endif
static float time_limit   = 0;          /* Effective time limit              */
static float time_elapsed = 0;          /* Time elapsed                      */
static float timer        = 0.0f;       /* Clock                             */
static int   status       = GAME_NONE;  /* Outcome of the game               */

static struct game_tilt tilt;           /* Floor rotation                    */
static struct game_view view;           /* Current view                      */

static float view_k;

static float view_time;                 /* Manual rotation time              */
static float view_fade;

#define VIEW_FADE_MIN 0.2f
#define VIEW_FADE_MAX 1.0f

static float view_zoom_curr;            /* Current zoom level                */
static float view_zoom_start;           /* Starting zoom level               */
static float view_zoom_end;             /* Target zoom level                 */
static float view_zoom_time;            /* Running zoom animation time       */

#define ZOOM_DELAY (GROW_TIME * 0.5f)
#define ZOOM_TIME (ZOOM_DELAY + GROW_TIME)
#define ZOOM_MIN 0.75f
#define ZOOM_MAX 1.25f

static int   coins  = 0;                /* Collected coins                   */
static int   goal_e = 0;                /* Goal enabled flag                 */
static int   jump_e = 1;                /* Jumping enabled flag              */
static int   jump_b = 0;                /* Jump-in-progress flag             */
static float jump_dt;                   /* Jump duration                     */
static float jump_p[3];                 /* Jump destination                  */

#ifdef MAPC_INCLUDES_CHKP
static int   chkp_e  =  1;      /* New: Checkpoints; Checkpoint enabled flag */
static int   chkp_id = -1;
#endif

static int   powblock_b  = 0;
static float powblock_dt = 0.0f;

static float last_diraxis = 0.0f;

static float goal_lock_p[3];

static char curr_file_name[MAXSTR];

static float player_min_area[3];
static float player_max_area[3];

/*---------------------------------------------------------------------------*/

/*
 * This is an abstraction of the game's input state.  All input is
 * encapsulated here, and all references to the input by the game are
 * made here.
 */

struct input
{
    float s;
    float x;
    float z;
    float r;
    int   c;
};

static struct input input_current;

static void input_init(void)
{
    input_current.s = RESPONSE;
    input_current.x = 0;
    input_current.z = 0;
    input_current.r = 0;
    input_current.c = 0;
}

static void input_set_s(float s)
{
    input_current.s = s;
}

static void input_set_x(float x)
{
    if (x < -ANGLE_BOUND * powerup_get_tilt_multiply()) x = -ANGLE_BOUND * powerup_get_tilt_multiply();
    if (x >  ANGLE_BOUND * powerup_get_tilt_multiply()) x =  ANGLE_BOUND * powerup_get_tilt_multiply();

    input_current.x = x;

    if ((input_current.x < -41 && input_current.x <= 0)
     || (input_current.x >  41 && input_current.x >= 0))
    {
        config_set_d(CONFIG_TILTING_FLOOR, 0);
        config_save();
    }
}

static void input_set_z(float z)
{
    if (z < -ANGLE_BOUND * powerup_get_tilt_multiply()) z = -ANGLE_BOUND * powerup_get_tilt_multiply();
    if (z >  ANGLE_BOUND * powerup_get_tilt_multiply()) z =  ANGLE_BOUND * powerup_get_tilt_multiply();

    input_current.z = z;

    if ((input_current.z < -41 && input_current.z <= 0)
     || (input_current.z >  41 && input_current.z >= 0))
    {
        config_set_d(CONFIG_TILTING_FLOOR, 0);
        config_save();
    }
}

static void input_set_r(float r)
{
    if (r < -VIEWR_BOUND) r = -VIEWR_BOUND;
    if (r >  VIEWR_BOUND) r =  VIEWR_BOUND;

    input_current.r = status == GAME_NONE ? r : 0;
}

static void input_set_c(int c)
{
    input_current.c = c;
}

static float input_get_s(void)
{
    return input_current.s;
}

static float input_get_x(void)
{
    return input_current.x;
}

static float input_get_z(void)
{
    return input_current.z;
}

static float input_get_r(void)
{
    return input_current.r;
}

static int input_get_c(void)
{
    return input_current.c;
}

/*---------------------------------------------------------------------------*/

/*
 * Utility functions for preparing the "server" state and events for
 * consumption by the "client".
 */

static union cmd cmd;

static void game_cmd_map(const char *name, int ver_x, int ver_y)
{
    cmd.type          = CMD_MAP;
    cmd.map.name      = strdup(name);
    cmd.map.version.x = ver_x;
    cmd.map.version.y = ver_y;
    game_proxy_enq(&cmd);
}

static void game_cmd_eou(void)
{
    cmd.type = CMD_END_OF_UPDATE;
    game_proxy_enq(&cmd);
}

static void game_cmd_ups(void)
{
    cmd.type  = CMD_UPDATES_PER_SECOND;
    cmd.ups.n = UPS;
    game_proxy_enq(&cmd);
}

static void game_cmd_sound(const char *filename, float a)
{
    cmd.type = CMD_SOUND;

    cmd.sound.n = strdup(filename);
    cmd.sound.a = a;

    game_proxy_enq(&cmd);
}

#define audio_play(s, f) game_cmd_sound((s), (f))

static void game_cmd_goalopen(void)
{
    cmd.type = CMD_GOAL_OPEN;
    game_proxy_enq(&cmd);
}

static void game_cmd_updball(void)
{
    cmd.type = CMD_BALL_POSITION;
    v_cpy(cmd.ballpos.p, vary.uv[CURR_PLAYER].p);
    game_proxy_enq(&cmd);

    cmd.type = CMD_BALL_BASIS;
    v_cpy(cmd.ballbasis.e[0], vary.uv[CURR_PLAYER].e[0]);
    v_cpy(cmd.ballbasis.e[1], vary.uv[CURR_PLAYER].e[1]);
    game_proxy_enq(&cmd);

    cmd.type = CMD_BALL_PEND_BASIS;
    v_cpy(cmd.ballpendbasis.E[0], vary.uv[CURR_PLAYER].E[0]);
    v_cpy(cmd.ballpendbasis.E[1], vary.uv[CURR_PLAYER].E[1]);
    game_proxy_enq(&cmd);
}

static void game_cmd_updview(void)
{
    cmd.type = CMD_VIEW_POSITION;
    v_cpy(cmd.viewpos.p, view.p);
    game_proxy_enq(&cmd);

    cmd.type = CMD_VIEW_CENTER;
    v_cpy(cmd.viewcenter.c, view.c);
    game_proxy_enq(&cmd);

    cmd.type = CMD_VIEW_BASIS;
    v_cpy(cmd.viewbasis.e[0], view.e[0]);
    v_cpy(cmd.viewbasis.e[1], view.e[1]);
    game_proxy_enq(&cmd);
}

static void game_cmd_ballradius(void)
{
    cmd.type         = CMD_BALL_RADIUS;
    cmd.ballradius.r = vary.uv[CURR_PLAYER].r;
    game_proxy_enq(&cmd);
}

static void game_cmd_init_balls(void)
{
    cmd.type = CMD_CLEAR_BALLS;
    game_proxy_enq(&cmd);

    cmd.type = CMD_MAKE_BALL;
    game_proxy_enq(&cmd);

    game_cmd_updball();
    game_cmd_ballradius();
}

static void game_cmd_pkitem(int hi)
{
    cmd.type      = CMD_PICK_ITEM;
    cmd.pkitem.hi = hi;
    game_proxy_enq(&cmd);
}

static void game_cmd_jump(int e)
{
    cmd.type = e ? CMD_JUMP_ENTER : CMD_JUMP_EXIT;
    game_proxy_enq(&cmd);
}

#ifdef CMD_NBRX
static void game_cmd_tilt(void)
{
    cmd.type = CMD_TILT;

    q_cpy(cmd.tilt.q, tilt.q);

    game_proxy_enq(&cmd);
}
#else
static void game_cmd_tiltangles(void)
{
    cmd.type = CMD_TILT_ANGLES;

    cmd.tiltangles.x = tilt.rx;
    cmd.tiltangles.z = tilt.rz;

    game_proxy_enq(&cmd);
}
#endif

static void game_cmd_tiltaxes(void)
{
    cmd.type = CMD_TILT_AXES;

    v_cpy(cmd.tiltaxes.x, tilt.x);
    v_cpy(cmd.tiltaxes.z, tilt.z);

    game_proxy_enq(&cmd);
}

static void game_cmd_timer(void)
{
    cmd.type    = CMD_TIMER;
    cmd.timer.t = timer;
    game_proxy_enq(&cmd);
}

static void game_cmd_coins(void)
{
    cmd.type    = CMD_COINS;
    cmd.coins.n = coins;
    game_proxy_enq(&cmd);
}

static void game_cmd_status(void)
{
    cmd.type     = CMD_STATUS;
    cmd.status.t = status;
    game_proxy_enq(&cmd);
}

static void game_cmd_speedometer(void)
{
    cmd.type           = CMD_SPEEDOMETER;
    cmd.speedometer.xi = game_get_ballspeed();
    game_proxy_enq(&cmd);
}

#ifdef MAPC_INCLUDES_CHKP
static void game_cmd_chkp_disable(void)
{
    cmd.type = CMD_CHKP_DISABLE;
    game_proxy_enq(&cmd);
}
#endif

/*---------------------------------------------------------------------------*/

static int   automode     = 2;          /* New: Automatic camera             */
static float autorotspeed = 0.0f;       /* New: Automatic rotation speed     */

static int   fix_cam_used[MAX_PLAYERS]; /* New: Static camera activation     */
static int   fix_cam_lock[MAX_PLAYERS]; /* New: Static camera lock           */
static float fix_cam_alpha[MAX_PLAYERS];/* New: Static camera alpha          */
static float fix_cam_pos_targ
                 [MAX_PLAYERS][3];      /* New: Static camera target pos     */
static float fix_cam_pos[3] =
                 { 0.0f, 5.0f, 0.0f };  /* New: Static camera position       */

static float view_alt_velocity = 0;

static int grow_init(int ui, int type)
{
    struct v_ball *up = &vary.uv[ui];
    int size = up->size;

    if (type == ITEM_SHRINK)
        size = CLAMP(0, size - 1, 2);
    else if (type == ITEM_GROW)
        size = CLAMP(0, size + 1, 2);

    if (size != up->size)
    {
        const int old_size = up->size;

        up->r_vel = (up->sizes[size] - up->r) / GROW_TIME;
        up->size  = size;

        if      (size < old_size) return -1;
        else if (size > old_size) return +1;

        return 0;
    }

    return 0;
}

static void grow_step(int ui, float dt)
{
    struct v_ball *up = &vary.uv[ui];

    if (up->r_vel != 0.0f)
    {
        float r, dr;

        /* Calculate new size based on how long since you touched the coin... */

        r = up->r + up->r_vel * dt;

        if ((up->r < up->sizes[up->size] && r >= up->sizes[up->size]) ||
            (up->r > up->sizes[up->size] && r <= up->sizes[up->size]))
        {
            r = up->sizes[up->size];
            up->r_vel = 0.0f;
        }

        dr = r - up->r;

        /* No sinking through the floor! Keeps ball's bottom constant. */

        up->p[1] += dr;
        up->r = r;

        game_cmd_ballradius();
    }
}

/*---------------------------------------------------------------------------*/

static struct lockstep server_step;

void game_update_view(float dt);

static int game_check_map_border(int ui, float offset)
{
    float temp_offset = MAX(offset, 0);

    /*
     * Those are pushing altitude limits that can't proceed above
     * the max altitude limit: Y Player pos < Max Y Border
     */

    /*
    int border_ok = vary.uv[ui].p[0] > (player_min_area[0] - (temp_offset * 8.0f)) &&
                    vary.uv[ui].p[1] > (player_min_area[1] - temp_offset) &&
                    vary.uv[ui].p[2] > (player_min_area[2] - (temp_offset * 8.0f)) &&
                    vary.uv[ui].p[0] < (player_max_area[0] + (temp_offset * 8.0f)) &&
                    vary.uv[ui].p[2] < (player_max_area[2] + (temp_offset * 8.0f));
    */

    int border_ok = vary.uv[ui].p[1] > (player_min_area[1] - temp_offset);

    return border_ok;
}

static void game_init_map_border(int ui)
{
    player_min_area[0] = 65535;
    player_min_area[1] = 65535;
    player_min_area[2] = 65535;

    player_max_area[0] = -65535;
    player_max_area[1] = -65535;
    player_max_area[2] = -65535;

    int i;

    for (i = 0; i < vary.base->vc; i++)
    {
        if (vary.base->vv[i].p[0] < player_min_area[0])
            player_min_area[0] = vary.base->vv[i].p[0];
        if (vary.base->vv[i].p[1] < player_min_area[1])
            player_min_area[1] = vary.base->vv[i].p[1];
        if (vary.base->vv[i].p[2] < player_min_area[2])
            player_min_area[2] = vary.base->vv[i].p[2];

        if (vary.base->vv[i].p[0] > player_max_area[0])
            player_max_area[0] = vary.base->vv[i].p[0];
        if (vary.base->vv[i].p[1] > player_max_area[1])
            player_max_area[1] = vary.base->vv[i].p[1];
        if (vary.base->vv[i].p[2] > player_max_area[2])
            player_max_area[2] = vary.base->vv[i].p[2];
    }

    for (i = 0; i < vary.base->hc; i++)
    {
        if (vary.base->hv[i].p[0] < player_min_area[0])
            player_min_area[0] = vary.base->hv[i].p[0];
        if (vary.base->hv[i].p[1] < player_min_area[1])
            player_min_area[1] = vary.base->hv[i].p[1];
        if (vary.base->hv[i].p[2] < player_min_area[2])
            player_min_area[2] = vary.base->hv[i].p[2];

        if (vary.base->hv[i].p[0] > player_max_area[0])
            player_max_area[0] = vary.base->hv[i].p[0];
        if (vary.base->hv[i].p[1] > player_max_area[1])
            player_max_area[1] = vary.base->hv[i].p[1];
        if (vary.base->hv[i].p[2] > player_max_area[2])
            player_max_area[2] = vary.base->hv[i].p[2];
    }

    for (i = 0; i < vary.base->uc; i++)
    {
        if (vary.uv[i].p[0] < player_min_area[0])
            vary.uv[i].p[0] = player_min_area[0];
        if (vary.uv[i].p[0] > player_max_area[0])
            vary.uv[i].p[0] = player_max_area[0];
        if (vary.uv[i].p[1] < player_min_area[1])
            vary.uv[i].p[1] = player_min_area[1];
        if (vary.uv[i].p[2] < player_min_area[2])
            vary.uv[i].p[2] = player_min_area[2];
        if (vary.uv[i].p[2] > player_max_area[2])
            vary.uv[i].p[2] = player_max_area[2];
    }
}

/*---------------------------------------------------------------------------*/

#if ENABLE_MOON_TASKLOADER!=0 && !defined(SKIP_MOON_TASKLOADER)
int game_server_load_moon_taskloader(void *data, void *execute_data)
{
    struct game_moon_taskloader_info *mtli = (struct game_moon_taskloader_info *) execute_data;

    if (!mtli) return 0;

    //while (st_global_animating());

    struct game_sol_version { int x, y; } version = { 0, 0 };
    int i;

    /* Load SOL/SOLX data. */

    if (!game_base_load(curr_file_name))
        return (server_state = 0);

    if (!sol_load_vary(&vary, &game_base))
    {
        game_base_free(NULL);
        return (server_state = 0);
    }

    game_init_map_border(CURR_PLAYER);

    for (int i = 0; i < vary.xc; i++)
        curr_path_enabled_orcondition[i] = vary.xv[i].f;

#ifdef MAPC_INCLUDES_CHKP
    if (last_active)
        for (int i = 0; i < vary.xc; i++)
            curr_path_enabled_orcondition[i] = last_path_enabled_orcondition[i];
#endif

    server_state = 1;

    /* Get SOL/SOLX version. */

    version.x = 0;
    version.y = 0;

    for (i = 0; i < vary.base->dc; i++)
    {
        char *k = vary.base->av + vary.base->dv[i].ai;
        char *v = vary.base->av + vary.base->dv[i].aj;

        if (strcmp(k, "version") == 0)
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            if (sscanf_s(v,
#else
            if (sscanf(v,
#endif
                       "%d.%d", &version.x, &version.y) != 2)
            {
                /*
                 * Was:
                 *     log_errorf("SOL/SOLX key parameter \"version\" (%s) is not an valid version format!\n", v ? v : "unknown");
                 *     sol_free_vary(&vary);
                 *     game_base_free(NULL);
                 *     return (server_state = 0);
                 */
            }
    }

    /* Initialize the view (and put it at the ball). */

    if (input_get_c() != config_get_d(CONFIG_CAMERA))
        input_set_c(config_get_d(CONFIG_CAMERA));

    for (int ui = 0; ui < vary.base->uc && ui < MAX_PLAYERS; ui++)
    {
        if (ui != CURR_PLAYER) continue;

        fix_cam_lock[ui]  = 0;
        fix_cam_alpha[ui] = 0.0f;
        int uses_fix_cam  = (input_get_c() == CAM_AUTO && automode == CAM_2) ||
                             input_get_c() == CAM_2;

        /* Use target view as somewhere else */
        if (vary.base->wv)
            v_cpy(fix_cam_pos_targ[ui], vary.base->wv[0].p);
        else
            v_cpy(fix_cam_pos_targ[ui], vary.base->uv[ui].p);

        v_cpy(fix_cam_pos, fix_cam_pos_targ[ui]);

        while (fix_cam_pos[0] != fix_cam_pos_targ[ui][0] ||
               fix_cam_pos[1] != fix_cam_pos_targ[ui][1] ||
               fix_cam_pos[2] != fix_cam_pos_targ[ui][2])
            v_cpy(fix_cam_pos, fix_cam_pos_targ[ui]);

        game_view_set_static_cam_view(0, fix_cam_pos);

        if (uses_fix_cam)
        {
            fix_cam_alpha[ui] = 1.0f;
            game_view_init(&view);

#pragma region Static camera
            float c0[3] = { 0.0f, 0.0f, 0.0f };
            float p0[3] = { 0.0f, 0.0f, 0.0f };
            float c1[3] = { 0.0f, 0.0f, 0.0f };
            float p1[3] = { 0.0f, 0.0f, 0.0f };
            float v[3]  = { 0.0f, 0.0f, 0.0f };

            v_cpy(c0, vary.uv[CURR_PLAYER].p);
            v_cpy(p0, vary.uv[CURR_PLAYER].p);

            v_cpy(p1, fix_cam_pos);
            v_cpy(c1, vary.uv[CURR_PLAYER].p);

            /* Interpolate the views. */

            v_sub(v, p1, p0);
            v_mad(view.p, p0, v, fix_cam_alpha[ui] * fix_cam_alpha[ui]);

            v_sub(v, c1, c0);
            v_mad(view.c, c0, v, fix_cam_alpha[ui] * fix_cam_alpha[ui]);

            view.c[1] += view.dc;

            /* Orthonormalize the view basis */

            v_sub(view.e[2], view.p, view.c);
            e_orthonrm_xz(view.e);

            game_view_set_static_cam_view(1, fix_cam_pos);
#pragma endregion
        }
        else
            game_view_fly(&view, &vary, ui, 0.0f);
    }

    view.a = V_DEG(fatan2f(view.e[2][0], view.e[2][2]));

    view_alt_velocity = 0.0f;

    view_k = 1.0f;

    view_time = 0.0f;
    view_fade = 0.0f;

    view_zoom_curr = 1.0f;
    view_zoom_time = ZOOM_TIME;

    view_zoom_diff_end  = 0;
    view_zoom_diff_curr = 0;

    for (int ui = 0; ui < vary.base->uc && ui < MAX_PLAYERS; ui++)
    {
        if (ui != CURR_PLAYER) continue;

#ifdef MAPC_INCLUDES_CHKP
        /*
         * --- CHECKPOINT DATA ---
         * If you haven't loaded your view data for each checkpoints,
         * Data of view for your default will be used.
         */

        if (last_active)
            last_diraxis = last_view[ui].a;
#ifdef START_POS_ANGULAR_BETA
        else
            last_diraxis = vary.base->uv[ui].a;
#endif
#endif
    }

    /* Initialize simulation. */

#ifdef MAPC_INCLUDES_CHKP
    /*
     * --- CHECKPOINT DATA ---
     * If you haven't loaded simulation data for each checkpoints,
     * Simulations for your default data will be used.
     */
    if (!last_active)
#endif
    {
#if _WIN32 && _MSC_VER && ENABLE_NVIDIA_PHYSX==1
        sol_init_sim_physx(&vary);
#else
        sol_init_sim(&vary);
#endif
    }

    /* Send initial update. */

    game_cmd_map(curr_file_name, version.x, version.y);
    game_cmd_ups();
    game_cmd_timer();

    if (goal_e)
        game_cmd_goalopen();

    game_cmd_init_balls();

#ifdef MAPC_INCLUDES_CHKP
    /*
     * --- CHECKPOINT DATA ---
     * If you have loaded electricity data for each checkpoints,
     * The electricity will travel BACK IN TIME!
     */
    if (last_active)
    {
        checkpoints_respawn(&vary, game_proxy_enq, &chkp_id);

#ifndef NDEBUG
        assert(vary.uv[CURR_PLAYER].p[0] == last_chkp_ball[CURR_PLAYER].p[0] &&
               vary.uv[CURR_PLAYER].p[1] == last_chkp_ball[CURR_PLAYER].p[1] &&
               vary.uv[CURR_PLAYER].p[2] == last_chkp_ball[CURR_PLAYER].p[2] &&
               vary.uv[CURR_PLAYER].r    == last_chkp_ball[CURR_PLAYER].r    &&
               vary.uv[CURR_PLAYER].size == last_chkp_ball[CURR_PLAYER].size);
#endif
    }
    else chkp_id = -1;
#endif

#ifdef MAPC_INCLUDES_CHKP
    /*
     * --- CHECKPOINT DATA ---
     * If you have a coins for each checkpoints,
     * The level progressions will be restored.
     */
    if (last_active && coins != 0)
        game_cmd_coins();

    /*
     * --- CHECKPOINT DATA ---
     * If you have an backup camera transform for each checkpoints,
     * The camera view will be restored.
     */
    if (last_active)
        game_update_view(0);
    else
#endif
    game_cmd_updview();

    game_cmd_eou();

    /* Reset lockstep state. */

    lockstep_clr(&server_step);

    return server_state;
}

int game_server_init_moon_taskloader(const char *file_name, int t, int e,
                                     struct moon_taskloader_callback callback)
{
    memset(curr_file_name, 0, MAXSTR);
    SAFECPY(curr_file_name, file_name);

    game_server_free(NULL);

    /*
     * --- MAYHEM ---
     * Reduces time limit
     */
    int mayhem_time = 359999;
    if (t > 0
     && (
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
         !campaign_used() &&
#endif
        curr_mode() != MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_ZEN
     && curr_mode() != MODE_ZEN
#endif
         ))
        mayhem_time = (int) t * 0.75f;

#ifdef MAPC_INCLUDES_CHKP
    time_limit = MAX(last_active ? checkpoints_respawn_time_limit() :
                                   (float) t / 100.0f, 0);
#else
    time_limit = MAX((float) (t / 100.0f), 0);
#endif

#if NB_HAVE_PB_BOTH==1
    time_travel = 0.0f;
#endif

    if (
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        campaign_used() ||
#endif
        curr_mode() == MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_ZEN
        || curr_mode() == MODE_ZEN
#endif
        )
        time_limit = 0;

#ifdef MAPC_INCLUDES_CHKP
    /*
     * --- CHECKPOINT DATA ---
     * If you haven't loaded Level data for each checkpoints,
     * Levels for your default data will be used.
     */

    time_elapsed =  last_active ? checkpoints_respawn_time_elapsed() : 0.0f;
    coins        =  last_active ? checkpoints_respawn_coins() : 0;
    timer_hold   = !last_active;
#else
    time_elapsed = 0.0f;
    coins        = 0;
    timer_hold   = 1;
#endif

    timer = fabsf(time_elapsed - time_limit);
    status = GAME_NONE;

#ifdef MAPC_INCLUDES_CHKP
    if (!last_active)
        chkp_id = -1;
#endif

#ifdef LEVELGROUPS_INCLUDES_ZEN
    /*
     * --- MEDIATION ---
     * If you use an Zen mode, all Achevements will disabled
     * and sets to unlimited time.
     */
    if (mediation_enabled() == 1
#ifdef MAPC_INCLUDES_CHKP
        && !last_active
#endif
        )
        time_elapsed = 0;
#endif

    input_init();

    game_tilt_init(&tilt);

    /* Switchball uses an automatic camera. */

    automode     = 2;
    autorotspeed = 0;

    /* Initialize jump, chkp and goal states. */

    jump_e = 1;
    jump_b = 0;

    /* Initialize pow block. */

    powblock_b = 0;

#ifdef MAPC_INCLUDES_CHKP
    /* All checkpoints were removed in HARDCORE MODE! or last balls. */
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    if ((time_limit > 0 && timer <= 60)
        || (curr_mode() == MODE_HARDCORE) || curr_balls() < 1)
#else
    if ((timer_down && timer <= 60) || curr_balls() < 1)
#endif
        chkp_e = 0;
    else
        chkp_e = 1;
#endif

    goal_e = e ? 1 : 0;

    if (input_get_c() != config_get_d(CONFIG_CAMERA))
        input_set_c(config_get_d(CONFIG_CAMERA));

    struct game_moon_taskloader_info *mtli = game_create_mtli();

    if (mtli)
    {
        mtli->callback = callback;
        mtli->filename = strdup(file_name);

        callback.execute  = game_mtli_execute;
        callback.progress = game_mtli_progress;
        callback.done     = game_mtli_done;
        callback.data     = mtli;

        return moon_taskloader_load(file_name, callback);
    }

    return 0;
}
#endif

/*---------------------------------------------------------------------------*/

int game_server_init(const char *file_name, int t, int e)
{
    memset(curr_file_name, 0, MAXSTR);
    SAFECPY(curr_file_name, file_name);

    struct game_sol_version { int x, y; } version = { 0, 0 };
    int i;

#ifdef MAPC_INCLUDES_CHKP
    time_limit = MAX(last_active ? checkpoints_respawn_time_limit() :
                                   (float) t / 100.0f, 0);
#else
    time_limit = MAX((float) (t / 100.0f), 0);
#endif

#if NB_HAVE_PB_BOTH==1
    time_travel = 0.0f;
#endif

    if (
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        campaign_used() ||
#endif
        curr_mode() == MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_ZEN
        || curr_mode() == MODE_ZEN
#endif
        )
        time_limit = 0;

#ifdef MAPC_INCLUDES_CHKP
    /*
     * --- CHECKPOINT DATA ---
     * If you haven't loaded Level data for each checkpoints,
     * Levels for your default data will be used.
     */

    time_elapsed =  last_active ? checkpoints_respawn_time_elapsed() : 0.0f;
    coins        =  last_active ? checkpoints_respawn_coins() : 0;
    timer_hold   = !last_active;
#else
    time_elapsed = 0.0f;
    coins        = 0;
    timer_hold   = 1;
#endif

    timer = fabsf(time_elapsed - time_limit);
    status = GAME_NONE;

#ifdef MAPC_INCLUDES_CHKP
    if (!last_active)
        chkp_id = -1;
#endif

#ifdef LEVELGROUPS_INCLUDES_ZEN
    /*
     * --- MEDIATION ---
     * If you use an Zen mode, all Achevements will disabled
     * and sets to unlimited time.
     */
    if (mediation_enabled() == 1
#ifdef MAPC_INCLUDES_CHKP
        && !last_active
#endif
        )
        time_elapsed = 0;
#endif

    game_server_free(file_name);

    /* Load SOL/SOLX data. */

    if (!game_base_load(file_name))
        return (server_state = 0);

    if (!sol_load_vary(&vary, &game_base))
    {
        game_base_free(NULL);
        return (server_state = 0);
    }

    game_init_map_border(CURR_PLAYER);

    for (int i = 0; i < vary.xc; i++)
        curr_path_enabled_orcondition[i] = vary.xv[i].f;

#ifdef MAPC_INCLUDES_CHKP
    if (last_active)
        for (int i = 0; i < vary.xc; i++)
            curr_path_enabled_orcondition[i] = last_path_enabled_orcondition[i];
#endif

    server_state = 1;

    /* Get SOL/SOLX version. */

    version.x = 0;
    version.y = 0;

    for (i = 0; i < vary.base->dc; i++)
    {
        char *k = vary.base->av + vary.base->dv[i].ai;
        char *v = vary.base->av + vary.base->dv[i].aj;

        if (strcmp(k, "version") == 0)
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            if (sscanf_s(v,
#else
            if (sscanf(v,
#endif
                       "%d.%d", &version.x, &version.y) != 2)
            {
                /*
                 * Was:
                 *     log_errorf("SOL/SOLX key parameter \"version\" (%s) is not an valid version format!\n", v ? v : "unknown");
                 *     sol_free_vary(&vary);
                 *     game_base_free(NULL);
                 *     return (server_state = 0);
                 */
            }
    }

    input_init();

    game_tilt_init(&tilt);

    /* Switchball uses an automatic camera. */

    automode     = 2;
    autorotspeed = 0;

    /* Initialize jump, chkp and goal states. */

    jump_e = 1;
    jump_b = 0;

    /* Initialize pow block. */

    powblock_b = 0;

#ifdef MAPC_INCLUDES_CHKP
    /* All checkpoints were removed in HARDCORE MODE! or last balls. */
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    if ((time_limit > 0 && timer <= 60)
        || (curr_mode() == MODE_HARDCORE) || curr_balls() < 1)
#else
    if ((timer_down && timer <= 60) || curr_balls() < 1)
#endif
        chkp_e = 0;
    else
        chkp_e = 1;
#endif

    goal_e = e ? 1 : 0;

    /* Initialize the view (and put it at the ball). */

    if (input_get_c() != config_get_d(CONFIG_CAMERA))
        input_set_c(config_get_d(CONFIG_CAMERA));

    for (int ui = 0; ui < vary.base->uc && ui < MAX_PLAYERS; ui++)
    {
        if (ui != CURR_PLAYER) continue;

        fix_cam_lock[ui]  = 0;
        fix_cam_alpha[ui] = 0.0f;
        int uses_fix_cam  = cam_speed(input_get_c()) == 0;

        /* Use target view as somewhere else */
        if (vary.base->wv)
            v_cpy(fix_cam_pos_targ[ui], vary.base->wv[0].p);
        else
            v_cpy(fix_cam_pos_targ[ui], vary.base->uv[ui].p);

        v_cpy(fix_cam_pos, fix_cam_pos_targ[ui]);

        while (fix_cam_pos[0] != fix_cam_pos_targ[ui][0] ||
               fix_cam_pos[1] != fix_cam_pos_targ[ui][1] ||
               fix_cam_pos[2] != fix_cam_pos_targ[ui][2])
            v_cpy(fix_cam_pos, fix_cam_pos_targ[ui]);

        game_view_set_static_cam_view(0, fix_cam_pos);

        if (uses_fix_cam)
        {
            fix_cam_alpha[ui] = 1.0f;
            game_view_init(&view);

#pragma region Static camera
            float c0[3] = { 0.0f, 0.0f, 0.0f };
            float p0[3] = { 0.0f, 0.0f, 0.0f };
            float c1[3] = { 0.0f, 0.0f, 0.0f };
            float p1[3] = { 0.0f, 0.0f, 0.0f };
            float v[3]  = { 0.0f, 0.0f, 0.0f };

            v_cpy(c0, vary.uv[CURR_PLAYER].p);
            v_cpy(p0, vary.uv[CURR_PLAYER].p);

            v_cpy(p1, fix_cam_pos);
            v_cpy(c1, vary.uv[CURR_PLAYER].p);

            /* Interpolate the views. */

            v_sub(v, p1, p0);
            v_mad(view.p, p0, v, fix_cam_alpha[ui] * fix_cam_alpha[ui]);

            v_sub(v, c1, c0);
            v_mad(view.c, c0, v, fix_cam_alpha[ui] * fix_cam_alpha[ui]);

            view.c[1] += view.dc;

            /* Orthonormalize the view basis */

            v_sub(view.e[2], view.p, view.c);
            e_orthonrm_xz(view.e);

            game_view_set_static_cam_view(1, fix_cam_pos);
#pragma endregion
        }
        else
            game_view_fly(&view, &vary, ui, 0.0f);
    }

    view.a = V_DEG(fatan2f(view.e[2][0], view.e[2][2]));

    view_alt_velocity = 0.0f;

    view_k = 1.0f;

    view_time = 0.0f;
    view_fade = 0.0f;

    view_zoom_curr = 1.0f;
    view_zoom_time = ZOOM_TIME;

    view_zoom_diff_end  = 0;
    view_zoom_diff_rate = 0;
    view_zoom_diff_curr = 0;

    for (int ui = 0; ui < vary.base->uc && ui < MAX_PLAYERS; ui++)
    {
        if (ui != CURR_PLAYER) continue;

#ifdef MAPC_INCLUDES_CHKP
        /*
         * --- CHECKPOINT DATA ---
         * If you haven't loaded your view data for each checkpoints,
         * Data of view for your default will be used.
         */

        if (last_active)
            last_diraxis = last_view[ui].a;
#ifdef START_POS_ANGULAR_BETA
        else
            last_diraxis = vary.base->uv[ui].a;
#endif
#endif
    }

    /* Initialize simulation. */

#ifdef MAPC_INCLUDES_CHKP
    /*
     * --- CHECKPOINT DATA ---
     * If you haven't loaded simulation data for each checkpoints,
     * Simulations for your default data will be used.
     */
    if (!last_active)
#endif
    {
#if _WIN32 && _MSC_VER && ENABLE_NVIDIA_PHYSX==1
        sol_init_sim_physx(&vary);
#else
        sol_init_sim(&vary);
#endif
    }

    /* Send initial update. */

    game_cmd_map(file_name, version.x, version.y);
    game_cmd_ups();
    game_cmd_timer();

    if (goal_e)
        game_cmd_goalopen();

    game_cmd_init_balls();

#ifdef MAPC_INCLUDES_CHKP
    /*
     * --- CHECKPOINT DATA ---
     * If you have loaded electricity data for each checkpoints,
     * The electricity will travel BACK IN TIME!
     */
    if (last_active)
    {
        checkpoints_respawn(&vary, game_proxy_enq, &chkp_id);

#ifndef NDEBUG
        assert(vary.uv[CURR_PLAYER].p[0] == last_chkp_ball[CURR_PLAYER].p[0] &&
               vary.uv[CURR_PLAYER].p[1] == last_chkp_ball[CURR_PLAYER].p[1] &&
               vary.uv[CURR_PLAYER].p[2] == last_chkp_ball[CURR_PLAYER].p[2] &&
               vary.uv[CURR_PLAYER].r    == last_chkp_ball[CURR_PLAYER].r    &&
               vary.uv[CURR_PLAYER].size == last_chkp_ball[CURR_PLAYER].size);
#endif
    }
    else chkp_id = -1;
#endif

#ifdef MAPC_INCLUDES_CHKP
    /*
     * --- CHECKPOINT DATA ---
     * If you have a coins for each checkpoints,
     * The level progressions will be restored.
     */
    if (last_active && coins != 0)
        game_cmd_coins();

    /*
     * --- CHECKPOINT DATA ---
     * If you have an backup camera transform for each checkpoints,
     * The camera view will be restored.
     */
    if (last_active)
        game_update_view(0);
    else
#endif
    game_cmd_updview();

    game_cmd_eou();

    /* Reset lockstep state. */

    lockstep_clr(&server_step);

    return server_state;
}

void game_server_free(const char *next)
{
    if (server_state)
    {
#if _WIN32 && _MSC_VER && ENABLE_NVIDIA_PHYSX==1
        sol_quit_sim_physx(&vary);
#endif
        sol_free_vary(&vary);

        game_base_free(next);
    }

    server_state = 0;
}

/*---------------------------------------------------------------------------*/

void game_update_view(float dt)
{
    /* Current view scale. */

    view_zoom_diff_end = CLAMP(-1, view_zoom_diff_end + (view_zoom_diff_rate * dt), 0);

    view_zoom_diff_curr = flerp(view_zoom_diff_curr,
                                view_zoom_diff_end, dt * 5.f);

    if (view_zoom_time < ZOOM_TIME)
    {
        view_zoom_time += dt;

        if (view_zoom_time >= ZOOM_TIME)
        {
            view_zoom_time = ZOOM_TIME;
            view_zoom_curr = view_zoom_end;
            view_zoom_end  = 0.0f;
        }
        else if (view_zoom_time >= ZOOM_DELAY)
        {
            float a = (view_zoom_time - ZOOM_DELAY) / (ZOOM_TIME - ZOOM_DELAY);

            a = easeInOutBack(a);

            view_zoom_curr = view_zoom_start + (view_zoom_end - view_zoom_start) * a;
        }
    }

    const float SCL = vary.uv->r / vary.uv->sizes[1];

    struct game_view multiview1 = view;
    struct game_view multiview2; game_view_init(&multiview2);

    for (int ui = 0; ui < vary.base->uc && ui < MAX_PLAYERS; ui++)
    {
        if (ui != CURR_PLAYER) continue;

        /*
         * Switchball uses an automatic camera.
         */

        float spd = -1.0f;

        if (input_get_c() == CAM_AUTO)
        {
            switch (automode)
            {
                case CAM_1: spd =   0.25f; break;
                case CAM_2: spd =    0.0f; break;
                case CAM_3: spd = -0.001f; break;
            }

            /* If camera speed value does not matched by the Switchball, reset it. */

            if (config_get_d(CONFIG_CAMERA_1_SPEED) != 250)
                config_set_d(CONFIG_CAMERA_1_SPEED, 250);
            if (config_get_d(CONFIG_CAMERA_2_SPEED) != 0)
                config_set_d(CONFIG_CAMERA_2_SPEED, 0);
            if (config_get_d(CONFIG_CAMERA_3_SPEED) != -1)
                config_set_d(CONFIG_CAMERA_3_SPEED, -1);

            config_save();
        }
        else if (input_get_c() != CAM_AUTO)
        {
            if (automode != input_get_c())
                automode = input_get_c();

            spd = (float) cam_speed(input_get_c()) / 1000.0f;
        }

#pragma region Camera Modes
        fix_cam_used[ui] = cam_speed(input_get_c()) == 0;

        if (fix_cam_lock[ui])
            fix_cam_alpha[ui] = 1;
        if (fix_cam_used[ui] || fix_cam_lock[ui])
        {
            fix_cam_alpha[ui] += dt / 1.5f;
            fix_cam_alpha[ui] = MIN(fix_cam_alpha[ui], 1);
        }
        else
        {
            fix_cam_alpha[ui] -= dt / 1.5f;
            fix_cam_alpha[ui] = MAX(fix_cam_alpha[ui], 0);
        }
#pragma endregion

#pragma region Pow camera shake
        /* Pow camera shake for center camera view. */

        float c_shake[3] = { 0.0f, 0.0f, 0.0f };

        if (powblock_b > 0)
        {
            c_shake[0] = rand_between(-ANGLE_BOUND, ANGLE_BOUND) * (powblock_dt - 0.5f);
            c_shake[1] = rand_between(-ANGLE_BOUND, ANGLE_BOUND) * (powblock_dt - 0.5f);
            c_shake[2] = rand_between(-ANGLE_BOUND, ANGLE_BOUND) * (powblock_dt - 0.5f);
        }
#pragma endregion

#pragma region Static camera
        if (cam_speed(input_get_c()) == 0)
            v_lerp(fix_cam_pos, fix_cam_pos, fix_cam_pos_targ[ui], dt);

        float c0[3] = { 0.0f, 0.0f, 0.0f };
        float p0[3] = { 0.0f, 0.0f, 0.0f };
        float c1[3] = { 0.0f, 0.0f, 0.0f };
        float p1[3] = { 0.0f, 0.0f, 0.0f };
        float v[3]  = { 0.0f, 0.0f, 0.0f };

        v_cpy(c0, vary.uv[ui].p);
        v_cpy(p0, vary.uv[ui].p);
        
        v_cpy(p1, fix_cam_pos);
        v_cpy(c1, vary.uv[ui].p);

        /* Interpolate the views. */

        v_sub(v, p1, p0);
        v_mad(multiview2.p, p0, v, fix_cam_alpha[ui] * fix_cam_alpha[ui]);

        v_sub(v, c1, c0);
        v_mad(multiview2.c, c0, v, fix_cam_alpha[ui] * fix_cam_alpha[ui]);

        multiview2.c[1] += multiview2.dc;

        /* Apply camera shake. */

        v_add(multiview2.c, multiview2.c, c_shake);
        v_add(multiview2.c, multiview2.c, c_shake);

        /* Orthonormalize the view basis */

        v_sub(multiview2.e[2], multiview2.p, multiview2.c);
        e_orthonrm_xz(multiview2.e);

        /* Note the current view angle. */

        multiview2.a = V_DEG(fatan2f(multiview2.e[2][0], multiview2.e[2][2]));
#pragma endregion

#pragma region Manual or Chase camera
        float zoom_diff = CLAMP(0, 1 + view_zoom_diff_curr, 1);

        float dc = (multiview1.dc - flerp(0.25f, 0, zoom_diff)) *
                   (jump_b > 0 ? 2.0f * fabsf(jump_dt - 0.5f) : 1.0f);
        float da = (90.0f * input_get_r() * dt) * (config_get_d(CONFIG_CAMERA_ROTATE_MODE) == 1 ? -1 : 1);
        float k;

        float M[16], Y[3] = { 0.0f, 1.0f, 0.0f };
        float view_v[3]   = { 0.0f, 0.0f, 0.0f };

        /* Track manual rotation time. */

        if (da == 0.0f)
        {
            if (view_time < 0.0f)
            {
                /* Transition time is influenced by activity time. */

                view_fade = CLAMP(VIEW_FADE_MIN, -view_time, VIEW_FADE_MAX);
                view_time = 0.0f;
            }

            /* Inactivity. */

            view_time += dt;
        }
        else
        {
            if (view_time > 0.0f)
            {
                view_fade = 0.0f;
                view_time = 0.0f;
            }

            /* Activity (yes, this is negative). */

            view_time -= dt;
        }

        /* Center the view about the ball. */

        v_cpy(multiview1.c, vary.uv[ui].p);

        view_v[0] = -vary.uv[ui].v[0];
        view_v[1] =  0.0f;
        view_v[2] = -vary.uv[ui].v[2];

        /* Apply camera shake. */

        v_add(multiview1.c, multiview1.c, c_shake);
        v_add(multiview1.c, multiview1.c, c_shake);

        /* Compute view vector. */

        if (spd >= 0.0f)
        {
            /*
             * Viewpoint chases ball position.
             * Camera rotation must be freeze: jump_b == 0
             */

            if (da == 0.0f && jump_b == 0)
            {
                float s;

                v_sub(multiview1.e[2], multiview1.p, multiview1.c);
                v_nrm(multiview1.e[2], multiview1.e[2]);

                /* Gradually restore view vector convergence rate. */

                s = fpowf(view_time, 3.0f) / fpowf(view_fade, 3.0f);
                s = CLAMP(0.0f, s, 1.0f);

                v_mad(multiview1.e[2], multiview1.e[2],
                      view_v, v_len(view_v) * spd * s * dt);
            }
        }
        else
        {
            /* View vector is given by view angle. */

            multiview1.e[2][0] = fsinf(V_RAD(multiview1.a));
            multiview1.e[2][1] = 0.0;
            multiview1.e[2][2] = fcosf(V_RAD(multiview1.a));
        }

        if (spd > 0.1f)
            view_alt_velocity = flerp(view_alt_velocity,
                                      vary.uv[ui].v[1], 0.01f);
        else
            view_alt_velocity = flerp(view_alt_velocity, 0, 0.01f);

        /*
         * Switchball uses an automatic camera.
         * Camera rotation must be freeze: jump_b == 0
         */

        if (spd < 0 && jump_b == 0 &&
            input_get_c() == CAM_AUTO)
        {
            if (da == 0.0f && jump_b == 0)
            {
                float s;

                v_sub(multiview1.e[2], multiview1.p, multiview1.c);
                v_nrm(multiview1.e[2], multiview1.e[2]);

                float direction_v[3] = { 0.0f, 0.0f, 0.0f };

                /* Multiply the speeds */

                direction_v[2] = fcosf(last_diraxis / +180 * V_PI) * 2.5f;
                direction_v[0] = fsinf(last_diraxis / -180 * V_PI) * 2.5f;

                /* Gradually restore view vector convergence rate. */

                s = fpowf(view_time, 3.0f) / fpowf(view_fade, 3.0f);
                s = CLAMP(0.0f, s, 1.0f);

                v_mad(multiview1.e[2], multiview1.e[2],
                      direction_v, v_len(direction_v) * s * dt);
            }
        }

        /*
         * Apply manual rotation.
         * Camera rotation must be freeze: jump_b == 0
         */

        if (da != 0.0f && jump_b == 0)
        {
            m_rot(M, Y, V_RAD(da));
            m_vxfm(v, M, multiview1.e[2]);
            v_cpy(multiview1.e[2], v);
        }

        /* Orthonormalize the new view reference frame. */

        e_orthonrm_xz(multiview1.e);

        /* Compute the new view position. */

        k = 1.0f + v_dot(multiview1.e[2], view_v) / 10.0f;

        view_k = MAX(flerp(view_k, k, dt), 0.5f);

        v_scl(v,    multiview1.e[1], SCL * (multiview1.dp + flerp(3.25f, 0, zoom_diff)) * view_k);
        v_mad(v, v, multiview1.e[2], SCL * (multiview1.dz + flerp(4.0f,  0, zoom_diff)) * view_k);
        v_add(multiview1.p, v, vary.uv[ui].p);

        multiview1.p[1] -= view_alt_velocity;

        /* Compute the new view center. */

        v_cpy(multiview1.c, vary.uv[ui].p);
        v_mad(multiview1.c, multiview1.c, multiview1.e[1], SCL * dc);

        /* Note the current view angle. */

        multiview1.a = V_DEG(fatan2f(multiview1.e[2][0], multiview1.e[2][2]));
#pragma endregion

#pragma region Multiview interpolation
        view.a = flerp(multiview1.a, multiview2.a,
                       (-cosf(V_PI * fix_cam_alpha[ui]) / 2) + 0.5f);
        v_lerp(view.c, multiview1.c, multiview2.c,
                       (-cosf(V_PI * fix_cam_alpha[ui]) / 2) + 0.5f);
        v_lerp(view.p, multiview1.p, multiview2.p, fix_cam_alpha[ui]);

        v_lerp(view.e[0], multiview1.e[0], multiview2.e[0],
                          (-cosf(V_PI * fix_cam_alpha[ui]) / 2) + 0.5f);
        v_lerp(view.e[1], multiview1.e[1], multiview2.e[1],
                          (-cosf(V_PI * fix_cam_alpha[ui]) / 2) + 0.5f);
        v_lerp(view.e[2], multiview1.e[2], multiview2.e[2],
                          (-cosf(V_PI * fix_cam_alpha[ui]) / 2) + 0.5f);
#pragma endregion
    }

    game_cmd_updview();
}

#ifdef MAPC_INCLUDES_CHKP
void game_disable_chkp(void);
#endif

static void game_update_time(float dt, int b)
{
    if (curr_mode() == MODE_NONE) return; /* Cannot run timer in home room. */

    /*
     * The ticking clock.
     * Timer must be freeze: jump_b == 0 && timer_hold == 0
     */

#if NB_HAVE_PB_BOTH==1
    if (b && time_travel > 0.0f && timer_hold == 0 && jump_b == 0)
    {
        time_travel -= dt;

        return;
    }
#endif

    if (b && timer_hold == 0 && jump_b == 0)
    {
#if NB_HAVE_PB_BOTH==1
        if (time_travel < 0.0f)
        {
            time_elapsed += time_travel * -1;
            time_travel   = 0.0f;
        }
#endif

        time_elapsed += dt;

        if (time_limit > 0.0f && time_elapsed > time_limit)
            time_elapsed = time_limit;
        
        if (time_limit > 0.0f
#ifdef LEVELGROUPS_INCLUDES_ZEN
         && !mediation_enabled()
#endif
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
         && !campaign_used()
#endif
            )
            timer = fabsf(time_limit - time_elapsed);
        else
            timer = time_elapsed;

#ifdef MAPC_INCLUDES_CHKP
        if (chkp_e && time_limit > 0 && timer < 60.0f)
            game_disable_chkp();
#endif

        game_cmd_timer();
    }
}

/*
 * Start view zoom animation.
 */
static void zoom_init(float target)
{
    view_zoom_time  = 0.0f;
    view_zoom_start = view_zoom_curr;
    view_zoom_end   = CLAMP(ZOOM_MIN, target, ZOOM_MAX);
}

static int game_update_state(int bt)
{
    struct b_goal *zp;
    int hi, cami;

    //float p[3] = { 0.0f, 0.0f, 0.0f };

    /* New: Hold timer mode */

    if (vary.uv[CURR_PLAYER].p[0] < game_base.uv[CURR_PLAYER].p[0] - .01f ||
        vary.uv[CURR_PLAYER].p[0] > game_base.uv[CURR_PLAYER].p[0] + .01f ||
        vary.uv[CURR_PLAYER].p[1] < game_base.uv[CURR_PLAYER].p[1] - .25f ||
        vary.uv[CURR_PLAYER].p[1] > game_base.uv[CURR_PLAYER].p[1] + .01f ||
        vary.uv[CURR_PLAYER].p[2] < game_base.uv[CURR_PLAYER].p[2] - .01f ||
        vary.uv[CURR_PLAYER].p[2] > game_base.uv[CURR_PLAYER].p[2] + .01f)
        timer_hold = 0;

    /* Cannot update state in home room. */

    if (curr_mode() == MODE_NONE) return GAME_NONE;

    /* Test for an item. */

    if (bt && (hi = sol_item_test(&vary, NULL, ITEM_RADIUS)) != -1)
    {
        struct v_item *hp = vary.hv + hi;

        game_cmd_pkitem(hi);

        /* Temporary coin value leftover in the campaign. */

        if (hp->t == ITEM_COIN)
        {
            coins += hp->n * powerup_get_coin_multiply();
            game_cmd_coins();

            progress_rush_collect_coin_value(hp->n);

            audio_play(AUD_COIN, 1.0f);

            hp->t = ITEM_NONE;
        }

        /* Morph and coin sizes (includes campaign). */

        else if (hp->t == ITEM_GROW || hp->t == ITEM_SHRINK)
        {
            audio_play(AUD_COIN, 1.0f);

            switch (grow_init(CURR_PLAYER, hp->t))
            {
                case -1:
                    audio_play(AUD_SHRINK, 1.0f);
                    zoom_init(vary.uv->sizes[vary.uv->size] / vary.uv->sizes[1]);
                    break;

                case +1:
                    audio_play(AUD_GROW, 1.0f);
                    zoom_init(vary.uv->sizes[vary.uv->size] / vary.uv->sizes[1]);
                    break;

                case 0:
                    audio_play(AUD_DISABLED, 1.0f);
                    break;
            }

            hp->t = ITEM_NONE;
        }

        /* Increment time limits to avoid time-outs. */

        else if (hp->t == ITEM_CLOCK)
        {
            const float value = (float) hp->n;

            audio_play(AUD_CLOCK, 1.0f);

            /* Calculate elapsed time against coin clocks. */

            if (time_limit > 0)
                time_limit = time_limit + value;
            else
            {
#if NB_HAVE_PB_BOTH==1
                time_travel += value;
#else
                time_elapsed = MAX(0.0f, time_elapsed - value);
#endif
            }

            game_update_time(0.0f, bt);

            audio_play(AUD_COIN, 1.0f);

            hp->t = ITEM_NONE;
        }

#if NB_HAVE_PB_BOTH==1
        /* Pow Block: Drops all coins onto the ground after activated. */

        else if (hp->t == ITEM_2_2_0_POWBLOCK)
        {
            audio_play(AUD_2_2_0_USE_POW, 1.0f);

            powblock_b  = 1;
            powblock_dt = 0.0f;

            audio_play(AUD_COIN, 1.0f);

            hp->t = ITEM_NONE;
        }
#endif
    }

#if NB_HAVE_PB_BOTH==1 && defined(LEVELGROUPS_INCLUDES_CAMPAIGN)
    if (bt && (cami = campaign_camera_box_trigger_test(&vary, 0)) != -1)
    {
        struct campaign_cam_box_trigger *camtrigger = campaign_get_camera_box_trigger(cami);

        if (camtrigger->inside)
        {
            automode = camtrigger->cammode;

            if (last_diraxis !=
                camtrigger->camdirection)
            {
                view_fade    = CLAMP(VIEW_FADE_MIN, -view_time, VIEW_FADE_MAX);
                view_time    = 0.0f;
                last_diraxis = camtrigger->camdirection;
            }

            if (camtrigger->campositions[0] != 0 &&
                camtrigger->campositions[1] != 0 &&
                camtrigger->campositions[2] != 0)
                v_cpy(fix_cam_pos_targ[CURR_PLAYER], camtrigger->campositions);
        }
    }
#endif

    /* Test for a switch. */

    if (bt && (sol_swch_test(&vary, game_proxy_enq, CURR_PLAYER) == SWCH_INSIDE))
        audio_play(AUD_SWITCH, 1.f);

#ifdef MAPC_INCLUDES_CHKP
    /* New: Checkpoints */

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    if (curr_mode() != MODE_HARDCORE &&
        (curr_balls() != 0 && !progress_dead()))
#else
    if (curr_balls() != 0 && !progress_dead())
#endif
    {
        if (bt && chkp_e &&
            sol_chkp_test(&vary, game_proxy_enq, CURR_PLAYER,
                          &chkp_id) == CHKP_INSIDE)
        {
            audio_play(AUD_SWITCH, 1.f);

            for (int backupidx = 0; backupidx < vary.cc; backupidx++)
            {
                struct v_chkp *cp = &vary.cv[backupidx];

                /* Backup the checkpoint's SOL/SOLX varying data. */

                if (cp->e && backupidx == chkp_id)
                {
                    audio_play(AUD_CHKP, 1.f);

                    checkpoints_save_spawnpoint(vary, view, CURR_PLAYER);
                    checkpoints_save_last_data(time_elapsed, time_limit, coins);

                    for (int i = 0; i < vary.xc; i++)
                        last_path_enabled_orcondition[i] = curr_path_enabled_orcondition[i];
                }

                /*
                 * Discard other checkpoint's SOL/SOLX varying data,
                 * when other checkpoints is activated.
                 */

                else if (cp->f && !cp->e && backupidx != chkp_id)
                {
                    cp->f = 0;
                    union cmd cmd = { CMD_CHKP_TOGGLE };
                    cmd.chkptoggle.ci = backupidx;
                    game_proxy_enq(&cmd);
                }
            }
        }
    }
#endif

    /* Test for a jump. */

    if (bt && jump_e == 1 && jump_b == 0 && (sol_jump_test(&vary, jump_p, 0) ==
                                       JUMP_INSIDE))
    {
        jump_b  = 1;
        jump_e  = 0;
        jump_dt = 0.0f;

        audio_play(AUD_JUMP, 1.0f);

        game_cmd_jump(1);
    }
    if (jump_e == 0 && jump_b == 0 && (sol_jump_test(&vary, jump_p, 0) ==
                                       JUMP_OUTSIDE))
    {
        jump_e = 1;
        game_cmd_jump(0);
    }

#if ENABLE_DEDICATED_SERVER==1
    int net_marker_result = -1;
#endif

    /* Test for a goal. */

    if (bt && !timer_hold &&
        goal_e && (zp = sol_goal_test(&vary, NULL, CURR_PLAYER)))
    {
#if ENABLE_DEDICATED_SERVER==1
        networking_dedicated_levelstatus_send(curr_file_name, GAME_GOAL, p);
#endif
#if NB_HAVE_PB_BOTH==1
        v_cpy(goal_lock_p, zp->p);
        goal_lock_p[1] += 1.5f;
#endif

        audio_play(AUD_GOAL, 1.0f);
        return GAME_GOAL;
    }

    /* Border controls */

    if (bt && !timer_hold &&
        (vary.base->vc == 0 ||
         !game_check_map_border(CURR_PLAYER, 0.875f * 2.0f)))
    {
        v_cpy(fix_cam_pos, view.p);
        fix_cam_lock[CURR_PLAYER] = 1;

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        if (campaign_used() && campaign_hardcore())
            campaign_hardcore_set_coordinates(vary.uv[CURR_PLAYER].p[0],
                                              vary.uv[CURR_PLAYER].p[2]);
#endif

#if ENABLE_DEDICATED_SERVER==1
        networking_dedicated_levelstatus_send(curr_file_name, GAME_FALL, p);
#endif

        audio_play(AUD_FALL, 1.0f);
        return GAME_FALL;
    }

    /* Time controls */

    if (bt && !timer_hold &&
        time_limit > 0 && timer < 0.0f)
    {
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        if (campaign_used() && campaign_hardcore())
            campaign_hardcore_set_coordinates(vary.uv[CURR_PLAYER].p[0],
                                              vary.uv[CURR_PLAYER].p[2]);
#endif

#if ENABLE_DEDICATED_SERVER==1
        networking_dedicated_levelstatus_send(curr_file_name, GAME_TIME, p);
#endif

        audio_play(AUD_TIME, 1.0f);
        return GAME_TIME;
    }

    return GAME_NONE;
}

static int game_step(const float g[3], float dt, int bt)
{
    if (server_state)
    {
        float h[3];

        /*
         * Smooth jittery or discontinuous input.
         * Floor must be keep horizontally: jump_b == 0
         */

        tilt.rx += ((jump_b == 0 && status == GAME_NONE ? input_get_x() : 0) - tilt.rx) *
                   dt / MAX(dt, input_get_s());
        tilt.rz += ((jump_b == 0 && status == GAME_NONE ? input_get_z() : 0) - tilt.rz) *
                   dt / MAX(dt, input_get_s());

#ifdef CMD_NBRX
        game_tilt_calc(&tilt, view.e);

        game_cmd_tilt();
#else
        game_tilt_axes(&tilt, view.e);

        game_cmd_tiltaxes();
        game_cmd_tiltangles();
#endif

        grow_step(CURR_PLAYER, dt);

        game_tilt_grav(h, g, &tilt);

        if (powblock_b > 0 && status != GAME_TIME)
        {
            powblock_dt += dt;

            if (powblock_dt >= 0.5f)
                powblock_b = 0;
        }

        /*if (status == GAME_GOAL
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            && !campaign_used()
#endif
            )
        {
            float lock_goal_pos_mult[3] = { 0.0f, 0.0f, 0.0f };
            float dir_to_point[3]       = { 0.0f, 0.0f, 0.0f };

            v_sub(dir_to_point, goal_lock_p, vary.uv[CURR_PLAYER].p);
            float dst_to_point = v_len(dir_to_point);

            vary.uv[CURR_PLAYER].v[0] += (dir_to_point[0] * dst_to_point) / .55f;
            vary.uv[CURR_PLAYER].v[1] += (dir_to_point[1] * dst_to_point) / 2.5f;
            vary.uv[CURR_PLAYER].v[2] += (dir_to_point[2] * dst_to_point) / .55f;

            vary.uv[CURR_PLAYER].v[0] *= .96f;
            vary.uv[CURR_PLAYER].v[1] *= .96f;
            vary.uv[CURR_PLAYER].v[2] *= .96f;

            vary.uv[CURR_PLAYER].w[0] *= .995f;
            vary.uv[CURR_PLAYER].w[1] *= .995f;
            vary.uv[CURR_PLAYER].w[2] *= .995f;
        }*/

        if (jump_b > 0 && status != GAME_TIME)
        {
            jump_dt += dt;

            /* Handle a jump. */

            if (jump_dt >= 0.5f)
            {
                /* Translate view at the exact instant of the jump. */

                if (jump_b == 1)
                {
                    float dp[3] = { 0.0f, 0.0f, 0.0f };

                    v_sub(dp,     jump_p, vary.uv[CURR_PLAYER].p);
                    v_add(view.p, view.p, dp);

                    jump_b = 2;
                }

                /* Translate ball and hold it at the destination. */

                v_cpy(vary.uv[CURR_PLAYER].p, jump_p);
            }

            if (jump_dt >= 1.0f)
                jump_b = 0;
        }

#ifdef MAPC_INCLUDES_CHKP
        /* Make sure that is not running the simulation during checkpoints is busy! */
        else if (!checkpoints_busy && status != GAME_TIME)
#else
        else if (status != GAME_TIME)
#endif
        {
            /* Run the sim. */

            float b = 0;

#if _WIN32 && _MSC_VER && ENABLE_NVIDIA_PHYSX==1
            if (vary.sim_uses_px)
                b = sol_step_physx(&vary, game_proxy_enq, h, dt, CURR_PLAYER);
            else
#endif
                b = sol_step(&vary, game_proxy_enq, h, dt, CURR_PLAYER, NULL);

            /* Mix the sound of a ball bounce. */

            if (b > 0.5f)
            {
                float k = (b - 0.5f) * 2.0f;

                if      (vary.uv->r > vary.uv->sizes[1]) audio_play(AUD_BUMPL, k);
                else if (vary.uv->r < vary.uv->sizes[1]) audio_play(AUD_BUMPS, k);
                else                                     audio_play(AUD_BUMPM, k);
            }
        }

        game_cmd_updball();
        game_cmd_speedometer();

        game_view_set_static_cam_view(0, fix_cam_pos);
        game_update_view(dt);
        game_update_time(dt, bt);

        return game_update_state(bt);
    }
    return GAME_NONE;
}

static void game_server_iter(float dt)
{
    /*
     * HACK: Do not allow these functions as it causes
     * incoherence problems after timer expires.
     */

    if (status == GAME_TIME) return;

    float g[3] = { 0.0f, -9.8f, 0.0f };

    g[1] *= powerup_get_grav_multiply();

#if defined(MAPC_INCLUDES_CHKP) && defined(LEVELGROUPS_INCLUDES_CAMPAIGN)
    if (status == GAME_GOAL && !campaign_used())
#else
    if (status == GAME_GOAL)
#endif
    {
        /* TODO: Was flip'in through, or not? */

        g[0] *= -1.0f;
        g[1] *= -1.0f;
        g[2] *= -1.0f;
    }

#ifdef MAPC_INCLUDES_CHKP
    if (checkpoints_busy)
        v_cpy(g, GRAVITY_BUSY);
#endif

    if (status != GAME_NONE)
        game_step(g, dt, 0);
    else if ((status = game_step(g,
                                 dt,
                                 status == GAME_NONE
#ifdef MAPC_INCLUDES_CHKP
                              && !checkpoints_busy
#endif
                                 )) != GAME_NONE)
    {
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
        /* HACK: OK, but now, with WGCL's Emscripten first! */

        const int r = EM_ASM_INT({
            return Neverball.gamecore_mapmarker_try_place(UTF8ToString($0), $1, $2, $3, $4);
        }, curr_file_name, status,
           ROUND(vary.uv[CURR_PLAYER].p[0] * 100), ROUND(vary.uv[CURR_PLAYER].p[1] * 100), ROUND(vary.uv[CURR_PLAYER].p[2] * 100));
#endif
        game_cmd_status();
    }

    game_cmd_eou();
}

static struct lockstep server_step = { game_server_iter, DT };

void game_server_step(float dt)
{
    lockstep_run(&server_step, dt);
}

float game_server_blend(void)
{
    return lockstep_blend(&server_step);
}

/*---------------------------------------------------------------------------*/

void game_set_goal(void)
{
    if (goal_e) return;

    audio_play(AUD_SWITCH, 1.0f);
    goal_e = 1;

    game_cmd_goalopen();
}

#ifdef MAPC_INCLUDES_CHKP
void game_disable_chkp(void)
{
    if (chkp_e && time_limit > 0)
    {
        if (vary.cc)
            audio_play(AUD_SWITCH, 1.0f);

        chkp_e = 0;

        game_cmd_chkp_disable();
    }
}
#endif

/*---------------------------------------------------------------------------*/

void game_set_x(float k)
{
    input_set_x(-ANGLE_BOUND * k);

    input_set_s(config_get_d(CONFIG_JOYSTICK_RESPONSE) * 0.001f);
}

void game_set_z(float k)
{
    input_set_z(+ANGLE_BOUND * k);

    input_set_s(config_get_d(CONFIG_JOYSTICK_RESPONSE) * 0.001f);
}

void game_set_ang(float x, float z)
{
    input_set_x(x);
    input_set_z(z);
}

void game_set_pos(int x, int y)
{
    const float range = ANGLE_BOUND * 2;

    input_set_x(range * y / config_get_d(CONFIG_MOUSE_SENSE));
    input_set_z(range * x / config_get_d(CONFIG_MOUSE_SENSE));

    input_set_s(config_get_d(CONFIG_JOYSTICK_RESPONSE) * 0.001f);
}

void game_set_pos_max_speed(int x, int y)
{
    const float range = ANGLE_BOUND * 2;

    input_set_x(range * y / config_get_d(CONFIG_MOUSE_SENSE));
    input_set_z(range * x / config_get_d(CONFIG_MOUSE_SENSE));

    input_set_s(config_get_d(CONFIG_MOUSE_RESPONSE) * 0.001f);
}

void game_set_cam(int c)
{
    input_set_c(c);
}

void game_set_rot(float r)
{
    input_set_r(r);
}

/*---------------------------------------------------------------------------*/

float curr_time_limit(void)
{
    return time_limit;
}

float curr_time_elapsed(void)
{
    return time_elapsed;
}

/*---------------------------------------------------------------------------*/

/* New: Speedometer;
 * Calculates speed of the ball (used for displaying speed in HUD) */
float game_get_ballspeed(void)
{
    float vx = vary.uv[CURR_PLAYER].v[0];
    float vy = vary.uv[CURR_PLAYER].v[1];
    float vz = vary.uv[CURR_PLAYER].v[2];

    return sqrtf(vx * vx + vy * vy + vz * vz);
}

/* New: Time extension;
 * Extent specific time */
void game_extend_time(float extratime)
{
    status = GAME_NONE;
    game_cmd_status();
    time_limit += extratime;
}

/* New: Zoom;
 * Store with the zoom differences (just like a Switchball) */
void game_set_zoom(float diff)
{
    view_zoom_diff_end = CLAMP(-1, view_zoom_diff_end - diff, 0);
}

/* New: Zoom rate;
 * Store with the zoom rate differences (just like a Switchball) */
void game_set_zoom_rate(float diff_rate)
{
    view_zoom_diff_rate = diff_rate < -0.1f || diff_rate > 0.1f ?
                          diff_rate : 0.0f;
}
