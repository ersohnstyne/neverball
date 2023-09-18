/*
 * Copyright (C) 2003 Robert Kooima
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

#if NB_STEAM_API==1
#if _MSC_VER || __GNUC__

#include "steam/steam_api.h"

#ifdef __EMSCRIPTEN__
#error Cannot compile website in Steam games!
#endif

#ifdef FS_VERSION_1
#error Steam OS implemented, but NO DLCs detected!
#endif

#elif !_MSC_VER
#error MinGW not supported! Use Visual Studio instead!
#endif
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#if _WIN32 && __MINGW32__
#include <SDL3/SDL.h>
#else
#include <SDL.h>
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
#if NB_HAVE_PB_BOTH==1
#include "networking.h"
#include "account.h"
#ifndef __EMSCRIPTEN__
#include "console_control_gui.h"
#endif
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
#include "dbg_config.h"
#include "glext.h"
#include "audio.h"
#include "image.h"
#include "state.h"
#include "config.h"
#include "video.h"
#include "mtrl.h"
#include "course.h"
#include "hole.h"
#include "game.h"
#include "gui.h"
#include "hmd.h"
#include "fs.h"
#include "joy.h"
#include "fetch.h"
#ifndef FS_VERSION_1
#include "package.h"
#endif

#include "st_conf.h"
#include "st_all.h"
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

// This fixes some malfunctions instead
#define SDL_EVENT_ANTI_MALFUNCTIONS(events) do { events.type = 0; } while (0)

/*---------------------------------------------------------------------------*/

static void shot(void)
{
    static char filename[MAXSTR];

    int secdecimal = roundf(config_screenshot() / 10000);

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf(filename, MAXSTR,
#else
    sprintf(filename,
#endif
            "Screenshots/screen_%04d-%04d.png", secdecimal, config_screenshot());
    video_snap(filename);
}

/*---------------------------------------------------------------------------*/

static void toggle_wire(void)
{
    video_toggle_wire();
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

/*
 * Custom SDL event code for fetch events.
 */
static Uint32 FETCH_EVENT = (Uint32)-1;

/*
 * Push a custom SDL event on the queue from another thread.
 */
static void dispatch_fetch_event(void* data)
{
    SDL_Event e;

    memset(&e, 0, sizeof(e));

    e.type = FETCH_EVENT;
    e.user.data1 = data;

    /* This is thread safe. */

    SDL_PushEvent(&e);
}

/*
 * Start the fetch thread.
 *
 * SDL must be initialized at this point for fetch event dispatch to work.
 */
static void initialize_fetch(void)
{
    /* Get a custom event code for fetch events. */
    FETCH_EVENT = SDL_RegisterEvents(1);

    /* Start the thread. */
    fetch_init(dispatch_fetch_event);
}

/*---------------------------------------------------------------------------*/

int st_global_loop(void)
{
    return loop();
}

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
    joy_update();
#endif

    int event_threshold = 0;

    while (d && SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
            return 0;

#if defined(__EMSCRIPTEN__)
        case SDL_USEREVENT:
            switch (e.user.code)
            {
            case USER_EVENT_BACK:
                d = st_keybd(KEY_EXIT, 1);
                break;

            case USER_EVENT_PAUSE:
//#if NDEBUG
                if (video_get_grab())
                    goto_pause(1);
//#endif
                break;
            }
            break;
#endif

        switch (e.type)
        {
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

            if (wireframe_splitview)
            {
                splitview_crossed = 0;
                if (ax > video.device_w / 2)
                    splitview_crossed = 1;
            }

            st_point(ax, ay, dx, dy);

            break;

        case SDL_MOUSEBUTTONDOWN:
            d = st_click(e.button.button, 1);
            break;

        case SDL_MOUSEBUTTONUP:
            d = st_click(e.button.button, 0);
            break;
#endif

        case SDL_FINGERDOWN:
        case SDL_FINGERUP:
        case SDL_FINGERMOTION:
            d = st_touch(&e.tfinger);
            break;

        case SDL_KEYDOWN:

            c = e.key.keysym.sym;

#ifdef __APPLE__
            if (c == SDLK_q && e.key.keysym.mod & KMOD_GUI)
            {
                d = 0;
                break;
            }
#endif
#ifdef _WIN32
            if (c == SDLK_F4 && e.key.keysym.mod & KMOD_ALT)
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
                config_save();
                goto_state_full(curr_state(), 0, 0, 1);
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
                if (config_tst_d(CONFIG_KEY_FORWARD, c))
                    st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_Y0), -1.0f);
                else if (config_tst_d(CONFIG_KEY_BACKWARD, c))
                    st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_Y0), +1.0f);
                else if (config_tst_d(CONFIG_KEY_LEFT, c))
                    st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_X0), -1.0f);
                else if (config_tst_d(CONFIG_KEY_RIGHT, c))
                    st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_X0), +1.0f);
                else
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
                if (config_tst_d(CONFIG_KEY_FORWARD, c))
                    st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_Y0), 0.0f);
                else if (config_tst_d(CONFIG_KEY_BACKWARD, c))
                    st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_Y0), 0.0f);
                else if (config_tst_d(CONFIG_KEY_LEFT, c))
                    st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_X0), 0.0f);
                else if (config_tst_d(CONFIG_KEY_RIGHT, c))
                    st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_X0), 0.0f);
                else
                    d = st_keybd(e.key.keysym.sym, 0);
            }
            break;

        case SDL_WINDOWEVENT:
            switch (e.window.event)
            {
            case SDL_WINDOWEVENT_FOCUS_LOST:
                audio_suspend();
//#if NDEBUG
                if (video_get_grab())
                    goto_pause(1);
//#endif
                break;

            case SDL_WINDOWEVENT_FOCUS_GAINED:
                audio_resume();
                break;

            case SDL_WINDOWEVENT_MOVED:
                if (config_get_d(CONFIG_DISPLAY) != video_display())
                {
                    config_set_d(CONFIG_DISPLAY, video_display());
                    goto_state_full(curr_state(), 0, 0, 1);
                }
                break;

            case SDL_WINDOWEVENT_RESIZED:
                video_clear();
                video_resize(e.window.data1, e.window.data2);
                gui_resize();
                break;

            case SDL_WINDOWEVENT_SIZE_CHANGED:
                video_clear();
                video_resize(e.window.data1, e.window.data2);
                gui_resize();
                config_save();
                goto_state_full(curr_state(), 0, 0, 1);
                break;

            case SDL_WINDOWEVENT_MAXIMIZED:
                config_set_d(CONFIG_MAXIMIZED, 1);
                gui_resize();
                config_save();
                goto_state_full(curr_state(), 0, 0, 1);

            case SDL_WINDOWEVENT_RESTORED:
                config_set_d(CONFIG_MAXIMIZED, 0);
                gui_resize();
                config_save();
                goto_state_full(curr_state(), 0, 0, 1);
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
            if (e.type == FETCH_EVENT)
                fetch_handle_event(e.user.data1);
            break;
        }

        event_threshold += 1;

        if (event_threshold > 3)
            SDL_EVENT_ANTI_MALFUNCTIONS(e);
    }
    return d;
}

