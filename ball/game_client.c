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

#include <math.h>

#include "glext.h"
#include "vec3.h"
#include "geom.h"
#include "part.h"
#include "ball.h"
#include "image.h"
#include "audio.h"
#include "config.h"
#include "video.h"
#if ENABLE_DUALDISPLAY==1
#include "video_dualdisplay.h"
#endif
#if NB_HAVE_PB_BOTH==1
#include "solid_chkp.h"
#endif
#include "solid_draw.h"
#include "solid_all.h"

#ifdef MAPC_INCLUDES_CHKP
#include "checkpoints.h" /* New: Checkpoints */
#endif

#if ENABLE_MOON_TASKLOADER!=0
#define SKIP_MOON_TASKLOADER
#include "moon_taskloader.h"
#endif

#include "game_client.h"
#include "game_common.h"
#include "game_proxy.h"
#include "game_draw.h"

#include "progress.h"
#include "state.h"
#include "st_level.h"

#include "cmd.h"

#if NB_HAVE_PB_BOTH==1 && !defined(MAPC_INCLUDES_CHKP)
#error Security compilation error: Please enable checkpoints after joined PB+NB Discord Server!
#endif

/*---------------------------------------------------------------------------*/

int game_compat_map;                    /* Client/server map compat flag     */
int game_compat_campaign;               /* Campaign compat flag              */

/*---------------------------------------------------------------------------*/

#define CURR 0
#define PREV 1

static struct game_draw gd;
static struct game_lerp gl;

static int   status      = GAME_NONE;   /* Outcome of the game               */
static int   coins       = 0;           /* Collected coins                   */
static float speedometer = 0.0f;        /* New: Speedometer                  */

static struct cmd_state cs;             /* Command state                     */

struct game_sol_version { int x, y; }
       version;                         /* Current map version               */

/*---------------------------------------------------------------------------*/

