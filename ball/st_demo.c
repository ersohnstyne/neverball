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

#include <assert.h>
#include <string.h>

#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
#include "console_control_gui.h"
#endif

#include "gui.h"
#include "hud.h"
#include "set.h"
#include "binary.h"
#include "demo.h"
#include "progress.h"
#include "audio.h"
#include "config.h"
#include "util.h"
#include "common.h"
#include "demo_dir.h"
#include "video.h"
#include "geom.h"

#include "game_common.h"
#include "game_client.h"
#include "game_server.h"
#include "game_proxy.h"

#include "st_demo.h"
#include "st_title.h"
#include "st_shared.h"
#include "st_conf.h"

/*---------------------------------------------------------------------------*/

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

/* For manual fast forwards and scaning locations */
static int filter_cmd(const union cmd *cmd)
{
    return (cmd ? cmd->type != CMD_SOUND : 1);
}

/*---------------------------------------------------------------------------*/

#define DEMO_LINE 4
#define DEMO_STEP 8

static Array demo_items;

static int is_opened = 0;

static int first = 0;
static int total = 0;
static int last  = 0;
static int availibility = 0;

static int last_viewed = 0;

static int target_timer = 0;   /* This is the target time limit */
static int premaded_timer = 0; /* This is the premaded timer    */

static int allow_exact_versions = 0;

/*---------------------------------------------------------------------------*/

static int st_demo_version_read(fs_file fp, struct demo *d)
{
    int magic;
    int version;

    magic = get_index(fp);
    version = get_index(fp);

    if (version < DEMO_VERSION_MIN)
        demo_old_detected = 1;

    if (version > DEMO_VERSION)
        demo_requires_update = 1;

    return 0;
}

/*---------------------------------------------------------------------------*/

static int check_full_access(char * replay_pname) {
    const char *curr_player = config_get_s(CONFIG_PLAYER);

    if (strcmp(replay_pname, "PennySchloss") == 0)
    {
        if (strcmp(curr_player, "PennySchloss") == 0)
            return 1;
    }

    return 0;
}

static int stat_limit_busy = 0;
static int stat_max = 0;
static int stat_limit = 0;

static int time_max_minutes = 0;
static int time_limit_minutes = 10;

static int get_maximum_status()
{
    return stat_max;
}

static int get_limit_status()
{
    return stat_limit;
}

static void set_maximum_status(int currstat)
{
    stat_max = currstat;
}

static void set_limit_status(int limitstat)
{
    assert(!stat_limit_busy);
    stat_limit = limitstat;
}

static char * reported_player_name;
static char * reported_status;

static void set_replay_report(char *target_player_name, char *target_status)
{
    reported_player_name = target_player_name;
    reported_status = target_status;
}

/*---------------------------------------------------------------------------*/

static void demo_shared_fade(float alpha)
{
    hud_set_alpha(alpha);
}

/*---------------------------------------------------------------------------*/

enum
{
    DEMO_SELECT = GUI_LAST,
    DEMO_UPGRADE_LIMIT,
    DEMO_XBOX_LB,
    DEMO_XBOX_RB
};

