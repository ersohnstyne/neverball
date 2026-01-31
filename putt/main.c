/*
 * Copyright (C) 2025 Microsoft / Neverball authors / Jānis Rūcis
 *
 * NEVERPUTT is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */

/*
 * HACK: Remembering the code file differences:
 * Developers  who  programming  C++  can see more bedrock declaration
 * than C.  Developers  who  programming  C  can  see  few  procedural
 * declaration than  C++.  Keep  in  mind  when making  sure that your
 * extern code must associated. The valid file types are *.c and *.cpp,
 * so it's always best when making cross C++ compiler to keep both.
 * - Ersohn Styne
 */

/*---------------------------------------------------------------------------*/

#if _WIN32 && __MINGW32__
#include <SDL2/SDL.h>
#elif _WIN32 && _MSC_VER
#include <SDL.h>
#elif _WIN32
#error Security compilation error: No target include file in path for Windows specified!
#else
#include <SDL.h>
#endif

#if NB_STEAM_API==1
#if _MSC_VER || __GNUC__

#include <steam/steam_api.h>

#ifdef __EMSCRIPTEN__
#error Security compilation error: Cannot compile website in Steam games!
#endif

#ifdef FS_VERSION_1
#error Security compilation error: Steam OS implemented, but NO DLCs detected!
#endif

#elif _WIN32 && (!_MSC_VER || defined(__MINGW__))
#error Security compilation error: MinGW not supported! Use Visual Studio for Windows instead!
#elif _WIN64 && _MSC_VER < 1943
#error Security compilation error: Visual Studio 2022 requires MSVC 14.43.x and later version!
#elif _WIN32 && !_WIN64
#error Security compilation error: Game source code compilation requires x64 and not Win32!
#endif
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#if _MSC_VER
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")
#endif

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#if __cplusplus
extern "C" {
#endif

/*
 * HACK: Used with console version
 */
#include "console_control_gui.h"

#if NB_HAVE_PB_BOTH==1
#include "networking.h"
#include "account.h"
#endif

#include "accessibility.h"
#if __cplusplus
}
#endif

#ifndef VERSION
#include "version.h"
#endif
#if __cplusplus
extern "C" {
#endif
#if _WIN32 && _MSC_VER
#include "dbg_config.h"
#endif
#include "glext.h"
#include "audio.h"
#include "image.h"
#include "state.h"
#include "config.h"
#include "video.h"
#if ENABLE_DUALDISPLAY==1
#include "video_dualdisplay.h"
#endif
#include "mtrl.h"
#include "course.h"
#include "hole.h"
#include "game.h"
#include "gui.h"
#include "hmd.h"
#include "fs.h"
#include "joy.h"
#include "log.h"
#include "common.h"
#include "lang.h"
#include "key.h"
#include "fetch.h"
#include "package.h"
#include "strbuf/substr.h"
#include "strbuf/joinstr.h"

#if NB_HAVE_PB_BOTH==1
#include "st_setup.h"
#endif

#include "st_conf.h"
#include "st_all.h"
#include "st_common.h"
#include "st_package.h"

#include "main_share.h"

#if __cplusplus
}
#endif

#if NB_STEAM_API==1
const char TITLE[] = "Neverputt - Steam";
#elif NB_EOS_SDK==1
const char TITLE[] = "Neverputt - Epic Games";
#else
const char TITLE[] = "Neverputt";
#endif
const char ICON[] = "icon/neverputt.png";

/* This fixes some malfunctions instead */
#define SDL_EVENT_ANTI_MALFUNCTIONS(events) do { events.type = 0; } while (0)

/*---------------------------------------------------------------------------*/

static List opt_data_multi = NULL;

static char *opt_hole;
static char *opt_link;

