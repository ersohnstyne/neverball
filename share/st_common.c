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

#if _WIN32 && __MINGW32__
#include <SDL3/SDL.h>
#else
#include <SDL.h>
#endif

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
#include "console_control_gui.h"
#endif

#include "glext.h"
#include "config.h"
#include "audio.h"
#include "video.h"
#include "geom.h"
#include "lang.h"
#include "gui.h"

#include "st_common.h"

#define AUD_MENU "snd/menu.ogg"
#define AUD_BACK "snd/back.ogg"
#define AUD_DISABLED "snd/disabled.ogg"

/* Macros helps with the action game menu. */

#define GENERIC_GAMEMENU_ACTION                      \
        if (st_global_animating()) {                 \
            audio_play(AUD_DISABLED, 1.f);           \
            return 1;                                \
        } else audio_play(GUI_BACK == tok ?          \
                          AUD_BACK :                 \
                          (GUI_NONE == tok ?         \
                           AUD_DISABLED : AUD_MENU), \
                          1.0f)

#define GAMEPAD_GAMEMENU_ACTION_SCROLL(tok1, tok2, itemstep) \
        if (st_global_animating()) {                         \
            audio_play(AUD_DISABLED, 1.f);                   \
            return 1;                                        \
        } else if (tok == tok1 || tok == tok2) {             \
            if (tok == tok1)                                 \
                audio_play(first > 1 ?                       \
                           AUD_MENU : AUD_DISABLED, 1.0f);   \
            if (tok == tok2)                                 \
                audio_play(first + itemstep < total ?        \
                           AUD_MENU : AUD_DISABLED, 1.0f);   \
        } else GENERIC_GAMEMENU_ACTION

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

/*
 * Conf screen GUI helpers.
 */

void conf_slider(int id, const char *text,
                 int token, int value,
                 int *ids, int num)
{
    int jd, kd, i;

    if ((jd = gui_harray(id)) && (kd = gui_harray(jd)))
    {
        /* A series of empty buttons forms a "slider". */

        for (i = num - 1; i >= 0; i--)
        {
            ids[i] = gui_state(kd, NULL, GUI_SML, token, i);

            gui_set_hilite(ids[i], (i == value));
        }

        gui_label(jd, text, GUI_SML, 0, 0);
    }
}