static void game_run_cmd(const union cmd *cmd)
{
    if (gd.state)
    {
        struct game_view *view = &gl.view[CURR];
        struct game_tilt *tilt = &gl.tilt[CURR];

        struct s_vary *vary = &gd.vary;
        struct v_item *hp;

        float v[4];
        float dt;

        int idx;

        if (cs.next_update)
        {
            game_lerp_copy(&gl);
            cs.next_update = 0;
        }

        switch (cmd->type)
        {
            case CMD_END_OF_UPDATE:
                cs.got_tilt_axes = 0;
                cs.next_update = 1;

                if (cs.first_update)
                {
                    game_lerp_copy(&gl);
                    /* Hack to sync state before the next update. */
                    game_lerp_apply(&gl, &gd);
                    cs.first_update = 0;
                    break;
                }

                /* Compute gravity for particle effects. */

#if NB_HAVE_PB_BOTH==1 && defined(LEVELGROUPS_INCLUDES_CAMPAIGN)
                if (status == GAME_GOAL && !campaign_used())
#else
                if (status == GAME_GOAL)
#endif
                    game_tilt_grav(v, GRAVITY_PY, tilt);
                else
                    game_tilt_grav(v, GRAVITY_NY, tilt);

                /* Step particle, goal and jump effects. */

                if (cs.ups > 0)
                {
                    dt = 1.0f / cs.ups;

                    if (gd.goal_e && gl.goal_k[CURR] < 1.0f)
                        gl.goal_k[CURR] += dt;

#ifdef MAPC_INCLUDES_CHKP
                    if (!gd.chkp_e && gl.chkp_k[CURR] > 0.0f)
                        gl.chkp_k[CURR] -= dt;
#endif

                    if (gd.jump_b)
                    {
                    gl.jump_dt[CURR] += dt;

                    if (gl.jump_dt[PREV] >= 1.0f)
                        gd.jump_b = 0;
                    }

                    part_step(v, dt);
                }

                break;

            case CMD_MAKE_BALL:
                sol_lerp_cmd(&gl.lerp, &cs, cmd);
                break;

            case CMD_MAKE_ITEM:
                /* Not supported anymore. */
                break;

            case CMD_PICK_ITEM:
                /* Set up particle effects and discard the item. */

                if ((idx = cmd->pkitem.hi) >= 0 && idx < vary->hc)
                {
                    float p[3];

                    hp = &vary->hv[idx];

                    sol_entity_world(p, vary, hp->mi, hp->mj, hp->p);

                    item_color(hp, v);
                    part_burst(p, v);

                    hp->t = ITEM_NONE;
                }
                break;

            case CMD_TILT_ANGLES:
                tilt->rx = cmd->tiltangles.x;
                tilt->rz = cmd->tiltangles.z;

                if (!cs.got_tilt_axes)
                {
                    /*
                     * Neverball <= 1.5.1 does not send explicit tilt
                     * axes, rotation happens directly around view
                     * vectors.  So for compatibility if at the time of
                     * receiving tilt angles we have not yet received the
                     * tilt axes, we use the view vectors.
                     */

#ifdef CMD_NBRX
                    game_tilt_calc(tilt, view->e);
#else
                    game_tilt_axes(tilt, view->e);
#endif
                }
#ifdef CMD_NBRX
                else
                {
                    /* Use the axes we received via CMD_TILT_AXES. */

                    game_tilt_calc(tilt, NULL);
                }
#endif
                break;

            case CMD_SOUND:
                /* Play the sound. */

                if (cmd->sound.n)
                {
#if NB_HAVE_PB_BOTH!=1 || !defined(__EMSCRIPTEN__)
                    if (strcmp(AUD_TIME, cmd->sound.n) == 0 ||
                        strcmp(AUD_FALL, cmd->sound.n) == 0)
                        audio_narrator_play(cmd->sound.n);
#if NB_HAVE_PB_BOTH==1
                    /* 2.2 narrator sound equivalent. */

                    else if (strcmp(AUD_2_2_0_PICK_SS, cmd->sound.n) == 0)
                        audio_narrator_play(cmd->sound.n);
#endif
                    else
#endif
                        audio_play(cmd->sound.n, cmd->sound.a);
                }

                break;

            case CMD_TIMER:
                if (!gd.jump_b)
                {
                    gl.timer[PREV] = gl.timer[CURR];
                    gl.timer[CURR] = cmd->timer.t;
                }

                if (cs.first_update)
                {
                    gl.timer[PREV] = gl.timer[CURR] = cmd->timer.t;
                    game_compat_campaign = 0;
                }

                break;

            case CMD_STATUS:
                status = cmd->status.t;

                if (status == GAME_GOAL)
                    part_goal(gl.lerp.uv[0][0].p);

                break;

            case CMD_COINS:
                coins = cmd->coins.n;
                break;

            case CMD_JUMP_ENTER:
                gd.jump_b  = 1;
                gd.jump_e  = 0;
                gl.jump_dt[PREV] = 0.0f;
                gl.jump_dt[CURR] = 0.0f;
                break;

            case CMD_JUMP_EXIT:
                gd.jump_e = 1;
                break;

            case CMD_MOVE_PATH:
            case CMD_MOVE_TIME:
            /*
            case CMD_BODY_PATH:
            case CMD_BODY_TIME:
            */
                sol_lerp_cmd(&gl.lerp, &cs, cmd);
                break;

            case CMD_GOAL_OPEN:
                /*
                 * Enable the goal and make sure it's fully visible if
                 * this is the first update.
                 */

                if (!gd.goal_e)
                {
                    gd.goal_e = 1;
                    gl.goal_k[CURR] = cs.first_update ? 1.0f : 0.0f;
                }
                break;

            case CMD_SWCH_ENTER:
                if ((idx = cmd->swchenter.xi) >= 0 && idx < vary->xc)
                    vary->xv[idx].e = 1;
                break;

            case CMD_SWCH_TOGGLE:
                if ((idx = cmd->swchtoggle.xi) >= 0 && idx < vary->xc)
                    vary->xv[idx].f = !vary->xv[idx].f;
                break;

            case CMD_SWCH_EXIT:
                if ((idx = cmd->swchexit.xi) >= 0 && idx < vary->xc)
                    vary->xv[idx].e = 0;
                break;

            case CMD_UPDATES_PER_SECOND:
                cs.ups = cmd->ups.n;
                break;

            case CMD_BALL_RADIUS:
                sol_lerp_cmd(&gl.lerp, &cs, cmd);
                break;

            case CMD_CLEAR_ITEMS:
                /* Not supported anymore. */
                break;

            case CMD_CLEAR_BALLS:
                sol_lerp_cmd(&gl.lerp, &cs, cmd);
                break;

            case CMD_BALL_POSITION:
                sol_lerp_cmd(&gl.lerp, &cs, cmd);

                /*if (vary)
                {
                    float vx = vary->uv[cs.curr_ball].v[0];
                    float vy = vary->uv[cs.curr_ball].v[1];
                    float vz = vary->uv[cs.curr_ball].v[2];

                    speedometer = sqrtf(vx * vx + vy * vy + vz * vz);
                }*/

                break;

            case CMD_BALL_BASIS:
                sol_lerp_cmd(&gl.lerp, &cs, cmd);
                break;

            case CMD_BALL_PEND_BASIS:
                sol_lerp_cmd(&gl.lerp, &cs, cmd);
                break;

            case CMD_VIEW_POSITION:
                v_cpy(view->p, cmd->viewpos.p);
                break;

            case CMD_VIEW_CENTER:
                v_cpy(view->c, cmd->viewcenter.c);
                break;

            case CMD_VIEW_BASIS:
                v_cpy(view->e[0], cmd->viewbasis.e[0]);
                v_cpy(view->e[1], cmd->viewbasis.e[1]);
                v_crs(view->e[2], view->e[0], view->e[1]);
                break;

            case CMD_CURRENT_BALL:
                if ((idx = cmd->currball.ui) >= 0 && idx < vary->uc)
                    cs.curr_ball = idx;
                break;

            case CMD_PATH_FLAG:
                if ((idx = cmd->pathflag.pi) >= 0 && idx < vary->pc)
                    vary->pv[idx].f = cmd->pathflag.f;
                break;

            case CMD_STEP_SIMULATION:
                sol_lerp_cmd(&gl.lerp, &cs, cmd);
                break;

            case CMD_MAP:
                /*
                 * Note a version (mis-)match between the loaded map and what
                 * the server has. (This doesn't actually load a map.)
                 */
                game_compat_map = (version.x == cmd->map.version.x);
                break;

            case CMD_TILT_AXES:
                cs.got_tilt_axes = 1;

                v_cpy(tilt->x, cmd->tiltaxes.x);
                v_cpy(tilt->z, cmd->tiltaxes.z);
                break;

#ifdef CMD_NBRX
            case CMD_TILT:
                q_cpy(tilt->q, cmd->tilt.q);
                break;
#endif

#ifdef MAPC_INCLUDES_CHKP
            case CMD_CHKP_ENTER:
                if ((idx = cmd->chkpenter.ci) >= 0 && idx < vary->cc)
                    vary->cv[idx].e = 1;
                break;

            case CMD_CHKP_TOGGLE:
                if ((idx = cmd->chkptoggle.ci) >= 0 && idx < vary->cc)
                    vary->cv[idx].f = !vary->cv[idx].f;
                break;

            case CMD_CHKP_EXIT:
                if ((idx = cmd->chkpexit.ci) >= 0 && idx < vary->cc)
                    vary->cv[idx].e = 0;
                break;
#endif

            case CMD_SPEEDOMETER:
                speedometer = cmd->speedometer.xi;
                break;

            case CMD_ZOOM:
                /* Unsupported: Not used anymore. */

                break;

#ifdef MAPC_INCLUDES_CHKP
            case CMD_CHKP_DISABLE:
                if (gd.chkp_e)
                {
                    gd.chkp_e = 0;
                    gl.chkp_k[CURR] = cs.first_update ? 0.0f : 1.0f;
                }
#endif

            case CMD_NONE:
            case CMD_MAX:
                break;
        }
    }
}

