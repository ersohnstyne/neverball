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

/*
 * HACK: Used with console version
 */
#include "console_control_gui.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#if NB_HAVE_PB_BOTH==1
#include "networking.h"

#include "lang_switchball.h"
#include "account.h"
#endif

#include "common.h"
#include "config.h"
#include "geom.h"
#include "gui.h"
#include "transition.h"
#include "vec3.h"
#include "audio.h"
#include "image.h"
#include "video.h"
#include "key.h"
#ifndef VERSION
#include "version.h"
#endif

#include "game_server.h"
#include "game_client.h"
#include "game_common.h"

#include "st_intro_covid.h"
#include "st_intro.h"
#include "st_title.h"
#include "st_conf.h"
#include "st_shared.h"

#if NB_HAVE_PB_BOTH==1 && !defined(SKIP_END_SUPPORT)
#include "st_end_support.h"
#endif

/*---------------------------------------------------------------------------*/

#define INTRO_CELEBRATE_COMMIT

#define INTRO_ANIMATION_60_FPS (1 / 60)
#define MAINTENANCE_HOLD 0 > 0

#if NB_HAVE_PB_BOTH==1
#if defined(SKIP_END_SUPPORT)
#define RETURN_INTROLOGO_FINISHED \
    do { \
        return networking_connected() == 1 ? \
               goto_state(MAINTENANCE_HOLD ? &st_server_maintenance : \
                          (CHECK_ACCOUNT_ENABLED ? &st_title : \
                                                   &st_intro_accn_disabled)) : \
               goto_state(networking_connected() == 2 ? \
                          &st_intro_waitinternet : \
                          &st_intro_nointernet); \
    } while (0)

#define RETURN_VOID_INTROLOGO_FINISHED \
    do { \
               networking_connected() == 1 ? \
               goto_state(MAINTENANCE_HOLD ? &st_server_maintenance : \
                          (CHECK_ACCOUNT_ENABLED ? &st_title : \
                                                   &st_intro_accn_disabled)) : \
               goto_state(networking_connected() == 2 ? \
                          &st_intro_waitinternet : \
                          &st_intro_nointernet); \
    } while (0)
#else
#define RETURN_INTROLOGO_FINISHED \
    do { \
        return goto_end_support(networking_connected() == 1 ? \
                                (MAINTENANCE_HOLD ? &st_server_maintenance : \
                                 (CHECK_ACCOUNT_ENABLED ? &st_title : \
                                                          &st_intro_accn_disabled)) : \
                                (networking_connected() == 2 ? \
                                 &st_intro_waitinternet : \
                                 &st_intro_nointernet)); \
    } while (0)

#define RETURN_VOID_INTROLOGO_FINISHED \
    do { \
               goto_end_support(networking_connected() == 1 ? \
                                (MAINTENANCE_HOLD ? &st_server_maintenance : \
                                 (CHECK_ACCOUNT_ENABLED ? &st_title : \
                                                          &st_intro_accn_disabled)) : \
                                (networking_connected() == 2 ? \
                                 &st_intro_waitinternet : \
                                 &st_intro_nointernet)); \
    } while (0)
#endif
#else
#define RETURN_INTROLOGO_FINISHED \
    do { \
        return goto_end_support(networking_connected() == 1 ? \
                                (MAINTENANCE_HOLD ? &st_server_maintenance : \
                                 &st_title) : \
                                (networking_connected() == 2 ? \
                                 &st_intro_waitinternet : \
                                 &st_intro_nointernet)); \
    } while (0)

#define RETURN_VOID_INTROLOGO_FINISHED \
    do { \
               goto_end_support(networking_connected() == 1 ? \
                                (MAINTENANCE_HOLD ? &st_server_maintenance : \
                                 &st_title) : \
                                (networking_connected() == 2 ? \
                                 &st_intro_waitinternet : \
                                 &st_intro_nointernet)); \
    } while (0)
#endif

/*---------------------------------------------------------------------------*/

