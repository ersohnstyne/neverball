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

#if NB_HAVE_PB_BOTH==1
#include "networking.h"
#include "account.h"
#include "account_wgcl.h"
#include "campaign.h" /* New: Campaign */
#endif

#include "gui.h"
#include "transition.h"
#include "demo.h"
#include "set.h"
#include "progress.h"
#include "audio.h"
#include "config.h"
#include "util.h"
#include "common.h"
#include "key.h"

#include "activity_services.h"

#include "game_common.h"
#include "game_draw.h"

#include "st_package.h"
#include "st_common.h"
#if NB_HAVE_PB_BOTH==1
#include "st_wgcl.h"
#include "st_malfunction.h"
#endif
#include "st_name.h"
#include "st_level.h"
#include "st_set.h"
#include "st_title.h"
#include "st_start.h"
#include "st_shared.h"

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
#include "st_playmodes.h"
#endif
#include "st_play.h"

#if NB_STEAM_API==0 && NB_EOS_SDK==0
#define SET_ALWAYS_UNLOCKED
#endif

#if defined(__WII__)
/* We're using SDL 1.2 on Wii, which has SDLKey instead of SDL_Keycode. */
typedef SDLKey SDL_Keycode;
#endif

/*---------------------------------------------------------------------------*/

struct state st_levelgroup;

/*---------------------------------------------------------------------------*/

#if NB_HAVE_PB_BOTH!=1
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
#endif

/*---------------------------------------------------------------------------*/

#define SET_STEP 7

#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT)
#if NB_STEAM_API==0 && NB_EOS_SDK==0
#define SET_CHECK_LOCKED(idx)                                                 \
    (account_get_d(ACCOUNT_SET_UNLOCKS) <= idx && !is_boost_on() &&           \
     !config_cheat() &&                                                       \
      (server_policy_get_d(SERVER_POLICY_EDITION) < SET_UNLOCKABLE_EDITION && \
       server_policy_get_d(SERVER_POLICY_EDITION) != -1))
#else
#define SET_CHECK_LOCKED(idx)                                                 \
    (account_get_d(ACCOUNT_SET_UNLOCKS) <= idx && !is_boost_on() &&           \
     (server_policy_get_d(SERVER_POLICY_EDITION) < SET_UNLOCKABLE_EDITION &&  \
      server_policy_get_d(SERVER_POLICY_EDITION) != -1))
#endif
#else
#define SET_CHECK_LOCKED(idx) 0
#endif

static int set_manual_hotreload = 0;

static int total = 0;
static int first = 0;

static int shot_id;
static int desc_id;

static int do_init = 1;

//static int boost_id;
static int boost_on;

int is_boost_on(void)
{
    return boost_on == 1;
}

enum
{
    SET_SELECT = GUI_LAST,
    SET_TOGGLE_BOOST,
    SET_GET_MORE
};

static void set_refresh_packages_done(void* data1, void* data2)
{
    struct fetch_done *dn = data2;

    if (dn->success)
    {
#if NB_HAVE_PB_BOTH == 1
        goto_wgcl_addons_login(0, &st_set, 0);
#else
        goto_package(0, &st_set);
#endif
    }
    else audio_play("snd/uierror.ogg", 1.0f);
}

static unsigned int set_refresh_packages(void)
{
    package_change_category(PACKAGE_CATEGORY_LEVELSET);

    struct fetch_callback callback = { 0 };

    callback.data = NULL;
    callback.done = set_refresh_packages_done;

    return package_refresh(callback);
}

static int set_action(int tok, int val)
{
    GAMEPAD_GAMEMENU_ACTION_SCROLL(GUI_PREV, GUI_NEXT, SET_STEP);

#ifdef CONFIG_INCLUDES_ACCOUNT
    int set_name_locked = SET_CHECK_LOCKED(val);
#else
    int set_name_locked = 0;
#endif

    switch (tok)
    {
        case GUI_BACK:
            set_quit();
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            if (server_policy_get_d(SERVER_POLICY_LEVELGROUP_ONLY_LEVELSET))
                return exit_state(&st_title);
            else
                return exit_state(&st_levelgroup);
#else
            return exit_state(&st_title);
#endif
            break;

        case GUI_PREV:
            first = MAX(first - SET_STEP, 0);
            do_init = 0;
            return exit_state(&st_set);

            break;

        case GUI_NEXT:
            first = MIN(first + SET_STEP, total - 1);
            do_init = 0;
            return goto_state(&st_set);

            break;

        case SET_SELECT:
            if (set_name_locked) return 1;

            set_goto(val);
            activity_services_setname_update(set_name(val));
            return goto_state(&st_start);
            break;

        case SET_TOGGLE_BOOST:
            boost_on = !boost_on;
            set_manual_hotreload = 1;
            return goto_state(&st_set);
            break;

        case SET_GET_MORE:
#if ENABLE_FETCH!=0 && \
    !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
            set_refresh_packages();
#endif
            break;
    }

    return 1;
}