void game_client_sync(fs_file demo_fp)
{
    union cmd *cmdp;

    while ((cmdp = game_proxy_deq()))
    {
        if (demo_fp)
            cmd_put(demo_fp, cmdp);

        game_run_cmd(cmdp);

        cmd_free(cmdp);
    }
}

/*---------------------------------------------------------------------------*/

#if ENABLE_MOON_TASKLOADER!=0 && !defined(SKIP_MOON_TASKLOADER)
int game_client_load_moon_taskloader(void *data, void *execute_data)
{
    struct game_moon_taskloader_info *mtli = (struct game_moon_taskloader_info *) execute_data;

    if (!mtli) return 0;

    //while (st_global_animating());

    char *back_name = "", *grad_name = "";

    /* Load SOL/SOLX data. */

    if (!game_base_load(mtli->filename))
        return (gd.state = 0);

    if (!sol_load_vary(&gd.vary, &game_base))
    {
        game_base_free(NULL);
        return (gd.state = 0);
    }

#if NB_HAVE_PB_BOTH==1 && defined(MAPC_INCLUDES_CHKP)
    if (last_active)
    {
        int chkp_id = -1;
        checkpoints_respawn(&gd.vary, NULL, &chkp_id);
    }
#endif

    if (!sol_load_draw(&gd.draw, &gd.vary, config_get_d(CONFIG_SHADOW)))
    {
        sol_free_vary(&gd.vary);
        game_base_free(NULL);
        return (gd.state = 0);
    }

    gd.state = 1;

    /* Initialize game state. */

    game_tilt_init(&gd.tilt);
    game_view_init(&gd.view);

    view_zoom_diff_end  = 0;
    view_zoom_diff_curr = 0;

    gd.jump_e  = 1;
    gd.jump_b  = 0;
    gd.jump_dt = 0.0f;

#ifdef MAPC_INCLUDES_CHKP
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    if (curr_mode() == MODE_HARDCORE || curr_balls() == 0)
#else
    if (curr_balls() == 0)
#endif
    {
        /* All checkpoints were removed in HARDCORE MODE! or last balls. */
        gd.chkp_e = 0;
        gd.chkp_k = 0.0f;
    }
    else
    {
        gd.chkp_e = 1;
        gd.chkp_k = 1.0f;
    }
#endif

    gd.goal_e = 0;
    gd.goal_k = 0.0f;

    /* Initialize interpolation. */

    game_lerp_init(&gl, &gd);

    /* Initialize fade. */

    gd.fade_k =  1.0f;
    gd.fade_d = -2.0f;

    /* FIXME: Let Mojang done one of these! */

    gd.mojang_death_enabled_flags = 0;
    gd.mojang_death_time_now      = 0.0f;
    gd.mojang_death_time_percent  = 0.0f;
    gd.mojang_death_view_angle    = 0.0f;

    /* Load level info. */

    version.x = 0;
    version.y = 0;

    for (int i = 0; i < gd.vary.base->dc; i++)
    {
        char *k = gd.vary.base->av + gd.vary.base->dv[i].ai;
        char *v = gd.vary.base->av + gd.vary.base->dv[i].aj;

        if (strcmp(k, "back") == 0) back_name = v;
        if (strcmp(k, "grad") == 0) grad_name = v;

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
                 *     return (gd.state = 0);
                 */
            }
    }

    /*
     * If the version of the loaded map is 1, assume we have a version
     * match with the server.  In this way 1.5.0 replays don't trigger
     * bogus map compatibility warnings.  Post-1.5.0 replays will have
     * CMD_MAP override this.
     */

    /*
     * 1.5.0 replays will not be able to trigger any map compatibility
     * warnings, 2.1 are not affected.
     */

    game_compat_map = version.x == 1;

    /* Initialize particles. */

    part_reset();

    /* Initialize command state. */

    cmd_state_init(&cs);

    /* Initialize background. */

    back_init(grad_name);

    sol_load_full(&gd.back, back_name, 0);

    /* Initialize lighting. */

    light_reset();

    return gd.state;
}

