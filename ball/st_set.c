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

#include "networking.h"
#include "account.h"
#include "campaign.h" // New: Campaign
#endif

#include "gui.h"
#include "demo.h"
#include "set.h"
#include "progress.h"
#include "audio.h"
#include "config.h"
#include "util.h"
#include "common.h"

#include "game_common.h"
#include "game_draw.h"

#include "st_malfunction.h"
#include "st_name.h"
#include "st_level.h"
#include "st_set.h"
#include "st_title.h"
#include "st_start.h"
#include "st_shared.h"

#include "st_playmodes.h"
#include "st_play.h"

#include <assert.h>

#if NB_STEAM_API==0 && NB_EOS_SDK==0
#define SET_ALWAYS_UNLOCKED
#endif

/*---------------------------------------------------------------------------*/

struct state st_levelgroup;

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

/*---------------------------------------------------------------------------*/

#define SET_STEP 7

static int set_manual_hotreload = 0;

static int total = 0;
static int first = 0;

static int shot_id;
static int desc_id;

static int do_init = 1;

static int boost_id;
static int boost_on;

#ifndef FS_VERSION_1
static int *download_ids = NULL;
static int *name_ids = NULL;
#endif

int is_boost_on(void)
{
    return boost_on == 1;
}

void set_boost_on(int active)
{
    boost_on = active;
}

enum
{
    SET_SELECT = GUI_LAST,
    SET_TOGGLE_BOOST,
    SET_XBOX_LB,
    SET_XBOX_RB,
    SET_GET_MORE
};

#ifndef FS_VERSION_1
struct set_download_info
{
    char *set_file;
    char label[32];
};

static struct set_download_info *create_set_download_info(const char *set_file)
{
    struct set_download_info *dli = calloc(sizeof (*dli), 1);

    if (dli)
        dli->set_file = strdup(set_file);

    return dli;
}

static void free_set_download_info(struct set_download_info *dli)
{
    if (dli)
    {
        if (dli->set_file)
        {
            free(dli->set_file);
            dli->set_file = NULL;
        }

        free(dli);
        dli = NULL;
    }
}

static void set_download_progress(void *data1, void *data2)
{
    struct set_download_info *dli = (struct set_download_info *) data1;
    struct fetch_progress *pr = (struct fetch_progress *) data2;

    if (dli)
    {
        /* Sets may change places, so we can't keep set index around. */
        int set_index = set_find(dli->set_file);

        if (download_ids && set_index >= 0 && set_index < total)
        {
            int id = download_ids[set_index];

            if (id)
            {
                char label[32] = GUI_ELLIPSIS;

                if (pr->total > 0)
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                    sprintf_s(label, dstSize, "%3d%%", (int)(pr->now * 100.0 / pr->total) % 1000);
#else
                    sprintf(label, "%3d%%", (int)(pr->now * 100.0 / pr->total) % 1000);
#endif

                /* Compare against current label so we're not calling GL constantly. */
                /* TODO: just do this in gui_set_label. */

                if (strcmp(label, dli->label) != 0)
                {
                    SAFECPY(dli->label, label);
                    gui_set_label(id, label);
                }
            }
        }
    }
}

static void set_download_done(void *data1, void *data2)
{
    struct set_download_info *dli = (struct set_download_info *) data1;
    struct fetch_done *dn = (struct fetch_done *) data2;

    if (dli)
    {
        int set_index = set_find(dli->set_file);

        if (download_ids && set_index >= 0 && set_index < total)
        {
            int id = download_ids[set_index];

            if (id)
            {
                if (dn->finished)
                {
                    gui_remove(id);

                    download_ids[set_index] = 0;

                    if (name_ids && name_ids[set_index])
                    {
                        gui_set_label(name_ids[set_index], set_name(set_index));
                        gui_pulse(name_ids[set_index], 1.2f);
                    }
                }
                else
                {
                    gui_set_label(id, "!");
                    gui_set_color(id, gui_red, gui_red);
                }
            }
        }

        free_set_download_info(dli);
        dli = NULL;
    }
}
#endif