struct state st_intro_accn_disabled;
struct state st_intro_nointernet;
struct state st_intro_waitinternet;

struct state st_server_maintenance;

/*---------------------------------------------------------------------------*/

static int intro_init = 0;
static int intro_page;
static int intro_done;

static int intro_gui(void)
{
    int w = video.device_w;
    int h = video.device_h;

    const int ww = h * MIN(w, h) / 16;
    const int hh = ww / 16 * 10;

    int root_id;

    if ((root_id = gui_root()))
    {
#ifndef NDEBUG
        int devel_label_id;
        int build_date_lbl_id;

#if _DEBUG
        if ((devel_label_id = gui_label(root_id, _("DEBUG BUILD"),
                                                 GUI_SML, GUI_COLOR_RED)))
        {
            gui_clr_rect(devel_label_id);

            if (intro_page == 1)
                gui_slide(devel_label_id, GUI_N | GUI_FLING | GUI_EASE_BACK,
                                          0, 0.5f, 0);

            gui_layout(devel_label_id, -1, 1);
        }
#elif DEVEL_BUILD
        if ((devel_label_id = gui_label(root_id, _("DEVELOPMENT BUILD"),
                                                 GUI_SML, GUI_COLOR_RED)))
        {
            gui_clr_rect(devel_label_id);

            if (intro_page == 1)
                gui_slide(devel_label_id, GUI_N | GUI_FLING | GUI_EASE_BACK,
                                          0, 0.5f, 0);

            gui_layout(devel_label_id, -1, 1);
        }
#endif

#if defined(__DATE__)
        if ((build_date_lbl_id = gui_label(root_id, VERSION " (High) - " __DATE__,
                                                    GUI_TNY, GUI_COLOR_RED)))
        {
            gui_clr_rect(build_date_lbl_id);

            if (intro_page == 1)
                gui_slide(build_date_lbl_id, GUI_N | GUI_FLING | GUI_EASE_BACK,
                                             0, 0.5f, 0);

            gui_layout(build_date_lbl_id, +1, 1);
        }
#endif
#endif

        /* Developer and publisher logos */

        int image_id;

        const char intro_logo_image_path[2][MAXSTR] =
        {
            "gui/intro/pg_logo.jpg",
            "gui/intro/ae_logo.jpg"
        };

        if ((image_id = gui_image(root_id, intro_logo_image_path[intro_page - 1],
                                           ww, hh)))
        {
            gui_clr_rect(image_id);
            gui_layout(image_id, 0, 0);
        }
    }

    gui_set_alpha(root_id, 1.0f, 0);

    return root_id;
}

static int intro_enter(struct state *st, struct state *prev, int intent)
{
    audio_music_fade_out(1.0f);

    intro_done = 0;

    if (!intro_init)
    {
        intro_init = 1;
        intro_page = 1;

        audio_play(AUD_INTRO_LOGO, 1.0f);
    }
    else intro_page = 2;

    return intro_gui();
}

static int intro_leave(struct state *st, struct state *next, int id, int intent)
{
    if (next != &st_intro)
    {
        if (intro_page == 2)
            intro_init = 0;
    }

#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    if (next == &st_title)
    {
        EM_ASM({ CoreLauncher_EMSDK_RequestAutoSuggest_InputPreset(); });
    }
#endif

    gui_delete(id);
    return 0;
}

static void intro_paint(int id, float t)
{
    gui_paint(id);
}

static int intro_buttn(int b, int d)
{
    if (d)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b) ||
            config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b))
        {
            if (intro_page == 1)
                return goto_state(curr_state());

            int val = config_get_d(CONFIG_GRAPHIC_RESTORE_ID);
            if (val == -1)
                RETURN_INTROLOGO_FINISHED;
            else
            {
#ifndef NDEBUG
                assert(val != -1);
#endif
                goto_state(&st_intro_restore);
            }
        }
    }
    return 1;
}