static void gui_set(int id, int i)
{
    if (set_exists(i))
    {
        int set_text_name_id;
#if defined(LEVELGROUPS_INCLUDES_CAMPAIGN) && !defined(MAPC_INCLUDES_CHKP)
        int campaign_marked = 0;
#endif

#ifdef CONFIG_INCLUDES_ACCOUNT
        int set_name_locked = SET_CHECK_LOCKED(i);
#else
        int set_name_locked = 0;
#endif

        const char *curr_setname = set_name(i);
        char curr_setname_final[MAXSTR];
        char set_name_final[MAXSTR];

        if (!curr_setname)
        {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(curr_setname_final, MAXSTR,
#else
            sprintf(curr_setname_final,
#endif
                    _("Untitled set name (%d)"), i);
        }
        else
            SAFECPY(curr_setname_final, curr_setname);

        const char *curr_setid = set_id(i);
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

        if (i % SET_STEP == 0)
            set_text_name_id = gui_start(id, "XXXXXXXXXXXXXXXXXX",
                                             GUI_SML, SET_SELECT, i);
        else
            set_text_name_id = gui_state(id, "XXXXXXXXXXXXXXXXXX",
                                             GUI_SML, SET_SELECT, i);

        if (str_starts_with(curr_setid_final, "valentine"))
        {
            gui_set_color(set_text_name_id, gui_pnk, gui_red);

            SAFECPY(set_name_final, curr_setname_final);
        }
        else if (str_starts_with(curr_setid_final, "freeland"))
        {
            gui_set_color(set_text_name_id, gui_grn, gui_cya);

            SAFECPY(set_name_final, curr_setname_final);
        }
        else if (str_starts_with(curr_setid_final, "halloween"))
        {
            gui_set_color(set_text_name_id, gui_red, gui_yel);

            SAFECPY(set_name_final, curr_setname_final);
        }
        else if (str_starts_with(curr_setid_final, "christmas"))
        {
            gui_set_color(set_text_name_id, gui_red, gui_grn);

            SAFECPY(set_name_final, curr_setname_final);
        }
        else if (str_starts_with(curr_setid_final, "anime"))
        {
            gui_set_color(set_text_name_id, gui_cya, gui_blu);

            SAFECPY(set_name_final, curr_setname_final);
        }
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        else if (str_starts_with(curr_setid_final, "SB")
              || str_starts_with(curr_setid_final, "sb")
              || str_starts_with(curr_setid_final, "Sb")
              || str_starts_with(curr_setid_final, "sB"))
        {
#ifndef MAPC_INCLUDES_CHKP
            campaign_marked = 1;
#endif
            gui_set_color(set_text_name_id, GUI_COLOR_CYA);

            SAFECPY(set_name_final, curr_setname_final);
        }
#endif
        else SAFECPY(set_name_final, curr_setname_final);

        gui_set_trunc(set_text_name_id, TRUNC_TAIL);
        gui_set_label(set_text_name_id, set_name_final);

        if (set_name_locked)
        {
            gui_set_label(set_text_name_id, _("Locked"));
            gui_set_color(set_text_name_id, GUI_COLOR_GRY);
            gui_set_state(set_text_name_id, GUI_NONE, i);
        }
#ifndef MAPC_INCLUDES_CHKP
        else if (campaign_marked)
        {
            gui_set_label(set_text_name_id, _("MAPC requires CHKP"));
            gui_set_color(set_text_name_id, GUI_COLOR_RED);
            gui_set_state(set_text_name_id, GUI_NONE, i);
        }
#endif
    }
    else
        gui_label(id, "", GUI_SML, 0, 0);
}

#if ENABLE_MOON_TASKLOADER!=0
static int set_is_scanning_with_moon_taskloader = 0;

static int set_scan_moon_taskloader(void* data, void* execute_data)
{
    //while (st_global_animating());

    if (!do_init || set_manual_hotreload)
    {
        first = 0;

        total = set_init(boost_on);
        first = MIN(first, (total - 1) - ((total - 1) % SET_STEP));

        set_manual_hotreload = 0;
        do_init              = 1;
    }

    return 1;
}

static void set_scan_done_moon_taskloader(void* data, void* done_data)
{
    set_is_scanning_with_moon_taskloader = 0;

    if (total > 0)
    {
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
        /* FIXME: WGCL Narrator can do it! */

        EM_ASM({
            if (navigator.language.startsWith("ja") || navigator.language.startsWith("jp") || gameoptions_debug_locale_japanese)
                CoreLauncherOptions_GameOptions_PlayNarratorAudio("ja-JP/corelauncher_narrator_levelset_select.mp3");
        });
#elif NB_HAVE_PB_BOTH!=1 || !defined(__EMSCRIPTEN__)
        audio_narrator_play(AUD_START);
#endif
    }

    goto_state(curr_state());
}
#endif

static int set_gui(void)
{
    int w = video.device_w;
    int h = video.device_h;

    int id, jd, kd, ld;

    int i = 0;

#if ENABLE_MOON_TASKLOADER!=0
    if (set_is_scanning_with_moon_taskloader)
    {
        if ((id = gui_vstack(0)))
        {
            gui_title_header(id, _("Loading..."), GUI_MED, GUI_COLOR_DEFAULT);
            gui_space(id);
            gui_multi(id, _("Scanning Level Sets..."), GUI_SML, GUI_COLOR_WHT);

            gui_layout(id, 0, 0);

            return id;
        }
        else
            return 0;
    }
#endif

    if (total == 0)
    {
        if ((id = gui_vstack(0)))
        {
            gui_label(id, _("No Level Sets"), GUI_MED, GUI_COLOR_DEFAULT);
            gui_space(id);
            gui_back_button(id);

#if NB_HAVE_PB_BOTH==1
            if (server_policy_get_d(SERVER_POLICY_EDITION) >= 0)
            {
                if (boost_on)
                    gui_state(id, _("Revert to standard"), GUI_SML,
                                  SET_TOGGLE_BOOST, 0);
                else
                {
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
#if NB_STEAM_API==1
                    gui_state(id, _("Get Level Sets from Steam Workshop!"),
                                  GUI_SML, SET_GET_MORE, 0);
#elif NB_EOS_SDK==1
                    gui_state(id, _("Get Level Sets from Epic Games Store!"),
                                  GUI_SML, SET_GET_MORE, 0);
#else
                    gui_state(id, _("Get Level Sets from Addons!"),
                                  GUI_SML, SET_GET_MORE, 0);
#endif
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
#if defined(CONFIG_INCLUDES_ACCOUNT) && \
    !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
#if NB_STEAM_API==1
                if (account_get_d(ACCOUNT_PRODUCT_LEVELS) == 1 &&
                    server_policy_get_d(SERVER_POLICY_EDITION) > -1)
                {
                    /*if (!CHECK_ACCOUNT_BANKRUPT)
                    {
                        boost_id = gui_state(jd, _("Boost Rush"),
                                                 GUI_SML, SET_TOGGLE_BOOST, 0);
                        gui_set_hilite(boost_id, is_boost_on());
                        gui_space(jd);
                    }*/

                    gui_state(jd, _("Workshop"), GUI_SML, SET_GET_MORE, 0);
                }
                else
#endif
#endif
                    gui_label(jd, _("Level Set"), GUI_SML, GUI_COLOR_DEFAULT);

                gui_space(jd);
                gui_filler(jd);
                gui_navig(jd, total, first, SET_STEP);
            }
        }
        else if (video.aspect_ratio < 1.0f)
        {
            gui_navig(id, total, first, SET_STEP);
            gui_space(id);

#ifdef CONFIG_INCLUDES_ACCOUNT
            if ((jd = gui_hstack(id)))
            {
#if NB_STEAM_API==1
                if (account_get_d(ACCOUNT_PRODUCT_LEVELS) == 1 &&
                    server_policy_get_d(SERVER_POLICY_EDITION) > 0) {
                    gui_filler(jd);

                    /*if (!CHECK_ACCOUNT_BANKRUPT) {
                        boost_id = gui_state(jd, _("Boost Rush"),
                                                 GUI_SML, SET_TOGGLE_BOOST, 0);
                        gui_set_hilite(boost_id, is_boost_on());
                    }*/

                    gui_space(jd);
                    gui_state(jd, _("Workshop"), GUI_SML, SET_GET_MORE, 0);
                    gui_filler(jd);
                }

                head_id = jd;
#endif
            }
#endif
        }

        if ((jd = gui_vstack(id)))
        {
            gui_space(jd);

            if ((kd = gui_harray(jd)))
            {
                if (video.aspect_ratio >= 1.0f)
                {
                    const int ww = MIN(w, h) * 7 / 12;
                    const int hh = ww / 4 * 3;

                    shot_id = gui_image(kd, set_shot(first), ww, hh);

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

                if ((ld = gui_varray(kd)))
                {
                    for (i = first; i < first + SET_STEP; i++)
                        gui_set(ld, i);
                }
            }

            if (video.aspect_ratio >= 1.0f && !console_gui_shown())
            {
                gui_space(jd);

                desc_id = gui_multi(jd, " \n \n \n \n \n", GUI_SML, gui_yel, gui_wht);
            }
        }

        gui_layout(id, 0, 0);
    }

    return id;
}

static int set_enter(struct state *st, struct state *prev, int intent)
{
    if (prev == &st_title ||
        prev == &st_levelgroup)
        activity_services_group_update(AS_GROUP_CAMPAIGN);

#if NB_HAVE_PB_BOTH==1
    audio_music_fade_to(0.5f, is_boost_on() ? "bgm/boostrush.ogg" :
                                              "bgm/inter_world.ogg", 1);
#else
    audio_music_fade_to(0.5f, "gui/bgm/inter.ogg", 1);
#endif

    do_init = (prev == &st_set ||
               prev == &st_start
#if NB_HAVE_PB_BOTH==1
            || prev == &st_start_compat
#endif
              );

    set_manual_hotreload = !do_init || set_manual_hotreload;

    if (set_manual_hotreload)
    {
#if ENABLE_MOON_TASKLOADER!=0
        total = 0;
        first = 0;

        set_is_scanning_with_moon_taskloader = 1;

        struct moon_taskloader_callback callback = {0};
        callback.execute = set_scan_moon_taskloader;
        callback.done    = set_scan_done_moon_taskloader;

        moon_taskloader_load(NULL, callback);
#else
        if (set_manual_hotreload)
            first = 0;

        total = set_init(boost_on);
        first = MIN(first, (total - 1) - ((total - 1) % SET_STEP));

        if (total > 0)
        {
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
            /* FIXME: WGCL Narrator can do it! */

            EM_ASM({
                if (navigator.language.startsWith("ja") || navigator.language.startsWith("jp") || gameoptions_debug_locale_japanese)
                    CoreLauncherOptions_GameOptions_PlayNarratorAudio("ja-JP/corelauncher_narrator_levelset_select.mp3");
            });
#elif NB_HAVE_PB_BOTH!=1 || !defined(__EMSCRIPTEN__)
            audio_narrator_play(AUD_START);
#endif
        }

        set_manual_hotreload = 0;
#endif
    }

    if (set_manual_hotreload)
        set_manual_hotreload = 0;

#if NB_HAVE_PB_BOTH==1
    /* HACK: These two transition directions will be merged! */

    if (prev == &st_start)
    {
        const int transition_direction_upward = curr_mode() == MODE_BOOST_RUSH;
        return transition_slide_full(set_gui(), 1, !transition_direction_upward ? GUI_NW : GUI_SW, !transition_direction_upward ? GUI_NW : GUI_SW);
    }

    if (next == &st_campaign)
        return transition_slide_full(id, 0, 0, 0);
#endif

    if (prev == &st_set)
        return transition_page(set_gui(), 1, intent);

    return transition_slide(set_gui(), 1, intent);
}

static int set_leave(struct state *st, struct state *next, int id, int intent)
{
    do_init = 0;

    if (next == &st_title ||
        next == &st_levelgroup)
        activity_services_group_update(AS_GROUP_NONE);

    if (next == &st_null)
    {
        set_quit();
        game_client_free(NULL);
    }

#if NB_HAVE_PB_BOTH==1
    /* HACK: These two transition directions will be merged! */

    if (next == &st_start)
    {
        const int transition_direction_upward = curr_mode() == MODE_BOOST_RUSH;
        return transition_slide_full(id, 0, !transition_direction_upward ? GUI_NW : GUI_SW, !transition_direction_upward ? GUI_NW : GUI_SW);
    }
#endif

    if (set_manual_hotreload)
        gui_delete(id);
    else if (next == &st_set)
        return transition_page(id, 0, intent);
    else
        return transition_slide(id, 0, intent);

    return 0;
}

static void set_paint(int id, float t)
{
    game_client_draw(0, t);

    gui_paint(id);

#if ENABLE_MOON_TASKLOADER!=0
    if (set_is_scanning_with_moon_taskloader) return;
#endif
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    if (console_gui_shown())
        console_gui_list_paint();
#endif
}

static void set_over(int i)
{
#if ENABLE_MOON_TASKLOADER!=0
    if (set_is_scanning_with_moon_taskloader) return;
#endif

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
            const char *curr_setdesc = set_desc(i);

            gui_set_image(shot_id, set_shot(i));
            gui_set_multi(desc_id, curr_setdesc ? curr_setdesc : _("Unknown set description"));
        }
    }
}

static void set_point(int id, int x, int y, int dx, int dy)
{
    int jd = shared_point_basic(id, x, y);

#if ENABLE_MOON_TASKLOADER!=0
    if (set_is_scanning_with_moon_taskloader) return;
#endif

    if (jd && gui_token(jd) == SET_SELECT)
        set_over(gui_value(jd));
}

static void set_stick(int id, int a, float v, int bump)
{
    int jd = shared_stick_basic(id, a, v, bump);

#if ENABLE_MOON_TASKLOADER!=0
    if (set_is_scanning_with_moon_taskloader) return;
#endif

    if (jd && gui_token(jd) == SET_SELECT)
        set_over(gui_value(jd));
}

static int set_keybd(int c, int d)
{
#if ENABLE_MOON_TASKLOADER!=0
    if (set_is_scanning_with_moon_taskloader) return 1;
#endif

    if (d)
    {
        if (c == KEY_EXIT
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
         && current_platform == PLATFORM_PC
#endif
            )
            return set_action(GUI_BACK, 0);

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
#if ENABLE_MOON_TASKLOADER!=0
    if (set_is_scanning_with_moon_taskloader) return 1;
#endif

    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return set_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return set_action(GUI_BACK, 0);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L1, b) && first > 0)
            return set_action(GUI_PREV, 0);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_R1, b) &&
            first + SET_STEP < total)
            return set_action(GUI_NEXT, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
#ifndef MAPC_INCLUDES_CHKP
#error Security compilation error: Campaign is added, but requires Checkpoints!
#endif

static int campaign_rank_btn_id;

static int campaign_show_rank = 0;
static int campaign_theme_style_carousel;

static int campaign_theme_index   = 0;

static int campaign_theme_image_id;
static int campaign_theme_text_id;
static int campaign_theme_btn_id;

struct campaign_ranker
{
    const GLubyte* col_rank; const char *img_rank; const char *text_rank;
} campaign_ranks[] = {
    { gui_brn, "gui/ranks/rank_cadet_fhd.png",  N_("Airline Cadet")    },
    { gui_brn, "gui/ranks/rank_bronze_fhd.png", N_("Bronze Commander") },
    { gui_gry, "gui/ranks/rank_silver_fhd.png", N_("Silver Commander") },
    { gui_yel, "gui/ranks/rank_gold_fhd.png",   N_("Gold Commander")   },
    { gui_yel, "gui/ranks/rank_elite_fhd.png",  N_("Elite Commander")  }
};

const char campaign_rank_desc[][MAXSTR] = {
    N_("Complete Sky Level 3 to earn your wings"),
    N_("Achieve most silver medals to rank up"),
    N_("Achieve most gold medals to rank up"),
    N_("Complete first 12 levels and\n"
       "achieve all gold medals to get elite"),
    N_("Be best of best!")
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
    CAMPAIGN_RANK = GUI_LAST,
    CAMPAIGN_SELECT_THEME,
    CAMPAIGN_SELECT_LEVEL
};

static int   start_play_level_index;
static int   start_play_level_pending;
static float start_play_level_state_time;

static int campaign_level_play(int i)
{
    struct level *l = campaign_get_level(i);

    if (!l) return 0;

#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    EM_ASM({
        Neverball.gamecore_levelmap_load($0, UTF8ToString($1), false);
    }, l->num_indiv_theme, l->song);

    start_play_level_state_time = time_state() + 0.2;