static int demo_action(int tok, int val)
{
    audio_play(GUI_BACK == tok ? AUD_BACK : AUD_MENU, 1.0f);

    switch (tok)
    {
    case DEMO_XBOX_LB:
        if (first > 1) {
            first -= DEMO_STEP;
            return goto_state_full(&st_demo, GUI_ANIMATION_E_CURVE, GUI_ANIMATION_W_CURVE, 0);
        }
        break;

    case DEMO_XBOX_RB:
        if (first < total)
        {
            first += DEMO_STEP;
            if (first >= total)
                first -= DEMO_STEP;
            else
                return goto_state_full(&st_demo, GUI_ANIMATION_W_CURVE, GUI_ANIMATION_E_CURVE, 0);
        }
        break;

    case GUI_BACK:
        return goto_state(&st_title);

    case GUI_NEXT:
        first += DEMO_STEP;
        return goto_state_full(&st_demo, GUI_ANIMATION_W_CURVE, GUI_ANIMATION_E_CURVE, 0);
        break;

    case GUI_PREV:
        first -= DEMO_STEP;
        return goto_state_full(&st_demo, GUI_ANIMATION_E_CURVE, GUI_ANIMATION_W_CURVE, 0);
        break;
    case DEMO_UPGRADE_LIMIT:
        if (val)
        {
#ifndef DEMO_QUARANTINED_MODE
            config_set_d(CONFIG_ACCOUNT_LOAD, 3);
            config_save();
#endif
        }
        else
        {
            config_set_d(CONFIG_ACCOUNT_LOAD, 2);
            config_save();
        }
        break;
    case DEMO_SELECT:
    {
        struct demo *df;
        df = DEMO_GET(demo_items, val < total ? val : 0);

        if (!df)
        {
            return 1;
        }

        set_limit_status(config_get_d(CONFIG_ACCOUNT_LOAD));

        stat_limit_busy = 1;
        if (df->status == 3)
            set_maximum_status(3);
        else if (df->status == 1 || df->status == 0)
            set_maximum_status(2);
        else if (df->status == 2)
            set_maximum_status(1);
        stat_limit_busy = 0;

        {
            /* Make sure, that the status limit is underneath it. */
            if (get_maximum_status() > get_limit_status() || time_max_minutes > time_limit_minutes)
            {
                /* Status exceeded! Set reported replay. */
                set_replay_report(df->player, status_to_str(df->status));
                
                /* Stop execution. */
                return goto_state(&st_demo_restricted);
            }

            if (df)
            {
                fs_file fp;
#ifdef FS_VERSION_1
                if ((fp = fs_open(DIR_ITEM_GET(demo_items, val)->path, "r")))
#else
                if ((fp = fs_open_read(DIR_ITEM_GET(demo_items, val)->path)))
#endif
                {
                    SAFECPY(df->path, DIR_ITEM_GET(demo_items, val)->path);
                    SAFECPY(df->name, base_name_sans(DIR_ITEM_GET(demo_items, val)->path, ".nbr"));

                    st_demo_version_read(fp, df);

                    fs_close(fp);
                }

                if (demo_old_detected || demo_requires_update)
                {
                    /* Stop execution. */
                    return goto_state(&st_demo_restricted);
                }
            }

            if (progress_replay(DIR_ITEM_GET(demo_items, val)->path))
            {
                last_viewed = val;
                demo_play_goto(0);
                return goto_state(&st_demo_play);
            }
        }
        break;
    }
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static struct thumb
{
    int item;
    int shot;
    int name;
} thumbs[DEMO_STEP];

static int gui_demo_thumbs(int id)
{
    int w = video.device_w;
    int h = video.device_h;

    int jd, kd, ld;
    int i, j;

    struct thumb *thumb;

    if ((jd = gui_varray(id)))
        for (i = first; i < first + DEMO_STEP; i += DEMO_LINE)
            if ((kd = gui_harray(jd)))
            {
                for (j = i + DEMO_LINE - 1; j >= i; j--)
                {
                    thumb = &thumbs[j % DEMO_STEP];

                    thumb->item = j;

                    if (j < total)
                    {
                        if ((ld = gui_vstack(kd)))
                        {
                            gui_space(ld);

                            thumb->shot = gui_image(ld, " ", w / 6, h / 6);
                            thumb->name = gui_label(ld, " ", GUI_SML,
                                                    gui_wht, gui_wht);

                            gui_set_trunc(thumb->name, TRUNC_TAIL);
                            gui_set_state(ld, DEMO_SELECT, j);
                        }
                    }
                    else
                    {
                        gui_space(kd);

                        thumb->shot = 0;
                        thumb->name = 0;
                    }
                }
            }

    return jd;
}

static void gui_demo_update_thumbs(void)
{
    struct dir_item *item;
    struct demo *demo;
    int i;

    for (i = 0; i < ARRAYSIZE(thumbs) && thumbs[i].shot && thumbs[i].name; i++)
    {
        int stat_limit = config_get_d(CONFIG_ACCOUNT_LOAD);
        int stat_max = 0;
        demo_old_detected = 0;
        demo_requires_update = 0;

        item = DIR_ITEM_GET(demo_items, thumbs[i].item);
        demo = (struct demo *) item->data;

        if (demo)
        {
            fs_file fp;
#ifdef FS_VERSION_1
            if ((fp = fs_open(item->path, "r")))
#else
            if ((fp = fs_open_read(item->path)))
#endif
            {
                SAFECPY(demo->path, item->path);
                SAFECPY(demo->name, base_name_sans(item->path, ".nbr"));

                st_demo_version_read(fp, demo);

                fs_close(fp);
            }
        }

        if (demo)
        {
            if (demo->status == 3)
                stat_max = 3;
            else if (demo->status == 1 || demo->status == 0)
                stat_max = 2;
            else if (demo->status == 2)
                stat_max = 1;
        }

        gui_set_image(thumbs[i].shot, demo ? demo->shot : "");
        gui_set_label(thumbs[i].name, demo ? demo->name : base_name(item->path));
        gui_set_color(thumbs[i].name, gui_wht, gui_wht);
        
        if (stat_max > stat_limit && demo)
        {
            gui_set_image(thumbs[i].shot, stat_limit == 1 ? "gui/filters/single_filters.jpg" : "gui/filters/keep_filters.jpg");
            gui_set_color(thumbs[i].name, gui_red, gui_red);
        }
        else if (demo_requires_update && demo)
        {
            gui_set_image(thumbs[i].shot, "gui/filters/upgrade.jpg");
            gui_set_color(thumbs[i].name, gui_red, gui_red);
        }
        else if (demo_old_detected && demo)
        {
            gui_set_image(thumbs[i].shot, "gui/filters/downgrade.jpg");
            gui_set_color(thumbs[i].name, gui_red, gui_red);
        }
        else if (!demo)
            gui_set_image(thumbs[i].shot, "gui/filters/invalid.jpg");
    }

    demo_old_detected = 0;
    demo_requires_update = 0;
}

static int name_id;
static int time_id;
static int coin_id;
static int date_id;
static int status_id;
static int player_id;

static int gui_demo_status(int id)
{
    const char *status;
    int stat_len = 0;
    int jd, kd, ld;
    int s;

    /* Find the longest status string. */

    /*for (status = "", s = GAME_NONE; s < GAME_MAX + 1; s++)
        if (gui_measure(status_to_str(s), GUI_SML).w > stat_len)
            stat_len = gui_measure(status_to_str(s), GUI_SML).w;*/

    for (status = "", s = GAME_NONE; s < GAME_MAX + 1; s++)
        if (strlen(status_to_str(s)) > strlen(status))
            status = status_to_str(s);

    /* Build info bar with dummy values. */

    if ((jd = gui_hstack(id)))
    {
        gui_filler(jd);

        if ((kd = gui_hstack(jd)))
        {
            if ((ld = gui_vstack(kd)))
            {
                gui_filler(ld);

                time_id   = gui_clock(ld, 35000,  GUI_SML);
                coin_id   = gui_count(ld, 100,    GUI_SML);
                status_id = gui_label(ld, status, GUI_SML, gui_red, gui_red);

                gui_filler(ld);
            }

            if ((ld = gui_vstack(kd)))
            {
                gui_filler(ld);

                gui_label(ld, _("Time"),   GUI_SML, gui_wht, gui_wht);
                gui_label(ld, _("Coins"),  GUI_SML, gui_wht, gui_wht);
                gui_label(ld, _("Status"), GUI_SML, gui_wht, gui_wht);

                gui_filler(ld);
            }

            gui_set_rect(kd, GUI_ALL);
        }

        gui_space(jd);

        if ((kd = gui_hstack(jd)))
        {
            if ((ld = gui_vstack(kd)))
            {
                gui_filler(ld);

                name_id   = gui_label(ld, " ", GUI_SML, 0, 0);
                player_id = gui_label(ld, " ", GUI_SML, 0, 0);
                date_id   = gui_label(ld, date_to_str(time(NULL)),
                                      GUI_SML, 0, 0);

                gui_filler(ld);

                gui_set_trunc(name_id,   TRUNC_TAIL);
                gui_set_trunc(player_id, TRUNC_TAIL);
            }

            if ((ld = gui_vstack(kd)))
            {
                gui_filler(ld);

                gui_label(ld, _("Replay"), GUI_SML, gui_wht, gui_wht);
                gui_label(ld, _("Player"), GUI_SML, gui_wht, gui_wht);
                gui_label(ld, _("Date"),   GUI_SML, gui_wht, gui_wht);

                gui_filler(ld);
            }

            gui_set_rect(kd, GUI_ALL);
        }

        gui_filler(jd);
    }

    return jd;
}

static void gui_demo_update_status(int i)
{
    const struct demo *d;

    if (!total)
        return;

    /* They must be selected into the replay */
    d = DEMO_GET(demo_items, i < total ? i : 0);

    if (!d)
    {
        gui_set_label(name_id, " ");
        gui_set_label(date_id, "01.01.2003 00:00:00");
        gui_set_label(player_id, " ");
        gui_set_label(status_id, status_to_str(GAME_MAX));
        gui_set_count(coin_id, 0);
        gui_set_clock(time_id, 0);
        gui_set_color(name_id, gui_gry, gui_red);
        gui_set_color(date_id, gui_gry, gui_red);
        gui_set_color(player_id, gui_gry, gui_red);
        gui_set_color(status_id, gui_red, gui_red);
        return;
    }

    gui_set_label(name_id, d->name);
    gui_set_label(date_id, date_to_str(d->date));
    gui_set_label(player_id, d->player);

    if (d->status == GAME_GOAL)
        gui_set_color(status_id, gui_grn, gui_grn);
    else
        gui_set_color(status_id, gui_red, gui_red);

    gui_set_label(status_id, status_to_str(d->status));
    gui_set_count(coin_id, d->coins);
    gui_set_clock(time_id, d->timer);

    set_limit_status(config_get_d(CONFIG_ACCOUNT_LOAD));

    stat_limit_busy = 1;
    if (d->status == 3)
        set_maximum_status(3);
    else if (d->status == 1 || d->status == 0)
        set_maximum_status(2);
    else if (d->status == 2)
        set_maximum_status(1);
    stat_limit_busy = 0;

    time_max_minutes = d->timer / 6000;

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    if (d->mode == MODE_CAMPAIGN)
        time_limit_minutes = 60;
    else
#endif
        time_limit_minutes = 10;

    /* Has Full-access? */
    if (!check_full_access(d->name))
    {
        /* Make sure, that the status limit is underneath it. */
        if (get_maximum_status() > get_limit_status() || time_max_minutes > time_limit_minutes)
        {
            /* Status exceeded! Restrict some filters. */
            gui_set_color(name_id, gui_gry, gui_red);
            gui_set_color(date_id, gui_gry, gui_red);
            gui_set_color(player_id, gui_gry, gui_red);

            set_replay_report(d->player, status_to_str(d->status));
        }
        else
        {
            gui_set_color(name_id, gui_yel, gui_red);
            gui_set_color(date_id, gui_yel, gui_red);
            gui_set_color(player_id, gui_yel, gui_red);
        }
    }
    else
    {
        gui_set_color(name_id, gui_yel, gui_red);
        gui_set_color(date_id, gui_yel, gui_red);
        gui_set_color(player_id, gui_yel, gui_red);
    }
}

/*---------------------------------------------------------------------------*/

int standalone;

static int demo_restricted_gui(void)
{
    int id, jd, repid, repjd, repkd;

    if ((id = gui_vstack(0)))
    {
        if ((jd = gui_vstack(id)))
        {
            char infoattr[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(infoattr, dstSize, "%s: %s > %s", _("Player"), reported_player_name, reported_status);
#else
            sprintf(infoattr, "%s: %s > %s", _("Player"), reported_player_name, reported_status);
#endif
            if (demo_requires_update)
                repid = gui_label(jd, _("Update required!"), GUI_MED, gui_gry, gui_red);
            else if (time_max_minutes > time_limit_minutes)
                repid = gui_label(jd, _("Too long!"), GUI_MED, gui_gry, gui_red);
            else if (!demo_old_detected)
                repid = gui_label(jd, _("Filters restricted!"), GUI_MED, gui_gry, gui_red);
            else
                repid = gui_label(jd, _("Old replays detected!"), GUI_MED, gui_gry, gui_red);

            gui_pulse(repid, 1.2f);
            if (!standalone && !demo_requires_update && !demo_old_detected) {
                repjd = gui_label(jd, infoattr, GUI_SML, gui_red, gui_red);
                gui_pulse(repjd, 1.2f);
#if NB_STEAM_API==0 && NB_EOS_SDK==0
                if (config_cheat())
                {
                    repkd = gui_label(jd, config_get_d(CONFIG_ACCOUNT_LOAD) == 1 ? _("Only Finish") : _("Keep on board"), GUI_SML, gui_red, gui_red);
                    gui_pulse(repkd, 1.2f);
                }
#endif
            }
            gui_set_rect(jd, GUI_ALL);
        }
        gui_space(id);

        if (demo_requires_update)
            gui_multi(id, _("Please update your game, before\\watch the replay level!"), GUI_SML, gui_wht, gui_wht);
        else if (time_max_minutes > time_limit_minutes)
        {
            if (time_limit_minutes >= 60)
                gui_multi(id, _("You can't watch more than 60 minutes\\in a single level in campaign!"), GUI_SML, gui_wht, gui_wht);
            else
                gui_multi(id, _("You can't watch more than 10 minutes\\in a single level per set!"), GUI_SML, gui_wht, gui_wht);
        }
        else if (!demo_old_detected)
            gui_multi(id, _("You can't open selected replay,\\because it was restricted for you!"), GUI_SML, gui_wht, gui_wht);
        else
            gui_multi(id, _("You can't open selected replay, because\\you are using oldest version of the game!"), GUI_SML, gui_wht, gui_wht);

        gui_layout(id, 0, 0);
    }

    demo_old_detected = 0;
    demo_requires_update = 0;

    return id;
}

static int demo_restricted_enter(struct state *st, struct state *prev)
{
    audio_music_fade_out(0.f);
    audio_play(AUD_INTRO_SHATTER, 1.0f);
    return demo_restricted_gui();
}

static void demo_restricted_timer(int id, float dt)
{
    if (is_opened) game_step_fade(dt);

    gui_timer(id, dt);
}

static int demo_restricted_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT)
            if (is_opened)
                return goto_state(&st_demo_end);
            else
                return goto_state(&st_demo);
    }
    return 1;
}