static int intro_click(int b, int d)
{
    if (d == 1)
    {
        if (intro_page == 1)
            return goto_state(curr_state());

        int val = config_get_d(CONFIG_GRAPHIC_RESTORE_ID);
        if (val == -1)
            RETURN_INTROLOGO_FINISHED;
        else
        {
#ifndef NDEBUG
            assert(val != -1);
#endif
            return goto_state(&st_intro_restore);
        }
    }
    return 1;
}

static void intro_timer(int id, float dt)
{
    if (time_state() > 2 && !intro_done)
    {
        intro_done = 1;

        if (intro_page == 1)
        {
            goto_state(curr_state());
            return;
        }

        int val = config_get_d(CONFIG_GRAPHIC_RESTORE_ID);
        if (val == -1)
        {
            RETURN_VOID_INTROLOGO_FINISHED;
            return;
        }
        else
        {
#ifndef NDEBUG
            assert(val != -1);
#endif
            goto_state(&st_intro_restore);
        }
    }

    gui_timer(id, dt);
}

/*---------------------------------------------------------------------------*/

enum
{
    ACCOUNT_DISBALED_OPEN = GUI_LAST,
    ACCOUNT_DISBALED_CANCEL
};

static int intro_accn_disabled_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
            return 0; /* bye! */

        case ACCOUNT_DISBALED_OPEN:
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
#if defined(__EMSCRIPTEN__)
            EM_ASM({ window.open("https://pennyball.stynegame.de/communityguide"); });
#elif _WIN32
            system("explorer https://pennyball.stynegame.de/communityguide");
#elif defined(__APPLE__)
            system("open https://pennyball.stynegame.de/communityguide");
#elif defined(__linux__)
            system("x-www-browser https://pennyball.stynegame.de/communityguide");
#endif
#endif
            break;

        case ACCOUNT_DISBALED_CANCEL:
            return goto_state(&st_title);
    }

    return 1;
}

static int intro_accn_disabled_enter(struct state *st, struct state *prev, int intent)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("Account permanently banned"),
                             GUI_SML, gui_gry, gui_red);
        gui_space(id);

        if ((jd = gui_vstack(id)))
        {
            gui_multi(jd,
                _("We recently received a report for bad behaviour\n"
                  "by your account. Our moderators have reviewed in case\n"
                  "and identified that goes against Pennyball Community Standards."),
                GUI_SML, GUI_COLOR_WHT);
            gui_multi(jd,
                _("Your account is permanently banned, which means\n"
                  "you can't play Challenge or Hardcore."),
                GUI_SML, GUI_COLOR_RED);
            gui_set_rect(jd, GUI_ALL);
        }

        gui_space(id);

        if ((jd = gui_harray(id)))
        {
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
            gui_state(jd, _("Cancel"), GUI_SML, ACCOUNT_DISBALED_CANCEL, 0);
            gui_state(jd, _("Review"), GUI_SML, ACCOUNT_DISBALED_OPEN, 0);
#else
            gui_state(jd, _("Back"),   GUI_SML, ACCOUNT_DISBALED_CANCEL, 0);
#endif
            gui_start(jd, _("Exit"),   GUI_SML, GUI_BACK, 0);
        }
    }

    gui_layout(id, 0, 0);

    return transition_slide(id, 1, intent);
}

static int intro_accn_disabled_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
         && current_platform == PLATFORM_PC
#endif
            )
        {
#ifndef __EMSCRIPTEN__
            return 0; /* bye! */
#endif
        }
    }
    return 1;
}

static int intro_accn_disabled_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return intro_accn_disabled_action(gui_token(active),
                                              gui_value(active));
#ifndef __EMSCRIPTEN__
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return 0; /* bye! */
#endif
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

enum
{
    RESTORE_ACCEPT = GUI_LAST
};

enum RestoredGraphics
{
    RESTORE_NONE = -1,
    RESTORE_FULLSCREEN,
    RESTORE_RESOLUTION,
    RESTORE_VSYNC,
    RESTORE_MULTISAMPLE,
    RESTORE_REFLECTION,
    RESTORE_HMD,
    RESTORE_TEXTURES
};

