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

#define DISABLE_PANORAMA

#if _WIN32 && __MINGW32__
#include <SDL2/SDL.h>
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

#elif _WIN32 && !_MSC_VER
#error Security compilation error: MinGW not supported! Use Visual Studio instead!
#elif _WIN64 && _MSC_VER < 1940
#error Security compilation error: Visual Studio 2022 requires MSVC 14.38.x and later version!
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

#include <stdio.h>
#include <string.h>

#if __cplusplus
extern "C" {
#endif

#include "game_common.h"
#include "game_client.h"

#if NB_HAVE_PB_BOTH==1
#include "networking.h"
#include "account.h"
#ifndef __EMSCRIPTEN__
#include "console_control_gui.h"
#endif
#else
#include "st_end_support.h"
#endif

#ifdef MAPC_INCLUDES_CHKP
#include "checkpoints.h" /* New: Checkpoints */
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
#include "config.h"
#include "video.h"
#include "image.h"
#include "audio.h"
#include "demo.h"
#include "progress.h"
#include "gui.h"
#include "set.h"
#include "tilt.h"
#include "hmd.h"
#include "fs.h"
#include "common.h"
#include "text.h"
#include "mtrl.h"
#include "geom.h"
#include "joy.h"
#include "fetch.h"
#include "moon_taskloader.h"
#include "package.h"
#include "currency.h"
#if ENABLE_RFD==1
#include "rfd.h"
#endif
#include "log.h"
#include "game_client.h"
#include "substr.h"

#if NB_HAVE_PB_BOTH==1
#include "st_setup.h"
#endif

#include "st_malfunction.h"
#include "st_intro.h"
#include "st_conf.h"
#include "st_name.h"
#include "st_ball.h"
#include "st_title.h"
#include "st_demo.h"
#include "st_level.h"
#include "st_pause.h"
#include "st_play.h"
#include "st_fail.h"
#include "st_common.h"
#include "st_start.h"
#include "st_package.h"

#if __cplusplus
}
#endif

#if _DEBUG && _MSC_VER
#ifndef _CRTDBG_MAP_ALLOC
#pragma message(__FILE__": Missing CRT-Debugger include header, recreate: crtdbg.h")
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

#if NB_STEAM_API==1
const char TITLE[] = "Neverball - Steam";
#elif NB_EOS_SDK==1
const char TITLE[] = "Neverball - Epic Games";
#else
const char TITLE[] = "Neverball";
#endif
const char ICON[] = "icon/neverball.png";

#define SET_ALWAYS_UNLOCKED

/* This fixes some malfunctions instead */
#define SDL_EVENT_ANTI_MALFUNCTIONS(events) do { events.type = 0; } while (0)

/*---------------------------------------------------------------------------*/

static List opt_data_multi = NULL;
static List opt_level_multi = NULL;

static char *opt_replay;
static char *opt_link;

#if ENABLE_DEDICATED_SERVER==1
static char *opt_ipaddr;

#ifndef DISABLE_PANORAMA
static char *opt_panorama;

#define opt_usage \
    "Usage: %s [options ...]\n" \
    "Options:\n" \
    "  -h, --help                show this usage message.\n" \
    "  -p, --panorama            snap background file as panorama (Only, if Unity is supported).\n" \
    "  -v, --version             show version.\n" \
    "  -s, --screensaver         show screensaver.\n" \
    "      --safetysetup         show safety video (not yet implemented).\n" \
    "  -d, --data <dir>          use 'dir' as game data directory.\n" \
    "  -r, --replay <file>       play the replay 'file'.\n" \
    "  -l, --level <file>        load the level 'file'\n" \
    "      --link <file>         open the named asset\n" \
    "      --ipv4 <address>      use the IPv4 address for dedicated game network.\n" \
    "      --ipv6 <address>      use the IPv6 address for dedicated game network.\n"
#else
#define opt_usage \
    "Usage: %s [options ...]\n" \
    "Options:\n" \
    "  -h, --help                show this usage message.\n" \
    "  -v, --version             show version.\n" \
    "  -s, --screensaver         show screensaver.\n" \
    "      --safetysetup         show safety video (not yet implemented).\n" \
    "  -d, --data <dir>          use 'dir' as game data directory.\n" \
    "  -r, --replay <file>       play the replay 'file'.\n" \
    "  -l, --level <file>        load the level 'file'\n" \
    "      --link <file>         open the named asset\n" \
    "      --ipv4 <address>      use the IPv4 address for dedicated game network.\n" \
    "      --ipv6 <address>      use the IPv6 address for dedicated game network.\n"
#endif
#else
#ifndef DISABLE_PANORAMA
static char *opt_panorama;