static int demo_restricted_buttn(int b, int d)
{
    if (d)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            if (is_opened)
                return goto_state(&st_demo_end);
            else
                return goto_state(&st_demo);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            if (is_opened)
                return goto_state(&st_demo_end);
            else
                return goto_state(&st_demo);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

int detailpanel;
int scanfastforwards = 0;
int threshold = 0;

static int demo_scan_allowance_gui()
{
    char cancelattr[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(cancelattr, dstSize, _("Scan in progress...\\To cancel scanning, press %s"), SDL_GetKeyName(KEY_EXIT));
#else
    sprintf(cancelattr, _("Scan in progress...\\To cancel scanning, press %s"), SDL_GetKeyName(KEY_EXIT));
#endif
    detailpanel = gui_multi(0, cancelattr, GUI_SML, gui_wht, gui_wht);
    gui_layout(detailpanel, 0, 0);
    return detailpanel;
}

static int demo_scan_allowance_enter(struct state *st, struct state *prev)
{
    threshold = 0;
    /* Scan levels */
    game_proxy_filter(filter_cmd);
    set_limit_status(config_get_d(CONFIG_ACCOUNT_LOAD));

    premaded_timer = curr_clock();
    return demo_scan_allowance_gui();
}

static void demo_scan_allowance_timer(int id, float dt)
{
    if (premaded_timer == -1) { premaded_timer = curr_clock(); return; }

    int aim_timer = premaded_timer - curr_clock();

    stat_limit_busy = 1;
    if (curr_status() == 3)
        set_maximum_status(3);
    else if (curr_status() == 1 || curr_status() == 0)
        set_maximum_status(2);
    else if (curr_status() == 2)
        set_maximum_status(1);
    stat_limit_busy = 0;

    if (get_maximum_status() > get_limit_status() && (!scanfastforwards || standalone))
    {
        demo_replay_stop(0);
        goto_state(&st_demo_restricted);
        //game_client_blend(demo_replay_blend());
        return;
    }

    if (!demo_replay_step(dt))
    {
        demo_replay_stop(0);
        if (!progress_replay(curr_demo()))
        {
            if (!standalone) goto_state(&st_demo); else { SDL_Event e = { SDL_QUIT }; SDL_PushEvent(&e); }
        }
        else
        {
            threshold++;
            if (threshold == 5)
            {
                goto_state(&st_demo_play);
            }
        }
    }
    else
    {
        progress_step();
        //game_client_blend(0);
    }

    if (!standalone)
    {
        if (premaded_timer > 0)
        {
            if (aim_timer >= target_timer - 100)
            {
                demo_replay_manual_speed(1);
                scanfastforwards = 0;
                return;
            }
        }
        else if (curr_clock() >= target_timer - 100)
        {
            demo_replay_manual_speed(1);
            scanfastforwards = 0;
            return;
        }

        scanfastforwards = 1;
        demo_replay_manual_speed(target_timer / 100);
    }
    else
        demo_replay_manual_speed(32);
}

static int demo_scan_allowance_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT)
        {
            demo_replay_stop(0);
            return standalone ? 0 : goto_state(&st_demo);
        }
    }
    return 1;
}