#else
    start_play_level_state_time = time_state();
#endif

    start_play_level_index   = i;
    start_play_level_pending = 1;

    audio_play(AUD_STARTGAME, 1.0f);
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    return 0;
#else
    return progress_play(l);
#endif
}

static int campaign_level_play_timer(float dt)
{
    if (start_play_level_pending &&
        time_state() >= start_play_level_state_time)
    {
        start_play_level_pending = 0;

        if (progress_play(campaign_get_level(start_play_level_index)))
        {
            activity_services_mode_update(AS_MODE_CAMPAIGN);

            return goto_play_level();
        }
    }

    return 0;
}

static int campaign_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
            if (campaign_show_rank)
            {
                campaign_show_rank = 0;
                return exit_state(&st_campaign);
            }
            else if (campaign_theme_used())
            {
                campaign_theme_quit();
                return exit_state(&st_campaign);
            }
            else if (server_policy_get_d(SERVER_POLICY_LEVELGROUP_ONLY_CAMPAIGN))
                return exit_state(&st_title);
            else
                return exit_state(&st_levelgroup);

        case CAMPAIGN_RANK:
            campaign_show_rank = 1;
            return goto_state(&st_campaign);
            break;

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
            return goto_state(&st_playmodes);

        case CAMPAIGN_SELECT_THEME:
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
            /* FIXME: WGCL Narrator can do it! */

            EM_ASM({
                if (navigator.language.startsWith("ja") || navigator.language.startsWith("jp") || gameoptions_debug_locale_japanese) {
                    switch ($0) {
                        case 0:
                            CoreLauncherOptions_GameOptions_PlayNarratorAudio("ja-JP/corelauncher_narrator_campaign_sky.mp3");
                            break;
                        case 1:
                            CoreLauncherOptions_GameOptions_PlayNarratorAudio("ja-JP/corelauncher_narrator_campaign_ice.mp3");
                            break;
                        case 2:
                            CoreLauncherOptions_GameOptions_PlayNarratorAudio("ja-JP/corelauncher_narrator_campaign_cave.mp3");
                            break;
                        case 3:
                            CoreLauncherOptions_GameOptions_PlayNarratorAudio("ja-JP/corelauncher_narrator_campaign_cloud.mp3");
                            break;
                        case 4:
                            CoreLauncherOptions_GameOptions_PlayNarratorAudio("ja-JP/corelauncher_narrator_campaign_lava.mp3");
                            break;
                    }
                }
            }, campaign_theme_index);