#define opt_usage \
    "Usage: %s [options ...]\n" \
    "Options:\n" \
    "  -h, --help                show this usage message.\n" \
    "  -p, --panorama            snap background file as panorama (Only, if Unity is supported).\n" \
    "  -v, --version             show version.\n" \
    "  -s, --screensaver         show screensaver.\n" \
    "      --safetysetup         show safety video (not yet implemented).\n" \
    "  -d, --data <dir>          use 'dir' as game data directory.\n" \
    "  -r, --replay <file>       play the replay 'file'.\n" \
    "  -l, --level <file>        load the level 'file'\n" \
    "      --link <file>         open the named asset\n"
#else
#define opt_usage \
    "Usage: %s [options ...]\n" \
    "Options:\n" \
    "  -h, --help                show this usage message.\n" \
    "  -v, --version             show version.\n" \
    "  -s, --screensaver         show screensaver.\n" \
    "      --safetysetup         show safety video (not yet implemented).\n" \
    "  -d, --data <dir>          use 'dir' as game data directory.\n" \
    "  -r, --replay <file>       play the replay 'file'.\n" \
    "  -l, --level <file>        load the level 'file'\n" \
    "      --link <file>         open the named asset\n"
#endif
#endif

static int opt_screensaver;

#define opt_error(option) \
    fprintf(stderr, "Option '%s' requires an argument.\n", option)

#define frame_smooth (1.0f / 25.0f) * 1000.0f

static void opt_init(int argc, char **argv)
{
    int i;

    /* Scan argument list. */

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help")    == 0)
        {
            printf(opt_usage, argv[0]);
            exit(EXIT_SUCCESS);
        }

#ifndef DISABLE_PANORAMA
        if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--panorama") == 0)
        {
            if (i + 1 == argc)
            {
                opt_error(argv[i]);
                exit(EXIT_FAILURE);
            }
            opt_panorama = argv[++i];
        }
#endif

        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0)
        {
            printf("%s\n", VERSION);
            exit(EXIT_SUCCESS);
        }
        
        if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--screensaver") == 0)
        {
            opt_screensaver = 1;
        }

        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--data")    == 0)
        {
            if (i + 1 == argc)
            {
                opt_error(argv[i]);
                exit(EXIT_FAILURE);
            }
            opt_data_multi = list_cons(strdup(argv[i + 1]), opt_data_multi);
            log_printf("Added data path to opt_data_multi: %s\n", argv[i + 1]);
            i++;
        }

        if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--replay")  == 0)
        {
            if (i + 1 == argc)
            {
                opt_error(argv[i]);
                exit(EXIT_FAILURE);
            }
            opt_replay = argv[i++];
        }

        if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--level")  == 0)
        {
            if (i + 1 == argc)
            {
                opt_error(argv[i]);
                exit(EXIT_FAILURE);
            }
            opt_level_multi = list_cons(strdup(argv[i + 1]), opt_level_multi);
            log_printf("Added map level path to opt_level_multi: %s\n", argv[i + 1]);
            i++;
        }

        if (strcmp(argv[i], "--link") == 0)
        {
            if (i + 1 == argc)
            {
                opt_error(argv[i]);
                exit(EXIT_FAILURE);
            }
            opt_link = argv[i++];
        }

#if ENABLE_DEDICATED_SERVER==1
        if (strcmp(argv[i], "--ipv4") == 0 || strcmp(argv[i], "--ipv6") == 0)
        {
            if (i + 1 == argc)
            {
                opt_error(argv[i]);
                exit(EXIT_FAILURE);
            }

            opt_ipaddr = argv[i++];
        }
#endif

        /* Perform magic on a single unrecognized argument. */

        /*if (argc == 2)
        {
            size_t len = strlen(argv[i]);
            int level = 0;

            if (len > 4)
            {
                char *ext = argv[i] + len - 4;

                if (strcmp(ext, ".map") == 0)
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                    strncpy_s(ext, MAXSTR, ".sol", 4);
#else
                    strncpy(ext, ".sol", 4);
#endif

                if (strcmp(ext, ".sol") == 0)
                    level = 1;
            }

            if (level)
                opt_level = argv[i];
            else
                opt_replay = argv[i];

            break;
        }*/
    }
}

#undef opt_usage
#undef opt_error

static void opt_quit(void)
{
    while (opt_data_multi)
    {
        free(opt_data_multi->data);
        opt_data_multi->data = NULL;
        opt_data_multi = list_rest(opt_data_multi);
    }

    opt_replay = NULL;

    while (opt_level_multi)
    {
        free(opt_level_multi->data);
        opt_level_multi->data = NULL;
        opt_level_multi = list_rest(opt_data_multi);
    }

    opt_link   = NULL;

#if ENABLE_DEDICATED_SERVER==1
    opt_ipaddr = NULL;
#endif
}

/*---------------------------------------------------------------------------*/