static int set_action(int tok, int val)
{
    GAMEPAD_GAMEMENU_ACTION_SCROLL(SET_XBOX_LB, SET_XBOX_RB, SET_STEP);

#ifdef CONFIG_INCLUDES_ACCOUNT
    int set_name_locked = (account_get_d(ACCOUNT_SET_UNLOCKS) <= val
#if NB_STEAM_API == 0 && NB_EOS_SDK == 0
        && !config_cheat()
#endif
        ) && !is_boost_on() &&
        (server_policy_get_d(SERVER_POLICY_EDITION) < SET_UNLOCKABLE_EDITION &&
            server_policy_get_d(SERVER_POLICY_EDITION) != -1);
#else
    int set_name_locked = 0;
#endif

    switch (tok)
    {
    case SET_XBOX_LB:
        if (first > 1) {
            first -= SET_STEP;
            do_init = 0;
            return goto_state_full(&st_set, GUI_ANIMATION_E_CURVE, GUI_ANIMATION_W_CURVE, 0);
        }
        break;

    case SET_XBOX_RB:
        if (first + SET_STEP < total)
        {
            first += SET_STEP;
            do_init = 0;
            return goto_state_full(&st_set, GUI_ANIMATION_W_CURVE, GUI_ANIMATION_E_CURVE, 0);
        }
        break;

    case GUI_BACK:
        set_quit();
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        if (server_policy_get_d(SERVER_POLICY_LEVELGROUP_ONLY_LEVELSET))
            return goto_state(&st_title);
        else
            return goto_state_full(&st_levelgroup, GUI_ANIMATION_E_CURVE, GUI_ANIMATION_W_CURVE, 0);
#else
        return goto_state(&st_title);
#endif
        break;

    case GUI_PREV:
        first -= SET_STEP;

        do_init = 0;
        return goto_state_full(&st_set, GUI_ANIMATION_E_CURVE, GUI_ANIMATION_W_CURVE, 0);

        break;

    case GUI_NEXT:
        first += SET_STEP;

        do_init = 0;
        return goto_state_full(&st_set, GUI_ANIMATION_W_CURVE, GUI_ANIMATION_E_CURVE, 0);

        break;

    case SET_SELECT:
        if (set_name_locked) return 1;

#ifndef FS_VERSION_1
        if (set_is_installed(val))
        {
#endif
            set_goto(val);
            return goto_state_full(&st_start, GUI_ANIMATION_N_CURVE, GUI_ANIMATION_S_CURVE, 0);
#ifndef FS_VERSION_1
        }
        else if (set_is_downloadable(val))
        {
            struct fetch_callback callback = { 0 };

            callback.progress = set_download_progress;
            callback.done = set_download_done;
            callback.data = create_set_download_info(set_file(val));

            if (!set_download(val, callback))
            {
                free_set_download_info(callback.data);
                callback.data = NULL;
            }
            else
            {
                if (download_ids && download_ids[val])
                {
                    gui_set_label(download_ids[val], GUI_ELLIPSIS);
                    gui_set_color(download_ids[val], gui_grn, gui_grn);
                }
            }

            return 1;
        }
        else if (set_is_downloading(val))
        {
            return 1;
        }
#endif
        break;

    case SET_TOGGLE_BOOST:
        boost_on = !boost_on;
        return goto_state_full(&st_set, boost_on ? GUI_ANIMATION_S_CURVE : GUI_ANIMATION_N_CURVE, boost_on ? GUI_ANIMATION_N_CURVE : GUI_ANIMATION_S_CURVE, 0);
        break;

    case SET_GET_MORE:
#if NB_STEAM_API==1
#elif NB_EOS_SDK==1
#else
#if _WIN32
        system("start msedge https://drive.google.com/drive/folders/1YJzHckEBn15rNemvFy56Ig5skY3KX3Rp");
#elif __APPLE__
        system("open https://drive.google.com/drive/folders/1YJzHckEBn15rNemvFy56Ig5skY3KX3Rp");
#else
        system("x-www-browser https://drive.google.com/drive/folders/1YJzHckEBn15rNemvFy56Ig5skY3KX3Rp");
#endif
#endif
        break;
    }

    return 1;
}

#ifndef FS_VERSION_1
static void gui_set_download(int id, int i)
{
    int jd;

#ifdef CONFIG_INCLUDES_ACCOUNT
    int set_name_locked = account_get_d(ACCOUNT_SET_UNLOCKS) <= i && !is_boost_on()
#if NB_STEAM_API == 0 && NB_EOS_SDK == 0
        && !config_cheat()
#endif
        && (server_policy_get_d(SERVER_POLICY_EDITION) < SET_UNLOCKABLE_EDITION &&
            server_policy_get_d(SERVER_POLICY_EDITION) != -1);
#else
    int set_name_locked = 0xfffffff;
#endif

    if ((jd = gui_hstack(id)))
    {
        /* Create an illusion of center alignment. */
        char *label = concat_string("         ", set_name(i), NULL);

        int button_id, name_id;

        button_id = gui_label(jd, "100%", GUI_SML, gui_grn, gui_grn);

        if (set_is_downloading(i))
            gui_set_label(button_id, GUI_ELLIPSIS);
        else
            gui_set_label(button_id, GUI_ARROW_DN);

        if (download_ids)
            download_ids[i] = button_id;
        
        name_id = gui_label(jd, "TTTTTTTTTTTTTT", GUI_SML, set_name_locked ? gui_gry : gui_wht, set_name_locked ? gui_gry : gui_wht);

        gui_set_trunc(name_id, TRUNC_TAIL);
        gui_set_label(name_id, set_name_locked ? _("Locked") : label);
        gui_set_fill(name_id);

        if (name_ids)
            name_ids[i] = name_id;

        gui_set_state(jd, SET_SELECT, i);
        gui_set_rect(jd, GUI_ALL);

        free(label);
    }
}
#endif