int game_client_init_moon_taskloader(const char *file_name,
                                     struct moon_taskloader_callback callback)
{
    game_client_free(NULL);

    /*
     * --- CHECKPOINT DATA ---
     * If you haven't loaded Level data for each checkpoints,
     * Levels for your default data will be used.
     */

#if NB_HAVE_PB_BOTH==1 && defined(MAPC_INCLUDES_CHKP)
    coins  = last_active ? checkpoints_respawn_coins() : 0;
#else
    coins  = 0;
#endif
    speedometer = 0;
    status = GAME_NONE;

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

static int ball_visible = 0;

int  game_client_init(const char *file_name)
{
    char *back_name = "", *grad_name = "";

    /*
     * --- CHECKPOINT DATA ---
     * If you haven't loaded Level data for each checkpoints,
     * Levels for your default data will be used.
     */

#if NB_HAVE_PB_BOTH==1 && defined(MAPC_INCLUDES_CHKP)
    coins  = last_active ? checkpoints_respawn_coins() : 0;
#else
    coins  = 0;
#endif
    speedometer = 0;
    status = GAME_NONE;

    game_client_free(file_name);

    /* Load SOL/SOLX data. */

    if (!game_base_load(file_name))
        return (gd.state = 0);

    if (!sol_load_vary(&gd.vary, &game_base))
    {
        game_base_free(NULL);
        return (gd.state = 0);
    }

#if NB_HAVE_PB_BOTH==1 && defined(MAPC_INCLUDES_CHKP)
    if (last_active)
    {
        int chkp_id = -1;
        checkpoints_respawn(&gd.vary, NULL, &chkp_id);
    }
#endif

    if (!sol_load_draw(&gd.draw, &gd.vary, config_get_d(CONFIG_SHADOW)))
    {
        sol_free_vary(&gd.vary);
        game_base_free(NULL);
        return (gd.state = 0);
    }

    gd.state = 1;

    /* Initialize game state. */

    game_tilt_init(&gd.tilt);
    game_view_init(&gd.view);

    view_zoom_diff_end  = 0;
    view_zoom_diff_curr = 0;

    gd.jump_e  = 1;
    gd.jump_b  = 0;
    gd.jump_dt = 0.0f;

#ifdef MAPC_INCLUDES_CHKP
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    if (curr_mode() == MODE_HARDCORE || curr_balls() == 0)
#else
    if (curr_balls() == 0)
#endif
    {
        /* All checkpoints were removed in HARDCORE MODE! or last balls. */
        gd.chkp_e = 0;
        gd.chkp_k = 0.0f;
    }
    else
    {
        gd.chkp_e = 1;
        gd.chkp_k = 1.0f;
    }
#endif

    gd.goal_e = 0;
    gd.goal_k = 0.0f;

    /* Initialize interpolation. */

    game_lerp_init(&gl, &gd);

    /* Initialize fade. */

    gd.fade_k =  1.0f;
    gd.fade_d = -2.0f;

    /* FIXME: Let Mojang done one of these! */

    gd.mojang_death_enabled_flags = 0;
    gd.mojang_death_time_now      = 0.0f;
    gd.mojang_death_time_percent  = 0.0f;
    gd.mojang_death_view_angle    = 0.0f;

    /* Load level info. */

    version.x = 0;
    version.y = 0;

    for (int i = 0; i < gd.vary.base->dc; i++)
    {
        char *k = gd.vary.base->av + gd.vary.base->dv[i].ai;
        char *v = gd.vary.base->av + gd.vary.base->dv[i].aj;

        if (strcmp(k, "back") == 0) back_name = v;
        if (strcmp(k, "grad") == 0) grad_name = v;

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
                 *     return (gd.state = 0);
                 */
            }
    }