int  conf_slider_v2(int id, const char *text, int tok, int val)
{
    int jd, kd, ld = 0, md;

    if ((jd = gui_harray(id)) && (kd = gui_hstack(jd)))
    {
#ifdef SWITCHBALL_GUI
        ld = gui_maybe_img(kd, "gui/navig/arrow_right_disabled.png",
                               "gui/navig/arrow_right.png",
                               tok, GUI_NONE, 1);
#else
        ld = gui_maybe(kd, GUI_TRIANGLE_RIGHT, tok, GUI_NONE, 1);
#endif
        gui_set_state(ld, tok, CLAMP(0, val + 1, 10));

        gui_filler(kd);

        md = gui_label(kd, _("XXXXXX"), GUI_SML, gui_wht, gui_wht);

        if (val == 0)
            gui_set_label(md, _("Off"));
        else
        {
            char valattr[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(valattr, MAXSTR,
#else
            sprintf(valattr,
#endif
                    "%d %%", val * 10);

            gui_set_label(md, valattr);
        }

        gui_filler(kd);

#ifdef SWITCHBALL_GUI
        ld = gui_maybe_img(kd, "gui/navig/arrow_left_disabled.png",
                               "gui/navig/arrow_left.png",
                               tok, GUI_NONE, 1);
#else
        ld = gui_maybe(kd, GUI_TRIANGLE_RIGHT, tok, GUI_NONE, 1);
#endif
        gui_set_state(ld, tok, CLAMP(0, val - 1, 10));

        gui_label(jd, text, GUI_SML, 0, 0);
    }

    return md;
}

void conf_set_slider_v2(int id, int val)
{
    if (val == 0)
        gui_set_label(id, _("Off"));
    else
    {
        char valattr[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(valattr, MAXSTR,
#else
        sprintf(valattr,
#endif
                "%d %%", val * 10);

        gui_set_label(id, valattr);
    }
}

int conf_state(int id, const char *label, const char *text, int token)
{
    int jd, kd, rd = 0;

    if ((jd = gui_harray(id)) && (kd = gui_harray(jd)))
    {
        rd = gui_state(kd, text, GUI_SML, token, 0);
        gui_label(jd, label, GUI_SML, 0, 0);
    }

    return rd;
}

void conf_toggle(int id, const char *label, int token, int value,
                 const char *text1, int value1,
                 const char *text0, int value0)
{
    int jd, kd;

    if ((jd = gui_harray(id)) && (kd = gui_harray(jd)))
    {
        int btn0, btn1;

        btn0 = gui_state(kd, text0, GUI_SML, token, value0);
        btn1 = gui_state(kd, text1, GUI_SML, token, value1);

        gui_set_hilite(btn0, (value == value0));
        gui_set_hilite(btn1, (value == value1));

        gui_label(jd, label, GUI_SML, 0, 0);
    }
}

void conf_header(int id, const char *text, int token)
{
    int jd;

    if ((jd = gui_hstack(id)))
    {
        gui_label(jd, text, GUI_SML, 0, 0);
        gui_filler(jd);
        gui_space(jd);

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        if (current_platform == PLATFORM_PC)
#endif
            gui_start(jd, _("Back"), GUI_SML, token, 0);
    }

    gui_space(id);
}

void conf_select(int id, const char *text, int token, int value,
                 const struct conf_option *opts, int num)
{
    int jd, kd, ld;
    int i;

    if ((jd = gui_harray(id)) && (kd = gui_harray(jd)))
    {
        for (i = 0; i < num; i++)
        {
            ld = gui_state(kd, _(opts[i].text), GUI_SML,
                           token, opts[i].value);

            gui_set_hilite(ld, (opts[i].value == value));
        }

        gui_label(jd, text, GUI_SML, 0, 0);
    }
}

/*---------------------------------------------------------------------------*/

/*
 * Code shared by most screens (not just conf screens).
 *
 * FIXME This probably makes ball/st_shared.c obsolete.
 */

static int (*common_action)(int tok, int val);

void common_init(int (*action_fn)(int, int))
{
    common_action = action_fn;
}

void common_leave(struct state *st, struct state *next, int id)
{
    gui_delete(id);
}

void common_paint(int id, float st)
{
    gui_paint(id);
}

void common_timer(int id, float dt)
{
    gui_timer(id, dt);
}

void common_point(int id, int x, int y, int dx, int dy)
{
    gui_pulse(gui_point(id, x, y), 1.2f);
}

void common_stick(int id, int a, float v, int bump)
{
    gui_pulse(gui_stick(id, a, v, bump), 1.2f);
}

int common_click(int b, int d)
{
    if (gui_click(b, d))
    {
        int active = gui_active();
        return common_action(gui_token(active), gui_value(active));
    }
    return 1;
}

int common_keybd(int c, int d)
{
    return (d && c == KEY_EXIT) ? common_action(GUI_BACK, 0) : 1;
}

int common_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return common_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return common_action(GUI_BACK, 0);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L1, b))
            return common_action(GUI_PREV, 0);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_R1, b))
            return common_action(GUI_NEXT, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

/*
 * Code shared by conf screens.
 */

static int common_allowfade;
static int is_common_bg;

void conf_common_init(int (*action_fn)(int, int), int allowfade)
{
    common_allowfade = allowfade;
    if (common_allowfade)
    {
        back_init("back/gui.png");
        is_common_bg = 1;
#if NB_HAVE_PB_BOTH==1
        audio_music_fade_to(0.5f, switchball_useable() ? "bgm/title-switchball.ogg" :
                                                       _("bgm/title.ogg"));
#else
        audio_music_fade_to(0.5f, "gui/bgm/inter.ogg");
#endif
    }

    common_init(action_fn);
}

void conf_common_leave(struct state *st, struct state *next, int id)
{
    config_save();

    if (common_allowfade && is_common_bg)
    {
        is_common_bg = 0;
        back_free();
    }

    gui_delete(id);
}

void conf_common_paint(int id, float t)
{
    video_push_persp((float) config_get_d(CONFIG_VIEW_FOV), 0.1f, FAR_DIST);
    {
        if (is_common_bg)
            back_draw_easy();
    }
    video_pop_matrix();

    gui_paint(id);
}

/*---------------------------------------------------------------------------*/

enum
{
    VIDEO_SCREENANIMATIONS = GUI_LAST,
    VIDEO_DISPLAY,
    VIDEO_FULLSCREEN,
    VIDEO_RESOLUTION,
    VIDEO_TEXTURES,
    VIDEO_HMD,
    VIDEO_AUTO_CONFIGURE,
    VIDEO_ADVANCED
};

static struct state *video_back;

static int video_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    int f = config_get_d(CONFIG_FULLSCREEN);
    int w = config_get_d(CONFIG_WIDTH);
    int h = config_get_d(CONFIG_HEIGHT);
    int r = 1;

    /* Revert values */
    int oldHmd   = config_get_d(CONFIG_HMD);
    int oldF     = config_get_d(CONFIG_FULLSCREEN);
    int oldRefl  = config_get_d(CONFIG_REFLECTION);
    int oldVsync = config_get_d(CONFIG_VSYNC);
    int oldSamp  = config_get_d(CONFIG_MULTISAMPLE);
    int oldText  = config_get_d(CONFIG_TEXTURES);

    switch (tok)
    {
        case GUI_BACK:
            goto_state(video_back);
            video_back = NULL;
            break;

        case VIDEO_SCREENANIMATIONS:
            config_set_d(CONFIG_SCREEN_ANIMATIONS, val);
            goto_state_full(&st_video, 0, 0, 1);

            config_save();
            break;

        case VIDEO_DISPLAY:
            goto_state(&st_display);
            break;

        case VIDEO_FULLSCREEN:
            if (oldF == val)
                return 1;

            goto_state_full(&st_null, 0, 0, 1);
            r = video_fullscreen(val);
            if (r)
                goto_state_full(&st_video, 0, 0, 1);
            else
            {
                r = video_fullscreen(oldF);
                if (r)
                    goto_state_full(&st_video, 0, 0, 1);
                else
                {
                    config_set_d(CONFIG_GRAPHIC_RESTORE_ID, 0);
                    config_set_d(CONFIG_GRAPHIC_RESTORE_VAL1, val);
                }
            }

            config_save();

            break;

        case VIDEO_RESOLUTION:
#ifndef RESIZEABLE_WINDOW
            goto_state(&st_resol);
#endif
            break;

        case VIDEO_HMD:
            if (oldHmd == val)
                return 1;

#if defined(__EMSCRIPTEN__) || NB_STEAM_API==1
            goto_state(&st_restart_required);
#else
            goto_state(&st_null);
            config_set_d(CONFIG_HMD, val);
            config_set_d(CONFIG_GRAPHIC_RESTORE_ID, 6);
            config_set_d(CONFIG_GRAPHIC_RESTORE_VAL1, val);
            r = video_mode(f, w, h);

            if (r)
                goto_state(&st_video);
            else
            {
                config_set_d(CONFIG_HMD, oldHmd);
                r = video_mode(f, w, h);
                if (r)
                    goto_state(&st_video);
            }
#endif

            config_save();

            break;

        case VIDEO_TEXTURES:
            if (oldText == val)
                return 1;

            goto_state(&st_null);
            config_set_d(CONFIG_TEXTURES, val);
            goto_state(&st_video);

            config_save();

            break;

        case VIDEO_AUTO_CONFIGURE:
            goto_state(&st_null);
            r = video_mode_auto_config(f, w, h);
            if (r)
                goto_state(&st_video);
            else
            {
                r = video_mode(f, w, h);
                if (r)
                    goto_state(&st_video);
            }

            config_save();

            break;
        case VIDEO_ADVANCED:
            goto_state(&st_video_advanced);
            break;
    }

    return r;
}