static void shot(void)
{
#if NB_HAVE_PB_BOTH==1
    if (game_setup_process())
        return;
#endif

#if NB_STEAM_API==0
    static char filename[MAXSTR];

    int secdecimal = ROUND(config_screenshot() / 10000);

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(filename, MAXSTR,
#else
    sprintf(filename,
#endif
            "Screenshots/screen_%04d-%04d.png",
            secdecimal, config_screenshot());
    video_snap(filename);
#endif
}

/*---------------------------------------------------------------------------*/

static void toggle_wire(void)
{
    video_toggle_wire();
}

/*---------------------------------------------------------------------------*/

static int handle_key_dn(SDL_Event *e)
{
    int d = 1;
    int c = e->key.keysym.sym;

    /* SDL made me do it. */
#ifdef __APPLE__
    if (c == SDLK_q  && e->key.keysym.mod & KMOD_GUI)
#endif
#ifdef _WIN32
    if (c == SDLK_F4 && e->key.keysym.mod & KMOD_ALT)
#endif
    {
        st_exit();
        return 0;
    }

    switch (c)
    {
#if NB_STEAM_API==0 && NB_EOS_SDK==0
        case KEY_SCREENSHOT:
            shot();
            break;
        case KEY_FPS:
            config_tgl_d(CONFIG_FPS);
            break;
        case KEY_WIREFRAME:
            if (config_cheat())
                toggle_wire();
            break;
        case KEY_RESOURCES:
            if (config_cheat())
            {
                light_load();
                mtrl_reload();
            }
        break;
#endif
        case SDLK_RETURN:
            d = st_buttn(config_get_d(CONFIG_JOYSTICK_BUTTON_A), 1);
            break;
        case KEY_FULLSCREEN:
            video_fullscreen(!config_get_d(CONFIG_FULLSCREEN));
            config_save();
            goto_state_full(curr_state(), 0, 0, 1);
            break;
        case KEY_EXIT:
            d = st_keybd(KEY_EXIT, 1);
            break;

        default:
            if (config_tst_d(CONFIG_KEY_FORWARD, c))
            {
                arrow_downcounter[0] += 1;
                arrow_downcounter[0]  = CLAMP(0, arrow_downcounter[0], 1);
                st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_Y0), -1);
            }
            else if (config_tst_d(CONFIG_KEY_BACKWARD, c))
            {
                arrow_downcounter[1] += 1;
                arrow_downcounter[1]  = CLAMP(0, arrow_downcounter[1], 1);
                st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_Y0), 1);
            }
            else if (config_tst_d(CONFIG_KEY_LEFT, c))
            {
                arrow_downcounter[2] += 1;
                arrow_downcounter[2]  = CLAMP(0, arrow_downcounter[2], 1);
                st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_X0), -1);
            }
            else if (config_tst_d(CONFIG_KEY_RIGHT, c))
            {
                arrow_downcounter[3] += 1;
                arrow_downcounter[3]  = CLAMP(0, arrow_downcounter[3], 1);
                st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_X0), 1);
            }
            else
                d = st_keybd(e->key.keysym.sym, 1);
    }

    return d;
}

static int handle_key_up(SDL_Event *e)
{
    int d = 1;
    int c = e->key.keysym.sym;

    switch (c)
    {
        case SDLK_RETURN:
            d = st_buttn(config_get_d(CONFIG_JOYSTICK_BUTTON_A), 0);
            break;
        case KEY_EXIT:
            d = st_keybd(KEY_EXIT, 0);
            break;
        default:
            if (config_tst_d(CONFIG_KEY_FORWARD, c))
            {
                arrow_downcounter[0] -= 1;
                arrow_downcounter[0]  = CLAMP(0, arrow_downcounter[0], 1);
                st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_Y0), 0);
            }
            else if (config_tst_d(CONFIG_KEY_BACKWARD, c))
            {
                arrow_downcounter[1] -= 1;
                arrow_downcounter[1]  = CLAMP(0, arrow_downcounter[1], 1);
                st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_Y0), 0);
            }
            else if (config_tst_d(CONFIG_KEY_LEFT, c))
            {
                arrow_downcounter[2] -= 1;
                arrow_downcounter[2]  = CLAMP(0, arrow_downcounter[2], 1);
                st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_X0), 0);
            }
            else if (config_tst_d(CONFIG_KEY_RIGHT, c))
            {
                arrow_downcounter[3] -= 1;
                arrow_downcounter[3]  = CLAMP(0, arrow_downcounter[3], 1);
                st_stick(config_get_d(CONFIG_JOYSTICK_AXIS_X0), 0);
            }
            else
                d = st_keybd(e->key.keysym.sym, 0);
    }

    return d;
}

#ifdef __EMSCRIPTEN__
enum
{
    USER_EVENT_BACK  = -1,
    USER_EVENT_PAUSE = 0
};

void push_user_event(int code)
{
    SDL_Event event = { SDL_USEREVENT };
    event.user.code = code;
    SDL_PushEvent(&event);
}
#endif

/*---------------------------------------------------------------------------*/

#if ENABLE_FETCH!=0
/*
 * Custom SDL event code for fetch events.
 */
Uint32 FETCH_EVENT = -2u;

/*
 * Push a custom SDL event on the queue from another thread.
 */