static void opt_init(int argc, char **argv)
{
    int i;

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--data") == 0)
        {
            if (i++ < argc)
            {
                opt_data_multi = list_cons(strdup(argv[i]), opt_data_multi);
                log_printf("Added data path to opt_data_multi: %s\n", argv[i]);
            }
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--hole") == 0)
        {
            if (i++ < argc)
                opt_hole = argv[i];
        }
        else if (strcmp(argv[i], "--link") == 0)
        {
            if (i++ < argc)
                opt_link = argv[i];
        }
    }

    if (argc == 2)
    {
        size_t len = strlen(argv[1]);

        if (len > 4)
        {
            char *ext = argv[1] + len - 4;

            if (strcmp(ext, ".map") == 0)
                strcpy(ext, ".sol");

            if (strcmp(ext, ".sol") == 0)
                opt_hole = argv[1];
        }
    }
}

static void opt_quit(void)
{
    while (opt_data_multi)
    {
        free(opt_data_multi->data);
        opt_data_multi->data = NULL;
        opt_data_multi = list_rest(opt_data_multi);
    }

    opt_hole = NULL;
    opt_link = NULL;
}

/*---------------------------------------------------------------------------*/

static void shot(void)
{
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
#if NB_HAVE_PB_BOTH==1
    if (game_setup_process())
        return;
#endif

#if NB_STEAM_API==0
    char filename_primary[MAXSTR];

    const int decimal    = config_screenshot();
    const int secdecimal = ROUND(decimal / 10000);

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(filename_primary, MAXSTR,
#else
    sprintf(filename_primary,
#endif
            "Screenshots/screen_%04d-%04d.png",
            secdecimal, decimal);
    video_snap(filename_primary);

#if ENABLE_DUALDISPLAY==1
    char filename_secondary[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(filename_secondary, MAXSTR,
#else
    sprintf(filename_secondary,
#endif
            "Screenshots/screen_%04d-%04d_secondary.png",
            secdecimal, decimal);

    video_dualdisplay_snap(filename_secondary);
#endif
#endif
#endif
}

/*---------------------------------------------------------------------------*/

static void toggle_wire(void)
{
    glToggleWireframe_();
}

/*---------------------------------------------------------------------------*/

#if defined(__EMSCRIPTEN__)
enum {
    USER_EVENT_BACK = -1,
    USER_EVENT_PAUSE = 0
};

void EMSCRIPTEN_KEEPALIVE push_user_event(int code)
{
    SDL_Event event = { SDL_USEREVENT };
    event.user.code = code;
    SDL_PushEvent(&event);
}
#endif

/*---------------------------------------------------------------------------*/

static int goto_hole(const char *hole)
{
    const char *path = fs_resolve(hole);
    int loaded = 0;

    if (path)
    {
        hole_init(NULL);

        if (hole_load(0, path) &&
            hole_load(1, path) &&
            hole_goto(1, 1))
            return 1;
    }

    return 0;
}

/*---------------------------------------------------------------------------*/

/*
 * Handle the link option.
 *
 * This navigates to the appropriate screen, if the asset was found.
 */
static int link_handle(const char *link)
{
    if (!(link && *link)) return 0;

    int processed = 0;

    log_printf("Link: handling %s\n", link);

    if (str_starts_with(link, "holes-"))
    {
        /* Search installed sets and package list. */

        const size_t prefix_len = strcspn(link, "/");

        const char *set_part = SUBSTR(link, 0, prefix_len);
        const char *map_part = SUBSTR(link, prefix_len + 1, 64);
        const char *set_file = JOINSTR(set_part, ".txt");

        int index;
        int found_level = 0;

        log_printf("Link: searching for course %s\n", set_file);

        course_init();

        if ((index = set_find(set_file)) >= 0)
        {
            log_printf("Link: found course match for %s\n", set_file);

            course_goto(index);

            /*if (map_part && *map_part)
            {
                const char *sol_basename = JOINSTR(map_part, ".sol");
                struct level *level;

                log_printf("Link: searching for hole %s\n", sol_basename);

                if ((level = set_find_level(sol_basename)))
                {
                    log_printf("Link: found hole match for %s\n", sol_basename);

                    goto_state(&st_next);
                    found_level = 1;
                    processed = 1;
                }
                else log_errorf("Link: no such hole\n");
            }*/

            if (!found_level)
            {
                game_kill_fade();
                goto_state(&st_next);
                processed = 1;
            }
        }
        else if ((index = package_search(set_file)) >= 0)
        {
            log_printf("Link: found package match for %s\n", set_file);
            goto_package(index, &st_title);
            processed = 1;
        }
        else log_errorf("Link: no such course or package\n", link);
    }

    return processed;
}

/*---------------------------------------------------------------------------*/

static void refresh_packages_done(void *data, void *extra_data)
{
    struct state *start_state = (struct state *) data;

    if (opt_link && link_handle(opt_link))
        return;

#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    if (!check_game_setup())
    {
        goto_game_setup(start_state, 0, 0, 0);
        return;
    }
#endif

    goto_state(start_state);
}

/*
 * Start package refresh and go to given state when done.
 */
static void main_preload(struct state *start_state, int (*start_fn)(struct state *))
{
    struct fetch_callback callback = { 0 };

    callback.data = start_state;
    callback.done = refresh_packages_done;

    goto_state(&st_loading);

    /* Link processing works best with a package list. */

    if (package_refresh(callback))
    {
        /* Callback takes care of link processing and starting screen. */
        return;
    }

    /* But attempt it even without a package list. */

    if (opt_link && link_handle(opt_link))
    {
        /* Link processing navigates to the appropriate screen. */
        return;
    }

    /* Otherwise, go to the starting screen. */

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    if (!check_game_setup())
    {
        goto_game_setup(start_state, start_fn, 0, 0);
        return;
    }
#endif

    if (start_fn)
        start_fn(start_state);
    else
        goto_state(start_state);
}

/*---------------------------------------------------------------------------*/

static int loop(void);
int st_global_loop(void)
{
    return loop();
}

static struct SDL_TouchFingerEvent opt_touch_event;

static int opt_touch_anchor = 0;

static int loop(void)
{
    SDL_Event e;
    int d = 1;
    int c;

    int ax, ay, dx, dy;

#ifdef __EMSCRIPTEN__
    double clientWidth, clientHeight;

    int w, h;

    emscripten_get_element_css_size("#canvas", &clientWidth, &clientHeight);

    w = (int) clientWidth;
    h = (int) clientHeight;

    if (w != video.window_w || h != video.window_h)
        video_set_window_size(w, h);
#endif

#if NB_PB_WITH_XBOX==1
    d = joy_update();
#endif

    /* Process SDL events. */

    int event_threshold = 0;

    while (d && SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
        {
            d = 0;
            break;
        }

        if (opt_touch)
        {
            switch (e.type)
            {
                case SDL_MOUSEMOTION:
                    /* Convert to bottom left origin. */

                    ax = +e.motion.x;
                    ay = -e.motion.y + video.window_h;
                    dx = +e.motion.xrel;
                    dy = (config_get_d(CONFIG_MOUSE_INVERT) ?
                         +e.motion.yrel : -e.motion.yrel);

                    /* Scale to viewport pixels. */

                    ax = ROUND(ax * video.device_scale);
                    ay = ROUND(ay * video.device_scale);
                    dx = ROUND(dx * video.device_scale);
                    dy = ROUND(dy * video.device_scale);

                    opt_touch_event.type = SDL_FINGERMOTION;
                    opt_touch_event.x    =         (((float) ax) / video.device_w);
                    opt_touch_event.y    = (1.0f - (((float) ay) / video.device_h));

                    if (opt_touch_anchor)
                    {
                        opt_touch_event.dx += ((float) +dx) / video.device_w;
                        opt_touch_event.dy += ((float) -dy) / video.device_h;

                        opt_touch_event.dx = CLAMP(-1, opt_touch_event.dx, +1);
                        opt_touch_event.dy = CLAMP(-1, opt_touch_event.dy, +1);
                    }

                    if (config_get_d(CONFIG_MOUSE_INVERT))
                        opt_touch_event.dy *= -1;

                break;

                case SDL_MOUSEBUTTONDOWN:
                    opt_touch_anchor = 1;
                    opt_touch_event.type = SDL_FINGERDOWN;
                    break;

                case SDL_MOUSEBUTTONUP:
                    opt_touch_anchor     = 0;
                    opt_touch_event.dx   = 0;
                    opt_touch_event.dy   = 0;
                    opt_touch_event.type = SDL_FINGERUP;
                    break;
            }
        }

        switch (e.type)
        {
#if defined(__EMSCRIPTEN__)
            case SDL_USEREVENT:
                switch (e.user.code)
                {
                    case USER_EVENT_BACK:
                        d = st_keybd(KEY_EXIT, 1);
                        break;

                    case USER_EVENT_PAUSE:
                        if (video_get_grab())
                            goto_pause(1);
                        break;
                }
                break;
#endif
#if NEVERBALL_FAMILY_API == NEVERBALL_PC_FAMILY_API
            case SDL_MOUSEMOTION:
                /* Convert to OpenGL coordinates. */

                ax = +e.motion.x;
                ay = -e.motion.y + video.window_h;
                dx = +e.motion.xrel;
                dy = (config_get_d(CONFIG_MOUSE_INVERT) ?
                      +e.motion.yrel : -e.motion.yrel);

                /* Scale to viewport pixels. */

                ax = ROUND(ax * video.device_scale);
                ay = ROUND(ay * video.device_scale);
                dx = ROUND(dx * video.device_scale);
                dy = ROUND(dy * video.device_scale);

                if (opt_touch)       st_touch(&opt_touch_event);
                else if (!opt_touch) st_point(ax, ay, dx, dy);

                break;

            case SDL_MOUSEBUTTONDOWN:
                if (opt_touch)       d = st_touch(&opt_touch_event);
                else if (!opt_touch) d = st_click(e.button.button, 1);
                break;

            case SDL_MOUSEBUTTONUP:
                if (opt_touch)       d = st_touch(&opt_touch_event);
                else if (!opt_touch) d = st_click(e.button.button, 0);
                break;
#endif

            case SDL_FINGERDOWN:
            case SDL_FINGERUP:
            case SDL_FINGERMOTION:
                d = st_touch(&e.tfinger);
                break;

            case SDL_KEYDOWN:
                c = e.key.keysym.sym;

#ifndef __EMSCRIPTEN__
#ifdef __APPLE__
                if (c == SDLK_q  && e.key.keysym.mod & KMOD_GUI)
#endif
#ifdef _WIN32
                if (c == SDLK_F4 && e.key.keysym.mod & KMOD_ALT)
#endif
                {
                    d = 0;
                    break;
                }
#endif

                switch (c)
                {
#if !defined(STEAM_GAMES)
                    case KEY_SCREENSHOT:
                        shot();
                        break;
                    case KEY_FPS:
                        config_tgl_d(CONFIG_FPS);
                        break;
                    case KEY_WIREFRAME:
                        toggle_wire();
                        break;
                    case KEY_FULLSCREEN:
                        video_fullscreen(!config_get_d(CONFIG_FULLSCREEN));
#if ENABLE_DUALDISPLAY==1
                        video_dualdisplay_fullscreen(config_get_d(CONFIG_FULLSCREEN));
#endif
                        config_save();
                        goto_state(curr_state());
                        break;
#endif
                    case SDLK_RETURN:
                        d = st_buttn(config_get_d(CONFIG_JOYSTICK_BUTTON_A), 1);
                        break;
                    case SDLK_ESCAPE:
                        if (video_get_grab())
                            d = st_buttn(config_get_d(CONFIG_JOYSTICK_BUTTON_START), 1);
                        else
                            d = st_buttn(config_get_d(CONFIG_JOYSTICK_BUTTON_B), 1);
                    break;

                    default:
#if NEVERBALL_FAMILY_API == NEVERBALL_PC_FAMILY_API
                    if (config_tst_d(CONFIG_KEY_FORWARD, c))
                        st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_Y0), -1.0f);
                    else if (config_tst_d(CONFIG_KEY_BACKWARD, c))
                        st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_Y0), +1.0f);
                    else if (config_tst_d(CONFIG_KEY_LEFT, c))
                        st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_X0), -1.0f);
                    else if (config_tst_d(CONFIG_KEY_RIGHT, c))
                        st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_X0), +1.0f);
                    else