#endif
            campaign_theme_init();
            return goto_state(&st_campaign);

        case CAMPAIGN_SELECT_LEVEL:
            if (check_handsoff())
                return goto_handsoff(&st_campaign);

            progress_exit();
            progress_init(MODE_CAMPAIGN);
            game_fade(+4.0);

            audio_music_stop();

            if (campaign_level_play(val))
            {
                activity_services_mode_update(AS_MODE_CAMPAIGN);

                return goto_play_level();
            }

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
            gui_set_color(campaign_theme_text_id, GUI_COLOR_WHT);
            gui_set_state(campaign_theme_btn_id, CAMPAIGN_SELECT_THEME, 0);
        }
        else
        {
            gui_set_color(campaign_theme_text_id, GUI_COLOR_GRY);
            gui_set_state(campaign_theme_btn_id, GUI_NONE, 0);
        }
    }

    return 1;
}

static int campaign_worldselect_carousel_gui(int id)
{
    campaign_theme_style_carousel = 1;

    const int w = video.device_w;
    const int h = video.device_h;

    int jd, kd, ld, md;

    if ((jd = gui_hstack(id)))
    {
        gui_filler(jd);

        /* Arrows are very tricky. But now, in REVERSE! */
#ifndef __EMSCRIPTEN__
        if (!campaign_theme_used() && current_platform == PLATFORM_PC && !console_gui_shown())
#else
        if (!campaign_theme_used())
#endif
        {
#ifdef SWITCHBALL_GUI
            gui_maybe_img(jd, "gui/navig/arrow_right_disabled.png",
                              "gui/navig/arrow_right.png",
                              GUI_NEXT, GUI_NONE, 1);
#else
            gui_maybe(jd, GUI_TRIANGLE_RIGHT, GUI_NEXT, GUI_NONE, 1);
#endif
            gui_space(jd);
        }

        if ((kd = gui_vstack(jd)))
        {
            if (!campaign_theme_used())
            {
                if ((ld = gui_hstack(kd)))
                {
                    gui_filler(ld);

                    if ((md = gui_vstack(ld)))
                    {
                        const int ww = 3 * MIN(w, h) / 6;
                        const int hh = ww / 4 * 3;

                        if (campaign_level_unlocks[campaign_theme_index] > 0)
                        {
                            campaign_theme_image_id = gui_image(md, campaign_theme_images[campaign_theme_index],
                                                                    ww, hh);
                            campaign_theme_text_id = gui_label(md, _(campaign_theme_texts[campaign_theme_index]),
                                                                   GUI_SML, GUI_COLOR_WHT);
                        }
                        else
                        {
                            campaign_theme_image_id = gui_image(md, "gui/campaign/locked.jpg",
                                                                    ww, hh);
                            campaign_theme_text_id = gui_label(md, _("Locked"),
                                                                   GUI_SML, GUI_COLOR_GRY);
                        }

                        gui_filler(md);
                    }
                }

                gui_filler(ld); campaign_theme_btn_id = ld;
                gui_set_state(campaign_theme_btn_id,
                              campaign_level_unlocks[campaign_theme_index] > 0 ?
                              CAMPAIGN_SELECT_THEME : GUI_NONE, 0);
            }
            else
            {
                if ((ld = gui_hstack(kd)))
                {
                    const int ww = 3 * MIN(w, h) / 6;
                    const int hh = ww / 4 * 3;

                    gui_filler(ld);
                    gui_image(ld, campaign_theme_images[campaign_theme_index],
                                  ww, hh);
                    gui_filler(ld);
                }

                gui_space(kd);

                if ((ld = gui_harray(kd)))
                {
                    for (int i = 5; i > -1; i--)
                    {
                        /* Classic campaign levels are needed in the level group system */

                        struct level *l = campaign_get_level((campaign_theme_index * 6) + i);

                        const GLubyte *fore = gui_gry;
                        const GLubyte *back = gui_gry;

                        if (l && l->file[0])
                        {
                            /* Got the level? Continue searching. */

                            if (level_opened(l))
                            {
                                fore = level_completed(l) ? gui_yel :
                                                            gui_red;
                                back = level_completed(l) ? gui_grn :
                                                            gui_yel;
                            }

                            md = gui_label(ld, level_name(l),
                                               GUI_SML, back, fore);

                            if (level_opened(l))
                            {
                                gui_set_state(md, CAMPAIGN_SELECT_LEVEL,
                                                  (campaign_theme_index * 6) + i);

                                if (i == 0)
                                    gui_focus(md);
                            }
                        }
                        else
                            gui_label(ld, " ", GUI_SML, GUI_COLOR_BLK);
                    }
                }
            }
        }

#ifndef __EMSCRIPTEN__
        if (!campaign_theme_used() && current_platform == PLATFORM_PC && !console_gui_shown())
#else
        if (!campaign_theme_used())
#endif
        {
            gui_space(jd);
#ifdef SWITCHBALL_GUI
            gui_maybe_img(jd, "gui/navig/arrow_left_disabled.png",
                              "gui/navig/arrow_left.png",
                              GUI_PREV, GUI_NONE, 1);
#else
            gui_maybe(kd, GUI_TRIANGLE_LEFT, GUI_PREV, GUI_NONE, 1);
#endif
        }

        gui_filler(jd);
    }

    return jd;
}