static int demo_scan_allowance_buttn(int b, int d)
{
    if (d)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
        {
            demo_replay_stop(0);
            return standalone ? 0 : goto_state(&st_demo);
        }
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int demo_gui(void)
{
    int id, jd;

    id = gui_vstack(0);

    if (total && availibility)
    {
        if ((jd = gui_hstack(id)))
        {
            if (total != availibility)
            {
                char availibility_header_monitor[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(availibility_header_monitor, dstSize, _("Replays unlocked: %d/%d"), availibility, total);
#else
                sprintf(availibility_header_monitor, _("Replays unlocked: %d/%d"), availibility, total);
#endif

                int header_id = gui_label(jd, availibility_header_monitor, GUI_SML, 0, 0);

                float availibility_percent = ((float) availibility / (float) total);

                if (availibility_percent >= 0.75f)
                    gui_set_color(header_id, gui_wht, gui_cya);
                else if (availibility_percent >= 0.5f)
                    gui_set_color(header_id, gui_wht, gui_grn);
                else if (availibility_percent >= 0.25f)
                    gui_set_color(header_id, gui_wht, gui_yel);
                else
                    gui_set_color(header_id, gui_gry, gui_red);
            }
            else
            {
                gui_label(jd, _("Select Replay"), GUI_SML, 0, 0);
            }

            gui_filler(jd);
            gui_navig(jd, total, first, DEMO_STEP);
        }

        gui_demo_thumbs(id);
        gui_space(id);
        gui_demo_status(id);

        gui_layout(id, 0, 0);

        gui_demo_update_thumbs();
        gui_demo_update_status(last_viewed);
    }
    else if (total && !availibility)
    {
        gui_title_header(id, _("All replays locked!"), GUI_MED, gui_gry, gui_red);
        gui_space(id);
        gui_multi(id, _("Open the file manager to delete\\or backup your replays."), GUI_SML, gui_wht, gui_wht);

#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
        if (current_platform == PLATFORM_PC)
        {
            gui_space(id);
            gui_state(id, _("Back"), GUI_SML, GUI_BACK, 0);
        }
#endif

        gui_layout(id, 0, 0);
    }
    else if (!total && !availibility)
    {
        gui_title_header(id, _("No Replays"), GUI_MED, 0, 0);
        gui_space(id);
        gui_multi(id, _("Your Replays will appear here\\once you've recorded."), GUI_SML, gui_wht, gui_wht);

#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
        if (current_platform == PLATFORM_PC)
        {
            gui_space(id);
            gui_state(id, _("Back"), GUI_SML, GUI_BACK, 0);
        }
#endif

        gui_layout(id, 0, 0);
    }
    else
    {
        assert(0 && "Unknown state!");
        goto_state(&st_title);
    }

    return id;
}

static int demo_enter(struct state *st, struct state *prev)
{
    audio_setspeed(1.0f);
    set_limit_status(config_get_d(CONFIG_ACCOUNT_LOAD));

#if defined(COVID_HIGH_RISK)
    st_demo_headbacktoscan:
#endif

    is_opened = 0; availibility = 0;
    game_proxy_filter(NULL);

    /*if (!demo_items || (prev == &st_demo_del))
    {
        if (demo_items)
        {
            demo_dir_free(demo_items);
            demo_items = NULL;
        }

        demo_items = demo_dir_scan();
        total = array_len(demo_items);
    }*/

    if (demo_items)
    {
        demo_dir_free(demo_items);
        demo_items = NULL;
    }

    demo_items = demo_dir_scan();
    total = array_len(demo_items);

    first       = first < total ? first : 0;
    last        = MIN(first + DEMO_STEP - 1, total - 1);
    last_viewed = MIN(MAX(first, last_viewed), last);

    if (demo_items && total)
    {
        Array quarantined_demo_items = demo_dir_scan();
        demo_dir_load(quarantined_demo_items, 0, total - 1);
        int quarantined_index = 0;
        while (quarantined_index < total)
        {
            struct demo *targetDemo = DEMO_GET(quarantined_demo_items, quarantined_index);
            if (targetDemo)
            {
                stat_limit_busy = 1;
                if (targetDemo->status == 3)
                    set_maximum_status(3);
                else if (targetDemo->status == 1 || targetDemo->status == 0)
                    set_maximum_status(2);
                else if (targetDemo->status == 2)
                    set_maximum_status(1);
                stat_limit_busy = 0;

                if (get_maximum_status() > get_limit_status())
                {
#if defined(COVID_HIGH_RISK)
                    log_errorf("Replay files deleted due covid high risks!: %s", targetDemo->path);
                    fs_remove(targetDemo->path);

                    goto st_demo_headbacktoscan;
#else
                    availibility -= 1;
#endif
                }
                else
                    availibility += 1;
            }

            quarantined_index++;
        }

        demo_dir_free(quarantined_demo_items);
        quarantined_demo_items = NULL;

        assert(quarantined_index == total);
    }

    if (total)
        demo_dir_load(demo_items, first, last);

    if (prev == &st_title || prev == &st_demo_end || prev == &st_demo_del)
    {
        if (total && availibility == 0)
            audio_narrator_play("snd/lockdown_all.ogg");
        else if (total != availibility)
            audio_narrator_play("snd/lockdown_least.ogg");
    }

    audio_music_fade_to(0.5f, switchball_useable() ? "bgm/title-switchball.ogg" : "bgm/title.ogg");

    return demo_gui();
}

static void demo_leave(struct state *st, struct state *next, int id)
{
    if (next == &st_title)
    {
        demo_dir_free(demo_items);
        demo_items = NULL;
    }

    gui_delete(id);
}

static void demo_paint(int id, float t)
{
    game_client_draw(0, t);

    gui_paint(id);
#if !defined(__EMSCRIPTEN__)
    xbox_control_list_gui_paint();
#endif
}

static void demo_timer(int id, float dt)
{
    gui_timer(id, dt);
}

static void demo_point(int id, int x, int y, int dx, int dy)
{
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
    if (current_platform == PLATFORM_PC)
        xbox_toggle_gui(0);
#endif

    int jd = shared_point_basic(id, x, y);

    if (jd && gui_token(jd) == DEMO_SELECT)
        gui_demo_update_status(gui_value(jd));
}

static void demo_stick(int id, int a, float v, int bump)
{
#if !defined(__EMSCRIPTEN__)
    xbox_toggle_gui(1);
#endif
    int jd = shared_stick_basic(id, a, v, bump);

    if (jd && gui_token(jd) == DEMO_SELECT)
        gui_demo_update_status(gui_value(jd));
}

static int demo_keybd(int c, int d)
{
    if (d)
    {
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
        if (c == KEY_EXIT && current_platform == PLATFORM_PC)
            return demo_action(GUI_BACK, 0);
#else
        if (c == KEY_EXIT)
            return demo_action(GUI_BACK, 0);
#endif
    }
    return 1;
}

static int demo_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
        {
            if (total)
                return demo_action(gui_token(active), gui_value(active));
            else
                return demo_action(GUI_BACK, 0);
        }

        if (total)
        {
            if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L1, b) && first > 0)
                return demo_action(DEMO_XBOX_LB, 0);
            if (config_tst_d(CONFIG_JOYSTICK_BUTTON_R1, b) && first + DEMO_STEP < total)
                return demo_action(DEMO_XBOX_RB, 0);
        }

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return demo_action(GUI_BACK, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int demo_freeze_all;

static int demo_paused;
static int show_hud;
static int check_compat;
static int speed;

static int speed_manual;

static float prelude;

/* Timers needed */
int demo_timer_curr;
int demo_timer_last;

int downwards;

/* Keyboard inputs */
static int faster;

void demo_play_goto(int s)
{
    set_limit_status(config_get_d(CONFIG_ACCOUNT_LOAD));
    standalone   = s;
    check_compat = 1;
    is_opened = 1;
}

static int demo_play_gui(void)
{
    int id;

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("Replay"), GUI_LRG, gui_blu, gui_grn);
        gui_layout(id, 0, 0);
        gui_pulse(id, 1.2f);
    }

    return id;
}