static void remove_all(void)
{
    config_set_d(CONFIG_GRAPHIC_RESTORE_ID, -1);
    config_set_d(CONFIG_GRAPHIC_RESTORE_VAL1, 0);
    config_set_d(CONFIG_GRAPHIC_RESTORE_VAL2, 0);
}

static int intro_restore_action(int tok, int val)
{
    /*
     * If the restore graphic case does not matched them,
     * use default values.
     */
    int r = 0;

    int f = config_get_d(CONFIG_FULLSCREEN);
    int w = config_get_d(CONFIG_WIDTH);
    int h = config_get_d(CONFIG_HEIGHT);

    switch (tok)
    {
    case RESTORE_ACCEPT:
        switch (config_get_d(CONFIG_GRAPHIC_RESTORE_ID))
        {
            case RESTORE_FULLSCREEN:
                r = video_fullscreen(config_get_d(CONFIG_GRAPHIC_RESTORE_VAL1));
                if (r)
                {
                    remove_all();
                    RETURN_INTROLOGO_FINISHED;
                }
                break;

            case RESTORE_RESOLUTION:
                r = 1;
                video_set_window_size(config_get_d(CONFIG_GRAPHIC_RESTORE_VAL1),
                                      config_get_d(CONFIG_GRAPHIC_RESTORE_VAL2));

                remove_all();
                RETURN_INTROLOGO_FINISHED;
                break;

            case RESTORE_VSYNC:
                goto_state(&st_null);
                int oldVsync = config_get_d(CONFIG_VSYNC);
                config_set_d(CONFIG_VSYNC, config_get_d(CONFIG_GRAPHIC_RESTORE_VAL1));
#if ENABLE_DUALDISPLAY==1
                r = video_mode(f, w, h) && video_dualdisplay_mode(f, w, h);
#else
                r = video_mode(f, w, h);
#endif
                if (r)
                {
                    remove_all();
                    RETURN_INTROLOGO_FINISHED;
                }
                else
                config_set_d(CONFIG_VSYNC, oldVsync);
                break;

            case RESTORE_MULTISAMPLE:
                goto_state(&st_null);
                int oldSamp = config_get_d(CONFIG_MULTISAMPLE);
                config_set_d(CONFIG_MULTISAMPLE,
                             config_get_d(CONFIG_GRAPHIC_RESTORE_VAL1));
#if ENABLE_DUALDISPLAY==1
                r = video_mode(f, w, h) && video_dualdisplay_mode(f, w, h);
#else
                r = video_mode(f, w, h);
#endif
                if (r)
                {
                    remove_all();
                    RETURN_INTROLOGO_FINISHED;
                }
                else
                    config_set_d(CONFIG_MULTISAMPLE, oldSamp);
                break;

            case RESTORE_REFLECTION:
                goto_state(&st_null);
                int oldRefl = config_get_d(CONFIG_REFLECTION);
                config_set_d(CONFIG_REFLECTION,
                             config_get_d(CONFIG_GRAPHIC_RESTORE_VAL1));
#if ENABLE_DUALDISPLAY==1
                r = video_mode(f, w, h) && video_dualdisplay_mode(f, w, h);
#else
                r = video_mode(f, w, h);
#endif
                if (r)
                {
                    remove_all();
                    RETURN_INTROLOGO_FINISHED;
                }
                else
                    config_set_d(CONFIG_REFLECTION, oldRefl);
                break;

            case RESTORE_HMD:
                goto_state(&st_null);
                int oldHmd = config_get_d(CONFIG_HMD);
                config_set_d(CONFIG_HMD,
                             config_get_d(CONFIG_GRAPHIC_RESTORE_VAL1));
#if ENABLE_DUALDISPLAY==1
                r = video_mode(f, w, h) && video_dualdisplay_mode(f, w, h);
#else
                r = video_mode(f, w, h);
#endif
                if (r)
                {
                    remove_all();
                    RETURN_INTROLOGO_FINISHED;
                }
                else
                    config_set_d(CONFIG_HMD, oldHmd);
                break;

            case RESTORE_TEXTURES:
                goto_state(&st_null);
                config_set_d(CONFIG_TEXTURES, val);
                remove_all();
                RETURN_INTROLOGO_FINISHED;
                break;
        }

        break;

        case GUI_BACK:
            config_set_d(CONFIG_GRAPHIC_RESTORE_ID, -1);
            config_set_d(CONFIG_GRAPHIC_RESTORE_VAL1, 0);
            config_set_d(CONFIG_GRAPHIC_RESTORE_VAL2, 0);
            RETURN_INTROLOGO_FINISHED;
            break;
    }

    return r;
}