    /*
     * If the version of the loaded map is 1, assume we have a version
     * match with the server.  In this way 1.5.0 replays don't trigger
     * bogus map compatibility warnings.  Post-1.5.0 replays will have
     * CMD_MAP override this.
     */

    /*
     * 2.2 and later: We always trigger map compatibility warnings
     * in the future version.
     */

    game_compat_map = 0; /* Was: version.x == 1 */

    /* Initialize particles. */

    part_reset();

    /* Initialize command state. */

    cmd_state_init(&cs);

    /* Initialize background. */

    back_init(grad_name);

    sol_load_full(&gd.back, back_name, 0);

    /* Initialize lighting. */

    light_reset();

    return gd.state;
}

void game_client_toggle_show_balls(int visible)
{
    ball_visible = visible;
}

void game_client_free(const char *next)
{
    if (gd.state)
    {
        gd.state = 0;

        game_proxy_clr();

        back_free();

        game_lerp_free(&gl);

        sol_free_full(&gd.back);
        sol_free_draw(&gd.draw);
        sol_free_vary(&gd.vary);

        game_base_free(next);
    }
}

int game_client_get_jump_b(void)
{
    return gd.jump_b;
}

/*---------------------------------------------------------------------------*/

void game_client_blend(float a)
{
    gl.alpha = a;
}