#endif
                        d = st_keybd(e.key.keysym.sym, 1);
                }
                break;

            case SDL_KEYUP:
                c = e.key.keysym.sym;

                switch (c)
                {
                    case SDLK_RETURN:
                        d = st_buttn(config_get_d(CONFIG_JOYSTICK_BUTTON_A), 0);
                        break;
                    case SDLK_ESCAPE:
                        if (video_get_grab())
                            d = st_buttn(config_get_d(CONFIG_JOYSTICK_BUTTON_START), 0);
                        else
                            d = st_buttn(config_get_d(CONFIG_JOYSTICK_BUTTON_B), 0);
                        break;

                    default:
#if NEVERBALL_FAMILY_API == NEVERBALL_PC_FAMILY_API
                        if (config_tst_d(CONFIG_KEY_FORWARD, c))
                            st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_Y0), 0.0f);
                        else if (config_tst_d(CONFIG_KEY_BACKWARD, c))
                            st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_Y0), 0.0f);
                        else if (config_tst_d(CONFIG_KEY_LEFT, c))
                            st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_X0), 0.0f);
                        else if (config_tst_d(CONFIG_KEY_RIGHT, c))
                            st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_X0), 0.0f);
                        else
#endif
                            d = st_keybd(e.key.keysym.sym, 0);
                }
                break;

            case SDL_WINDOWEVENT:
                switch (e.window.event)
                {
                    case SDL_WINDOWEVENT_FOCUS_LOST:
                        audio_suspend();
                        if (video_get_grab())
                            goto_pause(1);
                        break;

                    case SDL_WINDOWEVENT_FOCUS_GAINED:
                        audio_resume();
                        break;

                    case SDL_WINDOWEVENT_MOVED:
                        if (config_get_d(CONFIG_DISPLAY) != video_display())
                        {
                            config_set_d(CONFIG_DISPLAY, video_display());
                            goto_state(curr_state());
                        }
                        break;

                    case SDL_WINDOWEVENT_RESIZED:
                        video_clear();
                        video_resize(e.window.data1, e.window.data2);
#if ENABLE_DUALDISPLAY==1
                        video_dualdisplay_resize(e.window.data1, e.window.data2);
                        video_set_window_size(e.window.data1, e.window.data2);
                        video_dualdisplay_set_window_size(e.window.data1, e.window.data2);
#endif
                        gui_resize();
                        break;

                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                        video_clear();
                        video_resize(e.window.data1, e.window.data2);
#if ENABLE_DUALDISPLAY==1
                        video_dualdisplay_resize(e.window.data1, e.window.data2);
                        video_set_window_size(e.window.data1, e.window.data2);
                        video_dualdisplay_set_window_size(e.window.data1, e.window.data2);
#endif
                        gui_resize();
                        config_save();
                        goto_state(curr_state());
                        break;

                    case SDL_WINDOWEVENT_MAXIMIZED:
                        config_set_d(CONFIG_MAXIMIZED, 1);
                        gui_resize();
                        config_save();
                        goto_state(curr_state());
                        break;

                    case SDL_WINDOWEVENT_RESTORED:
                        config_set_d(CONFIG_MAXIMIZED, 0);
                        gui_resize();
                        config_save();
                        goto_state(curr_state());
                        break;
                }
                break;

