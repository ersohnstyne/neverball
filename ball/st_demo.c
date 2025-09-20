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

#include <string.h>

#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

/*
 * HACK: Used with console version
 */
#include "console_control_gui.h"

#include "gui.h"
#include "transition.h"
//#include "hud.h"
//#include "set.h"
#include "binary.h"
//#include "demo.h"
//#include "progress.h"
//#include "audio.h"
//#include "config.h"
#include "util.h"
#include "common.h"
#include "demo_dir.h"
//#include "video.h"
//#include "geom.h"
//#include "vec3.h"
#include "text.h"

//#include "game_common.h"
//#include "game_client.h"
//#include "game_server.h"
//#include "game_proxy.h"

#include "st_play_sync.h"

#include "st_demo.h"
#include "st_title.h"
#include "st_shared.h"
#include "st_conf.h"

#if defined(__WII__)
/* We're using SDL 1.2 on Wii, which has SDLKey instead of SDL_Keycode. */
typedef SDLKey SDL_Keycode;
#endif

/*---------------------------------------------------------------------------*/

static int switchball_useable(void)
{
    const SDL_Keycode k_auto = config_get_d(CONFIG_KEY_CAMERA_TOGGLE);
    const SDL_Keycode k_cam1 = config_get_d(CONFIG_KEY_CAMERA_1);
    const SDL_Keycode k_cam2 = config_get_d(CONFIG_KEY_CAMERA_2);
    const SDL_Keycode k_cam3 = config_get_d(CONFIG_KEY_CAMERA_3);
    const SDL_Keycode k_caml = config_get_d(CONFIG_KEY_CAMERA_L);
    const SDL_Keycode k_camr = config_get_d(CONFIG_KEY_CAMERA_R);

    SDL_Keycode k_arrowkey[4] = {
        config_get_d(CONFIG_KEY_FORWARD),
        config_get_d(CONFIG_KEY_LEFT),
        config_get_d(CONFIG_KEY_BACKWARD),
        config_get_d(CONFIG_KEY_RIGHT)
    };

    if (k_auto == SDLK_c && k_cam1 == SDLK_3 && k_cam2 == SDLK_1 && k_cam3 == SDLK_2 &&
        k_caml == SDLK_RIGHT && k_camr == SDLK_LEFT &&
        k_arrowkey[0] == SDLK_w && k_arrowkey[1] == SDLK_a && k_arrowkey[2] == SDLK_s && k_arrowkey[3] == SDLK_d)
        return 1;
    else if (k_auto == SDLK_c && k_cam1 == SDLK_3 && k_cam2 == SDLK_1 && k_cam3 == SDLK_2 &&
             k_caml == SDLK_d && k_camr == SDLK_a &&
             k_arrowkey[0] == SDLK_UP && k_arrowkey[1] == SDLK_LEFT && k_arrowkey[2] == SDLK_DOWN && k_arrowkey[3] == SDLK_RIGHT)
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

struct state st_demo_restricted;
struct state st_demo_end;
struct state st_demo_del;
struct state st_demo_compat;
struct state st_demo_look;

/*---------------------------------------------------------------------------*/

#define DEMO_LINE 4
#define DEMO_STEP 8

static Array demo_items;

static int is_opened = 0;

static int first = 0;
static int total = 0;
static int last  = 0;
static int availibility = 0;

static int selected    = 0;
static int last_viewed = 0;

static int allow_exact_versions = 0;

/*---------------------------------------------------------------------------*/

static int st_demo_version_read(fs_file fp, struct demo *d)
{
    const int demo_file_magic = get_index(fp);
    const int demo_file_version = get_index(fp);

    if (demo_file_magic == DEMO_MAGIC) return 0;

    if (demo_file_version > DEMO_VERSION)
    {
        demo_requires_update = 1;
        return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/

static int stat_limit_busy = 0;
static int stat_max = 0;

static int time_max_minutes = 0;
static int time_limit_minutes = 10;

static char *reported_player_name;
static char *reported_status;

static int get_max_game_stat()
{
    return stat_max;
}

static void set_max_game_stat(int currstat)
{
    stat_max = currstat;
}

static int get_limit_game_stat()
{
    return config_get_d(CONFIG_ACCOUNT_LOAD);
}

static void set_replay_report(char *target_player_name, const char *target_status)
{
    reported_player_name = target_player_name;

    if (reported_status)
    {
        free(reported_status);
        reported_status = NULL;
    }

    reported_status = strdup(target_status);
}

/*---------------------------------------------------------------------------*/

static void demo_shared_fade(float alpha)
{
    hud_set_alpha(alpha);
}

/*---------------------------------------------------------------------------*/

enum
{
    DEMO_PLAY = GUI_LAST,
    DEMO_SELECT,
    DEMO_UPGRADE_LIMIT,
    DEMO_DELETE
};

static void demo_select(int i);

static int demo_action(int tok, int val)
{
    GAMEPAD_GAMEMENU_ACTION_SCROLL(GUI_PREV, GUI_NEXT, DEMO_STEP);

    switch (tok)
    {
        case GUI_BACK:
            return exit_state(&st_title);

        case GUI_PREV:
            first = MAX(first - DEMO_STEP, 0);

            return exit_state(&st_demo);
            break;

        case GUI_NEXT:
            first = MIN(first + DEMO_STEP, total - 1);

            return goto_state(&st_demo);
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
            demo_select(val);
            break;

        case DEMO_PLAY:
        {
            if (val != selected)
                return 1;

            struct demo *df;

            if (!DEMO_CHECK_GET(df, demo_items, selected < total ? selected : 0)) return 1;

            stat_limit_busy = 1;

            switch (df->status)
            {
                case GAME_GOAL: set_max_game_stat(1); break;
                case GAME_FALL: set_max_game_stat(3); break;
                default:        set_max_game_stat(2);
            }

            time_max_minutes = df->timer / 6000;

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            if (df->mode == MODE_CAMPAIGN)
                time_limit_minutes = 60;
            else
#endif
                time_limit_minutes = 10;

            stat_limit_busy = 0;

            /* Make sure, that the status limit is underneath it. */

            if (get_max_game_stat() > get_limit_game_stat() ||
                time_max_minutes > time_limit_minutes)
            {
                /* Max status exceeds limits! Set reported replay. */
                set_replay_report(df->player, status_to_str(df->status));

                /* Stop execution. */
                return goto_state(&st_demo_restricted);
            }

            const char *demo_path = DIR_ITEM_GET(demo_items, selected)->path;

            if (df)
            {
                fs_file fp;

                if ((fp = fs_open_read(demo_path)))
                {
                    SAFECPY(df->path, demo_path);
                    SAFECPY(df->name, base_name_sans(demo_path,
                                                     str_ends_with(demo_path, ".nbrx") ? ".nbrx" : ".nbr"));

                    st_demo_version_read(fp, df);

                    fs_close(fp);
                }

                if (demo_requires_update)
                {
                    /* Stop execution. */
                    return goto_state(&st_demo_restricted);
                }
            }

            if (progress_replay(demo_path))
            {
                last_viewed = selected;
                allow_exact_versions = 1;
                return demo_play_goto(0);
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
    int shot_id;
    int name_id;
    int thumb_id;
} thumbs[DEMO_STEP];

static int gui_demo_thumbs(int id)
{
    int w = video.device_w;
    int h = video.device_h;

    int jd, kd, ld, md;

    struct thumb *thumb;

    if ((jd = gui_hstack(id)))
    {
        gui_filler(jd);

        if ((kd = gui_varray(jd)))
            for (int i = first; i < first + DEMO_STEP; i += DEMO_LINE)
                if ((ld = gui_harray(kd)))
                {
                    for (int j = i + DEMO_LINE - 1; j >= i; j--)
                    {
                        thumb = &thumbs[j % DEMO_STEP];

                        thumb->item = j;

                        if (j < total)
                        {
                            if ((md = gui_vstack(ld)))
                            {
                                const int ww = MIN(w, h) * 2 / 9;
                                const int hh = ww / 4 * 3;

                                gui_space(md);

                                thumb->shot_id = gui_image(md, " ", ww, hh);
                                thumb->name_id = gui_label(md, " ", GUI_TNY,
                                                           GUI_COLOR_WHT);

                                gui_set_trunc(thumb->name_id, TRUNC_TAIL);
                                gui_set_state(md, DEMO_SELECT, j);

                                thumb->thumb_id = md;
                            }
                        }
                        else
                        {
                            gui_space(ld);

                            thumb->shot_id = 0;
                            thumb->name_id = 0;
                            thumb->thumb_id = 0;
                        }
                    }
                }

        gui_filler(jd);
    }

    return jd;
}

static void gui_demo_update_thumbs(void)
{
    struct dir_item *item;
    struct demo     *demo;
    
    for (int i = 0;
         i < ARRAYSIZE(thumbs) && thumbs[i].shot_id && thumbs[i].name_id;
         i++)
    {
        int stat_limit = config_get_d(CONFIG_ACCOUNT_LOAD);
        int stat_max = 0;

        demo_requires_update = 0;

        item = DIR_ITEM_GET(demo_items, thumbs[i].item);
        demo = (struct demo *) item->data;

        if (demo)
        {
            fs_file fp;

            if ((fp = fs_open_read(item->path)))
            {
                SAFECPY(demo->path, item->path);
                SAFECPY(demo->name, base_name_sans(item->path, str_ends_with(item->path, ".nbrx") ? ".nbrx" : ".nbr"));

                st_demo_version_read(fp, demo);

                fs_close(fp);
            }

            if (demo->status == 3)
                stat_max = 3;
            else if (demo->status == 1 || demo->status == 0)
                stat_max = 2;
            else if (demo->status == 2)
                stat_max = 1;
        }

        gui_set_label(thumbs[i].name_id, demo ? demo->name :
                                                base_name(item->path));
        gui_set_color(thumbs[i].name_id, GUI_COLOR_RED);

        if (demo)
        {
            if (stat_max > stat_limit)
                gui_set_image(thumbs[i].shot_id,
                              stat_max > 2 ?
                              "gui/filters/keep_filters.jpg" :
                              "gui/filters/single_filters.jpg");
            else if (demo_requires_update)
                gui_set_image(thumbs[i].shot_id, "gui/filters/upgrade.jpg");
            else
            {
                gui_set_image(thumbs[i].shot_id, demo ? demo->shot : "");
                gui_set_color(thumbs[i].name_id, GUI_COLOR_WHT);
            }
        }
        else
            gui_set_image(thumbs[i].shot_id, "gui/filters/invalid.jpg");
    }

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
    //int stat_len = 0;
    int jd, kd, ld;
    int s;

    /* Find the longest status string. */

    /*for (status = "", s = GAME_NONE; s < GAME_MAX + 1; s++)
        if (gui_measure(status_to_str(s), GUI_SML).w > stat_len)
            stat_len = gui_measure(status_to_str(s), GUI_SML).w;*/

    for (status = "", s = GAME_NONE; s < GAME_MAX + 1; s++)
        if (text_length(status_to_str(s)) > text_length(status))
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
                status_id = gui_label(ld, status, GUI_SML, GUI_COLOR_RED);

                gui_filler(ld);
            }

            if ((ld = gui_vstack(kd)))
            {
                gui_filler(ld);

                gui_label(ld, _("Time"),   GUI_SML, GUI_COLOR_WHT);
                gui_label(ld, _("Coins"),  GUI_SML, GUI_COLOR_WHT);
                gui_label(ld, _("Status"), GUI_SML, GUI_COLOR_WHT);

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
                
                name_id   = gui_label(ld, "XXXXXXXXXXXXXXXXX", GUI_SML, 0, 0);
                player_id = gui_label(ld, "XXXXXXXXXXXXXXXXX", GUI_SML, 0, 0);
                date_id   = gui_label(ld, "XXXXXXXXXXXXXXXXX", GUI_SML, 0, 0);

                gui_filler(ld);

                gui_set_trunc(name_id,   TRUNC_TAIL);
                gui_set_trunc(player_id, TRUNC_TAIL);
                gui_set_trunc(date_id,   TRUNC_TAIL);

                gui_set_label(name_id,   " ");
                gui_set_label(player_id, " ");
                gui_set_label(date_id,   date_to_str(time(NULL)));
            }

            if ((ld = gui_vstack(kd)))
            {
                gui_filler(ld);

                gui_label(ld, _("Replay"), GUI_SML, GUI_COLOR_WHT);
                gui_label(ld, _("Player"), GUI_SML, GUI_COLOR_WHT);
                gui_label(ld, _("Date"),   GUI_SML, GUI_COLOR_WHT);

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

    if (!DEMO_CHECK_GET(d, demo_items, i < total ? i : 0))
    {
        gui_set_label(name_id,   " ");
        gui_set_label(date_id,   "01.01.2003 00:00:00");
        gui_set_label(player_id, " ");
        gui_set_label(status_id, status_to_str(GAME_MAX));
        gui_set_count(coin_id,   0);
        gui_set_clock(time_id,   0);

        gui_set_color(name_id,   gui_gry, gui_red);
        gui_set_color(date_id,   gui_gry, gui_red);
        gui_set_color(player_id, gui_gry, gui_red);
        gui_set_color(status_id, GUI_COLOR_RED);
        return;
    }

    stat_limit_busy = 1;

    switch (d->status)
    {
    case GAME_GOAL: set_max_game_stat(1); break;
    case GAME_FALL: set_max_game_stat(3); break;
    default:        set_max_game_stat(2);
    }

    time_max_minutes = d->timer / 6000;

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    if (d->mode == MODE_CAMPAIGN)
        time_limit_minutes = 60;
    else
#endif
        time_limit_minutes = 10;

    const int stat_high_limit = get_max_game_stat() > get_limit_game_stat(),
              time_high_limit = time_max_minutes > time_limit_minutes;

    stat_limit_busy = 0;

    gui_set_label(name_id, d->name);
    gui_set_label(date_id, date_to_str(d->date));
    gui_set_label(player_id, d->player);

    if (d->status == GAME_GOAL)
        gui_set_color(status_id, GUI_COLOR_GRN);
    else
        gui_set_color(status_id, GUI_COLOR_RED);

    gui_set_label(status_id, status_to_str(d->status));

    gui_set_count(coin_id, d->coins);
    gui_set_clock(time_id, d->timer);

    /* Make sure, that the level status limit is underneath it. */

    if (stat_high_limit || time_high_limit)
    {
        /* Max level status exceeds limits! */

        gui_set_color(name_id, gui_gry, gui_red);
        gui_set_color(date_id, gui_gry, gui_red);
        gui_set_color(player_id, gui_gry, gui_red);
    }
    else
    {
        gui_set_color(name_id, GUI_COLOR_DEFAULT);
        gui_set_color(date_id, GUI_COLOR_DEFAULT);
        gui_set_color(player_id, GUI_COLOR_DEFAULT);
    }
}

static void demo_select(int demo)
{
    gui_set_hilite(thumbs[selected % DEMO_STEP].thumb_id, 0);

    selected = demo;

    gui_set_hilite(thumbs[selected % DEMO_STEP].thumb_id, 1);

    gui_demo_update_status(demo);
}

/*---------------------------------------------------------------------------*/

int standalone;

static int demo_restricted_gui(void)
{
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
    int id, jd, kd, ld, md;
#else
    int id, jd, kd, ld;
#endif

    if ((id = gui_vstack(0)))
    {
        if ((jd = gui_vstack(id)))
        {
            char infoattr[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(infoattr, MAXSTR,
#else
            sprintf(infoattr,
#endif
                    "%s: %s > %s", _("Player"),
                    reported_player_name, reported_status);

            if (demo_requires_update)
                kd = gui_label(jd, _("Update required!"),
                                      GUI_MED, gui_red, gui_blk);
            else if (time_max_minutes > time_limit_minutes)
                kd = gui_label(jd, _("Too long!"),
                                      GUI_MED, gui_red, gui_blk);
            else
                kd = gui_label(jd, _("Filters restricted!"),
                                      GUI_MED, gui_red, gui_blk);

            gui_pulse(kd, 1.2f);

            if (!standalone && !demo_requires_update)
            {
                ld = gui_label(jd, infoattr, GUI_SML, GUI_COLOR_RED);
                gui_pulse(ld, 1.2f);

#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
                if (config_cheat())
                {
                    md = gui_label(jd, config_get_d(CONFIG_ACCOUNT_LOAD) == 1 ?
                                          _("Only Finish") : _("Keep on board"),
                                          GUI_SML, GUI_COLOR_RED);
                    gui_pulse(md, 1.2f);
                }
#endif
            }
            gui_set_rect(jd, GUI_ALL);
        }

        gui_space(id);

        if (demo_requires_update)
            gui_multi(id, _("Please update your game, before\n"
                            "watch the replay level!"),
                          GUI_SML, GUI_COLOR_WHT);
        else if (time_max_minutes > time_limit_minutes)
        {
            if (time_limit_minutes >= 60)
                gui_multi(id, _("You can't watch more than 60 minutes\n"
                                "in a single level in campaign!"),
                              GUI_SML, GUI_COLOR_WHT);
            else
                gui_multi(id, _("You can't watch more than 10 minutes\n"
                                "in a single level per set!"),
                              GUI_SML, GUI_COLOR_WHT);
        }
        else
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
            gui_multi(id, _("You can't open selected replay, because\n"
                            "it was restricted by the WGCL's system settings!"),
                          GUI_SML, GUI_COLOR_WHT);
#elif NB_HAVE_PB_BOTH==1
            gui_multi(id, _("You can't open selected replay, because\n"
                            "it was restricted by the account settings!"),
                          GUI_SML, GUI_COLOR_WHT);
#else
            gui_multi(id, _("You can't open selected replay, because\n"
                            "it was restricted by the in-game settings!"),
                          GUI_SML, GUI_COLOR_WHT);
#endif

        demo_requires_update = 0;

        gui_layout(id, 0, 0);
    }

    return id;
}

static int demo_restricted_enter(struct state *st, struct state *prev, int intent)
{
    audio_music_fade_out(0.0f);
    audio_play(AUD_UI_SHATTER, 1.0f);
    return transition_slide(demo_restricted_gui(), 1, intent);
}

static int demo_restricted_leave(struct state *st, struct state *next, int id, int intent)
{
    if (next == &st_null)
    {
        demo_dir_free(demo_items);
        demo_items = NULL;

        game_server_free(NULL);
        game_client_free(NULL);
    }

    if (reported_status)
    {
        free(reported_status);
        reported_status = NULL;
    }

    return transition_slide(id, 0, intent);
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
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        if (c == KEY_EXIT && current_platform == PLATFORM_PC)
#else
        if (c == KEY_EXIT)
#endif
        {
            if (is_opened)
                return goto_state(&st_demo_end);
            else
                return exit_state(&st_demo);
        }
    }
    return 1;
}

static int demo_restricted_buttn(int b, int d)
{
    if (d && (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b) ||
              config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b)))
    {
        if (is_opened)
            return goto_state(&st_demo_end);
        else
            return exit_state(&st_demo);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int demo_manual_hotreload = 0;
static int demo_hotreload = 0;

#if ENABLE_MOON_TASKLOADER!=0
static int demo_is_scanning_with_moon_taskloader = 0;

static int demo_scan_moon_taskloader(void *data, void *execute_data)
{
    //while (st_global_animating());

    if (demo_hotreload)
    {
        if (demo_items)
        {
            demo_dir_free(demo_items);
            demo_items = NULL;
        }

        demo_items = demo_dir_scan();
        total = array_len(demo_items);

        demo_hotreload = 0;
    }

    return 1;
}

static void demo_scan_done_moon_taskloader(void *data, void *done_data)
{
    demo_is_scanning_with_moon_taskloader = 0;

    goto_state(curr_state());
}
#endif

static int demo_gui(void)
{
    int id, jd;

#if ENABLE_MOON_TASKLOADER!=0
    if (demo_is_scanning_with_moon_taskloader)
    {
        if ((id = gui_vstack(0)))
        {
            gui_title_header(id, _("Loading..."), GUI_MED, GUI_COLOR_DEFAULT);
            gui_space(id);
            gui_multi(id, _("Scanning NBR replay files..."), GUI_SML, GUI_COLOR_WHT);

            gui_layout(id, 0, 0);

            return id;
        }
        else
            return 0;
    }
#endif

    if (total && availibility)
    {
        if ((id = gui_vstack(0)))
        {
            if ((jd = gui_hstack(id)))
            {
                if (total != availibility)
                {
                    char availibility_header_monitor[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                    sprintf_s(availibility_header_monitor, MAXSTR,
#else
                    sprintf(availibility_header_monitor,
#endif
                            _("Replays unlocked: %d/%d"), availibility, total);

                    int header_id = gui_label(jd, availibility_header_monitor,
                        GUI_SML, 0, 0);

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
                    gui_label(jd, _("Select Replay"), GUI_SML, 0, 0);

                gui_filler(jd);
                gui_space(jd);
                gui_navig(jd, total, first, DEMO_STEP);
            }

            if ((jd = gui_vstack(id)))
            {
                gui_demo_thumbs(jd);

                gui_space(jd);

                gui_demo_status(jd);
            }

            gui_layout(id, 0, 0);

            gui_demo_update_thumbs();
            gui_demo_update_status(last_viewed);

            demo_select(first);
        }
    }
    else if (total && !availibility)
    {
        if ((id = gui_vstack(0)))
        {
            gui_title_header(id, _("All replays locked!"),
                                 GUI_MED, gui_red, gui_blk);
            gui_space(id);
            gui_multi(id, _("Open the file manager to delete\n"
                            "or backup your replays."),
                          GUI_SML, GUI_COLOR_WHT);

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            if (current_platform == PLATFORM_PC && !console_gui_shown())
#endif
            {
                gui_space(id);
                gui_back_button(id);
            }

            gui_layout(id, 0, 0);
        }
    }
    else if (!total && !availibility)
    {
        if ((id = gui_vstack(0)))
        {
            gui_title_header(id, _("No Replays"), GUI_MED, GUI_COLOR_DEFAULT);
            gui_space(id);
            gui_multi(id, _("Your Replays will appear here\n"
                            "once you've recorded."),
                          GUI_SML, GUI_COLOR_WHT);

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            if (current_platform == PLATFORM_PC && !console_gui_shown())
#endif
            {
                gui_space(id);
                gui_back_button(id);
            }

            gui_layout(id, 0, 0);
        }
    }
    else
        return exit_state(&st_title);

    return id;
}

static int demo_enter(struct state *st, struct state *prev, int intent)
{
    is_opened = 0; availibility = 0;
    game_proxy_filter(NULL);

    demo_hotreload = (prev != &st_demo && prev != &st_demo_del) ||
                     demo_manual_hotreload;

    demo_manual_hotreload = 0;

    if (demo_hotreload)
    {
#if ENABLE_MOON_TASKLOADER!=0
        demo_is_scanning_with_moon_taskloader = 1;

        struct moon_taskloader_callback callback = {0};
        callback.execute = demo_scan_moon_taskloader;
        callback.done    = demo_scan_done_moon_taskloader;

        moon_taskloader_load(NULL, callback);
#else
        if (demo_items)
        {
            demo_dir_free(demo_items);
            demo_items = NULL;
        }

        demo_items = demo_dir_scan();
        total = array_len(demo_items);
#endif
    }

#if ENABLE_MOON_TASKLOADER!=0
    if (demo_is_scanning_with_moon_taskloader)
        return transition_slide(demo_gui(), 1, intent);
#endif

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
            struct demo *d;

            if (DEMO_CHECK_GET(d, quarantined_demo_items, quarantined_index))
            {
                stat_limit_busy = 1;

                switch (d->status)
                {
                    case GAME_GOAL: set_max_game_stat(1); break;
                    case GAME_FALL: set_max_game_stat(3); break;
                    default:        set_max_game_stat(2);
                }

                stat_limit_busy = 0;

                if (get_max_game_stat() <= get_limit_game_stat())
                    availibility += 1;
            }

            quarantined_index++;
        }

        demo_dir_free(quarantined_demo_items);
        quarantined_demo_items = NULL;
    }

    if (total)
        demo_dir_load(demo_items, first, last);

    if (prev != &st_demo || demo_hotreload)
    {
#if 0 /* FIXME: Must have more narrator sounds! */
        if (total && availibility == 0)
            audio_narrator_play("snd/lockdown_all.ogg");
        else if (total != availibility)
            audio_narrator_play("snd/lockdown_least.ogg");
#endif
    }

#if NB_HAVE_PB_BOTH==1
    audio_music_fade_to(0.0f, switchball_useable() ? "bgm/title-switchball.ogg" :
                                                     BGM_TITLE_CONF_LANGUAGE, 1);
#else
    audio_music_fade_to(0.0f, "gui/bgm/inter.ogg", 1);
#endif

    if (demo_hotreload)
    {
        demo_hotreload = 0;
        return demo_gui();
    }

    if (prev == &st_demo)
        return transition_page(demo_gui(), 1, intent);

    return transition_slide(demo_gui(), 1, intent);
}

static int demo_leave(struct state *st, struct state *next, int id, int intent)
{
    if (next == &st_title ||
        next == &st_null)
    {
        demo_dir_free(demo_items);
        demo_items = NULL;

        if (next == &st_null)
        {
            game_server_free(NULL);
            game_client_free(NULL);
        }
    }

    if (demo_manual_hotreload)
    {
        gui_delete(id);
        return 0;
    }

    if (next == &st_demo)
        return transition_page(id, 0, intent);

    return transition_slide(id, 0, intent);
}

static void demo_paint(int id, float t)
{
    game_client_draw(0, t);

    gui_paint(id);
#if ENABLE_MOON_TASKLOADER!=0
    if (!demo_is_scanning_with_moon_taskloader)
#endif
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        if (console_gui_shown())
            console_gui_list_paint();
#endif
    ;
}

static void demo_stick(int id, int a, float v, int bump)
{
#if ENABLE_MOON_TASKLOADER!=0
    if (demo_is_scanning_with_moon_taskloader)
        return;
#endif

    int jd = shared_stick_basic(id, a, v, bump);

    if (gui_token(jd) == DEMO_SELECT)
        demo_select(gui_value(jd));
}

static int demo_keybd(int c, int d)
{
#if ENABLE_MOON_TASKLOADER!=0
    if (demo_is_scanning_with_moon_taskloader)
        return 1;
#endif

    if (d)
    {
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        if (c == KEY_EXIT && current_platform == PLATFORM_PC)
#else
        if (c == KEY_EXIT)
#endif
            return demo_action(GUI_BACK, 0);
        if (c == KEY_LOOKAROUND)
        {
            demo_manual_hotreload = 1;
            return goto_state(&st_demo);
        }
    }
    return 1;
}

static int demo_buttn(int b, int d)
{
#if ENABLE_MOON_TASKLOADER!=0
    if (demo_is_scanning_with_moon_taskloader)
        return 1;
#endif

    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
        {
            if (total)
            {
                int token = gui_token(active);
                int value = gui_value(active);

                if (token == DEMO_SELECT && value == selected)
                    return demo_action(DEMO_PLAY, value);
                else
                    return demo_action(token, value);
            }
            else
                return demo_action(GUI_BACK, 0);
        }

        if (total)
        {
            if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L1, b) && first > 0)
                return demo_action(GUI_PREV, 0);
            if (config_tst_d(CONFIG_JOYSTICK_BUTTON_R1, b) &&
                first + DEMO_STEP < total)
                return demo_action(GUI_NEXT, 0);
        }

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return demo_action(GUI_BACK, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

#define DEMO_SET_SPEED(s)         \
    do {                          \
        speed = s;                \
        demo_replay_speed(speed); \
        hud_speed_pulse(speed);   \
    } while (0)

#define DEMO_CHANGE_SPEED(s, d)             \
    do {                                    \
        if (d > 0) speed = SPEED_UP(speed); \
        if (d < 0) speed = SPEED_DN(speed); \
        demo_replay_speed(speed);           \
        hud_speed_pulse(speed);             \
    } while (0)

#define DEMO_UPDATE_SPEED(s, scl)                               \
    do switch (speed) {                                         \
        case SPEED_SLOWESTESTEST: scl = (1.0f / 128.0f); break; \
        case SPEED_SLOWESTESTER:  scl = (1.0f / 64.0f);  break; \
        case SPEED_SLOWESTEST:    scl = (1.0f / 32.0f);  break; \
        case SPEED_SLOWESTER:     scl = (1.0f / 16.0f);  break; \
        case SPEED_SLOWEST:       scl = (1.0f / 8.0f);   break; \
        case SPEED_SLOWER:        scl = (1.0f / 4.0f);   break; \
        case SPEED_SLOW:          scl = (1.0f / 2.0f);   break; \
        case SPEED_FAST:          scl = 2.0f;            break; \
        case SPEED_FASTER:        scl = 4.0f;            break; \
        case SPEED_FASTEST:       scl = 8.0f;            break; \
        case SPEED_FASTESTER:     scl = 16.0f;           break; \
        case SPEED_FASTESTEST:    scl = 32.0f;           break; \
        case SPEED_FASTESTESTER:  scl = 64.0f;           break; \
        case SPEED_FASTESTESTEST: scl = 128.0f;          break; \
        default: scl = 1.0f;                                    \
    } while (0)

static int demo_paused;
static int check_compat;
static int speed;
static int transition;

static int speed_manual;

static float prelude;

static float smoothfix_slowdown_time;

/* Timers needed */
static int demo_timer_curr;
static int demo_timer_last;
static int demo_timer_down = -1;

/* Keyboard inputs */
static int faster;

static int filter_cmd_goal(const union cmd* cmd)
{
    return (cmd ? cmd->type != CMD_SOUND &&
                  cmd->type != CMD_TIMER    : 1);
}

int demo_pause_goto(int paused)
{
    demo_paused = paused && curr_status() == GAME_NONE;

    return goto_state(&st_demo_end);
}

int demo_play_goto(int s)
{
    standalone   = s;
    check_compat = 1;
    is_opened = 1;

    return goto_state(game_compat_map ? &st_demo_play :
                                        &st_demo_compat);
}

static int demo_play_gui(void)
{
    int id;

    if ((id = gui_title_header(0, _("Replay"), GUI_LRG, gui_blu, gui_grn)))
    {
        gui_layout(id, 0, 0);
        gui_pulse(id, 1.2f);
    }

    return id;
}

static int demo_play_enter(struct state *st, struct state *prev, int intent)
{
    smoothfix_slowdown_time = 0;

    video_hide_cursor();

    hud_update(0, 0.0f);

    if (demo_paused ||
        prev == &st_demo_play ||
        prev == &st_demo_look)
    {
        demo_paused = 0;
        prelude = 0;
        audio_music_fade_in(0.5f);
        hud_show(0.0f);
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
        EM_ASM({ Neverball.WGCLshowGameHUD(); });
#endif
        return 0;
    }

    /*
     * Post-1.5.1 replays include view data in the first update, this
     * line is currently left in for compatibility with older replays.
     */
    //game_client_fly(0.0f);

    hud_show(0.9f);

    demo_timer_last = 0;
    demo_timer_down = -1;

    if (get_max_game_stat() > get_limit_game_stat())
    {
        goto_state(&st_demo_restricted);
        return 0;
    }

    prelude = 1.0f;

    speed_manual = 0;

    speed = SPEED_NORMAL;
    demo_replay_speed(speed);
    transition = 0;

    int id = demo_play_gui();
    gui_slide(id, GUI_E | GUI_FLING | GUI_EASE_BACK, 0, 0.5f, 0);
    return id;
}

static int demo_play_leave(struct state *st, struct state *next, int id, int intent)
{
    if (next == &st_null)
    {
        demo_replay_stop(0);

        demo_dir_free(demo_items);
        demo_items = NULL;

        game_server_free(NULL);
        game_client_free(NULL);
    }

    prelude = 0.0f;

    video_show_cursor();

    hud_hide();
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    EM_ASM({ Neverball.WGCLhideGameHUD(); });
#endif

    gui_delete(id);
    return 0;
}

static void demo_play_paint(int id, float t)
{
    game_client_draw(0, t);

    if (!speed_manual &&
        (hud_visibility() || config_get_d(CONFIG_SCREEN_ANIMATIONS)))
        hud_paint();

    if (config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_set_alpha(id, CLAMP(0, 1.5f - time_state(), 1), GUI_ANIMATION_NONE);

    if (!st_global_animating() && id &&
        (time_state() < prelude || config_get_d(CONFIG_SCREEN_ANIMATIONS)))
        gui_paint(id);
}

static void demo_play_timer(int id, float dt)
{
    ST_PLAY_SYNC_SMOOTH_FIX_TIMER(smoothfix_slowdown_time);

    float timescale = 1.0f;
    DEMO_UPDATE_SPEED(speed, timescale);

    if (!game_client_get_jump_b() && !st_global_animating())
        geom_step(speed_manual ? dt * 2 :
                                 (speed == SPEED_NONE ? 0 : dt * timescale));

    game_step_fade(dt);
    hud_update_camera_direction(curr_viewangle());
    gui_timer(id, dt);

    if (!speed_manual || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        hud_timer(dt);

    if (time_state() >= 1.0f && !transition)
    {
        gui_slide(id, GUI_W | GUI_FLING | GUI_EASE_BACK | GUI_BACKWARD, 0, 0.6f, 0);
        transition = 1;
    }

    demo_timer_last = demo_timer_curr;
    demo_timer_curr = curr_clock();

    if (demo_timer_down == -1 && time_state() >= prelude)
        demo_timer_down = demo_timer_last > demo_timer_curr;

    game_proxy_filter(curr_status() != GAME_NONE ? filter_cmd_goal : 0);

    /* Pause briefly before starting playback. */

    if (time_state() < prelude || st_global_animating())
        return;

#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    EM_ASM({ Neverball.WGCLshowGameHUD(); });
#endif

    if (demo_timer_down)
    {
        if      (demo_timer_curr < 1    && speed > SPEED_SLOWESTESTEST)
            DEMO_SET_SPEED(SPEED_SLOWESTESTEST);
        else if (demo_timer_curr < 12   && speed > SPEED_SLOWESTESTER)
            DEMO_SET_SPEED(SPEED_SLOWESTESTER);
        else if (demo_timer_curr < 25   && speed > SPEED_SLOWESTEST)
            DEMO_SET_SPEED(SPEED_SLOWESTEST);
        else if (demo_timer_curr < 50   && speed > SPEED_SLOWESTER)
            DEMO_SET_SPEED(SPEED_SLOWESTER);
        else if (demo_timer_curr < 100  && speed > SPEED_SLOWEST)
            DEMO_SET_SPEED(SPEED_SLOWEST);
        else if (demo_timer_curr < 200  && speed > SPEED_SLOWER)
            DEMO_SET_SPEED(SPEED_SLOWER);
        else if (demo_timer_curr < 500  && speed > SPEED_SLOW)
            DEMO_SET_SPEED(SPEED_SLOW);
        else if (demo_timer_curr < 1000 && speed > SPEED_NORMAL)
        {
            speed_manual = 0;
            DEMO_SET_SPEED(SPEED_NORMAL);
        }
    }

    if (!demo_replay_step(dt) && !st_global_animating())
        demo_pause_goto(0);
    else if (!st_global_animating())
    {
        progress_step();
        game_client_blend(demo_replay_blend());
    }
}

static void demo_play_stick(int id, int a, float v, int bump)
{
    if (!bump || speed_manual)
        return;

    if (config_tst_d(CONFIG_JOYSTICK_AXIS_Y0, a))
    {
        if (v + axis_offset[1] < 0) DEMO_CHANGE_SPEED(speed,  1);
        if (v + axis_offset[1] > 0) DEMO_CHANGE_SPEED(speed, -1);
    }
}

static void demo_play_wheel(int x, int y)
{
    if (y > 0 && !speed_manual) DEMO_CHANGE_SPEED(speed,  1);
    if (y < 0 && !speed_manual) DEMO_CHANGE_SPEED(speed, -1);
}

static int demo_play_click(int b, int d)
{
    if (d && time_state() > prelude)
    {
        if (config_tst_d(CONFIG_MOUSE_CAMERA_R, b))
        {
            demo_replay_manual_speed(2.0f); speed_manual = 1;
            game_proxy_filter(curr_status() != GAME_NONE ? filter_cmd_goal :
                                                           filter_cmd);
            audio_music_fade_out(0.2f);
        }
        else if (b == SDL_BUTTON_LEFT)
            return demo_pause_goto(1);
    }
    else if (time_state() > prelude)
    {
        if (config_tst_d(CONFIG_MOUSE_CAMERA_R, b))
        {
            demo_replay_speed(speed);
            speed_manual = 0;
        }

        game_proxy_filter(curr_status() != GAME_NONE ? filter_cmd_goal : NULL);
        audio_music_fade_in(0.5f);
    }

    return 1;
}

static int demo_play_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT && !speed_manual)
            return demo_pause_goto(1);

        if ((c == KEY_POSE || c == KEY_TOGGLESHOWHUD) && !speed_manual)
            toggle_hud_visibility(!hud_visibility());

        if (c == SDLK_8 && !faster) DEMO_SET_SPEED(SPEED_SLOWESTESTEST);
        if (c == SDLK_7 && !faster) DEMO_SET_SPEED(SPEED_SLOWESTESTER);
        if (c == SDLK_6 && !faster) DEMO_SET_SPEED(SPEED_SLOWESTEST);
        if (c == SDLK_5 && !faster) DEMO_SET_SPEED(SPEED_SLOWESTER);
        if (c == SDLK_4 && !faster) DEMO_SET_SPEED(SPEED_SLOWEST);
        if (c == SDLK_3 && !faster) DEMO_SET_SPEED(SPEED_SLOWER);
        if (c == SDLK_2 && !faster) DEMO_SET_SPEED(SPEED_SLOW);
        if (c == SDLK_1)            DEMO_SET_SPEED(SPEED_NORMAL);
        if (c == SDLK_2 && faster)  DEMO_SET_SPEED(SPEED_FAST);
        if (c == SDLK_3 && faster)  DEMO_SET_SPEED(SPEED_FASTER);
        if (c == SDLK_4 && faster)  DEMO_SET_SPEED(SPEED_FASTEST);
        if (c == SDLK_5 && faster)  DEMO_SET_SPEED(SPEED_FASTESTER);
        if (c == SDLK_6 && faster)  DEMO_SET_SPEED(SPEED_FASTESTEST);
        if (c == SDLK_7 && faster)  DEMO_SET_SPEED(SPEED_FASTESTESTER);
        if (c == SDLK_8 && faster)  DEMO_SET_SPEED(SPEED_FASTESTESTEST);
        if (c == SDLK_LSHIFT)       faster = 1;

        if (c == KEY_LOOKAROUND)
            return goto_state(&st_demo_look);
    }
    else if (c == SDLK_LSHIFT) faster = 0;
    return 1;
}

static int demo_play_buttn(int b, int d)
{
    if (d)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b) ||
            config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
            return demo_pause_goto(1);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

enum
{
    DEMO_CONF = GUI_LAST,
    DEMO_DEL,
    DEMO_QUIT,
    DEMO_REPLAY,
    DEMO_CONTINUE
};

static int demo_end_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case DEMO_QUIT:
            demo_paused = 0;

        case GUI_BACK:
            if (!demo_paused)
            {
                is_opened = 0;
                demo_replay_stop(0);
            }

            return exit_state(demo_paused ? &st_demo_play :
                                           (!standalone ? &st_demo : 0));

        case DEMO_CONF:
            return goto_conf(&st_demo_end, 1, 1);

        case DEMO_DEL:
            demo_paused = 0;
            return goto_state(&st_demo_del);

        case DEMO_REPLAY:
            if (demo_paused)
                demo_paused = 0;

            demo_replay_stop(0);

            if (progress_replay(curr_demo()))
                return exit_state(&st_demo_play);

        case DEMO_CONTINUE:
            return exit_state(&st_demo_play);
    }
    return 1;
}

static int demo_end_gui(void)
{
    int id, jd, kd;

    if ((id = gui_vstack(0)))
    {
        kd = gui_title_header(id, demo_paused ? _("Replay Paused") : _("Replay Ends"),
                                  GUI_LRG, gui_gry, gui_red);

        gui_space(id);
        gui_state(id, _("Options"), GUI_SML, DEMO_CONF, 0);
        gui_space(id);

        if ((jd = gui_harray(id)))
        {
            int btn0_id = 0, btn1_id = 0;

            if (demo_paused || !console_gui_shown())
                gui_state(jd, _("Exit"), GUI_SML, DEMO_QUIT, 0);

            /* Microsoft and Windows Games can do it! */
#ifndef _MSC_VER
            if (!standalone)
                gui_state(kd, _("Delete"), GUI_SML, DEMO_DEL, 0);
#endif

            btn0_id = gui_state(jd, _("Repeat"), GUI_SML, DEMO_REPLAY, 0);

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            if (demo_paused && current_platform == PLATFORM_PC && !console_gui_shown())
#else
            if (demo_paused)
#endif
                btn1_id = gui_start(jd, _("Continue"), GUI_SML, DEMO_CONTINUE, 0);

            /* Only that is limit underneath it */
            if (!(get_max_game_stat() <= get_limit_game_stat() &&
                  allow_exact_versions)) {
                gui_set_color(btn0_id, GUI_COLOR_GRY);
                gui_set_color(btn1_id, GUI_COLOR_GRY);
                gui_set_state(btn0_id, GUI_NONE, 0);
                gui_set_state(btn1_id, GUI_NONE, 0);
            }
        }

        gui_pulse (kd, 1.2f);
        gui_layout(id, 0, 0);
    }

    return id;
}

static int demo_end_enter(struct state *st, struct state *prev, int intent)
{
    speed_manual = 0;
    faster       = 0;

    if (!demo_paused)
        game_proxy_filter(NULL);

    audio_music_fade_out(demo_paused ? 0.2f : 2.0f);

    if (demo_paused && prev == &st_demo_play)
        audio_play("snd/2.2/game_pause.ogg", 1.0f);

    hud_hide();
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    EM_ASM({ Neverball.WGCLhideGameHUD(); });
#endif

    return transition_slide(demo_end_gui(), 1, intent);
}

static void demo_end_paint(int id, float t)
{
    game_client_draw(0, t);

    gui_paint(id);

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    if (console_gui_shown())
    {
        if (demo_paused)
            console_gui_paused_paint();
        else
            console_gui_replay_eof_paint();
    }
#endif
    if (hud_visibility() || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        hud_paint();
}

static void demo_end_timer(int id, float dt)
{
    game_step_fade(dt);
    gui_timer(id, dt);
    hud_timer(dt);

    hud_update(config_get_d(CONFIG_SCREEN_ANIMATIONS), dt);
}

static int demo_end_keybd(int c, int d)
{
    if (d && c == KEY_EXIT)
        return demo_end_action(!demo_paused || allow_exact_versions ?
                               GUI_BACK : GUI_NONE, 0);

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
                return demo_end_action(GUI_BACK, 0);
        }
        else if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return demo_end_action(GUI_BACK, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int demo_del_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    if (st_global_animating() || tok == GUI_NONE)
        return 1;

    switch (tok)
    {
        case DEMO_DEL:  demo_replay_stop(1);
        case DEMO_QUIT: return exit_state(&st_demo);
    }

    return 1;
}

static int demo_del_gui(void)
{
    int id, jd, kd;

    if ((id = gui_vstack(0)))
    {
        kd = gui_title_header(id, _("Delete Replay?"), GUI_MED, GUI_COLOR_RED);
        gui_space(id);

        if ((jd = gui_vstack(id)))
        {
            if (!allow_exact_versions && get_max_game_stat() > get_limit_game_stat())
                gui_multi(jd,
                          _("The current replay with mismatched or\n"
                            "unknown level version including exceeded\n"
                            "level status limit will being deleted\n"
                            "from the user data."),
                          GUI_SML, GUI_COLOR_WHT);
            else if (!allow_exact_versions)
                gui_multi(jd,
                          _("The current replay with mismatched or\n"
                            "unknown level version will being deleted\n"
                            "from the user data."),
                          GUI_SML, GUI_COLOR_WHT);
            else if (get_max_game_stat() > get_limit_game_stat())
                gui_multi(jd,
                          _("The current replay with exceeded\n"
                            "level status limit will being deleted\n"
                            "from the user data."),
                          GUI_SML, GUI_COLOR_WHT);

            char warning_text[MAXSTR];

            SAFECPY(warning_text, _("Once deleted this replay,\n"
                                    "this action cannot be undone."));

            gui_multi(jd,
                      warning_text,
                      GUI_SML,
                      !allow_exact_versions || get_max_game_stat() > get_limit_game_stat() ? gui_red : gui_wht,
                      !allow_exact_versions || get_max_game_stat() > get_limit_game_stat() ? gui_red : gui_wht);

            gui_set_rect(jd, GUI_ALL);
        }

        gui_space(id);

        if ((jd = gui_harray(id)))
        {
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            if (current_platform == PLATFORM_PC)
            {
#endif
                if (get_max_game_stat() <= get_limit_game_stat() && allow_exact_versions)
                {
                    gui_start(jd, _("Keep"), GUI_SML, DEMO_QUIT, 0);
                    gui_state(jd, _("Delete"), GUI_SML, DEMO_DEL, 0);
                }
                else
                {
                    gui_label(jd, _("Keep"), GUI_SML, GUI_COLOR_GRY);
                    gui_start(jd, _("Delete"), GUI_SML, DEMO_DEL, 0);
                }
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
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

static int demo_del_enter(struct state *st, struct state *prev, int intent)
{
    audio_music_fade_out(demo_paused ? 0.2f : 1.0f);

    audio_play(AUD_WARNING, 1.0f);

    return transition_slide(demo_del_gui(), 1, intent);
}

static int demo_del_keybd(int c, int d)
{
    if (d && c == KEY_EXIT)
    {
        if (!allow_exact_versions || get_max_game_stat() > get_limit_game_stat())
            audio_play(AUD_DISABLED, 1.0f);
        else return demo_del_action(DEMO_QUIT, 0);
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
        {
            if (!allow_exact_versions || get_max_game_stat() > get_limit_game_stat())
                audio_play(AUD_DISABLED, 1.0f);
            else return demo_del_action(DEMO_QUIT, 0);
        }
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int demo_compat_gui(void)
{
    int id;

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("Warning!"), GUI_MED, GUI_COLOR_RED);
        gui_space(id);

        gui_multi(id, _("The current replay was recorded with a\n"
                        "different (or unknown) version of this level.\n"
                        "Be prepared to encounter visual errors.\n"),
                  GUI_SML, GUI_COLOR_WHT);

        gui_layout(id, 0, 0);
    }

    return id;
}

static int demo_compat_enter(struct state *st, struct state *prev, int intent)
{
    audio_play(AUD_WARNING, 1.0f);

    check_compat         = 0;
    allow_exact_versions = 0;

    return transition_slide(demo_compat_gui(), 1, intent);
}

static void demo_compat_timer(int id, float dt)
{
    game_step_fade(dt);
    gui_timer(id, dt);
}

static int demo_compat_keybd(int c, int d)
{
    if (d && c == KEY_EXIT)
        return demo_pause_goto(0);

    return 1;
}

static int demo_compat_buttn(int b, int d)
{
    if (d)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return goto_state(&st_demo_play);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return demo_pause_goto(0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static float phi, theta;

static int   demo_look_panning;
static float demo_look_stick_x[2],
             demo_look_stick_y[2],
             demo_look_stick_z;

static int demo_look_enter(struct state *st, struct state *prev, int intent)
{
    demo_look_stick_x[0] = 0;
    demo_look_stick_y[0] = 0;
    demo_look_stick_x[1] = 0;
    demo_look_stick_y[1] = 0;
    demo_look_stick_z = 0;
    phi = 0;
    theta = 0;
    return 0;
}

static int demo_look_leave(struct state *st, struct state *next, int id, int intent)
{
    if (next == &st_null)
    {
        demo_replay_stop(0);

        demo_dir_free(demo_items);
        demo_items = NULL;

        game_server_free(NULL);
        game_client_free(NULL);
    }

    return 0;
}

static void demo_look_timer(int id, float dt)
{
    theta += demo_look_stick_x[1] *  2 * dt;
    phi   += demo_look_stick_y[1] * -2 * dt;

    if (phi > +90.0f)    phi    = +90.0f;
    if (phi < -90.0f)    phi    = -90.0f;

    if (theta > +180.0f) theta -= 360.0f;
    if (theta < -180.0f) theta += 360.0f;

    float look_moves[2] = {
        (fcosf((V_PI * theta) / 180) * +demo_look_stick_x[0]) +
        (fsinf((V_PI * theta) / 180) * -demo_look_stick_y[0]),
        (fcosf((V_PI * theta) / 180) * +demo_look_stick_y[0]) +
        (fsinf((V_PI * theta) / 180) * +demo_look_stick_x[0]) };

    game_look_v2(look_moves[0]     * (dt * 5),
                 demo_look_stick_z * (dt * 5),
                 look_moves[1]     * (dt * 5),
                 phi, theta);
}

static void demo_look_paint(int id, float t)
{
    game_client_draw(0, t);
}

static void demo_look_point(int id, int x, int y, int dx, int dy)
{
    if (demo_look_panning)
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

static void demo_look_stick(int id, int a, float v, int bump)
{
    if (config_tst_d(CONFIG_JOYSTICK_AXIS_X0, a))
        demo_look_stick_x[0] = v;
    if (config_tst_d(CONFIG_JOYSTICK_AXIS_Y0, a))
        demo_look_stick_y[0] = v;
    if (config_tst_d(CONFIG_JOYSTICK_AXIS_X1, a))
        demo_look_stick_x[1] = v;
    if (config_tst_d(CONFIG_JOYSTICK_AXIS_Y1, a))
        demo_look_stick_y[1] = v;
}

static int demo_look_click(int b, int d)
{
    if (d && config_tst_d(CONFIG_MOUSE_CAMERA_R, b))
        demo_look_panning = 1;
    else if (config_tst_d(CONFIG_MOUSE_CAMERA_R, b))
        demo_look_panning = 0;

    return 1;
}

static int demo_look_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT || c == KEY_LOOKAROUND)
            return exit_state(&st_demo_play);
        if (c == SDLK_LSHIFT)
            demo_look_stick_z = -1;
        if (c == SDLK_SPACE)
            demo_look_stick_z = 1;
    }
    else if (!d)
    {
        if (c == SDLK_LSHIFT || c == SDLK_SPACE)
            demo_look_stick_z = 0;
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

struct state st_demo_restricted = {
    demo_restricted_enter,
    demo_restricted_leave,
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
    shared_timer,
    shared_point,
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
    demo_play_click,
    demo_play_keybd,
    demo_play_buttn,
    demo_play_wheel,
    NULL,
    demo_shared_fade
};

struct state st_demo_end = {
    demo_end_enter,
    demo_leave,
    demo_end_paint,
    demo_end_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    demo_end_keybd,
    demo_end_buttn
};

struct state st_demo_del = {
    demo_del_enter,
    demo_leave,
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
    demo_leave,
    shared_paint,
    demo_compat_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click_basic,
    demo_compat_keybd,
    demo_compat_buttn
};

struct state st_demo_look = {
    demo_look_enter,
    demo_look_leave,
    demo_look_paint,
    demo_look_timer,
    demo_look_point,
    demo_look_stick,
    NULL,
    demo_look_click,
    demo_look_keybd,
    demo_look_buttn
};