void game_client_draw(int pose, float t)
{
    if (gd.state && !progress_loading())
    {
        if (status == GAME_FALL)
        {
            if (gd.mojang_death_enabled_flags)
            {
                const float mojang_death_time_dt = t - gd.mojang_death_time_now;

                if (mojang_death_time_dt < 1.f)
                    gd.mojang_death_time_percent =
                        MIN(100, gd.mojang_death_time_percent + (mojang_death_time_dt * 0.05f));
                else audio_play("snd/2.2/border_death.ogg", 1.0f);

                gd.mojang_death_time_now = t;
            }
            else
            {
                gd.mojang_death_view_angle = gd.view.a;
                v_cpy(client_view_center_fixed, gd.vary.uv[0].p);
            }

            gd.mojang_death_enabled_flags = 1;
        }

        game_lerp_apply(&gl, &gd);

        if (gd.mojang_death_enabled_flags)
            game_view_death(&gd.view, client_view_center_fixed, 0, gd.mojang_death_view_angle);

        game_draw(&gd, ball_visible ? pose : POSE_LEVEL, t);
    }
}

void game_client_maxspeed(float a, int f)
{
    game_draw_set_maxspeed(a, f);
}

/*---------------------------------------------------------------------------*/

int curr_viewangle(void)
{
    return V_DEG(fatan2f(gl.view[CURR].e[2][0], gl.view[CURR].e[2][2]));
}

int curr_clock(void)
{
    return ROUND(flerp(gl.timer[PREV], gl.timer[CURR], gl.alpha) * 100.0f);
}

int curr_coins(void)
{
    return coins;
}

int curr_status(void)
{
    return status;
}

float curr_speedometer(void)
{
    return speedometer;
}

/*---------------------------------------------------------------------------*/

void game_look(float phi, float theta)
{
    struct game_view *view = &gl.view[CURR];

    view->c[0] = view->p[0] + fsinf(V_RAD(theta)) * fcosf(V_RAD(phi));
    view->c[1] = view->p[1] +                       fsinf(V_RAD(phi));
    view->c[2] = view->p[2] - fcosf(V_RAD(theta)) * fcosf(V_RAD(phi));

    v_sub(view->e[2], view->p, view->c);
    e_orthonrm_xz(view->e);

    gl.view[PREV] = gl.view[CURR];
}

void game_look_v2(float dx, float dy, float dz, float phi, float theta)
{
    struct game_view *view = &gl.view[CURR];

    view->p[0] += dx;
    view->p[1] += dy;
    view->p[2] += dz;

    game_look(phi, theta);
}

/*---------------------------------------------------------------------------*/

void game_disable_fade(int e)
{
    gd.fade_disabled = e;
}

void game_kill_fade(void)
{
    gd.fade_disabled = 1;

    gd.fade_k = 0.0f;
    gd.fade_d = 0.0f;
}

void game_kill_fade(void)
{
    gd.fade_k = 0.0f;
    gd.fade_d = 0.0f;
}

void game_step_fade(float dt)
{
    if ((gd.fade_k < 1.0f && gd.fade_d > 0.0f) ||
        (gd.fade_k > 0.0f && gd.fade_d < 0.0f))
        gd.fade_k += gd.fade_d * dt;

    if (gd.fade_k < 0.0f)
    {
        gd.fade_k = 0.0f;
        gd.fade_d = 0.0f;
    }
    if (gd.fade_k > 1.0f)
    {
        gd.fade_k = 1.0f;
        gd.fade_d = 0.0f;
    }
}

void game_fade(float d)
{
    gd.fade_disabled = 0;
    gd.fade_d        = d;
}

void game_fade_color(float r, float g, float b)
{
    sol_fade_color(r, g, b);
}

/*---------------------------------------------------------------------------*/

void game_client_fly(float k)
{
    /* TODO: Add player index for multiplayers. */

    game_view_fly(&gl.view[CURR], &gd.vary, 0, k);

    gl.view[PREV] = gl.view[CURR];
}