static int campaign_gui(void)
{
    const int w = video.device_w;
    const int h = video.device_h;

    int id, jd, kd;

    if (campaign_show_rank)
    {
        if ((id = gui_vstack(0)))
        {
            if ((jd = gui_vstack(id)))
            {
                const int ww = 7 * MIN(w, h) / 16;
                const int hh = ww / 16 * 9;

                if ((kd = gui_hstack(jd)))
                {
                    gui_filler(kd);
                    gui_image(kd, campaign_ranks[campaign_rank()].img_rank,
                                  ww, hh);
                    gui_filler(kd);
                }

                gui_label(jd, _(campaign_ranks[campaign_rank()].text_rank),
                              GUI_SML, gui_wht, campaign_ranks[campaign_rank()].col_rank);
                gui_multi(jd, _(campaign_rank_desc[campaign_rank()]),
                              GUI_SML, GUI_COLOR_WHT);

                gui_set_rect(jd, GUI_ALL);
            }

            gui_space(id);
            gui_start(id, _("OK"), GUI_SML, GUI_BACK, 0);

            gui_layout(id, 0, 0);
        }

        return id;
    }

    if ((id = gui_vstack(0)))
    {
        if ((jd = gui_hstack(id)))
        {
            if (!campaign_theme_used())
                gui_label(jd, _("Select World"), GUI_SML, 0, 0);
            else
                gui_label(jd, _(campaign_theme_texts[campaign_theme_index]),
                              GUI_SML, 0, 0);

            gui_space(jd);
            gui_filler(jd);

#ifndef __EMSCRIPTEN__
            if (current_platform == PLATFORM_PC && !console_gui_shown())
#endif
                gui_back_button(jd);
        }

        gui_space(id);

        campaign_rank_btn_id = gui_start(id, _(campaign_ranks[campaign_rank()].text_rank),
                                             GUI_SML, CAMPAIGN_RANK, 0);
        gui_set_color(campaign_rank_btn_id, gui_wht, campaign_ranks[campaign_rank()].col_rank);
        gui_pulse(campaign_rank_btn_id, 1.2f);

        gui_space(id);

        campaign_worldselect_carousel_gui(id);

        if (!campaign_theme_used() &&
            server_policy_get_d(SERVER_POLICY_PLAYMODES_ENABLED))
        {
            gui_space(id);
            gui_state(id, _("Play Modes"), GUI_SML, 999, 0);
        }

        gui_layout(id, 0, 0);
    }

    return id;
}