static int intro_restore_gui(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        int         doubles = 0;
        char        restore_attr[MAXSTR];
        char        restore_singles[MAXSTR],
                    restore_doubles[MAXSTR];
        const char *gfx_target_name   = "",
                   *gfx_target_values = "";

        int restore_statement = config_get_d(CONFIG_GRAPHIC_RESTORE_ID);
        int restore_val_1     = config_get_d(CONFIG_GRAPHIC_RESTORE_VAL1);
        int restore_val_2     = config_get_d(CONFIG_GRAPHIC_RESTORE_VAL1);

        switch (restore_statement)
        {
            case RESTORE_NONE:
                break;

            case RESTORE_FULLSCREEN:
#ifndef NDEBUG
                assert(restore_val_1 == 0 || restore_val_1 == 1);
#endif
                gfx_target_name   = _("Fullscreen");
                gfx_target_values = (restore_val_1 == 1 ? N_("On") : N_("Off"));
                break;

            case RESTORE_RESOLUTION:
#ifndef NDEBUG
#if _WIN32
                assert(restore_val_1 >= 800 && restore_val_2 >= 600);
#else
                assert(restore_val_1 >= 320 && restore_val_2 >= 240);
#endif
#endif
                gfx_target_name = _("Resolution");
                doubles         = 1;
                break;

            case RESTORE_VSYNC:
#ifndef NDEBUG
                assert(restore_val_1 > 0);
#endif
                gfx_target_name   = _("V-Sync");
                gfx_target_values = (restore_val_1 == 1 ? N_("On") : N_("Off"));
                break;

            case RESTORE_MULTISAMPLE:
#ifndef NDEBUG
                assert(restore_val_1 > -1);
#endif
                gfx_target_name = _("Antialiasing");
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(restore_attr, MAXSTR,
#else
                sprintf(restore_attr,
#endif
                        "%dx", restore_val_1);

                gfx_target_values = (restore_val_1 == 0 ? restore_attr : N_("Off"));
                break;

            case RESTORE_REFLECTION:
#ifndef NDEBUG
                assert(restore_val_1 == 0 || restore_val_1 == 1);
#endif
                gfx_target_name   = _("Reflection");
                gfx_target_values = (restore_val_1 == 1 ? N_("On") : N_("Off"));
                break;

            case RESTORE_HMD:
#ifndef NDEBUG
                assert(restore_val_1 == 0 || restore_val_1 == 1);
#endif
                gfx_target_name   = _("HMD");
                gfx_target_values = (restore_val_1 == 1 ? N_("On") : N_("Off"));
                break;

            case RESTORE_TEXTURES:
#ifndef NDEBUG
                assert(restore_val_1 == 1 || restore_val_1 == 2);
#endif
                gfx_target_name   = _("Textures");
                gfx_target_values = (restore_val_1 == 1 ? N_("High") : N_("Low"));
                break;

#ifndef NDEBUG
            default:
                assert(0 && "No restore graphics found!");
#endif
        }

        gui_title_header(id, _("Restore Graphics"), GUI_MED, GUI_COLOR_DEFAULT);
        gui_space(id);

        if (doubles) {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(restore_doubles, MAXSTR,
#else
            sprintf(restore_doubles,
#endif
                    _("The game, that you've set up some graphics\n"
                      "has a crash. Would you restore them now?\n"
                      "%s: %i x %i"),
                    gfx_target_name, restore_val_1, restore_val_2);

            gui_multi(id, restore_doubles, GUI_SML, GUI_COLOR_WHT);
        }
        else
        {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(restore_singles, MAXSTR,
#else
            sprintf(restore_singles,
#endif
                    _("The game, that you've set up some graphics\n"
                      "has a crash. Would you restore them now?\n"
                      "%s: %s"),
                    gfx_target_name, _(gfx_target_values));

            gui_multi(id, restore_singles, GUI_SML, GUI_COLOR_WHT);
        }

        gui_space(id);

        if ((jd = gui_harray(id)))
        {
            gui_state(jd, _("Cancel"), GUI_SML, GUI_BACK, 0);
            gui_start(jd, _("Restore now!"), GUI_SML, RESTORE_ACCEPT, 0);
        }
    }

    gui_layout(id, 0, 0);

    return id;
}

static int intro_restore_enter(struct state *st, struct state *prev, int intent)
{
    return transition_slide(intro_restore_gui(), 1, intent);
}

static int intro_restore_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
         && current_platform == PLATFORM_PC
#endif
            )
            return intro_restore_action(GUI_BACK, 0);
    }
    return 1;
}