/*---------------------------------------------------------------------------*/

#define STUDIO_CAM_SCALE 0.015625f
#define STUDIO_TRANSITION 1.0f

#define STUDIO_MAX_TIME_MEDIUM 2.5f
#define STUDIO_MAX_TIME_FAST 1.0f

static int studio_safetyintro;

static int studio_map_index = 12;
static float studio_time_length;

static const char studio_map[15][256] =
{
    "map-easy/coins.sol",
    "map-medium/accordian.sol",
    "map-hard/curbs.sol",
    "map-easy/lollipop.sol",
    "map-tones/blue.sol",
    "map-hard/sync.sol",
    "map-easy/hole.sol",
    "map-medium/learngrow.sol",
    "map-hard/frogger.sol",
    "map-easy/corners.sol",
    "map-mym/scrambling.sol",
    "map-hard/ring.sol",
    "map-easy/mover.sol",
    "map-medium/timer.sol",
    "map-hard/movers.sol"
};

/* Netradiant position coordinates */
static float studio_cam_from_pos[15][2][3] =
{
    { { 232, -96, 544 } , { 304, -96, 544 } },
    { { -80, -1184, 744 } , { 16, -1184, 744 } },
    { { 784, -416, 554 } , { 832, -320, 568 } },
    { { -1152, 514, 640 } , { -1259, 544, 723 } },
    { { 640, -224, 421 } , { 672, -128, 448 } },
    { { 512, -256, 192 } , { 536, -214, 112 } },
    { { -828, 272, 360 } , { -512, 128, 704 } },
    { { -232, -1584, 444 } , { -608, -1416, 143 } },
    { { -152, -136, 160 } , { 104, -80, 145 } },
    { { 656, -176, 160 } , { 656, -176, -48 } },
    { { -768, -576, 448 } , { -768, -208, -128 } },
    { { 0, -576, 320 } , { 256, -576, -32 } },
    { { 688, -120, 80 } , { 104, -328, 152 } },
    { { -456, 8, 360 } , { -112, -322, 156 } },
    { { -560, -400, 576 } , { 544, -424, -120 } }
};

/* Netradiant position coordinates */
static float studio_cam_center_pos[15][2][3] =
{
    { { 96, 288, 16 }, { 74, 288, 16 } },
    { { -48, -624, 312 }, { -16, -624, 312 } },
    { { 128, 272, 24 }, { 128, 320, 24 } },
    { { 0, 1141, 6 }, { 0, 1141, 6 } },
    { { 256, 256, 0 }, { 352, 160, 0 } },
    { { 88, 432, 95 }, { 104, 376, 64 } },
    { { 64, 576, 4 }, { 67, 566, 4 } },
    { { 0, 0, 144 }, { 0, 0, 144 } },
    { { 64, 384, 32 }, { 64, 384, 32 } },
    { { 88, 736, 120 }, { 88, 736, 120 } },
    { { 0, 1048, -40 }, { 0, 1048, -40 } },
    { { 192, 576, 28 }, { 192, 576, 28 } },
    { { 64, 576, 32 }, { 64, 576, 32 } },
    { { 608, 192, 56 }, { 336, 56, 56 } },
    { { -120, 728, 56 }, { 88, 728, 56 } }
};

static float studio_cam_rot_roll[15][2] =
{
    {  0.0f,  0.0f },
    {  0.0f,  0.0f },
    {  0.0f,  0.0f },
    {  0.0f,  0.0f },
    {  0.0f,  0.0f },
    { -7.0f,  4.0f },
    { -9.0f,  3.0f },
    { -5.0f,  5.0f },
    {  8.0f, -3.0f },
    {  5.0f, -6.0f },
    { -2.0f,  4.0f },
    { -3.0f,  2.0f },
    {  3.0f, -5.0f },
    { -4.0f,  0.0f },
    {  5.0f, -5.0f }
};

static void game_client_next_studio_map(void)
{
    if (studio_map_index == 14)
        studio_map_index = 0;
    else
        studio_map_index++;

    if (game_client_init(studio_map[studio_map_index]))
    {
        game_client_toggle_show_balls(1);

        union cmd cmd = { CMD_GOAL_OPEN };

        game_proxy_enq(&cmd);
        game_client_sync(NULL);

        game_kill_fade();
    }
    else
        studio_map_index--;
}