/*---------------------------------------------------------------------------*/

static char *opt_data;
static char *opt_hole;

static void opt_parse(int argc, char **argv)
{
    int i;

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--data") == 0)
        {
            if (++i < argc)
                opt_data = argv[i];
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--hole") == 0)
        {
            if (++i < argc)
                opt_hole = argv[i];
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

/*---------------------------------------------------------------------------*/

static int em_cached_cam;

struct main_loop
{
    Uint32 now;
    unsigned int done : 1;
};

static void step(void* data)
{
    struct main_loop* mainloop = (struct main_loop*) data;

    int running = loop();

    if (running)
    {
        Uint32 now = SDL_GetTicks();
        Uint32 dt = (now - mainloop->now);

        if (0 < dt && dt < 100)
        {
            /* Step the game state. */
#define frame_smooth (1.f / 25.f) * 1000.f
            float deltaTime = config_get_d(CONFIG_SMOOTH_FIX) ? MIN(frame_smooth, dt) : MIN(100.f, dt);
#undef frame_smooth

            CHECK_GAMESPEED(20, 100);
            float speedPercent = (float) accessibility_get_d(ACCESSIBILITY_SLOWDOWN) / 100;
            st_timer((0.001f * deltaTime) * speedPercent);

            /* Render. */

            hmd_step();

            if (viewport_wireframe > 1)
            {
                video_render_fill_or_line(0);
                st_paint(0.001f * now, 1);
                video_render_fill_or_line(1);
                st_paint(0.001f * now, 0);
            }
            else
                st_paint(0.001f * now, 1);

            video_swap();
        }

        mainloop->now = now;
    }

    mainloop->done = !running;

#ifdef __EMSCRIPTEN__
    if (mainloop->done)
    {
        emscripten_cancel_main_loop();
        main_quit();
    }

    EM_ASM({
        Neverputt.quit();
    });
#endif
}

static int main_init(int argc, char* argv[])
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

    opt_parse(argc, argv);

    log_init("Neverputt", "neverputt.log");
    config_paths(opt_data);
    fs_mkdir("Screenshots");

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
#if NEVERBALL_FAMILY_API == NEVERBALL_SWITCH_FAMILY_API \
    && defined(SDL_HINT_JOYSTICK_HIDAPI_VERTICAL_JOY_CONS)
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_VERTICAL_JOY_CONS, "1");
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

    return 1;
}

static void main_quit()
{
    mtrl_quit();
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

#ifndef FS_VERSION_1
    package_quit();
    fetch_quit();
#endif

    audio_free();

    networking_quit();

    log_quit();
    fs_quit();

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

int main(int argc, char *argv[])
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

        initialize_fetch();

#ifndef FS_VERSION_1
        package_init();
#endif

#if NEVERBALL_FAMILY_API != NEVERBALL_PC_FAMILY_API || NB_PB_WITH_XBOX==1
        joy_init();
#endif

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

        /* Intitialize accessibility. */

        accessibility_init();
#if NB_STEAM_API==0 && NB_EOS_SDK==0
        accessibility_load();
#endif

        /* Initialize the audio. */

        audio_init();

        /* Initialize localization. */

        lang_init();

        /* Cache Neverball's camera setting. */

        em_cached_cam = config_get_d(CONFIG_CAMERA);

        /* Initialize the video. */

        if (video_init())
        {
            struct main_loop mainloop = { 0 };

            /* Material system. */

            mtrl_init();

            /* Run the main game loop. */

            mainloop.now = SDL_GetTicks();

            init_state(&st_null);

            if (opt_hole)
            {
                const char *path = fs_resolve(opt_hole);
                int loaded = 0;

                if (path)
                {
                    hole_init(NULL);

                    if (hole_load(0, path) &&
                        hole_load(1, path) &&
                        hole_goto(1, 1))
                    {
                        goto_state(&st_next);
                        loaded = 1;
                    }
                }

                if (!loaded)
                    goto_state(&st_title);
            }
            else
                goto_state(&st_title);

#ifdef __EMSCRIPTEN__
            emscripten_set_main_loop_arg(step, (void*) &mainloop, 0, 1);
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