static int campaign_gui_comingsoon(void)
{
    int id;

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("Campaign"), GUI_MED, GUI_COLOR_DEFAULT);

        gui_space(id);

        gui_multi(id, _("This level group and modes can be played in\n"
                        "version 2.20.X and later."),
                      GUI_SML, GUI_COLOR_WHT);

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        if (current_platform == PLATFORM_PC && !console_gui_shown())
        {
            gui_space(id);
            gui_back_button(id);
        }
#endif

        gui_layout(id, 0, 0);
    }

    return id;
}

static void campaign_prepare(struct state *prev)
{
    campaign_level_unlocks[0] = 0;
    campaign_level_unlocks[1] = 0;
    campaign_level_unlocks[2] = 0;
    campaign_level_unlocks[3] = 0;
    campaign_level_unlocks[4] = 0;

    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 6; j++)
        {
            struct level *l = campaign_get_level((i * 6) + j);

            if (l && l->file[0] && !l->is_locked)
                campaign_level_unlocks[i]++;
        }

    if (prev == &st_levelgroup)
    {
        progress_exit();
        progress_init(MODE_CAMPAIGN);
        campaign_theme_index = 0;
    }
}

static int campaign_enter(struct state *st, struct state *prev, int intent)
{
    if (prev == &st_title ||
        prev == &st_levelgroup)
        activity_services_group_update(AS_GROUP_CAMPAIGN);

    audio_music_fade_to(0.5f, "bgm/inter_local.ogg", 1);

    campaign_prepare(prev);

#if NB_HAVE_PB_BOTH==1
    /* HACK: These two transition directions will be merged! */

    if (prev == &st_playmodes)
#ifndef NDEBUG
        return transition_slide_full(campaign_gui(), 1, GUI_NW, GUI_NW);
#else
        return transition_slide_full(campaign_gui_comingsoon(), 1, GUI_NW, GUI_NW);
#endif

#ifndef NDEBUG
    return transition_slide_full(campaign_gui(), 1, 0, 0);
#else
    return transition_slide_full(campaign_gui_comingsoon(), 1, 0, 0);
#endif
#else
#ifndef NDEBUG
    return transition_slide(campaign_gui(), 1, intent);
#else
    return transition_slide(campaign_gui_comingsoon(), 1, intent);
#endif
#endif
}