static int intro_restore_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return intro_restore_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return intro_restore_action(GUI_BACK, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int nointernet_transition_busy;

static int nointernet_gui(void)
{
    int id;

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("No internet connection!"),
                             GUI_MED, gui_gry, gui_red);
        gui_space(id);
        if (networking_standalone())
            gui_multi(id, _("Not to worry, you can play offline!"),
                          GUI_SML, GUI_COLOR_WHT);
        else
            gui_multi(id, _("Please check your internet connection\n"
                            "or configure your router first.\n"
                            "(e.g. Wi-Fi settings or ethernet)"),
                          GUI_SML, GUI_COLOR_WHT);

        gui_layout(id, 0, 0);
    }

    return id;
}

static int nointernet_enter(struct state *st, struct state *prev, int intent)
{
    nointernet_transition_busy = 0;
    return transition_slide(nointernet_gui(), 1, intent);
}

static void nointernet_timer(int id, float dt)
{
    gui_timer(id, dt);

    if (networking_connected() == 1 && !nointernet_transition_busy)
    {
        nointernet_transition_busy = 1;
        goto_state(&st_title);
    }
}

static int nointernet_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
         && current_platform == PLATFORM_PC
#endif
            )
        {
#ifndef __EMSCRIPTEN__
            return 0; /* bye! */
#endif
        }
    }
    return 1;
}

static int nointernet_buttn(int b, int d)
{
    if (d)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b) &&
            networking_standalone())
            return goto_state(&st_title);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

int connect_done = 0;

static int waitinternet_gui(void)
{
    int id;

    if ((id = gui_title_header(0, _("Waiting for server..."),
                                  GUI_SML, GUI_COLOR_WHT)))
        gui_layout(id, 0, 0);

    return id;
}

static int waitinternet_enter(struct state *st, struct state *prev, int intent)
{
    connect_done = 0;
    return transition_slide(waitinternet_gui(), 1, intent);
}

static void waitinternet_timer(int id, float dt)
{
    if (connect_done) return;
    if (networking_connected() == 0)
    {
        connect_done = 1;
        goto_state(&st_intro_nointernet);
    }
    else if (networking_connected() == 1)
    {
        connect_done = 1;
        goto_state(&st_title);
    }
}

static int waitinternet_keybd(int c, int d)
{
    return 1;
}

static int waitinternet_buttn(int b, int d)
{
    return 1;
}

/*---------------------------------------------------------------------------*/

enum {
    MAINTENANCE_OFFLINE = GUI_LAST
};

static int server_maintenance_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
            return 0; /* bye! */
            break;

        case MAINTENANCE_OFFLINE:
            return goto_state(&st_title);
            break;
    }
    return 1;
}