static void gui_set(int id, int i)
{
    int set_text_name_id;
#ifdef CONFIG_INCLUDES_ACCOUNT
    int set_name_locked = (account_get_d(ACCOUNT_SET_UNLOCKS) <= i
#if NB_STEAM_API == 0 && NB_EOS_SDK == 0
        && !config_cheat()
#endif
        ) && !is_boost_on() &&
        (server_policy_get_d(SERVER_POLICY_EDITION) < SET_UNLOCKABLE_EDITION &&
            server_policy_get_d(SERVER_POLICY_EDITION) != -1);
#else
    int set_name_locked = 0;
#endif

    if (set_exists(i))
    {
#ifndef FS_VERSION_1
        if (set_is_downloadable(i) || set_is_downloading(i))
        {
            gui_set_download(id, i);
        }
        else
        {
#endif
            if (i % SET_STEP == 0)
            {
                if (gui_measure(set_name(i), GUI_SML).w > config_get_d(CONFIG_WIDTH) / 2.3f)
                {
                    set_text_name_id = gui_label(id, "TTTTTTTTTTTTTTTTTTTTTT", GUI_SML, gui_wht, gui_wht);
                    gui_set_trunc(set_text_name_id, TRUNC_TAIL);
                    gui_set_state(set_text_name_id, SET_SELECT, i);
                    gui_set_label(set_text_name_id, set_name(i));

                    if (set_name_locked)
                    {
                        gui_set_trunc(set_text_name_id, TRUNC_TAIL);
                        gui_set_label(set_text_name_id, _("Locked"));
                        gui_set_color(set_text_name_id, gui_gry, gui_gry);
                    }
                }
                else
                {
                    set_text_name_id = gui_start(id, "TTTTTTTTTTTTTTTTTTTTTT", GUI_SML, SET_SELECT, i);
                    gui_set_label(set_text_name_id, set_name(i));

                    if (set_name_locked)
                    {
                        gui_set_trunc(set_text_name_id, TRUNC_TAIL);
                        gui_set_label(set_text_name_id, _("Locked"));
                        gui_set_color(set_text_name_id, gui_gry, gui_gry);
                    }
                }
            }
            else
            {
                if (gui_measure(set_name(i), GUI_SML).w > config_get_d(CONFIG_WIDTH) / 2.3f)
                {
                    set_text_name_id = gui_label(id, "TTTTTTTTTTTTTTTTTTTTTT", GUI_SML, gui_wht, gui_wht);
                    gui_set_state(set_text_name_id, SET_SELECT, i);
                    gui_set_trunc(set_text_name_id, TRUNC_TAIL);
                    gui_set_label(set_text_name_id, set_name(i));

                    if (set_name_locked)
                    {
                        gui_set_trunc(set_text_name_id, TRUNC_TAIL);
                        gui_set_label(set_text_name_id, _("Locked"));
                        gui_set_color(set_text_name_id, gui_gry, gui_gry);
                    }
                }
                else
                {
                    set_text_name_id = gui_state(id, "TTTTTTTTTTTTTTTTTTTTTT", GUI_SML, SET_SELECT, i);
                    gui_set_label(set_text_name_id, set_name(i));

                    if (set_name_locked)
                    {
                        gui_set_trunc(set_text_name_id, TRUNC_TAIL);
                        gui_set_label(set_text_name_id, _("Locked"));
                        gui_set_color(set_text_name_id, gui_gry, gui_gry);
                    }
                }
            }
#ifndef FS_VERSION_1
        }
#endif
    }
    else
        gui_label(id, "", GUI_SML, 0, 0);
}

static int set_gui(void)
{
    int w = video.device_w;
    int h = video.device_h;

    int id, jd, kd;

    int i = 0;

    if (total == 0)
    {
        if ((id = gui_vstack(0)))
        {
            gui_label(id, _("No Level Sets"), GUI_MED, gui_yel, gui_red);
            gui_space(id);
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
            if (current_platform == PLATFORM_PC)
#endif
                gui_start(id, _("Back"), GUI_SML, GUI_BACK, 0);

#if NB_HAVE_PB_BOTH==1
            if (server_policy_get_d(SERVER_POLICY_EDITION) >= 0)
            {
                if (boost_on)
                    gui_state(id, _("Revert to standard"), GUI_SML, SET_TOGGLE_BOOST, 0);
                else
                {
#if NB_STEAM_API==1
                    gui_state(id, _("Get Level Sets from Steam Workshop!"), GUI_SML, SET_GET_MORE, 0);
#elif NB_EOS_SDK==1
                    gui_state(id, _("Get Level Sets from Epic Games Store!"), GUI_SML, SET_GET_MORE, 0);
#else
                    gui_state(id, _("Get Level Sets from Website!"), GUI_SML, SET_GET_MORE, 0);
#endif
                }
            }
#endif

            gui_layout(id, 0, 0);
        }
        return id;
    }

    if ((id = gui_vstack(0)))
    {
        if (video.aspect_ratio >= 1.0f)
        {
            if ((jd = gui_hstack(id)))
            {
#ifdef CONFIG_INCLUDES_ACCOUNT
                if (account_get_d(ACCOUNT_PRODUCT_LEVELS) == 1 && server_policy_get_d(SERVER_POLICY_EDITION) > -1) {
                    boost_id = gui_state(jd, _("Boost Rush"), GUI_SML, SET_TOGGLE_BOOST, 0);
                    gui_set_hilite(boost_id, is_boost_on());

                    gui_space(jd);
#if NB_STEAM_API==1
                    gui_state(jd, _("Workshop"), GUI_SML, SET_GET_MORE, 0);
#else
                    gui_state(jd, _("Get more!"), GUI_SML, SET_GET_MORE, 0);
#endif
                    gui_space(jd);
                }
                else
#endif
                {
                    gui_label(jd, _("Level Set"), GUI_SML, gui_yel, gui_red);
                    gui_space(jd);
                }

                gui_filler(jd);
                gui_navig(jd, total, first, SET_STEP);
            }
        }

        if (video.aspect_ratio < 1.0f)
        {
            gui_navig(id, total, first, SET_STEP);
            gui_space(id);
#ifdef CONFIG_INCLUDES_ACCOUNT
            if ((jd = gui_hstack(id)))
            {
                if (account_get_d(ACCOUNT_PRODUCT_LEVELS) == 1 && server_policy_get_d(SERVER_POLICY_EDITION) > 0) {
                    gui_filler(jd);
                    boost_id = gui_state(jd, _("Boost Rush"), GUI_SML, SET_TOGGLE_BOOST, 0);
                    gui_set_hilite(boost_id, is_boost_on());

                    gui_space(jd);
#if NB_STEAM_API==1
                    gui_state(jd, _("Workshop"), GUI_SML, SET_GET_MORE, 0);
#else
                    gui_state(jd, _("Get more!"), GUI_SML, SET_GET_MORE, 0);
#endif
                    gui_filler(jd);
                }
            }
#endif
        }

        gui_space(id);

        if ((jd = gui_harray(id)))
        {
            if (video.aspect_ratio >= 1.0f)
            {
                shot_id = gui_image(jd, set_shot(first), 7 * w / 16, 7 * h / 16);

#if NB_HAVE_PB_BOTH==1
                if ((account_get_d(ACCOUNT_SET_UNLOCKS) <= i
#if NB_STEAM_API == 0 && NB_EOS_SDK == 0
                    && !config_cheat()
#endif
                    ) && !is_boost_on() &&
                    (server_policy_get_d(SERVER_POLICY_EDITION) < SET_UNLOCKABLE_EDITION &&
                        server_policy_get_d(SERVER_POLICY_EDITION) != -1))
                    gui_set_image(shot_id, "gui/campaign/locked.jpg");
#endif
            }

            if ((kd = gui_varray(jd)))
            {
                for (i = first; i < first + SET_STEP; i++)
                    gui_set(kd, i);
            }
        }

        if (video.aspect_ratio >= 1.0f)
        {
            gui_space(id);
            desc_id = gui_multi(id, " \\ \\ \\ \\ \\", GUI_SML, gui_yel, gui_wht);
        }

        gui_layout(id, 0, 0);
    }

#ifndef FS_VERSION_1
    /* --- AUTODOWNLOAD LEVELSET --- */

    /* NOTE: It can be download via unlocked level set. */

    for (int autodwnl_id = 0; autodwnl_id < total; autodwnl_id++)
    {
        if (set_is_installed(autodwnl_id))
        {
            gui_set_color(download_ids[autodwnl_id], gui_wht, gui_grn);
        }
        
        if (set_is_downloadable(autodwnl_id) && !set_is_downloading(autodwnl_id) && !set_is_installed(autodwnl_id))
        {
            struct fetch_callback callback = { 0 };

            callback.progress = set_download_progress;
            callback.done = set_download_done;
            callback.data = create_set_download_info(set_file(autodwnl_id));

            if (!set_download(autodwnl_id, callback))
            {
                free_set_download_info(callback.data);
                callback.data = NULL;
            }
            else
            {
                if (download_ids && download_ids[autodwnl_id])
                {
                    gui_set_label(download_ids[autodwnl_id], GUI_ELLIPSIS);
                    gui_set_color(download_ids[autodwnl_id], gui_yel, gui_yel);
                }
            }
        }
        
        if (set_is_downloadable(autodwnl_id) == PACKAGE_ERROR && !set_is_installed(autodwnl_id))
        {
            if (download_ids && download_ids[autodwnl_id])
            {
                gui_set_label(download_ids[autodwnl_id], GUI_ELLIPSIS);
                gui_set_color(download_ids[autodwnl_id], gui_red, gui_red);
            }
        }
    }

    /* --- END AUTODOWNLOAD LEVELSET --- */
#endif

    return id;
}