#if NEVERBALL_FAMILY_API != NEVERBALL_PC_FAMILY_API && NB_PB_WITH_XBOX==0
            case SDL_JOYAXISMOTION:
                joy_axis(e.jaxis.which, e.jaxis.axis, JOY_VALUE(e.jaxis.value));
                break;

            case SDL_JOYBUTTONDOWN:
                d = joy_button(e.jbutton.which, e.jbutton.button, 1);
                break;

            case SDL_JOYBUTTONUP:
                joy_button(e.jbutton.which, e.jbutton.button, 0);
                break;

            case SDL_JOYDEVICEADDED:
                joy_add(e.jdevice.which);
                break;

            case SDL_JOYDEVICEREMOVED:
                joy_remove(e.jdevice.which);
                break;
#endif

            default:
#if ENABLE_FETCH!=0 && !defined(__EMSCRIPTEN__)
                if (e.type == FETCH_EVENT)
                    fetch_handle_event(e.user.data1);
#endif
                break;
        }

        event_threshold += 1;

        if (event_threshold > 3)
            SDL_EVENT_ANTI_MALFUNCTIONS(e);
    }

    return d;
}

/*---------------------------------------------------------------------------*/

static int em_cached_cam;

struct main_loop
{
    Uint32 now;

    Uint32 motionblur_leftover;