void game_client_step_safetyintro(float);

void game_client_step_studio(float deltatime)
{
    if (deltatime > 0.017f)
        return;

    if (studio_safetyintro)
    {
        game_client_step_safetyintro(deltatime);
        return;
    }

    float studio_max_time = STUDIO_MAX_TIME_MEDIUM + STUDIO_TRANSITION;

    if (studio_time_length > studio_max_time)
    {
        studio_time_length = 0;
        game_client_next_studio_map();
    }
    else
        studio_time_length += deltatime;

    float pos[3]    = { 0.0f, 0.0f, 0.0f };
    float center[3] = { 0.0f, 0.0f, 0.0f };

    v_lerp(pos, studio_cam_from_pos[studio_map_index][0], studio_cam_from_pos[studio_map_index][1], (studio_time_length / studio_max_time));
    v_lerp(center, studio_cam_center_pos[studio_map_index][0], studio_cam_center_pos[studio_map_index][1], (studio_time_length / studio_max_time));

    v_scl(pos, pos, STUDIO_CAM_SCALE);
    v_scl(center, center, STUDIO_CAM_SCALE);

    float realPos[3]    = { +pos[0],    +pos[2],    -pos[1] };
    float realCenter[3] = { +center[0], +center[2], -center[1] };

    for (int i = 0; i < 2; i++)
        game_view_set_pos_and_target(&gl.view[i], &gd.vary, realPos, realCenter);

    gd_rotate_roll = CLAMP(-45, flerp(studio_cam_rot_roll[studio_map_index][0], studio_cam_rot_roll[studio_map_index][1], (studio_time_length / studio_max_time)), 45);
}

int game_client_init_safetyintro(void);

int game_client_init_studio(int alternatives)
{
    if (alternatives)
        return game_client_init_safetyintro();

    if (game_client_init(studio_map[studio_map_index]))
    {
        game_client_toggle_show_balls(1);

        union cmd cmd = { CMD_GOAL_OPEN };

        game_proxy_enq(&cmd);
        game_client_sync(NULL);

        game_kill_fade();

        return 1;
    }

    return 0;
}

/*---------------------------------------------------------------------------*/

static float safetyintro_totaltime = 0;

float safetyintro_cam_center_pos[3] = { 0, +80, 0 };

void game_client_step_safetyintro(float deltatime)
{
    float pos[3]    = { 0.0f, 0.0f, 0.0f };
    float center[3] = { 0.0f, 0.0f, 0.0f };

    if (safetyintro_totaltime > 8)
        safetyintro_totaltime -= 8;

    safetyintro_totaltime += deltatime / 3;

    pos[0] =  fsinf((safetyintro_totaltime / 4) * V_PI) * (192 * 1.5f);
    pos[1] =  flerp(180, 192, fcosf((safetyintro_totaltime / 4) * V_PI));
    pos[2] = -fcosf((safetyintro_totaltime / 4) * V_PI) * (192 * 1.5f);

    v_cpy(center, safetyintro_cam_center_pos);

    v_scl(pos, pos, STUDIO_CAM_SCALE);
    v_scl(center, center, STUDIO_CAM_SCALE);

    /* Should be used this tilt? */

    gd.tilt.rx = flerp(0, 10, fsinf((safetyintro_totaltime / 2) * V_PI));

    game_tilt_calc(&gd.tilt, 0);

    v_cpy(gl.tilt[CURR].q, gd.tilt.q);
    gl.tilt[CURR].q[3] = gd.tilt.q[3];

    for (int i = 0; i < 2; i++)
        game_view_set_pos_and_target(&gl.view[i], &gd.vary, pos, center);
}

int game_client_init_safetyintro(void)
{
    if (game_client_init("gui/safety-intro.sol"))
    {
        game_client_toggle_show_balls(1);

        /* HACK: Does not have a goal. */

        back_init("back/sky-SB.png");

        sol_load_full(&gd.back, "map-back/sky-SB.sol", 0);

        /* Initialize lighting. */

        light_reset();

        game_kill_fade();

        return 1;
    }

    return 0;
}

/*---------------------------------------------------------------------------*/