static int set_enter(struct state *st, struct state *prev)
{
    if (do_init || set_manual_hotreload)
    {
        if (set_manual_hotreload)
            first = 0;

        total = set_init(boost_on);
        first = MIN(first, (total - 1) - ((total - 1) % SET_STEP));

        audio_music_fade_to(0.5f, "bgm/inter.ogg");

        if (total != 0)
        {
            if (prev != &st_set || set_manual_hotreload)
                audio_narrator_play(AUD_START);
        }

        set_manual_hotreload = 0;
    }
    else do_init = 1;

#ifndef FS_VERSION_1
    if (download_ids)
    {
        free(download_ids);
        download_ids = NULL;
    }

    download_ids = (int *) calloc(total, sizeof (*download_ids));

    if (name_ids)
    {
        free(name_ids);
        name_ids = NULL;
    }

    name_ids = (int *) calloc(total, sizeof (*name_ids));
#endif

    return set_gui();
}

void set_leave(struct state *st, struct state *next, int id)
{
    if (next == &st_set || next == &st_start
#if NB_HAVE_PB_BOTH==1
        || next == &st_start_compat
#endif
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        || next == &st_levelgroup
#endif
        )
        do_init = 1;

#ifndef FS_VERSION_1
    if (download_ids)
    {
        free(download_ids);
        download_ids = NULL;
    }

    if (name_ids)
    {
        free(name_ids);
        name_ids = NULL;
    }
#endif

    gui_delete(id);
}

static void set_paint(int id, float t)
{
    game_client_draw(0, t);

    gui_paint(id);
#ifndef __EMSCRIPTEN__
    if (xbox_show_gui())
        xbox_control_list_gui_paint();
#endif
}

static void set_over(int i)
{
    if (video.aspect_ratio >= 1.0f)
    {
#if NB_HAVE_PB_BOTH==1
        if ((account_get_d(ACCOUNT_SET_UNLOCKS) <= i
#if NB_STEAM_API == 0 && NB_EOS_SDK == 0
            && !config_cheat()
#endif
            ) && !is_boost_on() &&
            (server_policy_get_d(SERVER_POLICY_EDITION) < SET_UNLOCKABLE_EDITION &&
                server_policy_get_d(SERVER_POLICY_EDITION) != -1))
        {
            gui_set_image(shot_id, "gui/campaign/locked.jpg");
            gui_set_multi(desc_id, _("Complete previous set to unlock"));
        }
        else
#endif
        {
            gui_set_image(shot_id, set_shot(i));
            gui_set_multi(desc_id, set_desc(i));
        }
    }
}

static void set_point(int id, int x, int y, int dx, int dy)
{
#ifndef __EMSCRIPTEN__
    xbox_toggle_gui(0);
#endif
    int jd = shared_point_basic(id, x, y);

    if (jd && gui_token(jd) == SET_SELECT)
        set_over(gui_value(jd));
}

static void set_stick(int id, int a, float v, int bump)
{
#ifndef __EMSCRIPTEN__
    xbox_toggle_gui(1);
#endif
    int jd = shared_stick_basic(id, a, v, bump);

    if (jd && gui_token(jd) == SET_SELECT)
        set_over(gui_value(jd));
}