    unsigned int done : 1;
};

#define frame_smooth (1.f / 25.f) * 1000.f

static void step_primary_screen(Uint32 now, Uint32 dt, int allow_clear)
{
    if (0 < dt && dt < 1000)
    {
        /* Step the game state. */

        float deltaSecond = config_get_d(CONFIG_SMOOTH_FIX) ?
                            MIN(frame_smooth, dt) : MIN(100.0f, dt);

        CHECK_GAMESPEED(20, 100);
        float speedPercent = (float) accessibility_get_d(ACCESSIBILITY_SLOWDOWN) / 100;
        st_timer(MAX((0.001f * deltaSecond) * speedPercent, 0));

        hmd_step();
    }

    /* Render. */

    st_paint(0.001f * now, 1);
}

static void step(void *data)
{
    struct main_loop* mainloop = (struct main_loop *) data;

    int running = loop();

    if (running)
    {
        Uint32 now = SDL_GetTicks();
        Uint32 dt  = (now - mainloop->now);

#if ENABLE_DUALDISPLAY==1
        video_set_current();
#endif

#if ENABLE_MOTIONBLUR!=0
        if (config_get_d(CONFIG_MOTIONBLUR))
        {
            /* TODO: Do we have 90 FPS? */

            int first_frame = 1;

            const float expected_dt = (1.0f / 90) * 1000.f;
            mainloop->motionblur_leftover += config_get_d(CONFIG_SMOOTH_FIX) ?
                                             MIN(frame_smooth, dt) : MIN(100.0f, dt);

            while (mainloop->motionblur_leftover > expected_dt)
            {
                const float blur_alpha = mainloop->motionblur_leftover > expected_dt ?
                                         ((float) (expected_dt / mainloop->motionblur_leftover)) :
                                         1.0f;

                video_motionblur_alpha_set(CLAMP(0, blur_alpha, 1));

                Uint32 fixed_dt = MIN(mainloop->motionblur_leftover, expected_dt);

                step_primary_screen(mainloop->now + (dt - mainloop->motionblur_leftover),
                                    fixed_dt, first_frame);

                mainloop->motionblur_leftover -= expected_dt;

                first_frame = 0;
            }

            video_motionblur_alpha_set(1);

            step_primary_screen(mainloop->now + (dt - mainloop->motionblur_leftover),
                                mainloop->motionblur_leftover, 0);

            mainloop->motionblur_leftover = 0;
        }
        else
        {
            video_motionblur_alpha_set(1);
#endif

            step_primary_screen(now, dt, 1);

#if ENABLE_MOTIONBLUR!=0
        }
#endif
        if (curr_state() == &st_null && time_state() >= 3.0f) video_clear();
        video_swap();

#if ENABLE_DUALDISPLAY==1
        video_dualdisplay_set_current();
        video_dualdisplay_clear();

        video_dualdisplay_swap();
#endif

        mainloop->now = now;

        if (config_get_d(CONFIG_NICE))
            SDL_Delay((1.0f / 30.0f) * 1000);
    }

    mainloop->done = !running;

#ifdef __EMSCRIPTEN__
    if (mainloop->done)
    {
        emscripten_cancel_main_loop();
        main_quit();

        EM_ASM({
            Neverputt.quit();
        });
    }
#endif
}