static void dispatch_fetch_event(void *data)
{
    SDL_Event e;

    memset(&e, 0, sizeof (e));

    e.type       = FETCH_EVENT;
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
#endif

#if ENABLE_MOON_TASKLOADER!=0
/*
 * Custom SDL event code for moon taskloader events.
 */
Uint32 MOON_TASKLOADER_EVENT = -1u;

/*
 * Push a custom SDL event on the queue from another thread.
 */
static void dispatch_moon_taskloader_event(void* data)
{
    SDL_Event e;

    memset(&e, 0, sizeof(e));

    e.type = MOON_TASKLOADER_EVENT;
    e.user.data1 = data;

    /* This is thread safe. */

    SDL_PushEvent(&e);
}

/*
 * Start the moon taskloader thread.
 *
 * SDL must be initialized at this point for moon taskloader event dispatch to work.
 */
static void initialize_moon_taskloader(void)
{
    /* Get a custom event code for fetch events. */
    MOON_TASKLOADER_EVENT = SDL_RegisterEvents(1);

    /* Start the thread. */
    moon_taskloader_init(dispatch_moon_taskloader_event);
}
#endif

/*---------------------------------------------------------------------------*/

static int goto_level(const List level_multi)
{
    if (level_multi == NULL) return 0;

    List                p = NULL;
    static struct level lvl_v[30];

    int        lvl_count = 0;
    static int lvl_count_loaded = 0;

    static int lvl_classic = 0;
    static int lvl_bonus = 0;

    for (p = (const List) level_multi; p && lvl_count < 30; p = p->next)
    {
        struct level* lvl = &lvl_v[lvl_count_loaded];

        const char* path = fs_resolve((const char *) p->data);

        if (path &&
            (str_ends_with(path, ".csol") ||
             str_ends_with(path, ".sol")))
        {
            if (level_load(path, lvl))
            {
                /* No bonus or master levels allowed on this standalone */

                if (!lvl->is_master && !lvl->is_bonus)
                {
                    if (lvl_count_loaded > 0)
                    {
                        memset(lvl_v[lvl_count_loaded - 1].next, 0, sizeof (struct level));
                        lvl_v[lvl_count_loaded - 1].next = lvl;
                    }

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                    sprintf_s(lvl->name, MAXSTR,
#else
                    sprintf(lvl->name,
#endif
                            "%d", lvl_count_loaded + 1);

                    lvl_count_loaded++;
                }
                else memset(lvl, 0, sizeof (struct level));
            }
            else log_errorf("File %s is not in game path (at index %d)\n", p->data, lvl_count_loaded);
        }
        else log_errorf("File %s is not in game path (at index %d)\n", p->data, lvl_count_loaded);

        lvl_count++;
    }

    progress_init(MODE_STANDALONE);

    /* Check whether standalone set is loaded correctly. */

    if (lvl_count_loaded != 0 && progress_play(&lvl_v[0]))
    {
        /* Start standalone set! */

        return 1;
    }

    /* ...otherwise go to main menu screen. */

    progress_init(MODE_NONE);

    return 0;
}

/*---------------------------------------------------------------------------*/

/*
 * Handle the link option.
 *
 * This navigates to the appropriate screen, if the asset was found.
 *
 * Supported link types:
 *
 * --link set-easy
 * --link set-easy/peasy
 */
static int process_link(const char *link)
{
    int processed = 0;

    if (!(link && *link))
        return 0;

    log_printf("Processing link: %s\n", link);

    if (str_starts_with(link, "set-"))
    {
        /* Search installed sets and package list. */

        const size_t prefix_len = strcspn(link, "/");
        const char *set_part    = SUBSTR(link, 0, prefix_len);
        const char *map_part    = SUBSTR(link, prefix_len + 1, 64);

        char *set_file = concat_string(set_part, ".txt", NULL);

        if (set_file)
        {
            int index;
            int found_level = 0;

            log_printf("Link is a set reference, searching for %s.\n", set_file);

#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT) && defined(LEVELGROUPS_INCLUDES_CAMPAIGN)
            int career_unlocked = (server_policy_get_d(SERVER_POLICY_PLAYMODES_UNLOCKED_MODE_CAREER) ||
                                   campaign_career_unlocked());

            if (!career_unlocked)
            {
                log_errorf("Complete the game in campaign mode to link the set reference!\n");

                free(set_file);
                set_file = NULL;

                return 0;
            }
#endif

            set_init(0);

            if ((index = set_find(set_file)) >= 0)
            {
                log_printf("Found set with the given reference.\n");

                set_goto(index);

                if (map_part && *map_part)
                {
                    /* Search for the given level. */

                    char *sol_basename = concat_string(map_part, ".sol", NULL);

                    log_printf("Link is also a level reference, searching for %s\n", sol_basename);

                    if (sol_basename)
                    {
                        struct level *level = set_find_level(sol_basename);

                        if (level)
                        {
                            log_printf("Found level with the given reference.\n");

                            progress_init(MODE_NORMAL);

                            if (progress_play(level))
                            {
                                goto_play_level();
                                found_level = 1;
                                processed = 1;
                            }
                        }

                        free(sol_basename);
                        sol_basename = NULL;
                    }
                }

                if (!found_level)
                {
                    load_title_background();
                    game_kill_fade();
                    goto_state(&st_start);
                    processed = 1;
                }
            }
            else if ((index = package_search(set_file)) >= 0)
            {
                log_printf("Found package with the given reference.\n");
                goto_package(index, &st_title);
                processed = 1;
            }
            else log_printf("Link did not match.\n", link);

            free(set_file);
            set_file = NULL;
        }
        else log_printf("Link is bad.\n");
    }

    return processed;
}

/*---------------------------------------------------------------------------*/

static void refresh_packages_done(void *data, void *extra_data)
{
    struct state *start_state = (struct state *) data;

    if (!process_link(opt_link))
    {
        if (start_state)
            goto_state(start_state);
    }
}

/*
 * Start package refresh and go to given state when done.
 */
static void main_preload(struct state *start_state)
{
    struct fetch_callback callback = {0};

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

    if (process_link(opt_link))
    {
        /* Link processing navigates to the appropriate screen. */
        return;
    }

    /* Otherwise, go to the starting screen. */

#if NB_HAVE_PB_BOTH==1
    if (!check_game_setup())
    {
        goto_game_setup(start_state,
                        goto_name_setup,
                        goto_ball_setup);
        return;
    }
#endif

    goto_state(start_state);
}

/*---------------------------------------------------------------------------*/

#if ENABLE_DEDICATED_SERVER==1

Uint32 DEDICATED_SERVER_EVENT = -1u;
int networking_ec;

static void dispatch_networking_event(void *data)
{
    SDL_Event e;

    memset(&e, 0, sizeof (e));

    e.type       = DEDICATED_SERVER_EVENT;
    e.user.data1 = data;

    /* First, the variable values, before push the thread event. */

    SDL_PushEvent(&e);
}

static void dispatch_networking_error_event(int ec)
{
    networking_ec = ec;
}

#endif

/*---------------------------------------------------------------------------*/

static int loop(void);

int st_global_loop(void)
{
    return loop();
}

static int loop(void)
{
    SDL_Event e;

    int d = 1;

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

    while (d && SDL_PollEvent(&e))
    {
        switch (e.type)
        {
            case SDL_QUIT:
                st_exit();
                return 0;

#ifdef __EMSCRIPTEN__
            case SDL_USEREVENT:
                switch (e.user.code)
                {
                    case USER_EVENT_BACK:
                        d = st_keybd(KEY_EXIT, 1);
                        break;

                    case USER_EVENT_PAUSE:
                        if (video_get_grab())
                        {
                            if (curr_state() == &st_play_ready ||
                                curr_state() == &st_play_set   ||
                                curr_state() == &st_play_loop)
                                goto_pause(curr_state());
                        }
                        break;
                }
                break;
#endif

#if NEVERBALL_FAMILY_API == NEVERBALL_PC_FAMILY_API
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

#if NEVERBALL_FAMILY_API == NEVERBALL_PC_FAMILY_API
            case SDL_KEYDOWN:
                d = handle_key_dn(&e);
                break;

            case SDL_KEYUP:
                d = handle_key_up(&e);
                break;
#endif

            case SDL_WINDOWEVENT:
                switch (e.window.event)
                {
                    case SDL_WINDOWEVENT_FOCUS_LOST:
                        audio_suspend();
                        if (video_get_grab())
                        {
                            if (curr_state() == &st_play_ready ||
                                curr_state() == &st_play_set   ||
                                curr_state() == &st_play_loop)
                                play_pause_goto(curr_state());
                        }
                        else if (curr_state() == &st_demo_play)
                            demo_pause_goto();
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
                        /*log_printf("Resize event (%u, %dx%d)\n",
                                     e.window.windowID,
                                     e.window.data1,
                                     e.window.data2);*/

                        video_clear();
                        video_resize(e.window.data1, e.window.data2);
                        gui_resize();
                        break;

                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                        /*log_printf("Size change event (%u, %dx%d)\n",
                                     e.window.windowID,
                                     e.window.data1,
                                     e.window.data2);*/

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
                        break;

                    case SDL_WINDOWEVENT_RESTORED:
                        config_set_d(CONFIG_MAXIMIZED, 0);
                        gui_resize();
                        config_save();
                        goto_state_full(curr_state(), 0, 0, 1);
                        break;
                }
                break;

            case SDL_TEXTINPUT:
                text_input_str(e.text.text, 1);
                break;

#if NEVERBALL_FAMILY_API != NEVERBALL_PC_FAMILY_API && NB_PB_WITH_XBOX==0
            case SDL_JOYAXISMOTION:
                joy_axis(e.jaxis.which, e.jaxis.axis, JOY_VALUE(e.jaxis.value));
                break;

            case SDL_JOYBUTTONDOWN:
                d = joy_button(e.jbutton.which, e.jbutton.button, 1);
                break;

            case SDL_JOYBUTTONUP:
                d = joy_button(e.jbutton.which, e.jbutton.button, 0);
                break;

            case SDL_JOYDEVICEADDED:
                joy_add(e.jdevice.which);
                break;

            case SDL_JOYDEVICEREMOVED:
                joy_remove(e.jdevice.which);
                break;
#endif

            case SDL_MOUSEWHEEL:
                st_wheel(e.wheel.x, e.wheel.y);
                break;

            default:
#if ENABLE_MOON_TASKLOADER!=0
                if (e.type == MOON_TASKLOADER_EVENT)
                    moon_taskloader_handle_event(e.user.data1);
#endif
#if ENABLE_FETCH!=0
                if (e.type == FETCH_EVENT)
                    fetch_handle_event(e.user.data1);
#endif
#if ENABLE_DEDICATED_SERVER==1
                if (e.type == DEDICATED_SERVER_EVENT)
                {
                    /* FIXME: Waiting for handle event functions. */
                }
#endif
                break;
        }

        /* if (check_malfunctions())
            SDL_EVENT_ANTI_MALFUNCTIONS(e); */
    }

    /* Process events via the tilt sensor API. */

    if (tilt_stat())
    {
        int b;
        int s;

        st_angle(tilt_get_x(),
                 tilt_get_z());

        while (tilt_get_button(&b, &s))
        {
            const int X = config_get_d(CONFIG_JOYSTICK_AXIS_X0);
            const int Y = config_get_d(CONFIG_JOYSTICK_AXIS_Y0);
            const int L = config_get_d(CONFIG_JOYSTICK_DPAD_L);
            const int R = config_get_d(CONFIG_JOYSTICK_DPAD_R);
            const int U = config_get_d(CONFIG_JOYSTICK_DPAD_U);
            const int D = config_get_d(CONFIG_JOYSTICK_DPAD_D);

            if (b == L || b == R || b == U || b == D)
            {
                static int pad[4] = { 0, 0, 0, 0 };

                /* Track the state of the D-pad buttons. */

                if      (b == L) pad[0] = s;
                else if (b == R) pad[1] = s;
                else if (b == U) pad[2] = s;
                else if (b == D) pad[3] = s;

                /* Convert D-pad button events into joystick axis motion. */

                if      (pad[0] && !pad[1]) st_stick(X, -1.0f);
                else if (pad[1] && !pad[0]) st_stick(X, +1.0f);
                else                        st_stick(X,  0.0f);

                if      (pad[2] && !pad[3]) st_stick(Y, -1.0f);
                else if (pad[3] && !pad[2]) st_stick(Y, +1.0f);
                else                        st_stick(Y,  0.0f);
            }
            else d = st_buttn(b, s);
        }
    }

    return d;
}

/*---------------------------------------------------------------------------*/

static int is_replay(struct dir_item *item)
{
    return str_ends_with(item->path, ".nbr");
}

static int is_score_file(struct dir_item *item)
{
    return str_starts_with(item->path, "neverballhs-");
}

static void make_dirs_and_migrate(void)
{
    Array items;
    int i;

    const char *src;
    char *dst;

    fs_mkdir("Accounts");
    fs_mkdir("Campaign");

    if (fs_mkdir("Replays"))
    {
        if ((items = fs_dir_scan("", is_replay)))
        {
            for (i = 0; i < array_len(items); i++)
            {
                src = DIR_ITEM_GET(items, i)->path;
                dst = concat_string("Replays/", src, NULL);
                fs_rename(src, dst);
                free(dst);
                dst = NULL;
            }

            fs_dir_free(items);
        }
    }

    if (fs_mkdir("Scores"))
    {
        if ((items = fs_dir_scan("", is_score_file)))
        {
            for (i = 0; i < array_len(items); i++)
            {
                src = DIR_ITEM_GET(items, i)->path;
                dst = concat_string("Scores/",
                                    src + sizeof ("neverballhs-") - 1,
                                    ".txt",
                                    NULL);
                fs_rename(src, dst);
                free(dst);
                dst = NULL;
            }

            fs_dir_free(items);
        }
    }

    fs_mkdir("Screenshots");
}

/*---------------------------------------------------------------------------*/

static void main_quit(void);

struct main_loop
{
    Uint32 now;
    unsigned int done : 1;
};

static void step(void *data)
{
    struct main_loop *mainloop = (struct main_loop *) data;

    int running = loop();

    if (running)
    {
        Uint32 now = SDL_GetTicks();
        Uint32 dt  = (now - mainloop->now);

        if (0 < dt && dt < 1000)
        {
            /* Step the game state. */

            float deltaTime = config_get_d(CONFIG_SMOOTH_FIX) ?
                              MIN(frame_smooth, dt) : MIN(100.0f, dt);

            CHECK_GAMESPEED(20, 100);
            float speedPercent = (float) accessibility_get_d(ACCESSIBILITY_SLOWDOWN) / 100;
            st_timer(MAX((0.001f * deltaTime) * speedPercent, 0));
        }

        /* Render. */

        hmd_step();

        if (viewport_wireframe == 2 || viewport_wireframe == 3)
        {
            video_render_fill_or_line(0);
            st_paint(0.001f * now, 1);
            video_render_fill_or_line(1);
            st_paint(0.001f * now, 0);
        }
        else st_paint(0.001f * now, 1);

        video_swap();

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
        Neverball.quit();
    });
#endif
}

#ifndef DISABLE_PANORAMA
static void panorama_snap(char *panorama_sides)
{
    char *filename = concat_string("Screenshots/shot-panorama/panorama_volcano_",
                                   panorama_sides, ".png", NULL);

    video_clear();
    back_draw_easy();
    log_printf("Creating panorama image (%s)\n", panorama_sides);
    video_snap(filename);
    video_swap();

    free(filename);
    filename = NULL;
}

static void panorama_snap_sides(void)
{
    glMatrixMode(GL_MODELVIEW);
    {
        glRotatef(0, 0, 1, 0);
        panorama_snap("north");
        glLoadIdentity();
        glRotatef(90, 0, 1, 0);
        panorama_snap("east");
        glLoadIdentity();
        glRotatef(180, 0, 1, 0);
        panorama_snap("south");
        glLoadIdentity();
        glRotatef(-90, 0, 1, 0);
        panorama_snap("west");
        glLoadIdentity();

        glRotatef(-90, 1, 0, 0);
        panorama_snap("up");
        glLoadIdentity();
        glRotatef(90, 1, 0, 0);
        panorama_snap("down");
        glLoadIdentity();
    }
}
#endif

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

    opt_init(argc, argv);

    List p             = NULL;
    int  datadir_multi = 0;

    for (p = opt_data_multi; p; p = p->next)
    {
        if (!datadir_multi)
        {
            config_paths((const char *) p->data);
            datadir_multi = 1;
        }
        else
            fs_add_path_with_archives((const char *) p->data);
    }

    log_init("Neverball " VERSION, "neverball.log");
    config_log_userpath();
#if NB_HAVE_PB_BOTH!=1
    make_dirs_and_migrate();
#endif

    /* Initialize SDL. */

#ifdef SDL_HINT_TOUCH_MOUSE_EVENTS
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
#endif

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
#error Security compilation error: No Playstation HIDAPI specified!
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

    /* Initialize configuration. */

    config_init();
    config_load();

#if ENABLE_RFD==1
    /* Initialize RFD. */

    rfd_init();
    rfd_load();
#endif

#ifndef DISABLE_PANORAMA
    if (!opt_panorama)
    {
#endif
        /* Initialize currency units. */

        currency_init();

#if ENABLE_DEDICATED_SERVER==1
        if (opt_ipaddr)
            config_set_s(CONFIG_DEDICATED_IPADDRESS, opt_ipaddr);

        /* Initialize dedicated server. */

        if (networking_init(1))
        {
            DEDICATED_SERVER_EVENT = SDL_RegisterEvents(1);

            networking_init_dedicated_event(dispatch_networking_event,
                                            dispatch_networking_error_event);
        }
#else
        networking_init(1);
#endif
#if ENABLE_MOON_TASKLOADER!=0
        initialize_moon_taskloader();
#endif
#if ENABLE_FETCH!=0
        initialize_fetch();
#endif

        package_init();
#ifndef DISABLE_PANORAMA
    }
#endif

    /* Enable joystick events. */
#if NEVERBALL_FAMILY_API != NEVERBALL_PC_FAMILY_API || NB_PB_WITH_XBOX==1
    if (!joy_init())
        return 0;
#endif

#if NB_HAVE_PB_BOTH==1
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
#if NEVERBALL_FAMILY_API == NEVERBALL_STEAMDECK_FAMILY_API
    init_controller_type(PLATFORM_STEAMDECK);
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

#if NB_STEAM_API==1 || NB_EOS_SDK==1
    config_set_d(CONFIG_FPS, 0);
    config_save();
#endif

    /* Initialize accessibility. */

    accessibility_init();
#if NB_STEAM_API==0 && NB_EOS_SDK==0
    accessibility_load();
#endif

#ifndef DISABLE_PANORAMA
    if (!opt_panorama)
    {
#endif
        /* Initialize audio. */

        audio_init();

#if NB_HAVE_PB_BOTH==1
        /* Initialize account. */

        int account_res = account_init();

        if (account_res == 0)
        {
#if NB_STEAM_API==1
            log_errorf("Steam user is not logged in! Steam user must be logged in to play this game (SteamUser()->BLoggedOn() returned false).\n");
            return 0;
#elif NB_EOS_SDK==1
            return 0;
#endif
        }

        account_load();

        if (server_policy_get_d(SERVER_POLICY_EDITION) > 1 &&
            account_get_d(ACCOUNT_SET_UNLOCKS) < 1)
            account_set_d(ACCOUNT_SET_UNLOCKS, 1);

        account_save();
#endif

#if ENABLE_NLS==1 || _MSC_VER
        /* Initialize localization. */

        lang_init();
#endif

        tilt_init();

        if (!video_init())
            return 0;
#ifndef DISABLE_PANORAMA
    }
    else if (!video_mode(0, 1024, 1024)) return 0;
#endif

    /* Material system. */

    mtrl_init();

    /* Screen states. */

    init_state(&st_null);

#ifdef DEMO_QUARANTINED_MODE
    if (config_get_d(CONFIG_ACCOUNT_SAVE) > 2)
        config_set_d(CONFIG_ACCOUNT_SAVE, 2);
    if (config_get_d(CONFIG_ACCOUNT_LOAD) > 2)
        config_set_d(CONFIG_ACCOUNT_LOAD, 2);

    config_save();
#endif

    return 1;
}