static int demo_play_enter(struct state *st, struct state *prev)
{
    demo_freeze_all = 0;
    video_hide_cursor();

    if (demo_paused
        || prev == &st_demo_play
        || prev == &st_demo_look)
    {
        demo_paused = 0;
        prelude = 0;
        audio_music_fade_in(0.5f);
        return 0;
    }

    /*
     * Post-1.5.1 replays include view data in the first update, this
     * line is currently left in for compatibility with older replays.
     */
    //game_client_fly(0.0f);
    
    hud_update(0, 0.0f);

    if (get_maximum_status() > get_limit_status())
    {
        goto_state(&st_demo_restricted);
        return 0;
    }

    if (check_compat && !game_compat_map)
    {
        if (!game_compat_map)
            allow_exact_versions = 0;

        goto_state(&st_demo_compat);
        return 0;
    }
    else if (game_compat_map)
        allow_exact_versions = 1;

    prelude = 1.0f;

    speed_manual = 0;

    speed = SPEED_NORMAL;
    demo_replay_speed(speed);
    faster = 0;

    show_hud = 1;

    downwards = 0;
    demo_timer_last = 0;

    return demo_play_gui();
}

static void demo_play_leave(struct state *st, struct state *next, int id)
{
    gui_delete(id);

    video_show_cursor();
}