static int main_init(int argc, char *argv[])
{
    GAMEDBG_SIGFUNC_PREPARE;

#if NB_STEAM_API==1
    if (!SteamAPI_Init())
    {
        log_errorf("SteamAPI_Init() failed! Steam must be running to play this game.\n");
        return 0;
    }
#endif

    if (!fs_init(argc > 0 ? argv[0] : NULL))
    {
        fprintf(stderr, "Failure to initialize file system (%s)\n",
                        fs_error());
        return 0;
    }

    srand((int) time(NULL));

    opt_init(argc, argv);

    if (opt_data_multi)
        for (List p = opt_data_multi; p; p = p->next)
            fs_add_path_with_archives((const char*)p->data);

    config_paths(NULL);

    log_init("Neverputt " VERSION, "neverputt.log");
#if NB_HAVE_PB_BOTH!=1
    fs_mkdir("Screenshots");
#endif

#ifdef SDL_HINT_ENABLE_SCREEN_KEYBOARD
    SDL_SetHint(SDL_HINT_ENABLE_SCREEN_KEYBOARD, "0");
#endif

#ifdef SDL_HINT_TOUCH_MOUSE_EVENTS
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
#endif

#if defined(__EMSCRIPTEN__)
    SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, "#canvas");
#else
#if NEVERBALL_FAMILY_API == NEVERBALL_XBOX_FAMILY_API \
    && defined(SDL_HINT_JOYSTICK_HIDAPI_XBOX)
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_XBOX, "1");
#endif
#if NEVERBALL_FAMILY_API == NEVERBALL_XBOX_360_FAMILY_API \
    && defined(SDL_HINT_JOYSTICK_HIDAPI_XBOX_360) && defined(SDL_HINT_JOYSTICK_HIDAPI_XBOX_360_PLAYER_LED)
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_XBOX_360, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_XBOX_360_PLAYER_LED, "1");
#endif
#if NEVERBALL_FAMILY_API == NEVERBALL_PS_FAMILY_API
#if defined(SDL_HINT_JOYSTICK_HIDAPI_PS5) && defined(SDL_HINT_JOYSTICK_HIDAPI_PS5_PLAYER_LED)
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5_PLAYER_LED, "1");
#elif defined(SDL_HINT_JOYSTICK_HIDAPI_PS4)
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS4, "1");
#elif defined(SDL_HINT_JOYSTICK_HIDAPI_PS3)
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS3, "1");
#else
#error No Playstation HIDAPI specified!
#endif
#endif
#if NEVERBALL_FAMILY_API == NEVERBALL_SWITCH_FAMILY_API
#if defined(SDL_HINT_JOYSTICK_HIDAPI_JOY_CONS)
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_JOY_CONS, "1");
#endif
#if defined(SDL_HINT_JOYSTICK_HIDAPI_VERTICAL_JOY_CONS)
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_VERTICAL_JOY_CONS, "1");
#endif
#if defined(SDL_HINT_JOYSTICK_HIDAPI_COMBINE_JOY_CONS)
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_COMBINE_JOY_CONS, "1");
#endif
#endif
#endif