static int set_keybd(int c, int d)
{
    if (d)
    {
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
        if (c == KEY_EXIT && current_platform == PLATFORM_PC)
            return set_action(GUI_BACK, 0);
#else
        if (c == KEY_EXIT)
            return set_action(GUI_BACK, 0);
#endif
        if (c == KEY_LOOKAROUND)
        {
            set_manual_hotreload = 1;
            return goto_state(&st_set);
        }
    }
    return 1;
}

static int set_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return set_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return set_action(GUI_BACK, 0);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L1, b) && first > 0)
            return set_action(SET_XBOX_LB, 0);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_R1, b) && first + SET_STEP < total)
            return set_action(SET_XBOX_RB, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
#ifndef MAPC_INCLUDES_CHKP
#error Campaign is added, but requires Checkpoints!
#endif

int campaign_theme_unlocks = 1;
int campaign_theme_index = 0;

int campaign_theme_image_id;
int campaign_theme_text_id;
int campaign_theme_btn_id;

struct campaign_ranker
{
    const char *img_rank; const char *text_rank;
} campaign_ranks[] = {
    { "gui/ranks/rank_cadet.png", N_("NB Cadet") },
    { "gui/ranks/rank_bronze.png", N_("Bronze Commander") },
    { "gui/ranks/rank_silver.png", N_("Silver Commander") },
    { "gui/ranks/rank_gold.png", N_("Gold Commander") },
    { "gui/ranks/rank_elite.png", N_("Elite Commander") },
};

int campaign_level_unlocks[] = {
    0,
    0,
    0,
    0,
    0
};

static const char campaign_theme_texts[][12] =
{
    N_("Sky World"),
    N_("Ice World"),
    N_("Cave World"),
    N_("Cloud World"),
    N_("Lava World")
};

static const char campaign_theme_images[][23] =
{
    "gui/campaign/sky.jpg",
    "gui/campaign/ice.jpg",
    "gui/campaign/cave.jpg",
    "gui/campaign/cloud.jpg",
    "gui/campaign/lava.jpg"
};

enum
{
    CAMPAIGN_SELECT_THEME = GUI_LAST,
    CAMPAIGN_SELECT_LEVEL
};

static char *campaign_label_clock(int timer)
{
    char timeclock[MAXSTR];

    int clock_ms = ROUND(timer / 10) % 100;
    int clock_sec = ROUND(timer / 1000) % 60;
    int clock_min = ROUND(timer / 60000) % 60;

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(timeclock, dstSize, "%d:%02d.%02d", clock_min, clock_sec, clock_ms);
#else
    sprintf(timeclock, "%d:%02d.%02d", clock_min, clock_sec, clock_ms);
#endif

    return timeclock;
}

static int campaign_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
    case GUI_BACK:
        if (campaign_theme_used())
        {
            campaign_theme_quit();
            return goto_state_full(&st_campaign, GUI_ANIMATION_W_CURVE, GUI_ANIMATION_E_CURVE, 0);
        }
        else if (server_policy_get_d(SERVER_POLICY_LEVELGROUP_ONLY_CAMPAIGN))
            return goto_state(&st_title);
        else
            return goto_state_full(&st_levelgroup, GUI_ANIMATION_W_CURVE, GUI_ANIMATION_E_CURVE, 0);
    case GUI_NEXT:
        campaign_theme_index += 1;
        if (campaign_theme_index > 4)
            campaign_theme_index = 0;
        break;
    case GUI_PREV:
        campaign_theme_index -= 1;
        if (campaign_theme_index < 0)
            campaign_theme_index = 4;
        break;
    case 999:
        return goto_state_full(&st_playmodes, GUI_ANIMATION_N_CURVE, GUI_ANIMATION_S_CURVE, 0);

    case CAMPAIGN_SELECT_THEME:
        campaign_theme_init();
        return goto_state_full(&st_campaign, GUI_ANIMATION_E_CURVE, GUI_ANIMATION_W_CURVE, 0);

    case CAMPAIGN_SELECT_LEVEL:
        if (check_handsoff())
            return goto_handsoff(&st_campaign);

        progress_init(MODE_CAMPAIGN);

        audio_music_stop();
        audio_play(AUD_STARTGAME, 1.0f);
        game_fade(+4.0);
        if (progress_play(campaign_get_level(val)))
        {
            campaign_load_camera_box_trigger(level_name(curr_level()));

            if (config_get_d(CONFIG_ACCOUNT_SAVE) > 0 && curr_mode() != MODE_NONE && !demo_fp)
                return goto_state(&st_nodemo);

            return goto_state(&st_play_ready);
        }
        break;

    case SET_XBOX_LB:
        campaign_theme_index -= 1;
        if (campaign_theme_index < 0)
            campaign_theme_index = 4;
        break;

    case SET_XBOX_RB:
        campaign_theme_index += 1;
        if (campaign_theme_index > 4)
            campaign_theme_index = 0;
        break;
    }

    if (!campaign_theme_used())
    {
        gui_set_image(campaign_theme_image_id, "gui/campaign/locked.jpg");
        gui_set_label(campaign_theme_text_id, _("Locked"));

        if (campaign_level_unlocks[campaign_theme_index] > 0)
        {
            gui_set_image(campaign_theme_image_id, campaign_theme_images[campaign_theme_index]);
            gui_set_label(campaign_theme_text_id, _(campaign_theme_texts[campaign_theme_index]));
            gui_set_color(campaign_theme_text_id, gui_wht, gui_wht);
            gui_set_state(campaign_theme_btn_id, CAMPAIGN_SELECT_THEME, 0);
        }
        else
        {
            gui_set_color(campaign_theme_text_id, gui_gry, gui_gry);
            gui_set_state(campaign_theme_btn_id, GUI_NONE, 0);
        }
    }

    return 1;
}