static int campaign_leave(struct state *st, struct state *next, int id, int intent)
{
    campaign_theme_style_carousel = 0;

    if (next == &st_title ||
        next == &st_levelgroup)
        activity_services_group_update(AS_GROUP_NONE);

    if (next == &st_levelgroup ||
        next == &st_null)
    {
        campaign_quit();

        if (next == &st_null)
            game_client_free(NULL);
    }

#if NB_HAVE_PB_BOTH==1
    /* HACK: These two transition directions will be merged! */

    if (next == &st_playmodes)
        return transition_slide_full(id, 0, GUI_NW, GUI_NW);

    return transition_slide_full(id, 0, 0, 0);
#endif

    return transition_slide(id, 0, intent);
}

static void campaign_paint(int id, float t)
{
    game_client_draw(0, t);

    gui_paint(id);
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    if (console_gui_shown())
        console_gui_list_paint();
#endif
}

static void campaign_timer(int id, float dt)
{
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    campaign_level_play_timer(dt);
#endif

    gui_timer(id, dt);
    game_step_fade(dt);
}

static int campaign_keybd(int c, int d)
{
    if (d && (c == KEY_EXIT
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
           && current_platform == PLATFORM_PC
#endif
        ))
        return campaign_action(GUI_BACK, 0);

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

        if (!campaign_theme_used() && campaign_theme_style_carousel)
        {
            if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L1, b))
                return campaign_action(GUI_PREV, 0);
            if (config_tst_d(CONFIG_JOYSTICK_BUTTON_R1, b))
                return campaign_action(GUI_NEXT, 0);
        }
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

enum
{
    LEVELGROUP_CAMPAIGN = GUI_LAST,
    LEVELGROUP_LEVELSET
};

static int levelgroup_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
            return exit_state(&st_title);

        case LEVELGROUP_CAMPAIGN:
            if (campaign_init())
            {
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
                /* FIXME: WGCL Narrator can do it! */

                EM_ASM({
                    if (navigator.language.startsWith("ja") || navigator.language.startsWith("jp") || gameoptions_debug_locale_japanese)
                        CoreLauncherOptions_GameOptions_PlayNarratorAudio("ja-JP/corelauncher_narrator_gameoptions_campaign.mp3");
                });
#endif
                return goto_state(&st_campaign);
            }
            break;

        case LEVELGROUP_LEVELSET:
            return goto_state(&st_set);
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
            if (current_platform == PLATFORM_PC && !console_gui_shown())