static void demo_play_paint(int id, float t)
{
    game_client_draw(0, t);

    if ((show_hud || config_get_d(CONFIG_SCREEN_ANIMATIONS)) && (!speed_manual || config_get_d(CONFIG_SCREEN_ANIMATIONS)))
        hud_paint();

    if (time_state() < prelude)
        gui_paint(id);
}

static void demo_play_timer(int id, float dt)
{
    float timescale = 1.0f;
    toggle_hud_visibility(show_hud);
    switch (speed)
    {
    case SPEED_SLOWESTESTEST: timescale = (1.0f / 128.0f); break;
    case SPEED_SLOWESTESTER: timescale = (1.0f / 64.0f); break;
    case SPEED_SLOWESTEST: timescale = (1.0f / 32.0f); break;
    case SPEED_SLOWESTER: timescale = (1.0f / 16.0f); break;
    case SPEED_SLOWEST: timescale = (1.0f / 8.0f); break;
    case SPEED_SLOWER: timescale = (1.0f / 4.0f); break;
    case SPEED_SLOW: timescale = (1.0f / 2.0f); break;
    case SPEED_FAST: timescale = 2.0f; break;
    case SPEED_FASTER: timescale = 4.0f; break;
    case SPEED_FASTEST: timescale = 8.0f; break;
    case SPEED_FASTESTER: timescale = 16.0f; break;
    case SPEED_FASTESTEST: timescale = 32.0f; break;
    case SPEED_FASTESTESTER: timescale = 64.0f; break;
    case SPEED_FASTESTESTEST: timescale = 128.0f; break;
    }

    if (!game_client_get_jump_b())
        geom_step(speed == SPEED_NONE ? 0 : dt * timescale);

    if (speed_manual && !game_client_get_jump_b())
        geom_step(dt * 2);

    game_step_fade(dt);
    gui_timer(id, dt);

    if (!speed_manual || config_get_d(CONFIG_SCREEN_ANIMATIONS))
    {
        hud_timer(dt);
    }

    demo_timer_last = demo_timer_curr;
    demo_timer_curr = curr_clock();

    /* Pause briefly before starting playback. */

    if (time_state() < prelude)
        return;

    if (!downwards && demo_timer_last > demo_timer_curr)
        downwards = 1;

    if ((demo_timer_last > demo_timer_curr && demo_timer_curr != 0) || downwards) {
        if (demo_timer_curr < 1 && speed > SPEED_SLOWESTESTEST) { speed = SPEED_SLOWESTESTEST; demo_replay_speed(speed); hud_speed_pulse(speed); }
        else if (demo_timer_curr < 12 && speed > SPEED_SLOWESTESTER) { speed = SPEED_SLOWESTESTER; demo_replay_speed(speed); hud_speed_pulse(speed); }
        else if (demo_timer_curr < 25 && speed > SPEED_SLOWESTEST) { speed = SPEED_SLOWESTEST; demo_replay_speed(speed); hud_speed_pulse(speed); }
        else if (demo_timer_curr < 50 && speed > SPEED_SLOWESTER) { speed = SPEED_SLOWESTER; demo_replay_speed(speed); hud_speed_pulse(speed); }
        else if (demo_timer_curr < 100 && speed > SPEED_SLOWEST) { speed = SPEED_SLOWEST; demo_replay_speed(speed); hud_speed_pulse(speed); }
        else if (demo_timer_curr < 200 && speed > SPEED_SLOWER) { speed = SPEED_SLOWER; demo_replay_speed(speed); hud_speed_pulse(speed); }
        else if (demo_timer_curr < 500 && speed > SPEED_SLOW) { speed = SPEED_SLOW; demo_replay_speed(speed); hud_speed_pulse(speed); }
        else if (demo_timer_curr < 1000 && speed > SPEED_NORMAL) { speed = SPEED_NORMAL; demo_replay_speed(speed); hud_speed_pulse(speed); }
    }

    if (demo_freeze_all) return;

    if (!demo_replay_step(dt))
    {
        demo_freeze_all = 1;
        demo_paused = 0;
        goto_state(&st_demo_end);
    }
    else
    {
        progress_step();
        game_client_blend(demo_replay_blend());
    }
}

static void set_speed(int d)
{
    if (!speed_manual)
    {
        if (d > 0) speed = SPEED_UP(speed);
        if (d < 0) speed = SPEED_DN(speed);

        demo_replay_speed(speed);

        hud_speed_pulse(speed);
    }
}

static void demo_play_stick(int id, int a, float v, int bump)
{
    if (!bump)
        return;

    if (config_tst_d(CONFIG_JOYSTICK_AXIS_Y0, a))
    {
        if (v + axis_offset[1] < 0) set_speed(+1);
        if (v + axis_offset[1] > 0) set_speed(-1);
    }
}

static void demo_play_wheel(int x, int y)
{
    if (y > 0) set_speed(+1);
    if (y < 0) set_speed(-1);
}

static int demo_play_click(int b, int d)
{
    if (d && time_state() > prelude)
    {
        if (config_tst_d(CONFIG_MOUSE_CAMERA_R, b)) 
        {
            demo_replay_manual_speed(2.0f); speed_manual = 1;
            game_proxy_filter(filter_cmd);
            audio_music_fade_out(0.2f);
        }
        /*if (config_tst_d(CONFIG_MOUSE_CAMERA_L, b))
        {
            demo_replay_manual_speed(-2.0f); speed_manual = 1;
            game_proxy_filter(filter_cmd);
            audio_music_fade_out(0.2f);
        }*/
    }
    else if (time_state() > prelude)
    {
        if (config_tst_d(CONFIG_MOUSE_CAMERA_R, b))
            demo_replay_speed(speed); speed_manual = 0;
        /*if (config_tst_d(CONFIG_MOUSE_CAMERA_L, b))
            demo_replay_speed(speed); speed_manual = 0;
        */

        game_proxy_filter(NULL);
        audio_music_fade_in(0.5f);
    }

    return 1;
}