#if _cplusplus
    try {
#endif
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == -1)
    {
        log_errorf("Failure to initialize SDL (%s)\n", GAMEDBG_GETSTRERROR_CHOICES_SDL);
        return 0;
    }
#if _cplusplus
    } catch (const char *xS) {
        log_errorf("Failure to initialize SDL: Exception caught! (%s - %s)\n", GAMEDBG_GETSTRERROR_CHOICES_SDL, xS);
        return 0;
    } catch (...) {
        log_errorf("Failure to initialize SDL: Exception caught! (%s)\n", GAMEDBG_GETSTRERROR_CHOICES_SDL);
        return 0;
    }
#endif

#ifndef NDEBUG
    SDL_LogSetPriority(SDL_LOG_CATEGORY_ERROR, SDL_LOG_PRIORITY_DEBUG);
#endif

    return 1;
}

static void main_quit()
{
    goto_state(&st_null);

    mtrl_quit();
#if ENABLE_DUALDISPLAY==1
    video_dualdisplay_quit();
#endif
    video_quit();

    /* Restore Neverball's camera setting. */

    config_set_d(CONFIG_CAMERA, em_cached_cam);
    config_save();

    accessibility_save();
#ifdef CONFIG_INCLUDES_ACCOUNT
    account_save();
#endif

    lang_quit();

#if NEVERBALL_FAMILY_API != NEVERBALL_PC_FAMILY_API || NB_PB_WITH_XBOX==1
    joy_quit();
#endif

    config_quit();

    package_quit();
    fetch_quit();

    audio_free();

    networking_quit();

    log_quit();
    fs_quit ();
    opt_quit();

#if _cplusplus
    try {
#endif
        SDL_Quit();
#if _cplusplus
    } catch (...) {}
#endif

#if NB_STEAM_API==1
    /* We're done here */
    SteamAPI_Shutdown();
#endif

#if _WIN32 && _MSC_VER && _DEBUG && defined(_CRTDBG_MAP_ALLOC)
    _CrtDumpMemoryLeaks();
#endif
}