static int campaign_gui(void)
{
    int w = video.device_w;
    int h = video.device_h;

    int id, jd, kd, ld, md, nd;

    if ((id = gui_vstack(0)))
    {
        if ((jd = gui_hstack(id)))
        {
            if (!campaign_theme_used())
                gui_label(jd, _("Select World"), GUI_SML, 0, 0);
            else
                gui_label(jd, _(campaign_theme_texts[campaign_theme_index]), GUI_SML, 0, 0);

            gui_space(jd);
            gui_filler(jd);

#ifndef __EMSCRIPTEN__
            if (current_platform == PLATFORM_PC)
#endif
                gui_start(jd, _("Back"), GUI_SML, GUI_BACK, 0);
        }

        gui_space(id);

        if ((jd = gui_hstack(id)))
        {
            gui_filler(jd);
            gui_label(jd, _(campaign_ranks[campaign_rank()].text_rank), GUI_SML, gui_wht, gui_wht);
            gui_image(jd, campaign_ranks[campaign_rank()].img_rank, gui_measure("CAD", GUI_SML).w * 1.1f, gui_measure("CAD", GUI_SML).h * 1.1f);
            gui_filler(jd);
            gui_set_rect(jd, GUI_ALL);
            gui_pulse(jd, 1.2f);
        }

        gui_space(id);

        if ((kd = gui_hstack(id)))
        {
            gui_filler(kd);
            /* Arrows are very tricky. But now, in REVERSE! */
#ifndef __EMSCRIPTEN__
            if (!campaign_theme_used() && current_platform == PLATFORM_PC)
#else
            if (!campaign_theme_used())
#endif
            {
#ifdef SWITCHBALL_GUI
                gui_maybe_img(kd, "gui/navig/arrow_right_disabled.png", "gui/navig/arrow_right.png", GUI_NEXT, GUI_NONE, 1);
#else
                gui_maybe(kd, GUI_ARROW_RGHT, GUI_NEXT, GUI_NONE, 1);
#endif
                gui_space(kd);
            }

            if ((ld = gui_vstack(kd)))
            {
                if (!campaign_theme_used())
                {
                    if ((md = gui_hstack(ld)))
                    {
                        gui_filler(md);
                        if ((nd = gui_vstack(md)))
                        {
                            if (campaign_level_unlocks[campaign_theme_index] > 0)
                            {
                                campaign_theme_image_id = gui_image(nd, campaign_theme_images[campaign_theme_index], w / 3.3f, h / 2.4f);
                                campaign_theme_text_id = gui_label(nd, _(campaign_theme_texts[campaign_theme_index]), GUI_SML, gui_wht, gui_wht);
                            }
                            else
                            {
                                campaign_theme_image_id = gui_image(nd, "gui/campaign/locked.jpg", w / 3.3f, h / 2.4f);
                                campaign_theme_text_id = gui_label(nd, _("Locked"), GUI_SML, gui_gry, gui_gry);
                            }
                        }
                        gui_filler(md);
                    }
                    gui_filler(ld); campaign_theme_btn_id = ld;
                    gui_set_state(campaign_theme_btn_id, campaign_level_unlocks[campaign_theme_index] > 0 ? CAMPAIGN_SELECT_THEME : GUI_NONE, 0);
                }
                else
                {
                    if ((md = gui_hstack(ld)))
                    {
                        gui_filler(md);
                        gui_image(md, campaign_theme_images[campaign_theme_index], w / 3.3f, h / 2.4f);
                        gui_filler(md);
                    }
                    gui_space(ld);

                    if ((md = gui_harray(ld)))
                    {
                        for (int i = 5; i > -1; i--)
                        {
                            /* Campaign levels are needed in the system */
                            struct level *l = campaign_get_level((campaign_theme_index * 6) + i);

                            const GLubyte *fore = gui_gry;
                            const GLubyte *back = gui_gry;

                            if (!l)
                                gui_label(md, " ", GUI_SML, gui_blk, gui_blk);
                            else
                            {
                                /* Got the level? Continue searching. */

                                if (level_opened(l))
                                {
                                    fore = level_completed(l) ? gui_yel : gui_red;
                                    back = level_completed(l) ? gui_grn : gui_yel;
                                }

                                nd = gui_label(md, level_name(l), GUI_SML, back, fore);

                                if (level_opened(l))
                                {
                                    gui_set_state(nd, CAMPAIGN_SELECT_LEVEL, (campaign_theme_index * 6) + i);

                                    if (i == 0)
                                        gui_focus(nd);
                                }
                            }
                        }
                    }
                }
            }

#ifndef __EMSCRIPTEN__
            if (!campaign_theme_used() && current_platform == PLATFORM_PC)
#else
            if (!campaign_theme_used())
#endif
            {
                gui_space(kd);
#ifdef SWITCHBALL_GUI
                gui_maybe_img(kd, "gui/navig/arrow_left_disabled.png", "gui/navig/arrow_left.png", GUI_PREV, GUI_NONE, 1);
#else
                gui_maybe(kd, GUI_ARROW_LFT, GUI_PREV, GUI_NONE, 1);
#endif
            }

            gui_filler(kd);
        }

        if (!campaign_theme_used() && server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED))
        {
            gui_space(id);
            gui_state(id, _("Play Modes"), GUI_SML, 999, 0);
        }

        gui_layout(id, 0, 0);
    }

    return id;
}