static void main_quit(void)
{
#ifndef DISABLE_PANORAMA
    if (!opt_panorama)
#endif
    {
#if NB_STEAM_API==0 && NB_EOS_SDK==0
        accessibility_save();
#endif
#ifdef CONFIG_INCLUDES_ACCOUNT
        account_save      ();
#endif
        tilt_free         ();
    }

    config_save();

    /* Free loaded sets, in case of link processing. */

    set_quit();

    /* Free everything else. */

    goto_state_full(&st_null, 0, 0, 1);

    mtrl_quit ();
    video_quit();
    audio_free();
    lang_quit ();

#if NEVERBALL_FAMILY_API != NEVERBALL_PC_FAMILY_API || NB_PB_WITH_XBOX==1
    joy_quit();
#endif

#ifdef CONFIG_INCLUDES_ACCOUNT
    account_quit();
#endif

#if ENABLE_RFD==1
    rfd_quit();
#endif

    config_quit();

#ifndef DISABLE_PANORAMA
    if (!opt_panorama)
    {
#endif

        package_quit();
        fetch_quit  ();
#if ENABLE_MOON_TASKLOADER!=0
        moon_taskloader_quit();
#endif
#if NB_HAVE_PB_BOTH==1
        networking_quit();
#endif
#ifndef DISABLE_PANORAMA
    }
#endif

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

#if NB_EOS_SDK==1
    /* TODO: Add the EOS SDK shutdown! */
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
    SDL_Joystick *joy = NULL;
    struct main_loop mainloop = { 0 };

#if NB_HAVE_PB_BOTH==1
    struct state *start_state = &st_intro;
#else
    struct state *start_state = &st_title;
#endif

#if _cplusplus
    try {
#endif
    if (!main_init(argc, argv))
        return 1;
#if _cplusplus
    } catch (...) { return 1; }
#endif

#ifndef DISABLE_PANORAMA
    if (opt_panorama)
    {
        const char *path = fs_resolve(opt_panorama);
        geom_init();

        if (path)
        {
            back_init(path);

            int fov_cache = config_get_d(CONFIG_VIEW_FOV);
            config_set_d(CONFIG_VIEW_FOV, 90);

            video_set_perspective((float) config_get_d(CONFIG_VIEW_FOV), 0.1f, FAR_DIST);
            panorama_snap_sides();

            config_set_d(CONFIG_VIEW_FOV, fov_cache);
            back_free();

            log_printf("Panorama created! Placed in \"shot-panorama\"\n");
        }
        else log_errorf("File %s is not in game path\n", opt_panorama);

        geom_free();
    }
    else
#endif
    if (opt_replay &&
        fs_add_path(dir_name(opt_replay)) &&
        progress_replay(base_name(opt_replay)))
    {
        if (config_get_d(CONFIG_ACCOUNT_LOAD) > 1 && demo_play_goto(1))
            start_state = &st_demo_scan_allowance;
        else
            start_state = &st_title;
    }
    else if (opt_level_multi)
    {
        if (goto_level(opt_level_multi))
            start_state = &st_level;
        else
#ifdef SKIP_END_SUPPORT
            start_state = &st_title;
#else
            start_state = &st_end_support;
#endif
    }
    else if (opt_screensaver)
        start_state = &st_screensaver;
    else
    {
#if NB_HAVE_PB_BOTH==1
        log_printf("Attempt to show intro\n");
        start_state = &st_intro;
#else
        int val = config_get_d(CONFIG_GRAPHIC_RESTORE_ID);

        if (val == -1)
            start_state = &st_title;
        else
            start_state = &st_intro_restore;
#endif
    }

    main_preload(start_state);

    /* Run the main game loop. */

    mainloop.now = SDL_GetTicks();

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(step, (void *) &mainloop, 0, 1);
#else
#ifndef DISABLE_PANORAMA
    while (!mainloop.done && !opt_panorama)
        step(&mainloop);
#else
    while (!mainloop.done)
        step(&mainloop);
#endif

    main_quit();
#endif

    return 0;
}

/*---------------------------------------------------------------------------*/