#ifdef __cplusplus
extern "C"
#endif
int main_share(int argc, char *argv[])
{
    int retval = 0;

#if _cplusplus
    try {
#endif
    if (main_init(argc, argv))
    {
        config_init();
        config_load();

#if NB_HAVE_PB_BOTH==1
        /* Initialize account. */

        int account_res = account_init();

        if (account_res == 0)
        {
#if NB_STEAM_API==1
            log_errorf("Steam user is not logged in! Steam user must be logged in to play this game (SteamUser()->BLoggedOn() returned false).\n");
            return 1;
#elif NB_EOS_SDK==1
            return 1;
#endif
        }

        account_load();
#endif

        networking_init(0);

#if ENABLE_FETCH!=0
        initialize_fetch();
#endif

        package_init();

#if NEVERBALL_FAMILY_API != NEVERBALL_PC_FAMILY_API || NB_PB_WITH_XBOX==1
        joy_init();
#endif

#if NB_HAE_PB_BOTH==1
#if NEVERBALL_FAMILY_API == NEVERBALL_PC_FAMILY_API
        init_controller_type(PLATFORM_PC);
#endif
#if NEVERBALL_FAMILY_API == NEVERBALL_XBOX_FAMILY_API
        init_controller_type(PLATFORM_XBOX);
        config_set_d(CONFIG_JOYSTICK, 1);
        config_save();
#endif
#if NEVERBALL_FAMILY_API == NEVERBALL_XBOX_360_FAMILY_API
        init_controller_type(PLATFORM_XBOX);
        config_set_d(CONFIG_JOYSTICK, 1);
        config_save();
#endif
#if NEVERBALL_FAMILY_API == NEVERBALL_PS_FAMILY_API
        init_controller_type(PLATFORM_PS);
        config_set_d(CONFIG_JOYSTICK, 1);
        config_save();
#endif
#if NEVERBALL_FAMILY_API == NEVERBALL_SWITCH_FAMILY_API
        init_controller_type(PLATFORM_SWITCH);
        config_set_d(CONFIG_JOYSTICK, 1);
        config_save();
#endif
#if NEVERBALL_FAMILY_API == NEVERBALL_HANDSET_FAMILY_API
        init_controller_type(PLATFORM_HANDSET);
        config_set_d(CONFIG_JOYSTICK, 1);
        config_save();
#endif
#endif

        /* Intitialize accessibility. */

        accessibility_init();
#if NB_STEAM_API==0 && NB_EOS_SDK==0
        accessibility_load();
#endif

        /* Initialize the audio. */

        audio_init();

#if ENABLE_NLS==1 || _MSC_VER
        /* Initialize localization. */

        lang_init();
#endif

        /* Cache Neverball's camera setting. */

        em_cached_cam = config_get_d(CONFIG_CAMERA);

        /* Initialize the video. */

        if (video_init())
        {
#if ENABLE_DUALDISPLAY==1
            video_dualdisplay_init();
#endif

            struct main_loop mainloop = { 0 };

            struct state *start_state = &st_title;

            /* Material system. */

            mtrl_init();

            /* Screen states. */

            init_state(&st_null);

            start_state = goto_hole(opt_hole) ? &st_next : &st_title;

            /* Run the main game loop. */

            mainloop.now = SDL_GetTicks();

            main_preload(start_state, 0);

#ifdef __EMSCRIPTEN__
            emscripten_set_main_loop_arg(step, (void *) &mainloop, 0, 1);
#else
            while (!mainloop.done)
                step(&mainloop);
#endif
        }
        else
            retval = 1;
    }
    else
    {
        log_errorf("Failure to initialize SDL (%s)\n", GAMEDBG_GETSTRERROR_CHOICES_SDL);
        retval = 1;
    }
#if _cplusplus
    } catch (...) { return 1; }
#endif

    return retval;
}

/*---------------------------------------------------------------------------*/