static int demo_play_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT && !speed_manual)
        {
            demo_freeze_all = 1;
            demo_paused = 1;
            return goto_state(&st_demo_end);
        }

        if (c == KEY_POSE && !speed_manual)
            show_hud = !show_hud;

        if (c == SDLK_8 && !faster) { speed = SPEED_SLOWEST; demo_replay_speed(speed); hud_speed_pulse(speed); }
        if (c == SDLK_4 && !faster) { speed = SPEED_SLOWER; demo_replay_speed(speed); hud_speed_pulse(speed); }
        if (c == SDLK_2 && !faster) { speed = SPEED_SLOW; demo_replay_speed(speed); hud_speed_pulse(speed); }
        if (c == SDLK_1) { speed = SPEED_NORMAL; demo_replay_speed(speed); hud_speed_pulse(speed); }
        if (c == SDLK_2 && faster) { speed = SPEED_FAST; demo_replay_speed(speed); hud_speed_pulse(speed); }
        if (c == SDLK_4 && faster) { speed = SPEED_FASTER; demo_replay_speed(speed); hud_speed_pulse(speed); }
        if (c == SDLK_8 && faster) { speed = SPEED_FASTEST; demo_replay_speed(speed); hud_speed_pulse(speed); }
        if (c == SDLK_LSHIFT) { faster = 1; }

        /* Used centered player or special camera orientation */
        if (c == KEY_LOOKAROUND)
            return goto_state(&st_demo_look);
    }
    else
    {
        if (c == SDLK_LSHIFT) { faster = 0; }
    }
    return 1;
}

static int demo_play_buttn(int b, int d)
{
    if (d)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b) ||
            config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b) ||
            config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
        {
            demo_freeze_all = 1;
            demo_paused = 1;
            return goto_state(&st_demo_end);
        }
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

enum
{
    DEMO_KEEP = GUI_LAST,
    DEMO_DEL,
    DEMO_QUIT,
    DEMO_REPLAY,
    DEMO_CONTINUE
};

static int demo_end_action(int tok, int val)
{
    audio_play(AUD_MENU, 1.0f);

    switch (tok)
    {
    case DEMO_DEL:
        demo_paused = 0;
        return goto_state(&st_demo_del);
    case DEMO_KEEP:
        demo_paused = 0;
        demo_replay_stop(0);
        return goto_state(&st_demo);
    case DEMO_QUIT:
        is_opened = 0;
        demo_replay_stop(0);
        game_fade(+4.0);
        goto_state_full(&st_null, 0, 0, 0); /* bye! */

        game_server_free(NULL);
        game_client_free(NULL);
        game_base_free(NULL);
        return 0;
    case DEMO_REPLAY:
        if (demo_paused)
            demo_paused = 0;

        demo_replay_stop(0);
        progress_replay(curr_demo());
        return goto_state(&st_demo_play);
    case DEMO_CONTINUE:
        return goto_state(&st_demo_play);
    }
    return 1;
}

static int demo_end_gui(void)
{
    int id, jd, kd;

    if ((id = gui_vstack(0)))
    {
        if (demo_paused)
            kd = gui_title_header(id, _("Replay Paused"), GUI_LRG, gui_gry, gui_red);
        else
            kd = gui_title_header(id, _("Replay Ends"), GUI_LRG, gui_gry, gui_red);

        gui_space(id);

        if ((jd = gui_harray(id)))
        {
            if (standalone)
                gui_start(jd, _("Quit"), GUI_SML, DEMO_QUIT, 0);
            else
            {
                gui_start(jd, _("Keep"), GUI_SML, DEMO_KEEP, 0);
                gui_state(jd, _("Delete"), GUI_SML, DEMO_DEL, 0);
            }

            /* Only that is limit underneath it */
            if (get_maximum_status() <= get_limit_status() && allow_exact_versions) {
                gui_state(jd, _("Repeat"), GUI_SML, DEMO_REPLAY, 0);
                if (demo_paused)
                    gui_start(jd, _("Continue"), GUI_SML, DEMO_CONTINUE, 0);
            }
            else
            {
                gui_label(jd, _("Repeat"), GUI_SML, gui_gry, gui_gry);
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
                if (demo_paused && current_platform == PLATFORM_PC)
#else
                if (demo_paused)
#endif
                    gui_label(jd, _("Continue"), GUI_SML, gui_gry, gui_gry);
            }
        }

        gui_pulse(kd, 1.2f);
        gui_layout(id, 0, 0);
    }

    return id;
}

static int demo_end_enter(struct state *st, struct state *prev)
{
    audio_music_fade_out(demo_paused ? 0.2f : 1.0f);

    return demo_end_gui();
}

static void demo_end_paint(int id, float t)
{
    game_client_draw(0, t);

    gui_paint(id);
#if !defined(__EMSCRIPTEN__)
    if (demo_paused && !xbox_show_gui() && show_hud)
        hud_paint();

    if (xbox_show_gui())
    {
        if (demo_paused)
            xbox_control_paused_gui_paint();
        else
            xbox_control_replay_eof_paint();
    }
#else
    hud_paint();
#endif
}

static int demo_end_keybd(int c, int d)
{
    if (d)
    {
        if (demo_paused && c == KEY_EXIT && allow_exact_versions)
            return demo_end_action(DEMO_CONTINUE, 0);
    }
    return 1;
}

static int demo_end_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return demo_end_action(gui_token(active), gui_value(active));

        if (demo_paused && allow_exact_versions)
        {
            if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b) ||
                config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
                return demo_end_action(DEMO_CONTINUE, 0);
        }
        else
        {
            if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
                return demo_end_action(standalone ? DEMO_QUIT : DEMO_KEEP, 0);
        }
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int demo_del_action(int tok, int val)
{
    audio_play(AUD_MENU, 1.0f);
    demo_replay_stop(tok == DEMO_DEL);
    return goto_state(&st_demo);
}