static int server_maintenance_enter(struct state *st, struct state *prev, int intent)
{
    audio_music_fade_to(0.5f, "bgm/maintenance.ogg", 1);
    int id;

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("Server under maintenance!"),
                             GUI_MED, gui_gry, gui_red);
        gui_space(id);
        if (networking_standalone())
            gui_multi(id, _("It might take a while until\n"
                            "the server maintenance is success.\n"
                            "You can play offline instead!"),
                          GUI_SML, GUI_COLOR_WHT);
        else
            gui_multi(id, _("It might take a while until\n"
                            "the server maintenance is success."),
                          GUI_SML, GUI_COLOR_WHT);

        gui_space(id);

        if (networking_standalone())
            gui_state(id, _("Play offline"), GUI_SML, MAINTENANCE_OFFLINE, 0);

        gui_start(id, _("Exit"), GUI_SML, GUI_BACK, 0);

        gui_layout(id, 0, 0);
    }

    return transition_slide(id, 1, intent);
}

static int server_maintenance_leave(struct state *st, struct state *next, int id, int intent)
{
    return transition_slide(id, 0, intent);
}

static void server_maintenance_paint(int id, float t)
{
    gui_paint(id);
}

static void server_maintenance_timer(int id, float dt)
{
    gui_timer(id, dt);
}

static void server_maintenance_fade(float alpha)
{
}

static int server_maintenance_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b) &&
            networking_standalone())
            return server_maintenance_action(gui_token(active),
                                             gui_value(active));
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int screensaver_done = 0;

static void cleanup_screensaver(void)
{
    screensaver_done = 1;
}

static int screensaver_enter(struct state *st, struct state *prev, int intent)
{
    intro_done = 0;

    game_client_init_studio(1);

    return 0;
}

static int screensaver_leave(struct state *st, struct state *next, int id, int intent)
{
    if (next == &st_null)
        game_client_free(NULL);

    return 0;
}

static void screensaver_paint(int id, float t)
{
    game_client_draw(0, t);
    gui_paint(id);
}

static void screensaver_timer(int id, float dt)
{
    if (!screensaver_done)
    {
        float fixDeltatime = CLAMP(0.0f, dt, 1.0f);
        geom_step(fixDeltatime);
        game_client_step_studio(fixDeltatime);
    }
}

static int screensaver_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT)
        {
            cleanup_screensaver();
            return 0;
        }
    }
    return 1;
}

static int screensaver_buttn(int b, int d)
{
    if (d)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
        {
            cleanup_screensaver();
            return 0;
        }
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

struct state st_intro = {
    intro_enter,
    intro_leave,
    intro_paint,
    intro_timer,
    NULL,
    NULL,
    shared_angle,
    intro_click,
    NULL,
    intro_buttn
};

struct state st_intro_accn_disabled = {
    intro_accn_disabled_enter,
    shared_leave,
    shared_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    intro_accn_disabled_keybd,
    intro_accn_disabled_buttn
};

struct state st_intro_restore = {
    intro_restore_enter,
    shared_leave,
    shared_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    intro_restore_keybd,
    intro_restore_buttn
};

struct state st_intro_nointernet = {
    nointernet_enter,
    shared_leave,
    shared_paint,
    nointernet_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click_basic,
    nointernet_keybd,
    nointernet_buttn
};

struct state st_intro_waitinternet = {
    waitinternet_enter,
    shared_leave,
    shared_paint,
    waitinternet_timer,
    shared_point,
    NULL,
    shared_angle,
    NULL,
    waitinternet_keybd,
    waitinternet_buttn
};

struct state st_server_maintenance = {
    server_maintenance_enter,
    server_maintenance_leave,
    server_maintenance_paint,
    server_maintenance_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    NULL,
    server_maintenance_buttn,
    NULL,
    NULL,
    server_maintenance_fade
};

struct state st_screensaver = {
    screensaver_enter,
    screensaver_leave,
    screensaver_paint,
    screensaver_timer,
    NULL,
    NULL,
    shared_angle,
    NULL,
    screensaver_keybd,
    screensaver_buttn
};