static int campaign_enter(struct state *st, struct state *prev)
{
    audio_music_fade_to(0.5f, "bgm/title.ogg");

    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < 6; j++)
        {
            //log_printf("Campaign: Theme index %d / Level %d %s\n", i, ((i * 6) + j) + 1, campaign_get_level((i * 6) + j)->is_locked ? "Locked" : "Unlocked");
            campaign_level_unlocks[i] += campaign_get_level((i * 6) + j)->is_locked ? 0 : 1;
            //campaign_level_unlocks[i] += 1;
        }
    }

    if (prev == &st_levelgroup)
    {
        progress_init(MODE_CAMPAIGN);
        campaign_theme_index = 0;
    }

    return campaign_gui();
}

static void campaign_paint(int id, float t)
{
    game_client_draw(0, t);

    gui_paint(id);
#ifndef __EMSCRIPTEN__
    if (xbox_show_gui())
        xbox_control_list_gui_paint();
#endif
}

static int campaign_keybd(int c, int d)
{
    if (d)
    {
#ifndef __EMSCRIPTEN__
        if (c == KEY_EXIT && current_platform == PLATFORM_PC)
            return campaign_action(GUI_BACK, 0);
#else
        if (c == KEY_EXIT)
            return campaign_action(GUI_BACK, 0);
#endif
    }
    return 1;
}

static int campaign_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return campaign_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return campaign_action(GUI_BACK, 0);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L1, b))
            return campaign_action(SET_XBOX_LB, 0);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_R1, b))
            return campaign_action(SET_XBOX_RB, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

enum
{
    LEVELGROUP_CAMPAIGN = GUI_LAST,
    LEVELGROUP_LEVELSET
};

#ifndef FS_VERSION_1

static int campaign_download_id;

struct campaign_download_info
{
    char *campaign_file;
    char label[32];
};

static struct campaign_download_info *create_campaign_download_info(const char *set_file)
{
    struct campaign_download_info *dli = calloc(sizeof (*dli), 1);

    if (dli)
        dli->campaign_file = strdup(set_file);

    return dli;
}

static void free_campaign_download_info(struct campaign_download_info *dli)
{
    if (dli)
    {
        if (dli->campaign_file)
        {
            free(dli->campaign_file);
            dli->campaign_file = NULL;
        }

        free(dli);
        dli = NULL;
    }
}

static void campaign_download_progress(void *data1, void *data2)
{
    struct campaign_download_info *dli = data1;
    struct fetch_progress *pr = data2;

    if (dli)
    {
        int set_index = set_find(dli->campaign_file);

        if (download_ids && set_index >= 0 && set_index < total)
        {
            int id = download_ids[set_index];

            if (id)
            {
                char label[32] = GUI_ELLIPSIS;

                if (pr->total > 0)
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                    sprintf_s(label, dstSize, "%3d%%", (int)(pr->now * 100.0 / pr->total) % 1000);
#else
                    sprintf(label, "%3d%%", (int)(pr->now * 100.0 / pr->total) % 1000);
#endif

                if (strcmp(label, dli->label) != 0)
                {
                    SAFECPY(dli->label, label);
                    gui_set_label(id, label);
                }
            }
        }
    }
}

static void campaign_download_done(void *data1, void *data2)
{
    struct campaign_download_info *dli = data1;
    struct fetch_done *dn = data2;

    if (dli)
    {
        int set_index = set_find(dli->campaign_file);

        if (download_ids && set_index >= 0 && set_index < total)
        {
            int id = download_ids[set_index];

            if (id)
            {
                if (dn->finished)
                {
                    gui_remove(id);

                    download_ids[set_index] = 0;

                    if (name_ids && name_ids[set_index])
                    {
                        gui_set_label(name_ids[set_index], set_name(set_index));
                        gui_pulse(name_ids[set_index], 1.2f);
                    }
                }
                else
                {
                    gui_set_label(id, "!");
                    gui_set_color(id, gui_red, gui_red);
                }
            }
        }

        free_campaign_download_info(dli);
        dli = NULL;
    }
}

#endif

static int levelgroup_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
    case GUI_BACK:
        return goto_state(&st_title);
    case LEVELGROUP_CAMPAIGN:
        if (campaign_init())
            return goto_state_full(&st_campaign, GUI_ANIMATION_E_CURVE, GUI_ANIMATION_W_CURVE, 0);
        break;
    case LEVELGROUP_LEVELSET:
        return goto_state_full(&st_set, GUI_ANIMATION_W_CURVE, GUI_ANIMATION_E_CURVE, 0);
    }
    return 1;
}