static int demo_del_gui(void)
{
    int id, jd, kd;

    if ((id = gui_vstack(0)))
    {
        kd = gui_title_header(id, _("Delete Replay?"), GUI_MED, gui_red, gui_red);
        gui_space(id);
        gui_multi(id,
            _("Once deleted this replay, this action cannot be undone."),
            GUI_SML, gui_wht, gui_wht);
        gui_space(id);

        if ((jd = gui_harray(id)))
        {
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
            if (current_platform == PLATFORM_PC)
            {
#endif
                gui_start(jd, _("Keep"), GUI_SML, DEMO_KEEP, 0);
                gui_state(jd, _("Delete"), GUI_SML, DEMO_DEL, 0);
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
            }
            else
                gui_start(jd, _("Delete"), GUI_SML, DEMO_DEL, 0);
#endif
        }

        gui_pulse(kd, 1.2f);
        gui_layout(id, 0, 0);
    }

    return id;
}

static int demo_del_enter(struct state *st, struct state *prev)
{
    audio_music_fade_out(demo_paused ? 0.2f : 1.0f);

    return demo_del_gui();
}

static int demo_del_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT)
            return demo_del_action(GUI_BACK, 0);
    }
    return 1;
}

static int demo_del_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return demo_del_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return demo_del_action(DEMO_KEEP, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int demo_compat_gui(void)
{
    int id;

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("Warning!"), GUI_MED, gui_red, gui_red);
        gui_space(id);
        gui_multi(id, _("The current replay was recorded with a\\"
                        "different (or unknown) version of this level.\\"
                        "Be prepared to encounter visual errors.\\"),
                  GUI_SML, gui_wht, gui_wht);

        gui_layout(id, 0, 0);
    }

    return id;
}

static int demo_compat_enter(struct state *st, struct state *prev)
{
    check_compat = 0;

    return demo_compat_gui();
}

static void demo_compat_timer(int id, float dt)
{
    game_step_fade(dt);
    gui_timer(id, dt);
}

static int demo_compat_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT)
            return goto_state(&st_demo_end);
    }
    return 1;
}

static int demo_compat_buttn(int b, int d)
{
    if (d)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return goto_state(&st_demo_play);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return goto_state(&st_demo_end);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static float phi;
static float theta;

static float demo_look_stick_x, demo_look_stick_y;

static int demo_look_enter(struct state* st, struct state* prev)
{
    demo_look_stick_x = 0;
    demo_look_stick_y = 0;
    phi = 0;
    theta = 0;
    return 0;
}

static void demo_look_leave(struct state* st, struct state* next, int id)
{
}

static void demo_look_timer(int id, float dt)
{
    theta += demo_look_stick_x * 2;
    phi += demo_look_stick_y * -2;

    if (phi > +90.0f) phi = +90.0f;
    if (phi < -90.0f) phi = -90.0f;

    if (theta > +180.0f) theta -= 360.0f;
    if (theta < -180.0f) theta += 360.0f;

    game_look(phi, theta);
}

static void demo_look_paint(int id, float t)
{
    game_client_draw(0, t);
}

static void demo_look_point(int id, int x, int y, int dx, int dy)
{
#if NDEBUG
    phi += 90.0f * dy / video.device_h;
    theta += 180.0f * dx / video.device_w;

    if (phi > +90.0f) phi = +90.0f;
    if (phi < -90.0f) phi = -90.0f;

    if (theta > +180.0f) theta -= 360.0f;
    if (theta < -180.0f) theta += 360.0f;

    game_look(phi, theta);
#endif
}

static void demo_look_stick(int id, int a, float v, int bump)
{
    if (config_tst_d(CONFIG_JOYSTICK_AXIS_X0, a))
        demo_look_stick_x = v;
    if (config_tst_d(CONFIG_JOYSTICK_AXIS_Y0, a))
        demo_look_stick_y = v;
}

static int demo_look_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT || c == KEY_LOOKAROUND)
            return goto_state(&st_demo_play);
    }

    return 1;
}

static int demo_look_buttn(int b, int d)
{
    if (d && (config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b)))
        return goto_state(&st_demo_play);

    return 1;
}

/*---------------------------------------------------------------------------*/

struct state st_demo_scan_allowance = {
    demo_scan_allowance_enter,
    shared_leave,
    shared_paint,
    demo_scan_allowance_timer,
    shared_point,
    shared_stick,
    shared_angle,
    NULL,
    demo_scan_allowance_keybd,
    demo_scan_allowance_buttn
};

struct state st_demo_restricted = {
    demo_restricted_enter,
    shared_leave,
    shared_paint,
    demo_restricted_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click_basic,
    demo_restricted_keybd,
    demo_restricted_buttn
};

struct state st_demo = {
    demo_enter,
    demo_leave,
    demo_paint,
    demo_timer,
    demo_point,
    demo_stick,
    shared_angle,
    shared_click,
    demo_keybd,
    demo_buttn,
    NULL,
    NULL,
    demo_shared_fade
};

struct state st_demo_play = {
    demo_play_enter,
    demo_play_leave,
    demo_play_paint,
    demo_play_timer,
    NULL,
    demo_play_stick,
    NULL,
    shared_click_basic,
    demo_play_keybd,
    demo_play_buttn,
    demo_play_wheel,
    NULL,
    demo_shared_fade
};

struct state st_demo_end = {
    demo_end_enter,
    shared_leave,
    demo_end_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    demo_end_keybd,
    demo_end_buttn,
    NULL,
    NULL
};

struct state st_demo_del = {
    demo_del_enter,
    shared_leave,
    shared_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    demo_del_keybd,
    demo_del_buttn,
    NULL,
    NULL,
    demo_shared_fade
};

struct state st_demo_compat = {
    demo_compat_enter,
    shared_leave,
    shared_paint,
    demo_compat_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click_basic,
    demo_compat_keybd,
    demo_compat_buttn,
    NULL,
    NULL
};

struct state st_demo_look = {
    demo_look_enter,
    demo_look_leave,
    demo_look_paint,
    demo_look_timer,
    demo_look_point,
    demo_look_stick,
    NULL,
    NULL,
    demo_look_keybd,
    demo_look_buttn
};