#endif
                gui_back_button(jd);
        }

        gui_space(id);

        if ((kd = (video.aspect_ratio >= 1.0f ? gui_hstack(id) :
                                                gui_vstack(id))))
        {
            if ((ld = gui_vstack(kd)))
            {
                /*
                 * Start level sets (Enterprise and higher only
                 * or complete the campaign in Pro edition)
                 */

                const int ww = MIN(w, h) / 2.4f;
                const int hh = ww;

#ifndef SET_ALWAYS_UNLOCKED
                int set_was_unlocked = account_get_d(ACCOUNT_SET_UNLOCKS) > 0 &&
                    (server_policy_get_d(SERVER_POLICY_LEVELGROUP_UNLOCKED_LEVELSET) ||
                     campaign_career_unlocked())
#if NB_STEAM_API == 0 && NB_EOS_SDK == 0
                 || config_cheat()
#endif
                    ;

                if (video.aspect_ratio >= 1.0f)
                    gui_image(ld, set_was_unlocked ? "gui/levels/levelset.jpg" :
                                                     "gui/levels/levelset_locked.jpg",
                                  ww, hh);
                gui_label(ld, set_was_unlocked ? _("Level Set") : _("Locked"),
                              GUI_SML,
                              set_was_unlocked ? gui_wht : gui_gry,
                              set_was_unlocked ? gui_wht : gui_gry);
                gui_set_state(ld, set_was_unlocked ? LEVELGROUP_LEVELSET : GUI_NONE, 0);
                gui_filler(ld);
#else
                if (video.aspect_ratio >= 1.0f)
                    gui_image(ld, "gui/levels/levelset.jpg", ww, hh);
                gui_label(ld, _("Level Set"), GUI_SML, GUI_COLOR_WHT);
                gui_filler(ld);
                gui_set_state(ld, LEVELGROUP_LEVELSET, 0);
#endif
            }

            gui_space(kd);

            for (int i = 29; i > -1; i--)
            {
                if (level_opened(campaign_get_level(i)))
                {
                    if      (i > 23) campaign_level_unlocks[4] = 1;
                    else if (i > 17) campaign_level_unlocks[3] = 1;
                    else if (i > 11) campaign_level_unlocks[2] = 1;
                    else if (i > 5)  campaign_level_unlocks[1] = 1;
                    else             campaign_level_unlocks[0] = 1;
                }
            }

            if ((ld = gui_vstack(kd)))
            {
                const int ww = MIN(w, h) / 2.4f;
                const int hh = ww;

                if (campaign_level_unlocks[0] > 0)
                {
                    /*
                     * Starts campaign levels (Home and Pro Editions).
                     */
                    if (video.aspect_ratio >= 1.0f)
                        gui_image(ld, "gui/levels/campaign.jpg", ww, hh);

                    gui_label(ld, _("Campaign"), GUI_SML, GUI_COLOR_WHT);
                    gui_filler(ld);
                    gui_set_state(ld, LEVELGROUP_CAMPAIGN, 0);
                }
                else
                {
                    if (video.aspect_ratio >= 1.0f)
                        gui_image(ld, "gui/campaign/locked.jpg", ww, hh);

                    gui_label(ld, _("Locked"), GUI_SML, GUI_COLOR_GRY);
                    gui_filler(ld);
                    gui_set_state(ld, GUI_NONE, 0);
                }
                gui_focus(ld);
            }
        }
        gui_layout(id, 0, 0);
    }

    campaign_quit();

    return id;
}

static int levelgroup_enter(struct state *st, struct state *prev, int intent)
{
    if (prev != &st_campaign && prev != &st_set)
    {
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
        /* FIXME: WGCL Narrator can do it! */

        EM_ASM({
            if (navigator.language.startsWith("ja") || navigator.language.startsWith("jp") || gameoptions_debug_locale_japanese)
                CoreLauncherOptions_GameOptions_PlayNarratorAudio("ja-JP/corelauncher_narrator_levelgroup_start.mp3");
        });
#endif
    }

    campaign_init();

    if (prev == &st_campaign)
    {
        progress_exit();
        progress_init(MODE_NORMAL);
    }

    if      (server_policy_get_d(SERVER_POLICY_LEVELGROUP_ONLY_CAMPAIGN))
        return goto_state(&st_campaign);
    else if (server_policy_get_d(SERVER_POLICY_LEVELGROUP_ONLY_LEVELSET))
        return goto_state(&st_set);
    else if (server_policy_get_d(SERVER_POLICY_EDITION) > 1 &&
             account_get_d(ACCOUNT_SET_UNLOCKS) == 0)
        account_set_d(ACCOUNT_SET_UNLOCKS, 1);

    boost_on = 0;

#if NB_HAVE_PB_BOTH==1
    audio_music_fade_to(0.5f, "bgm/inter_local.ogg", 1);
#else
    audio_music_fade_to(0.5f, switchball_useable() ? "bgm/title-switchball.ogg" :
                                                     BGM_TITLE_CONF_LANGUAGE, 1);
#endif

#if NB_HAVE_PB_BOTH==1
    /* HACK: These two transition directions will be merged! */

    if (prev == &st_campaign)
        return transition_slide_full(levelgroup_gui(), 1, 0, 0);
#endif

    return transition_slide(levelgroup_gui(), 1, intent);
}

static int levelgroup_keybd(int c, int d)
{
    if (d && (c == KEY_EXIT
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
           && current_platform == PLATFORM_PC
#endif
        ))
        return levelgroup_action(GUI_BACK, 0);

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

static int goto_playgame_param(struct state *next_st)
{
#ifdef CONFIG_INCLUDES_ACCOUNT
    if (!account_wgcl_restart_attempt()) return 1;
#endif

    return goto_state(next_st);
}

int goto_playgame(void)
{
#ifdef CONFIG_INCLUDES_ACCOUNT
    if (!account_wgcl_restart_attempt()) return 1;
#endif

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
        return goto_name(&st_campaign, &st_title, goto_playgame_param, 0, 0);
    else if (server_policy_get_d(SERVER_POLICY_LEVELGROUP_ONLY_LEVELSET))
#endif
        return goto_name(&st_set, &st_title, goto_playgame_param, 0, 0);
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    else
        return goto_name(&st_levelgroup, &st_title, goto_playgame_param, 0, 0);
#endif
}

int goto_playmenu(int m)
{
    if (m == MODE_BOOST_RUSH)
        return exit_state(&st_set);

    return exit_state(
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                      campaign_used() ? &st_campaign :
#endif
                      &st_start);
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
    campaign_leave,
    campaign_paint,
    campaign_timer,
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