static int video_gui(void)
{
    int id, jd, autoconf_id;

    if ((id = gui_vstack(0)))
    {
        conf_header(id, _("Graphics"), GUI_BACK);

#ifdef SWITCHBALL_GUI

#ifndef __EMSCRIPTEN__
        SDL_DisplayMode dpyMode;
        SDL_GetCurrentDisplayMode(config_get_d(CONFIG_DISPLAY), &dpyMode);

        int dpy = config_get_d(CONFIG_DISPLAY);
        const char *display;
        if (!(display = SDL_GetDisplayName(dpy)))
            display = _("Unknown Display");

        char dpy_info[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(dpy_info,
#else
        sprintf(dpy_info,
#endif
                "%s\\%d x %d - %d Hz",
                display,
                dpyMode.w, dpyMode.h, dpyMode.refresh_rate);
        gui_multi(id, dpy_info, GUI_SML, gui_wht, gui_wht);

        gui_space(id);
#endif

        conf_toggle(id, _("Screen animations"), VIDEO_SCREENANIMATIONS,
                        config_get_d(CONFIG_SCREEN_ANIMATIONS), _("On"), 1, _("Off"), 0);

#if ENABLE_HMD
        gui_space(id);
        conf_toggle(id, _("HMD"), VIDEO_HMD,
            config_get_d(CONFIG_HMD), _("On"), 1, _("Off"), 0);
#endif

#if !defined(__EMSCRIPTEN__) && !defined(RESIZEABLE_WINDOW)
        char resolution[sizeof ("12345678 x 12345678")];
#endif

        gui_space(id);

        /* This ignores display configurations, sorry for incoherence problems. */
#if !defined(__EMSCRIPTEN__) && NB_STEAM_API==0
        if (SDL_GetNumVideoDisplays() > 1)
        {
            if ((jd = conf_state(id, _("Display"), "Super Longest Name",
                                     VIDEO_DISPLAY)))
            {
                gui_set_trunc(jd, TRUNC_TAIL);
                gui_set_label(jd, display);
            }
        }
#endif

        /* This ignores resolution and fullscreen configurations. */
#ifndef __EMSCRIPTEN__
#ifndef RESIZEABLE_WINDOW
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(resolution, MAXSTR,
#else
        sprintf(resolution,
#endif
                "%d x %d",
                video.window_w,
                video.window_h);

        if ((jd = conf_state(id, _("Resolution"), resolution,
            VIDEO_RESOLUTION)))
        {
            /*
             * Because we always use the desktop display mode, disable
             * display mode switching in fullscreen.
             */

            if (config_get_d(CONFIG_FULLSCREEN))
            {
                gui_set_state(jd, GUI_NONE, 0);
                gui_set_color(jd, gui_gry, gui_gry);
            }
        }
#endif

        conf_toggle(id, _("Fullscreen"), VIDEO_FULLSCREEN,
                        config_get_d(CONFIG_FULLSCREEN), _("On"), 1, _("Off"), 0);

        gui_space(id);

        conf_toggle(id, _("Textures"), VIDEO_TEXTURES,
                        config_get_d(CONFIG_TEXTURES), _("High"), 1, _("Low"), 2);
#endif

#else
        gui_multi(id, _("Switchball configurations\\"
                        "requires SWITCHBALL_GUI\\"
                        "definition preprocessors!"),
                      GUI_SML, gui_red, gui_red);
#endif
        gui_space(id);

        autoconf_id = gui_state(id, _("Auto Configure"),
                                    GUI_SML, VIDEO_AUTO_CONFIGURE, 0);
#if defined(__EMSCRIPTEN__) || NB_STEAM_API==1 || !defined(SWITCHBALL_GUI)
        if (autoconf_id)
        {
            gui_set_color(autoconf_id, gui_gry, gui_gry);
            gui_set_state(autoconf_id, GUI_NONE, 0);
        }
#endif
        gui_state(id, _("Advanced"), GUI_SML, VIDEO_ADVANCED, 0);
    }
    gui_layout(id, 0, 0);

    return id;
}

static int video_enter(struct state *st, struct state *prev)
{
    if (!video_back)
        video_back = prev;

    conf_common_init(video_action, 1);
    return video_gui();
}

/*---------------------------------------------------------------------------*/

enum
{
    VIDEO_ADVANCED_FULLSCREEN = GUI_LAST,
    VIDEO_ADVANCED_SMOOTH_FIX,
    VIDEO_ADVANCED_FORCE_SMOOTH_FIX,
    VIDEO_ADVANCED_DISPLAY,
    VIDEO_ADVANCED_RESOLUTION,
    VIDEO_ADVANCED_HMD,
    VIDEO_ADVANCED_REFLECTION,
    VIDEO_ADVANCED_BACKGROUND,
    VIDEO_ADVANCED_SHADOW,
    VIDEO_ADVANCED_MIPMAP,
    VIDEO_ADVANCED_ANISO,
    VIDEO_ADVANCED_VSYNC,
    VIDEO_ADVANCED_MULTISAMPLE,
    VIDEO_ADVANCED_TEXTURES
};

static struct state * video_advanced_back;

static int video_advanced_action(int tok, int val)
{
    int backups = 0;
    int f = config_get_d(CONFIG_FULLSCREEN);
    int w = config_get_d(CONFIG_WIDTH);
    int h = config_get_d(CONFIG_HEIGHT);
    int r = 1;

    /* Revert values */
    int oldHmd   = config_get_d(CONFIG_HMD);
    int oldF     = config_get_d(CONFIG_FULLSCREEN);
    int oldRefl  = config_get_d(CONFIG_REFLECTION);
    int oldVsync = config_get_d(CONFIG_VSYNC);
    int oldSamp  = config_get_d(CONFIG_MULTISAMPLE);
    int oldText  = config_get_d(CONFIG_TEXTURES);

    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
            goto_state(video_advanced_back);
            video_advanced_back = NULL;
            break;

        case VIDEO_ADVANCED_DISPLAY:
            goto_state(&st_display);
            break;

        case VIDEO_ADVANCED_RESOLUTION:
#ifndef RESIZEABLE_WINDOW
            goto_state(&st_resol);
#endif
            break;

        case VIDEO_ADVANCED_FULLSCREEN:
            if (oldF == val)
                return 1;

            backups = 1;
#if defined(__EMSCRIPTEN__) || NB_STEAM_API==1
            goto_state(&st_restart_required);
#else
            config_set_d(CONFIG_GRAPHIC_RESTORE_ID, 0);
            config_set_d(CONFIG_GRAPHIC_RESTORE_VAL1, val);
            config_save();
            goto_state_full(&st_null, 0, 0, 1);
            r = video_fullscreen(val);

            if (r)
                goto_state_full(&st_video_advanced, 0, 0, 1);
            else
            {
                r = video_fullscreen(val);
                if (r)
                    goto_state_full(&st_video_advanced, 0, 0, 1);
            }
#endif
            break;

        case VIDEO_ADVANCED_HMD:
            if (oldHmd == val)
                return 1;

            backups = 1;
#if defined(__EMSCRIPTEN__) || NB_STEAM_API==1
            config_set_d(CONFIG_HMD, val);
            goto_state(&st_restart_required);
#else
            config_set_d(CONFIG_HMD, val);
            config_set_d(CONFIG_GRAPHIC_RESTORE_ID, 6);
            config_set_d(CONFIG_GRAPHIC_RESTORE_VAL1, val);
            config_save();
            goto_state(&st_null);

            r = video_mode(f, w, h);

            if (r)
                goto_state(&st_video_advanced);
            else
            {
                config_set_d(CONFIG_HMD, oldHmd);
                r = video_mode(f, w, h);
                if (r)
                    goto_state(&st_video_advanced);
            }
#endif
            break;

            case VIDEO_ADVANCED_REFLECTION:
            if (oldRefl == val)
                return 1;

            backups = 1;
#if defined(__EMSCRIPTEN__) || NB_STEAM_API==1
            config_set_d(CONFIG_REFLECTION, val);
            goto_state(&st_restart_required);
#else
            config_set_d(CONFIG_REFLECTION, val);
            config_set_d(CONFIG_GRAPHIC_RESTORE_ID, 5);
            config_set_d(CONFIG_GRAPHIC_RESTORE_VAL1, val);
            config_save();
            goto_state(&st_null);

            r = video_mode(f, w, h);

            if (r)
                goto_state(&st_video_advanced);
            else
            {
                config_set_d(CONFIG_REFLECTION, oldRefl);
                r = video_mode(f, w, h);
                if (r)
                    goto_state(&st_video_advanced);
            }
#endif

            break;

        case VIDEO_ADVANCED_BACKGROUND:
            config_set_d(CONFIG_BACKGROUND, val);
            config_save();
            goto_state_full(&st_video_advanced, 0, 0, 1);
            break;

        case VIDEO_ADVANCED_SHADOW:
            config_set_d(CONFIG_SHADOW, val);
            config_save();
            goto_state_full(&st_video_advanced, 0, 0, 1);
            break;
#ifdef GL_GENERATE_MIPMAP_SGIS
        case VIDEO_ADVANCED_MIPMAP:
            config_set_d(CONFIG_MIPMAP, val);
            config_save();
            goto_state_full(&st_video_advanced, 0, 0, 1);
            break;
#endif

#ifdef GL_TEXTURE_MAX_ANISOTROPY_EXT
        case VIDEO_ADVANCED_ANISO:
            config_set_d(CONFIG_ANISO, val);
            config_save();
            goto_state_full(&st_video_advanced, 0, 0, 1);
            break;
#endif

        case VIDEO_ADVANCED_VSYNC:
            if (oldVsync == val)
                return 1;

            backups = 1;
#if defined(__EMSCRIPTEN__) || NB_STEAM_API==1
            config_set_d(CONFIG_VSYNC, val);
            goto_state(&st_restart_required);
#else
            config_set_d(CONFIG_VSYNC, val);
            config_set_d(CONFIG_GRAPHIC_RESTORE_ID, 3);
            config_set_d(CONFIG_GRAPHIC_RESTORE_VAL1, val);
            config_save();
            goto_state(&st_null);

            r = video_mode(f, w, h);

            if (r)
                goto_state(&st_video_advanced);
            else
            {
                config_set_d(CONFIG_VSYNC, oldVsync);
                r = video_mode(f, w, h);
                if (r)
                    goto_state(&st_video_advanced);
            }
#endif

            break;

        case VIDEO_ADVANCED_TEXTURES:
            if (oldText == val)
                return 1;

            backups = 1;
            config_set_d(CONFIG_TEXTURES, val);
            config_save();
            goto_state(&st_video_advanced);

            break;

        case VIDEO_ADVANCED_MULTISAMPLE:
            if (oldSamp == val)
                return 1;

            backups = 1;
#if defined(__EMSCRIPTEN__) || NB_STEAM_API==1
            config_set_d(CONFIG_MULTISAMPLE, val);
            goto_state(&st_restart_required);
#else
            config_set_d(CONFIG_MULTISAMPLE, val);
            config_set_d(CONFIG_GRAPHIC_RESTORE_ID, 4);
            config_set_d(CONFIG_GRAPHIC_RESTORE_VAL1, val);
            config_save();
            goto_state(&st_null);

            r = video_mode(f, w, h);
            if (r)
                goto_state(&st_video_advanced);
            else
            {
                config_set_d(CONFIG_MULTISAMPLE, oldSamp);
                r = video_mode(f, w, h);
                if (r)
                    goto_state(&st_video_advanced);
            }
#endif
            break;

        case VIDEO_ADVANCED_SMOOTH_FIX:
            config_set_d(CONFIG_SMOOTH_FIX, val);
            config_save();
            goto_state_full(&st_video_advanced, 0, 0, 1);
            break;

        case VIDEO_ADVANCED_FORCE_SMOOTH_FIX:
            config_set_d(CONFIG_FORCE_SMOOTH_FIX, val);
            config_save();
            goto_state_full(&st_video_advanced, 0, 0, 1);
            break;
    }

    if (r && backups)
    {
        config_set_d(CONFIG_GRAPHIC_RESTORE_ID, -1);
        config_set_d(CONFIG_GRAPHIC_RESTORE_VAL1, -1);
        config_save();
    }

    return r;
}

static int video_advanced_gui(void)
{
    static const struct conf_option multisample_opts_sixteen[] = {
        { N_("Off"), 0 },
        { N_("2x"),  2 },
        { N_("4x"),  4 },
        { N_("8x"),  8 },
        { N_("16x"), 16 },
    };

    static const struct conf_option multisample_opts_eight[] = {
        { N_("Off"), 0 },
        { N_("2x"),  2 },
        { N_("4x"),  4 },
        { N_("8x"),  8 },
    };

    static const struct conf_option multisample_opts_four[] = {
        { N_("Off"), 0 },
        { N_("2x"),  2 },
        { N_("4x"),  4 },
    };

    static const struct conf_option multisample_opts_two[] = {
        { N_("Off"), 0 },
        { N_("2x"),  2 },
    };

    int id, jd;

    if ((id = gui_vstack(0)))
    {
#ifdef SWITCHBALL_GUI
        /*
         * Build the common GUI
         *
         * In order: Resolution, render quality, texture quality, widescreen
         * point lights, bloom, motion blur, projections, distortions,
         * anti aliasing
         */

        conf_header(id, _("Graphics"), GUI_BACK);

        conf_toggle(id, _("Smooth fix"), VIDEO_ADVANCED_SMOOTH_FIX,
                        config_get_d(CONFIG_SMOOTH_FIX),
                        _("On"), 1, _("Off"), 0);

        conf_toggle(id, _("Force smooth fix"), VIDEO_ADVANCED_FORCE_SMOOTH_FIX,
                        config_get_d(CONFIG_FORCE_SMOOTH_FIX),
                        _("On"), 1, _("Off"), 0);

#ifdef GL_GENERATE_MIPMAP_SGIS
        conf_toggle(id, _("Mipmap"), VIDEO_ADVANCED_MIPMAP,
                        config_get_d(CONFIG_MIPMAP), _("On"), 1, _("Off"), 0);
#endif
#if defined(GL_TEXTURE_MAX_ANISOTROPY_EXT) && \
    GL_TEXTURE_MAX_ANISOTROPY_EXT != 0 && GL_TEXTURE_MAX_ANISOTROPY_EXT != -1
        conf_toggle(id, _("Anisotopic"), VIDEO_ADVANCED_ANISO,
                        config_get_d(CONFIG_ANISO),  _("On"), 2, _("Off"), 0);
#endif

        gui_space(id);

        conf_toggle(id, _("Reflection"),   VIDEO_ADVANCED_REFLECTION,
                        config_get_d(CONFIG_REFLECTION), _("On"), 1, _("Off"), 0);
        conf_toggle(id, _("Background"),   VIDEO_ADVANCED_BACKGROUND,
                        config_get_d(CONFIG_BACKGROUND), _("On"), 1, _("Off"), 0);
        conf_toggle(id, _("Shadow"),       VIDEO_ADVANCED_SHADOW,
                        config_get_d(CONFIG_SHADOW),     _("On"), 1, _("Off"), 0);

        gui_space(id);

        conf_select(id, _("Antialiasing"), VIDEO_ADVANCED_MULTISAMPLE,
                        config_get_d(CONFIG_MULTISAMPLE),
                        multisample_opts_sixteen,
                        ARRAYSIZE(multisample_opts_sixteen));
        conf_toggle(id, _("V-Sync"), VIDEO_ADVANCED_VSYNC,
                        config_get_d(CONFIG_VSYNC), _("On"), 1, _("Off"), 0);
#else
        char resolution[sizeof ("12345678 x 12345678")];
        const char *display;
        int dpy = config_get_d(CONFIG_DISPLAY);

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(resolution, MAXSTR,
#else
        sprintf(resolution,
#endif
                "%d x %d",
                video.window_w,
                video.window_h);

        
        if (!(display = SDL_GetDisplayName(dpy)))
            display = _("Unknown Display");
        conf_header(id, _("Graphics"), GUI_BACK);

        /* This ignores display configurations and resolutions. */
#ifndef __EMSCRIPTEN__
        if (SDL_GetNumVideoDisplays() > 1)
        {
            if ((jd = conf_state(id, _("Display"), "Super Longest Name",
                                     VIDEO_ADVANCED_DISPLAY)))
            {
                gui_set_trunc(jd, TRUNC_TAIL);
                gui_set_label(jd, display);
            }
        }

        conf_toggle(id, _("Fullscreen"), VIDEO_ADVANCED_FULLSCREEN,
                        config_get_d(CONFIG_FULLSCREEN), _("On"), 1, _("Off"), 0);
#endif
#ifndef RESIZEABLE_WINDOW
        if ((jd = conf_state(id, _("Resolution"), resolution,
            VIDEO_ADVANCED_RESOLUTION)))
        {
            /*
             * Because we always use the desktop display mode, disable
             * display mode switching in fullscreen.
             */

            if (config_get_d(CONFIG_FULLSCREEN))
            {
                gui_set_state(jd, GUI_NONE, 0);
                gui_set_color(jd, gui_gry, gui_gry);
            }
        }
#endif

#if ENABLE_HMD
        conf_toggle(id, _("HMD"), VIDEO_ADVANCED_HMD,
                        config_get_d(CONFIG_HMD), _("On"), 1, _("Off"), 0);
#endif

        conf_toggle(id, _("Smooth fix"), VIDEO_ADVANCED_SMOOTH_FIX,
                        config_get_d(CONFIG_SMOOTH_FIX), _("On"), 1, _("Off"), 0);

        gui_space(id);

        conf_toggle(id, _("V-Sync"), VIDEO_ADVANCED_VSYNC,
                        config_get_d(CONFIG_VSYNC), _("On"), 1, _("Off"), 0);

        conf_select(id, _("Antialiasing"), VIDEO_ADVANCED_MULTISAMPLE,
                        config_get_d(CONFIG_MULTISAMPLE),
                        multisample_opts_sixteen,
                        ARRAYSIZE(multisample_opts_sixteen));

        gui_space(id);

        conf_toggle(id, _("Reflection"), VIDEO_ADVANCED_REFLECTION,
                        config_get_d(CONFIG_REFLECTION), _("On"), 1, _("Off"), 0);
        conf_toggle(id, _("Background"), VIDEO_ADVANCED_BACKGROUND,
                        config_get_d(CONFIG_BACKGROUND), _("On"), 1, _("Off"), 0);
        conf_toggle(id, _("Shadow"), VIDEO_ADVANCED_SHADOW,
                        config_get_d(CONFIG_SHADOW), _("On"), 1, _("Off"), 0);

#ifdef GL_GENERATE_MIPMAP_SGIS
        conf_toggle(id, _("Mipmap"), VIDEO_ADVANCED_MIPMAP,
                        config_get_d(CONFIG_MIPMAP), _("On"), 1, _("Off"), 0);
#endif
#if defined(GL_TEXTURE_MAX_ANISOTROPY_EXT) && \
    GL_TEXTURE_MAX_ANISOTROPY_EXT != 0 && GL_TEXTURE_MAX_ANISOTROPY_EXT != -1
        conf_toggle(id, _("Anisotopic"), VIDEO_ADVANCED_ANISO,
                        config_get_d(CONFIG_ANISO), _("On"), 2, _("Off"), 0);
#endif

        conf_toggle(id, _("Textures"), VIDEO_ADVANCED_TEXTURES,
                        config_get_d(CONFIG_TEXTURES), _("High"), 1, _("Low"), 2);
#endif
        gui_layout(id, 0, 0);
    }

    return id;
}

static int video_advanced_enter(struct state *st, struct state *prev)
{
    if (!video_advanced_back)
        video_advanced_back = prev;

    conf_common_init(video_advanced_action, 1);
    return video_advanced_gui();
}

/*---------------------------------------------------------------------------*/

enum
{
    DISPLAY_SELECT = GUI_LAST
};

static struct state *display_back;

static int display_action(int tok, int val)
{
    int r = 1;

    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
            goto_state(display_back);
            display_back = NULL;
            break;

        case DISPLAY_SELECT:
            if (val != config_get_d(CONFIG_DISPLAY))
            {
                config_set_d(CONFIG_DISPLAY, val);
                video_set_display(val);
                goto_state_full(&st_display, 0, 0, 1);

                r = 1;
            }

        config_save();

        break;
    }

    return r;
}

static int display_gui(void)
{
    int id, jd;

    int i, n = SDL_GetNumVideoDisplays();

    if ((id = gui_vstack(0)))
    {
        conf_header(id, _("Display"), GUI_BACK);

#if _WIN32
        if (n > 1)
        {
            gui_multi(id,
                      _("Go to settings or press WIN+P\\"
                        "to change the multiple Displays."),
                      GUI_SML, gui_wht, gui_cya);

            gui_space(id);
        }
#endif

        for (i = 0; i < n; i++)
        {
            char name[MAXSTR];
            SDL_DisplayMode dpyMode;
            SDL_GetCurrentDisplayMode(i, &dpyMode);

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(name, MAXSTR,
#else
            sprintf(name,
#endif
                    "%d: %s\\%d x %d - %d Hz",
                    i + 1,
                    SDL_GetDisplayName(i),
                    dpyMode.w, dpyMode.h, dpyMode.refresh_rate);

            jd = gui_multi(id,
                           "XXXXXXXXXXXXXXXXXXX\\"
                           "XXXXXXXXXXXXXXXXXXX",
                           GUI_SML, gui_wht, gui_wht);
            gui_set_state (jd, DISPLAY_SELECT, i);
            gui_set_hilite(jd, (i == config_get_d(CONFIG_DISPLAY)));
            gui_set_trunc (jd, TRUNC_TAIL);
            gui_set_multi (jd, name);
        }

        gui_layout(id, 0, 0);
    }

    return id;
}

static int display_enter(struct state *st, struct state *prev)
{
    if (!display_back)
        display_back = prev;

    conf_common_init(display_action, 1);
    return display_gui();
}

/*---------------------------------------------------------------------------*/

#ifndef RESIZEABLE_WINDOW
struct mode
{
    int w;
    int h;
};

#if _WIN32
static const struct mode win32_modes[] = {
    { 3840, 2160 },
    { 2560, 1600 },
    { 2560, 1440 },
    { 2048, 1152 },
    { 1920, 1440 },
    { 1920, 1200 },
    { 1920, 1080 },
    { 1856, 1392 },
    { 1792, 1344 },
    { 1768, 992 },
    { 1680, 1200 },
    { 1680, 1050 },
    { 1600, 1200 },
    { 1600, 1024 },
    { 1600, 900 },
    { 1440, 1080 },
    { 1440, 1050 },
    { 1440, 900 },
    { 1366, 768 },
    { 1360, 768 },
    { 1280, 1024 },
    { 1280, 960 },
    { 1280, 800 },
    { 1280, 768 },
    { 1280, 720 },
    { 1280, 600 },
    { 1176, 664 },
    { 1152, 864 },
    { 1024, 768 },
    { 1024, 600 },
    { 800, 600 },
};
#else
static const struct mode modes[] = {
    { 3840, 2160 },
    { 2560, 1600 },
    { 2560, 1440 },
    { 2048, 1152 },
    { 1920, 1440 },
    { 1920, 1200 },
    { 1920, 1080 },
    { 1856, 1392 },
    { 1792, 1344 },
    { 1768, 992 },
    { 1680, 1200 },
    { 1680, 1050 },
    { 1600, 1200 },
    { 1600, 1024 },
    { 1600, 900 },
    { 1440, 1080 },
    { 1440, 1050 },
    { 1440, 900 },
    { 1366, 768 },
    { 1360, 768 },
    { 1280, 1024 },
    { 1280, 960 },
    { 1280, 800 },
    { 1280, 768 },
    { 1280, 720 },
    { 1280, 600 },
    { 1176, 664 },
    { 1152, 864 },
    { 1024, 768 },
    { 1024, 600 },
    { 848, 480 },
    { 800, 600 },
    { 640, 480 },
    { 480, 320 },
    { 320, 240 }
};
#endif

static struct mode customresol_modes[1024];

enum
{
    RESOL_MODE = GUI_LAST
};

static struct state *resol_back;

static int resol_action(int tok, int val)
{
    int r = 1;

    GENERIC_GAMEMENU_ACTION;

    int oldW = config_get_d(CONFIG_WIDTH);
    int oldH = config_get_d(CONFIG_HEIGHT);

    switch (tok)
    {
        case GUI_BACK:
            goto_state(resol_back);
            resol_back = NULL;
            break;

        case RESOL_MODE:
            if (customresol_modes[val].w == oldW &&
                customresol_modes[val].h == oldH) {
                log_printf("Resolutions remains the same size!\n");
                return 1;
            }

            video_set_window_size(customresol_modes[val].w,
                                  customresol_modes[val].h);
            r = 1;

            config_save();

            break;
    }

    return r;
}

static int resol_gui(void)
{
    int id, jd, kd;

    if ((id = gui_vstack(0)))
    {
        const int W = config_get_d(CONFIG_WIDTH);
        const int H = config_get_d(CONFIG_HEIGHT);

        int i = 0;
        int n = 0;

        SDL_DisplayMode custom_dm0;
        int curr_w = 0, curr_h = 0;
        memset(customresol_modes, 0, 1024);
        while (SDL_GetDisplayMode(video_display(), i, &custom_dm0) == 0)
        {
            if ((curr_w != custom_dm0.w || curr_h != custom_dm0.h)
#if _WIN32
                && custom_dm0.w >= 600 && custom_dm0.h >= 600
#endif
                )
            {
                curr_w = custom_dm0.w;
                curr_h = custom_dm0.h;
                customresol_modes[n].w = curr_w;
                customresol_modes[n].h = curr_h;
                n++;
            }

            i++;
        }

        char buff[sizeof ("1234567890 x 1234567890")] = "";

        conf_header(id, _("Resolution"), GUI_BACK);

        for (i = 0; i < n; i += 4)
        {
            if ((jd = gui_harray(id)))
            {
                for (int j = 3; j >= 0; j--)
                {
                    int m = i + j;

                    if (m < n)
                    {
                        SDL_DisplayMode dm;

#if !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                        sprintf_s(buff, MAXSTR,
#else
                        sprintf(buff,
#endif
                                "%d x %d",
                                customresol_modes[m].w, customresol_modes[m].h);

                        kd = gui_state(jd, buff, GUI_SML, RESOL_MODE, m);
                        gui_set_hilite(kd, (customresol_modes[m].w == W &&
                                            customresol_modes[m].h == H));

                        if (SDL_GetDesktopDisplayMode(video_display(), &dm) == 0)
                        {
                            if (dm.w < customresol_modes[m].w ||
                                dm.h < customresol_modes[m].h)
                            {
                                gui_set_color(kd, gui_gry, gui_gry);
                                gui_set_state(kd, GUI_NONE, m);
                            }
                        }
                    }
                    else
                        gui_space(jd);
                }
            }
        }

        if (n == 0)
            gui_label(id, _("Not available"), GUI_SML, 0, 0);

        gui_layout(id, 0, 0);
    }

    return id;
}

static int resol_enter(struct state *st, struct state *prev)
{
    if (!resol_back)
        resol_back = prev;

    conf_common_init(resol_action, 1);
    return resol_gui();
}

#endif

/*---------------------------------------------------------------------------*/

#define LANG_STEP 10

static Array langs;
static int   first;

enum
{
    LANG_DEFAULT = GUI_LAST,
    LANG_SELECT,
};

static struct state *lang_back;

static int lang_action(int tok, int val)
{
    int r = 1;

    struct lang_desc *desc;

    int total = 11;

    GAMEPAD_GAMEMENU_ACTION_SCROLL(GUI_PREV, GUI_NEXT, LANG_STEP);

    switch (tok)
    {
        case GUI_BACK:
            goto_state(lang_back);
            lang_back = NULL;
            break;

        case GUI_PREV:
            if (first > 1) {
                first -= LANG_STEP;
                return goto_state(&st_lang);
            }
            break;

        case GUI_NEXT:
            if (first + LANG_STEP < total)
            {
                first += LANG_STEP;
                return goto_state(&st_lang);
            }
            break;

        case LANG_DEFAULT:
            /* HACK: Reload resources to load the localized font. */
            goto_state(&st_null);
            config_set_s(CONFIG_LANGUAGE, "");
            lang_init();
            audio_play(_("snd/lang/preview.ogg"), 1.0f);
            goto_state(&st_lang);
            config_save();
            break;

        case LANG_SELECT:
            desc = LANG_GET(langs, val);
            goto_state(&st_null);
            config_set_s(CONFIG_LANGUAGE, desc->code);
            lang_init();
            audio_play(_("snd/lang/preview.ogg"), 1.0f);
            goto_state(&st_lang);
            config_save();
            break;
    }

    return r;
}

static int lang_gui(void)
{
    const int step = (first == 0 ? LANG_STEP - 1 : LANG_STEP);

    int id, jd;
    int i;

    if ((id = gui_vstack(0)))
    {
        if ((jd = gui_hstack(id)))
        {
            gui_label(jd, _("Language"), GUI_SML, 0, 0);
            gui_space(jd);
            gui_filler(jd);
            gui_navig(jd, array_len(langs), first, LANG_STEP);
        }

        gui_space(id);

        if (step < LANG_STEP)
        {
            int default_id;
            default_id = gui_state(id, _("Default"), GUI_SML, LANG_DEFAULT, 0);
            gui_set_hilite(default_id, !*config_get_s(CONFIG_LANGUAGE));
        }

        for (i = (step < LANG_STEP ? first : first - 1);
            i < (step < LANG_STEP ? first : first - 1) + step;
            i++)
        {
            if (i < array_len(langs))
            {
                struct lang_desc *desc = LANG_GET(langs, i);

                int lang_id;

                lang_id = gui_state(id, "XXXXXXXXXXXXXXXXXXXXXXXXXXXX",
                                         GUI_SML, LANG_SELECT, i);
                gui_set_label(lang_id,  " ");
                gui_set_trunc(lang_id, TRUNC_TAIL);

                gui_set_hilite(lang_id, (strcmp(config_get_s(CONFIG_LANGUAGE),
                                                desc->code) == 0));

                /* Set detailed locale informations. */

                char lang_infotext[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(lang_infotext, MAXSTR,
#else
                sprintf(lang_infotext,
#endif
                        "%s / %s", desc->name1, lang_name(desc));

                /* Set font and rebuild texture. */

                gui_set_font(lang_id, desc->font);
                gui_set_label(lang_id, lang_infotext);
            }
            else
                gui_label(id, " ", GUI_SML, 0, 0);
        }

        gui_layout(id, 0, 0);
    }

    return id;
}

static int lang_enter(struct state *st, struct state *prev)
{
    if (!langs)
    {
        langs = lang_dir_scan();
        first = 0;
    }

    if (!lang_back)
        lang_back = prev;

    conf_common_init(lang_action, 1);
    return lang_gui();
}

void lang_leave(struct state *st, struct state *next, int id)
{
    if (!(next == &st_lang || next == &st_null))
    {
        lang_dir_free(langs);
        langs = NULL;
    }

    conf_common_leave(st, next, id);
}

int lang_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return common_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return common_action(GUI_BACK, 0);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_L1, b) && first > 0)
            return common_action(GUI_PREV, 0);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_R1, b) &&
            first + 10 < array_len(langs))
            return common_action(GUI_NEXT, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static struct state *restart_required_back;

static int restart_required_action(int tok, int val)
{
    if (tok == GUI_BACK)
    {
        return goto_state(restart_required_back);
    }
    return 1;
}

static int restart_required_gui(void)
{
    int id;

    if ((id = gui_vstack(0)))
    {
#if defined(__EMSCRIPTEN__)
        gui_label(id, _("Reload required!"), GUI_MED, gui_red, gui_red);
#else
        gui_label(id, _("Restart required!"), GUI_MED, gui_red, gui_red);
#endif
        gui_space(id);
#if defined(__EMSCRIPTEN__)
        gui_multi(id, _("Please reload your page,\\to change this affects!"),
                      GUI_SML, gui_wht, gui_wht);
#else
        gui_multi(id, _("Please restart your game,\\to change this affects!"),
                      GUI_SML, gui_wht, gui_wht);
#endif
        gui_space(id);
        gui_state(id, _("OK"), GUI_SML, GUI_BACK, 0);
    }

    gui_layout(id, 0, 0);

    return id;
}

static int restart_required_enter(struct state *st, struct state *prev)
{
    if (!restart_required_back)
        restart_required_back = prev;

    conf_common_init(restart_required_action, 1);
    return restart_required_gui();
}

void restart_required_leave(struct state *st, struct state *next, int id)
{
    conf_common_leave(st, next, id);
}

/*---------------------------------------------------------------------------*/

static int loading_gui(void)
{
    int id = 0;

#if NB_HAVE_PB_BOTH!=1
    if ((id = gui_vstack(0)))
    {
        gui_label(id, _("Loading..."), GUI_SML, gui_wht, gui_wht);
        gui_layout(id, 0, 0);
    }
#endif

    return id;
}

static int loading_enter(struct state *st, struct state *prev)
{
#if NB_HAVE_PB_BOTH!=1
    back_init("back/space.png");
#endif
    return loading_gui();
}

static void loading_leave(struct state* st, struct state* next, int id)
{
    gui_delete(id);
}

static void loading_paint(int id, float t)
{
    conf_common_paint(id, t);

    gui_paint(id);
}

/*---------------------------------------------------------------------------*/

static int loading_gui(void)
{
    int id;

    if ((id = gui_vstack(0)))
    {
        gui_label(id, _("Loading..."), GUI_SML, gui_wht, gui_wht);
        gui_layout(id, 0, 0);
    }

    return id;
}

static int loading_enter(struct state *st, struct state *prev)
{
    return loading_gui();
}

static void loading_leave(struct state *st, struct state *next, int id)
{
    gui_delete(id);
}

static void loading_paint(int id, float t)
{
    gui_paint(id);
}

/*---------------------------------------------------------------------------*/

struct state st_video = {
    video_enter,
    conf_common_leave,
    conf_common_paint,
    common_timer,
    common_point,
    common_stick,
    NULL,
    common_click,
    common_keybd,
    common_buttn
};

struct state st_video_advanced = {
    video_advanced_enter,
    conf_common_leave,
    conf_common_paint,
    common_timer,
    common_point,
    common_stick,
    NULL,
    common_click,
    common_keybd,
    common_buttn
};

struct state st_display = {
    display_enter,
    conf_common_leave,
    conf_common_paint,
    common_timer,
    common_point,
    common_stick,
    NULL,
    common_click,
    common_keybd,
    common_buttn
};

#ifndef RESIZEABLE_WINDOW
struct state st_resol = {
    resol_enter,
    conf_common_leave,
    conf_common_paint,
    common_timer,
    common_point,
    common_stick,
    NULL,
    common_click,
    common_keybd,
    common_buttn
};
#endif

struct state st_lang = {
    lang_enter,
    lang_leave,
    conf_common_paint,
    common_timer,
    common_point,
    common_stick,
    NULL,
    common_click,
    common_keybd,
    lang_buttn
};

struct state st_restart_required = {
    restart_required_enter,
    restart_required_leave,
    conf_common_paint,
    common_timer,
    common_point,
    common_stick,
    NULL,
    common_click,
    common_keybd,
    common_buttn
};

struct state st_loading = {
    loading_enter,
    loading_leave,
    loading_paint
};

struct state st_loading = {
    loading_enter,
    loading_leave,
    loading_paint
};

/*---------------------------------------------------------------------------*/
