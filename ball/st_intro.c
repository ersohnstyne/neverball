/*
 * Copyright (C) 2024 Microsoft / Neverball authors
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
#include <assert.h>

#if NB_HAVE_PB_BOTH==1
#include "networking.h"
#ifndef __EMSCRIPTEN__
#include "console_control_gui.h"
#endif
#include "lang_switchball.h"
#include "account.h"
#endif

#include "common.h"
#include "config.h"
#include "geom.h"
#include "gui.h"
#include "vec3.h"
#include "audio.h"
#include "image.h"
#include "video.h"

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
               goto_state(networking_connected() == -1 ? \
                          &st_intro_waitinternet : \
                          &st_intro_nointernet); \
    } while (0)

#define RETURN_VOID_INTROLOGO_FINISHED \
    do { \
               networking_connected() == 1 ? \
               goto_state(MAINTENANCE_HOLD ? &st_server_maintenance : \
                          (CHECK_ACCOUNT_ENABLED ? &st_title : \
                                                   &st_intro_accn_disabled)) : \
               goto_state(networking_connected() == -1 ? \
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
                                (networking_connected() == -1 ? \
                                 &st_intro_waitinternet : \
                                 &st_intro_nointernet)); \
    } while (0)

#define RETURN_VOID_INTROLOGO_FINISHED \
    do { \
               goto_end_support(networking_connected() == 1 ? \
                                (MAINTENANCE_HOLD ? &st_server_maintenance : \
                                 (CHECK_ACCOUNT_ENABLED ? &st_title : \
                                                          &st_intro_accn_disabled)) : \
                                (networking_connected() == -1 ? \
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
                                (networking_connected() == -1 ? \
                                 &st_intro_waitinternet : \
                                 &st_intro_nointernet)); \
    } while (0)

#define RETURN_VOID_INTROLOGO_FINISHED \
    do { \
               goto_end_support(networking_connected() == 1 ? \
                                (MAINTENANCE_HOLD ? &st_server_maintenance : \
                                 &st_title) : \
                                (networking_connected() == -1 ? \
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

static int iframe;
static int image_id;
static int tip_id;

static int intro_soundqueue[2];

#if DEVEL_BUILD
static int devel_label_id;
#endif

static int intro_init = 0;
static int intro_done;

#ifdef SWITCHBALL_HAVE_TIP_AND_TUTORIAL
const char intro_tip[][256] =
{
    TIP_1,
    TIP_2,
    TIP_3,
    TIP_4,
    TIP_5,
    TIP_6,
    TIP_7,
    TIP_8
};

const char intro_tip_xbox[][256] =
{
    TIP_1,
    TIP_2,
    TIP_3_XBOX,
    TIP_4_XBOX,
    TIP_5,
    TIP_6_XBOX
};

const char intro_tip_ps4[][256] =
{
    TIP_1,
    TIP_2,
    TIP_3_PS4,
    TIP_4_XBOX,
    TIP_5,
    TIP_6_PS4
};

const char intro_covid_highrisk[][256] =
{
    "Stash your game transfer\nto reduce risks!",
    "Stash your replays with exceeded\nlevel status to reduce risks!",
    "Don't use challenge game mode\nto reduce risks!",
    "Use 3G+ rule to reduce risks!",
};

static void intro_create_tip(int id)
{
    int max_index = 7;
    int index_affect = config_get_d(CONFIG_TIPS_INDEX) + 1;

#if !defined(COVID_HIGH_RISK)
#ifndef __EMSCRIPTEN__
    if (current_platform != PLATFORM_PC)
        max_index = 5;
#endif

    if (index_affect > max_index)
        index_affect = 0;

    config_set_d(CONFIG_TIPS_INDEX, index_affect);
#ifndef __EMSCRIPTEN__
    if (current_platform == PLATFORM_PC)
    {
        if ((tip_id = gui_multi(id, _(intro_tip[index_affect]),
                                    GUI_SML, GUI_COLOR_WHT)))
        {
            //gui_set_rect(tip_id, GUI_TOP);
            gui_clr_rect(tip_id);
            gui_layout(tip_id, 0, -1);
        }
    }
    else if (current_platform == PLATFORM_PS)
    {
        if ((tip_id = gui_multi(id, _(intro_tip_ps4[index_affect]),
                                    GUI_SML, GUI_COLOR_WHT)))
        {
            //gui_set_rect(tip_id, GUI_TOP);
            gui_clr_rect(tip_id);
            gui_layout(tip_id, 0, -1);
        }
    }
    else
#endif
    {
        if ((tip_id = gui_multi(id, _(intro_tip_xbox[index_affect]),
                                    GUI_SML, GUI_COLOR_WHT)))
        {
            //gui_set_rect(tip_id, GUI_TOP);
            gui_clr_rect(tip_id);
            gui_layout(tip_id, 0, -1);
        }
    }
#else
    max_index = 3;
    index_affect = config_get_d(CONFIG_TIPS_INDEX) + 1;

    if (index_affect > max_index)
        index_affect = 0;

    if ((tip_id = gui_multi(id, _(intro_covid_highrisk[index_affect])
                                GUI_SML, GUI_COLOR_WHT)))
    {
        //gui_set_rect(tip_id, GUI_TOP);
        gui_clr_rect(tip_id);
        gui_layout(tip_id, 0, -1);
    }
#endif
}
#endif

static int intro_gui(void)
{
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    if (current_platform != PLATFORM_PC)
        xbox_toggle_gui(1);
#endif

    int w = video.device_w;
    int h = video.device_h;

    const int ww = h * MIN(w, h) / 16;
    const int hh = ww / 16 * 9;

    int root_id;

    if ((root_id = gui_root()))
    {
#ifdef SWITCHBALL_HAVE_TIP_AND_TUTORIAL
        // Switchball HD features

        intro_create_tip(root_id);
#endif

#if DEVEL_BUILD
        // Only debug and development builds

        char dev_str[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(dev_str, MAXSTR,
#else
        sprintf(dev_str,
#endif
                "   %s   ", _("DEVELOPMENT BUILD"));

        if ((devel_label_id = gui_label(root_id, dev_str,
                                                 GUI_SML, GUI_COLOR_RED)))
        {
            //gui_set_rect(devel_label_id, GUI_BOT);
            gui_clr_rect(devel_label_id);
            gui_layout(devel_label_id, 0, 1);
        }
#endif

        // Developer and publisher logos

        if ((image_id = gui_image(root_id, "gui/intro/0000.jpg", ww, hh)))
        {
            gui_clr_rect(image_id);
            gui_layout(image_id, 0, 0);
        }
    }

    return root_id;
}

static int intro_enter(struct state *st, struct state *prev)
{
    audio_music_fade_out(1.0f);

    if (!intro_init)
    {
        intro_soundqueue[0] = 0;
        intro_soundqueue[1] = 0;
        intro_done = 0;
        iframe = 0;

        intro_init = 1;
    }

    return intro_gui();
}

static void intro_leave(struct state *st, struct state *next, int id)
{
    if (next != &st_intro)
    {
        gui_delete(id);
        intro_init = 0;
    }
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
            int val = config_get_d(CONFIG_GRAPHIC_RESTORE_ID);
            if (val == -1)
                RETURN_INTROLOGO_FINISHED;
            else
            {
                assert(val != -1);
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
        int val = config_get_d(CONFIG_GRAPHIC_RESTORE_ID);
        if (val == -1)
            RETURN_INTROLOGO_FINISHED;
        else
        {
            assert(val != -1);
            return goto_state(&st_intro_restore);
        }
    }
    return 1;
}

static float frame_interval = 0;

static void intro_timer(int id, float dt)
{
    frame_interval += MIN(INTRO_ANIMATION_60_FPS, dt);
    if (frame_interval >= INTRO_ANIMATION_60_FPS)
    {
        frame_interval = 0;
        iframe += 1;
    }

    if (intro_soundqueue[0] == 1)
    {
        intro_soundqueue[0] = 2;
        audio_play(AUD_INTRO_THROW, 1.0f);
    }
    if (intro_soundqueue[1] == 1)
    {
        intro_soundqueue[1] = 2;
        audio_play(AUD_INTRO_SHATTER, 1.0f);
    }

    if (iframe > 1 && intro_soundqueue[0] == 0)
        intro_soundqueue[0] = 1;
    if (iframe > 29 && intro_soundqueue[1] == 0)
        intro_soundqueue[1] = 1;
    if (iframe > 160 && !intro_done)
    {
        intro_done = 1;
        int val = config_get_d(CONFIG_GRAPHIC_RESTORE_ID);
        if (val == -1)
        {
            RETURN_VOID_INTROLOGO_FINISHED;
            return;
        }
        else
        {
            assert(val != -1);
            goto_state(&st_intro_restore);
        }
    }

    if (intro_done) return;

    char introattr[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(introattr, MAXSTR,
#else
    sprintf(introattr,
#endif
            "gui/intro/%04d.jpg", iframe);

    gui_set_image(image_id, introattr);
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
#if defined(__EMSCRIPTEN__)
            EM_ASM({ window.open("https://neverball.org/community-standards.php"); }, 0);
#elif _WIN32
            system("start msedge https://neverball.org/community-standards.php");
#elif defined(__APPLE__)
            system("open https://neverball.org/community-standards.php");
#elif defined(__linux__)
            system("x-www-browser https://neverball.org/community-standards.php");
#endif

        case ACCOUNT_DISBALED_CANCEL:
            return goto_state(&st_title);
    }

    return 1;
}

static int intro_accn_disabled_enter(struct state *st, struct state *prev)
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
            gui_state(jd, _("Cancel"), GUI_SML, ACCOUNT_DISBALED_CANCEL, 0);
            gui_state(jd, _("Review"), GUI_SML, ACCOUNT_DISBALED_OPEN, 0);
            gui_start(jd, _("Exit"),   GUI_SML, GUI_BACK, 0);
        }
    }

    gui_layout(id, 0, 0);

    return id;
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
    RESTORE_DISPLAY,
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

            case RESTORE_DISPLAY:
                config_set_d(CONFIG_DISPLAY,
                             config_get_d(CONFIG_GRAPHIC_RESTORE_VAL1));
                r = 1;
                video_set_display(config_get_d(CONFIG_GRAPHIC_RESTORE_VAL1));
                remove_all();
                RETURN_INTROLOGO_FINISHED;
                break;

            case RESTORE_VSYNC:
                goto_state(&st_null);
                int oldVsync = config_get_d(CONFIG_VSYNC);
                config_set_d(CONFIG_VSYNC, config_get_d(CONFIG_GRAPHIC_RESTORE_VAL1));
                r = video_mode(f, w, h);
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
                r = video_mode(f, w, h);
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
                r = video_mode(f, w, h);
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
                r = video_mode(f, w, h);
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
        const char *gfx_target_name              = "";
        const char *gfx_target_values            = "";
        char        gfx_target_values_v2[MAXSTR];
        
        int restore_statement = config_get_d(CONFIG_GRAPHIC_RESTORE_ID);
        int restore_val_1     = config_get_d(CONFIG_GRAPHIC_RESTORE_VAL1);
        int restore_val_2     = config_get_d(CONFIG_GRAPHIC_RESTORE_VAL1);

        switch (restore_statement)
        {
            case RESTORE_NONE:
                break;

            case RESTORE_FULLSCREEN:
                assert(restore_val_1 == 0 || restore_val_1 == 1);
                gfx_target_name   = _("Fullscreen");
                gfx_target_values = (restore_val_1 == 1 ? _("On") : _("Off"));
                break;

            case RESTORE_RESOLUTION:
#if _WIN32
                assert(restore_val_1 >= 800 && restore_val_2 >= 600);
#else
                assert(restore_val_1 >= 320 && restore_val_2 >= 240);
#endif
                gfx_target_name = _("Resolution");
                doubles         = 1;
                break;

            case RESTORE_DISPLAY:
                assert(restore_val_1 > -1);
                gfx_target_name   = _("Display");
                gfx_target_values = SDL_GetDisplayName(restore_val_1);
                break;

            case RESTORE_VSYNC:
                assert(restore_val_1 > 0);
                gfx_target_name   = _("V-Sync");
                gfx_target_values = (restore_val_1 == 1 ? _("On") : _("Off"));
                break;

            case RESTORE_MULTISAMPLE:
                assert(restore_val_1 > -1);
                gfx_target_name = _("Antialiasing");
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(restore_attr, MAXSTR,
#else
                sprintf(restore_attr,
#endif
                        "%dx", restore_val_1);

                gfx_target_values = (restore_val_1 == 0 ? _(restore_attr) : _("Off"));
                break;

            case RESTORE_REFLECTION:
                assert(restore_val_1 == 0 || restore_val_1 == 1);
                gfx_target_name   = _("Reflection");
                gfx_target_values = (restore_val_1 == 1 ? _("On") : _("Off"));
                break;

            case RESTORE_HMD:
                assert(restore_val_1 == 0 || restore_val_1 == 1);
                gfx_target_name   = _("HMD");
                gfx_target_values = (restore_val_1 == 1 ? _("On") : _("Off"));
                break;

            case RESTORE_TEXTURES:
                assert(restore_val_1 == 1 || restore_val_1 == 2);
                gfx_target_name   = _("Textures");
                gfx_target_values = (restore_val_1 == 1 ? _("High") : _("Low"));
                break;

            default:
                assert(0 && "No restore graphics found!");
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
                    gfx_target_name, gfx_target_values);
            
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

static int intro_restore_enter(struct state *st, struct state *prev)
{
    iframe = 0;
    return intro_restore_gui();
}

static int intro_restore_keybd(int c, int d)
{
#ifndef __EMSCRIPTEN__
    xbox_toggle_gui(0);
#endif

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
            gui_multi(id, _("We're unable to connect the server!\n"
                            "Make sure, that is connected by the internet!"),
                          GUI_SML, GUI_COLOR_WHT);

        gui_layout(id, 0, 0);
    }

    return id;
}

static int nointernet_enter(struct state *st, struct state *prev)
{
    nointernet_transition_busy = 0;
    return nointernet_gui();
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

static int waitinternet_enter(struct state *st, struct state *prev)
{
    connect_done = 0;
    return waitinternet_gui();
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

static int background_id;

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

static int server_maintenance_enter(struct state *st, struct state *prev)
{
    audio_music_fade_to(0.5f, "bgm/maintenance.ogg");
    int id;

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("Server under maintenance!"),
                             GUI_MED, gui_gry, gui_red);
        gui_space(id);
        if (networking_standalone())
            gui_multi(id, _("It might take a while until\n"
                            "the server maintenance is finished.\n"
                            "You can play offline instead!"),
                          GUI_SML, GUI_COLOR_WHT);
        else
            gui_multi(id, _("It might take a while until\n"
                            "the server maintenance is finished."),
                          GUI_SML, GUI_COLOR_WHT);

        gui_space(id);

        if (networking_standalone())
            gui_state(id, _("Play offline"), GUI_SML, MAINTENANCE_OFFLINE, 0);

        gui_start(id, _("Exit"), GUI_SML, GUI_BACK, 0);

        gui_layout(id, 0, 0);
    }

    return id;
}

static void server_maintenance_leave(struct state *st, struct state *next, int id)
{
    gui_delete(id);
}

static void server_maintenance_paint(int id, float t)
{
    //gui_paint(background_id);
    gui_paint(id);
}

static void server_maintenance_timer(int id, float dt)
{
    //gui_timer(background_id, dt);
    gui_timer(id, dt);
}

static void server_maintenance_fade(float alpha)
{
    //gui_set_alpha(background_id, alpha, 0);
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

static int screensaver_enter(struct state *st, struct state *prev)
{
    intro_soundqueue[0] = 0;
    intro_soundqueue[1] = 0;
    intro_done = 0;
    iframe = 0;

    game_client_init_studio(1);

    return 0;
}

static void screensaver_leave(struct state *st, struct state *next, int id)
{
    gui_delete(id);
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
    shared_point,
    shared_stick,
    shared_angle,
    intro_click,
    NULL,
    intro_buttn,
    NULL,
    NULL,
    NULL
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
    screensaver_buttn,
    NULL,
    NULL,
    NULL
};