static int levelgroup_gui(void)
{
    int w = video.device_w;
    int h = video.device_h;

    campaign_level_unlocks[0] = 0;
    campaign_level_unlocks[1] = 0;
    campaign_level_unlocks[2] = 0;
    campaign_level_unlocks[3] = 0;
    campaign_level_unlocks[4] = 0;

    int id, jd, kd, ld;

    if ((id = gui_vstack(0)))
    {
        if ((jd = gui_hstack(id)))
        {
            if (video.aspect_ratio >= 1.0f)
                gui_label(jd, _("Choose your level group"), GUI_SML, 0, 0);

            gui_filler(jd);

#ifndef __EMSCRIPTEN__
            if (current_platform == PLATFORM_PC)
#endif
                gui_start(jd, _("Back"), GUI_SML, GUI_BACK, 0);
        }

        gui_space(id);

        if ((kd = (video.aspect_ratio >= 1.0f ? gui_hstack(id) : gui_vstack(id))))
        {
            if ((ld = gui_vstack(kd)))
            {
                /*
                 * Start level sets (Enterprise and higher only
                 * or complete the campaign in Pro edition)
                 */

#ifndef SET_ALWAYS_UNLOCKED
                int set_was_unlocked = account_get_d(ACCOUNT_SET_UNLOCKS) > 0
                    && (server_policy_get_d(SERVER_POLICY_LEVELGROUP_UNLOCKED_LEVELSET) || campaign_career_unlocked()
#if NB_STEAM_API == 0 && NB_EOS_SDK == 0
                        || config_cheat();
#endif

                if (video.aspect_ratio >= 1.0f)
                    gui_image(ld, set_was_unlocked ? "gui/levels/levelset.jpg" : "gui/levels/levelset_locked.jpg", w / 3.2f, h / 2.3f);
                gui_label(ld, set_was_unlocked ? _("Level Set") : _("Locked"), GUI_SML, set_was_unlocked ? gui_wht : gui_gry, set_was_unlocked ? gui_wht : gui_gry);
                gui_set_state(ld, set_was_unlocked ? LEVELGROUP_LEVELSET : GUI_NONE, 0);
                gui_filler(ld);
#else
                if (video.aspect_ratio >= 1.0f)
                    gui_image(ld, "gui/levels/levelset.jpg", w / 3.2f, h / 2.3f);
                gui_label(ld, _("Level Set"), GUI_SML, gui_wht, gui_wht);
                gui_filler(ld);
                gui_set_state(ld, LEVELGROUP_LEVELSET, 0);
#endif
            }

            gui_space(kd);

            for (int i = 29; i > -1; i--)
            {
                if (level_opened(campaign_get_level(i)))
                {
                    if (i > 23)
                        campaign_level_unlocks[4] = 1;
                    else if (i > 17)
                        campaign_level_unlocks[3] = 1;
                    else if (i > 11)
                        campaign_level_unlocks[2] = 1;
                    else if (i > 5)
                        campaign_level_unlocks[1] = 1;
                    else
                        campaign_level_unlocks[0] = 1;
                }
            }

            if ((ld = gui_vstack(kd)))
            {
                if (campaign_level_unlocks[0] > 0)
                {
                    /*
                     * Starts campaign levels (Home Edition
                     * and higher only).
                     */
                    if (video.aspect_ratio >= 1.0f)
                        gui_image(ld, "gui/levels/campaign.jpg", w / 3.2f, h / 2.3f);
                    gui_label(ld, _("Campaign"), GUI_SML, gui_wht, gui_wht);
                    gui_filler(ld);
                    gui_set_state(ld, LEVELGROUP_CAMPAIGN, 0);
                }
                else
                {
                    if (video.aspect_ratio >= 1.0f)
                        gui_image(ld, "gui/campaign/locked.jpg", w / 3.2f, h / 2.3f);
                    gui_label(ld, _("Locked"), GUI_SML, gui_gry, gui_gry);
                    gui_filler(ld);
                    gui_set_state(ld, GUI_NONE, 0);
                }
            }
        }
        gui_layout(id, 0, 0);
    }

    campaign_quit();

    return id;
}

static int levelgroup_enter(struct state *st, struct state *prev)
{
    campaign_init();

    if (prev == &st_campaign)
        progress_init(MODE_NORMAL);

    if (server_policy_get_d(SERVER_POLICY_LEVELGROUP_ONLY_CAMPAIGN))
        return goto_state(&st_campaign);
    else if (server_policy_get_d(SERVER_POLICY_LEVELGROUP_ONLY_LEVELSET))
        return goto_state(&st_set);

    set_boost_on(0);

    return levelgroup_gui();
}

static int levelgroup_keybd(int c, int d)
{
    if (d)
    {
#ifndef __EMSCRIPTEN__
        if (c == KEY_EXIT && current_platform == PLATFORM_PC)
            return levelgroup_action(GUI_BACK, 0);
#else
        if (c == KEY_EXIT)
            return levelgroup_action(GUI_BACK, 0);
#endif
    }
    return 1;
}

static int levelgroup_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return levelgroup_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return levelgroup_action(GUI_BACK, 0);
    }
    return 1;
}

#endif

/*---------------------------------------------------------------------------*/

int goto_playgame(void)
{
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    if (server_policy_get_d(SERVER_POLICY_LEVELGROUP_ONLY_CAMPAIGN))
        return goto_state(&st_campaign);
    else if (server_policy_get_d(SERVER_POLICY_LEVELGROUP_ONLY_LEVELSET))
#endif
        return goto_state(&st_set);
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    else
        return goto_state(&st_levelgroup);
#endif
}

int goto_playgame_register(void)
{
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    if (server_policy_get_d(SERVER_POLICY_LEVELGROUP_ONLY_CAMPAIGN))
        return goto_name(&st_campaign, &st_title, 0, 0, 0);
    else if (server_policy_get_d(SERVER_POLICY_LEVELGROUP_ONLY_LEVELSET))
#endif
        return goto_name(&st_set, &st_title, 0, 0, 0);
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    else
        return goto_name(&st_levelgroup, &st_title, 0, 0, 0);
#endif
}

int goto_playmenu(int m)
{
    switch (m)
    {
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    case MODE_CAMPAIGN: return goto_state(&st_campaign);
#endif
    case MODE_BOOST_RUSH: return goto_state(&st_set);
    }

    return goto_state(&st_start);
}

/*---------------------------------------------------------------------------*/

struct state st_set = {
    set_enter,
    set_leave,
    set_paint,
    shared_timer,
    set_point,
    set_stick,
    shared_angle,
    shared_click,
    set_keybd,
    set_buttn
};

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
struct state st_campaign = {
    campaign_enter,
    shared_leave,
    campaign_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    campaign_keybd,
    campaign_buttn
};

struct state st_levelgroup = {
    levelgroup_enter,
    shared_leave,
    campaign_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    levelgroup_keybd,
    levelgroup_buttn
};
#endif