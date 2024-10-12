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

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <string.h>

#if NB_HAVE_PB_BOTH==1
#include "networking.h"

#ifndef __EMSCRIPTEN__
#include "console_control_gui.h"
#endif

#include "account.h"
#include "account_wgcl.h"
#endif

#include "base_config.h"
#include "common.h"
#include "gui.h"
#include "vec3.h"
#include "demo.h"
#include "geom.h"
#include "audio.h"
#include "config.h"
#include "cmd.h"
#include "demo_dir.h"
#ifndef VERSION
#include "version.h"
#endif
#include "key.h"
#include "progress.h"
#include "text.h"

#include "activity_services.h"

#include "game_common.h"
#include "game_server.h"
#include "game_client.h"
#include "game_proxy.h"

#include "st_intro.h"

#include "st_shop.h"
#include "st_title.h"
#include "st_help.h"
#include "st_demo.h"
#include "st_conf.h"
#include "st_set.h"
#include "st_name.h"
#include "st_shared.h"
#if ENABLE_FETCH!=0
#include "st_package.h"
#endif
#include "st_play.h"

#if NB_HAVE_PB_BOTH==1
#include "st_wgcl.h"
#endif

#define TITLE_USE_DVD_BOX

#ifdef SWITCHBALL_GUI
#define SWITCHBALL_TITLE
#define SWITCHBALL_TITLE_BTN_V2
#endif

#define TITLE_BG_DEMO_INIT(_mode, _show_ball)  \
    do {                                       \
        game_client_toggle_show_balls(1);      \
        game_client_fly(0.0f);                 \
        real_time = 0.0f;                      \
        mode = _mode;                          \
    } while (0)

#if defined(__WII__)
/* We're using SDL 1.2 on Wii, which has SDLKey instead of SDL_Keycode. */
typedef SDLKey SDL_Keycode;
#endif

/*---------------------------------------------------------------------------*/

static int switchball_useable(void)
{
    const SDL_Keycode k_auto    = config_get_d(CONFIG_KEY_CAMERA_TOGGLE);
    const SDL_Keycode k_cam1    = config_get_d(CONFIG_KEY_CAMERA_1);
    const SDL_Keycode k_cam2    = config_get_d(CONFIG_KEY_CAMERA_2);
    const SDL_Keycode k_cam3    = config_get_d(CONFIG_KEY_CAMERA_3);
    const SDL_Keycode k_restart = config_get_d(CONFIG_KEY_RESTART);
    const SDL_Keycode k_caml    = config_get_d(CONFIG_KEY_CAMERA_L);
    const SDL_Keycode k_camr    = config_get_d(CONFIG_KEY_CAMERA_R);

    SDL_Keycode k_arrowkey[4];
    k_arrowkey[0] = config_get_d(CONFIG_KEY_FORWARD);
    k_arrowkey[1] = config_get_d(CONFIG_KEY_LEFT);
    k_arrowkey[2] = config_get_d(CONFIG_KEY_BACKWARD);
    k_arrowkey[3] = config_get_d(CONFIG_KEY_RIGHT);

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

int load_title_background(void)
{
#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT)
    game_client_toggle_show_balls(!CHECK_ACCOUNT_BANKRUPT);
#else
    game_client_toggle_show_balls(1);
#endif

    float home_pos[3];

    home_pos[0] = switchball_useable() ? 0.0f : -1.0f;
    home_pos[1] = 10.0f;
    home_pos[2] = 10.0f;

    game_view_set_static_cam_view(1, home_pos);

    if (switchball_useable())
    {
        if (game_client_init("gui/title/switchball-title.sol"))
        {
#if NB_HAVE_PB_BOTH==1
            if (CHECK_ACCOUNT_BANKRUPT) return 1;
#endif

            union cmd cmd = { CMD_GOAL_OPEN };

            game_proxy_enq(&cmd);
            game_client_sync(NULL);

            game_client_fly(1.0f);

            return 1;
        }
    }
    else if (game_client_init("gui/title/title.sol"))
    {
#if NB_HAVE_PB_BOTH==1
        if (CHECK_ACCOUNT_BANKRUPT) return 1;
#endif

        union cmd cmd = { CMD_GOAL_OPEN };

        game_proxy_enq(&cmd);
        game_client_sync(NULL);

        game_client_fly(1.0f);

        return 1;
    }

    return 0;
}

static const char *check_unlocked_demo(struct demo *rawdemo)
{
    int limit = config_get_d(CONFIG_ACCOUNT_LOAD);
    int max = 0;

    if (rawdemo->status == 3)
        max = 3;
    else if (rawdemo->status == 1 || rawdemo->status == 0)
        max = 2;
    else if (rawdemo->status == 2)
        max = 1;

    /* Can access into the replay? */
    return (max <= limit) ? rawdemo->path : NULL;
}

static const char *pick_demo(Array items)
{
    /* Do not allow pick replay, if replay files are empty. */

    int total;

    if ((total = array_len(items)) < 1) return NULL;

    int selectedDemo = rand_between(0, total - 1);

    demo_dir_load(items, 0, total - 1);

    struct demo *demo_data = ((struct demo *) ((struct dir_item *) array_get(items, selectedDemo))->data);

    /* Have demo data? */

    if (!demo_data) return NULL;

#if NB_HAVE_PB_BOTH==1
    /* Classic only? */

    if (demo_data->mode != MODE_NORMAL) return NULL;

    /* Less than ten minutes? */

    if ((demo_data->timer / 6000.0f) >= 10) return NULL;
#endif

    /* Compatibility supported? */

    if (!game_compat_map) return NULL;

    /* OK, demo file opened, but test! */

#if defined(COVID_HIGH_RISK)
    const char* demopath = check_unlocked_demo(demo_data);
    if (demopath)
        return demopath;
    else
    {
        log_errorf("Replay files deleted due covid high risks!: %s",
                   demo_data->path);
        demo_is_loaded = 0;
        demo_dir_free(items);
        fs_remove(demo_data->path);
        items = NULL;
        return NULL;
    }
#else
    return check_unlocked_demo(demo_data);
#endif
}

/*---------------------------------------------------------------------------*/

enum
{
    TITLE_MODE_NONE,

    TITLE_MODE_LEVEL,      /* Pan / 360 view */
    TITLE_MODE_LEVEL_FADE,
    TITLE_MODE_DEMO,       /* Player demos */
    TITLE_MODE_DEMO_FADE,

    TITLE_MODE_MAX
};

static int   title_load_lockscreen = 1;
static int   title_lockscreen = 1;
static int   title_can_unlock = 1;

static int   title_freeze_all;
static int   title_prequit;

static float real_time = 0.0f;
static int   mode      = TITLE_MODE_NONE;

static int   left_handed = 1;

static Array items;

static int play_id = 0;
static int support_exit = 1;

static const char editions_common[][26] =
{
    "%s Home Edition",
    "%s Professional Edition",
    "%s Enterprise Edition",
    "%s Education Edition",
    "%s Essential Edition",
    "%s Server Edition",
    "%s Datacenter Edition"
};

static const char editions_developer[][33] =
{
    "%s Home Edition / %s",
    "%s Professional Edition / %s",
    "%s Enterprise Edition / %s",
    "%s Education Edition / %s",
    "%s Essential Edition / %s",
    "%s Server Edition / %s",
    "%s Datacenter Edition / %s"
};

enum
{
    TITLE_PLAY = GUI_LAST,
    TITLE_SHOP,
    TITLE_HELP,
    TITLE_DEMO,
    TITLE_CONF,
    TITLE_PACKAGES,

    TITLE_UNLOCK_FULL_GAME = 99,

    TITLE_SOCIAL = 199
};

int edition_id;

static int title_check_playername(const char *regname)
{
    for (int i = 0; i < text_length(regname); i++)
    {
        if (regname[i] == '\\' ||
            regname[i] == '/'  ||
            regname[i] == ':'  ||
            regname[i] == '*'  ||
            regname[i] == '?'  ||
            regname[i] == '"'  ||
            regname[i] == '<'  ||
            regname[i] == '>'  ||
            regname[i] == '|')
            return 0;
    }

    return text_length(regname) >= 3;
}

static int title_goto_playgame(struct state *st)
{
    return goto_playgame();
}

static int title_action(int tok, int val)
{
    static const char keyphrase[] = "msxdev";
    static char queue[sizeof (keyphrase)] = "";

    char linkstr_code[MAXSTR], linkstr_cmd[MAXSTR];

    size_t queue_len = text_length(queue);

#if NB_HAVE_PB_BOTH==1
    const char title_social_url[3][MAXSTR] =
    {
        "https://gitea.stynegame.de/Neverball",
        "https://github.com/Neverball",
        "https://discord.gg/qnJR263Hm2/",
    };
#else
    const char title_social_url[2][MAXSTR] =
    {
        "https://github.com/Neverball",
        "https://discord.gg/HhMfr4N6H6/",
    };
#endif

    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case GUI_BACK:
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            if (support_exit)
#endif
            {
                title_prequit = 1;

                /* bye! */

                return 0;
            }
            break;

        case TITLE_SOCIAL:
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
#ifdef __EMSCRIPTEN__
            EM_ASM({ window.open(title_social_url[val]);}, 0);
#else
#if _WIN32
            SAFECPY(linkstr_cmd, "explorer ");
#elif defined(__APPLE__)
            SAFECPY(linkstr_cmd, "open ");
#elif defined(__linux__)
            SAFECPY(linkstr_cmd, "x-www-browser ");
#endif

            SAFECAT(linkstr_cmd, title_social_url[val]);

            system(linkstr_cmd);
#endif
#endif
            break;

        case TITLE_PLAY:
#if NB_HAVE_PB_BOTH==1
            if (server_policy_get_d(SERVER_POLICY_EDITION) == 0 &&
                !account_wgcl_name_read_only())
                return goto_wgcl_login(&st_title, 0, 0, title_goto_playgame);
#endif
            return title_check_playername(config_get_s(CONFIG_PLAYER)) ?
                                          goto_playgame() : goto_playgame_register();
            break;

#ifdef CONFIG_INCLUDES_ACCOUNT
        case TITLE_SHOP:
#if (NB_STEAM_API==1 || NB_EOS_SDK==1) || ENABLE_IAP==1
            if (val) goto_shop_iap(&st_title, &st_title, 0, 0, 0, 0, 1);
#else
            if (val) goto_shop_iap(&st_title, &st_title, 0, 0, 0, 0, 0);
#endif
            else     goto_shop(curr_state(), 0);
            break;
#endif
        case TITLE_HELP: return goto_state(&st_help); break;
        case TITLE_DEMO: return goto_state(&st_demo); break;
        case TITLE_CONF:
            game_fade(+4.0);
            return goto_conf(&st_title, 0, 0);
            break;
#if NB_HAVE_PB_BOTH!=1
#if ENABLE_FETCH!=0
        case TITLE_PACKAGES: return goto_state(&st_package); break;
#endif

        case TITLE_UNLOCK_FULL_GAME:
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
#if defined(__EMSCRIPTEN__)
            EM_ASM({ window.open("https://discord.gg/qnJR263Hm2"); }, 0);
#else
            SAFECPY(linkstr_code, "qnJR263Hm2");

#if _WIN32
            SAFECPY(linkstr_cmd, "start msedge https://discord.gg/");
#elif defined(__APPLE__)
            SAFECPY(linkstr_cmd, "open https://discord.gg/");
#elif defined(__linux__)
            SAFECPY(linkstr_cmd, "x-www-browser https://discord.gg/");
#endif

            SAFECAT(linkstr_cmd, linkstr_code);

            system(linkstr_cmd);
#endif
#endif
            break;
#endif
#ifndef __EMSCRIPTEN__
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
        case GUI_CHAR:

            /* Let the queue fill up. */

            if (queue_len < sizeof (queue) - 1)
            {
                queue[queue_len]     = (char) val;
                queue[queue_len + 1] = '\0';
            }

            /* Advance the queue before adding the new element. */

            else
            {
                int k;

                for (k = 1; k < queue_len; k++)
                    queue[k - 1] = queue[k];

                queue[queue_len - 1] = (char) val;
            }

            /* Developer or Public */
            char os_env[MAXSTR], dev_env[MAXSTR];
#if  NEVERBALL_FAMILY_API != NEVERBALL_PC_FAMILY_API || !defined(TITLE_USE_DVD_BOX)
#if ENABLE_HMD
            sprintf(dev_env, _(editions_developer[EDITION_CURRENT]),
                             "OpenHMD", _("Developer Mode"));
            sprintf(os_env, _(editions_common[EDITION_CURRENT]), "OpenHMD");
#elif defined(COMMUNITY_EDITION)
            sprintf(dev_env, _("%s Edition / %s"),
                             _("Community"), _("Developer Mode"));
            sprintf(os_env, _("%s Edition"), _("Community"));
#else
            if (current_platform == PLATFORM_PC)
            {
#if defined(__linux__)
                sprintf(dev_env, _("%s Edition / %s"),
                                 TITLE_PLATFORM_CINMAMON, _("Developer Mode"));
                sprintf(os_env, _("%s Edition"), TITLE_PLATFORM_CINMAMON);
#else
                sprintf(dev_env, _(editions_developer[EDITION_CURRENT]), 
                                 TITLE_PLATFORM_WINDOWS, _("Developer Mode"));
                sprintf(os_env, _(editions_common[EDITION_CURRENT]),
                                TITLE_PLATFORM_WINDOWS);
#endif
            }
            else if (current_platform == PLATFORM_XBOX)
            {
#if NEVERBALL_FAMILY_API == NEVERBALL_XBOX_360_FAMILY_API
                sprintf(dev_env, _("%s Edition / %s"),
                                 TITLE_PLATFORM_XBOX_360, _("Developer Mode"));
                sprintf(os_env, _("%s Edition"),
                                TITLE_PLATFORM_XBOX_360);
#else
                sprintf(dev_env, _("%s Edition / %s"),
                                 TITLE_PLATFORM_XBOX_ONE, _("Developer Mode"));
                sprintf(os_env, _("%s Edition"),
                                TITLE_PLATFORM_XBOX_ONE);
#endif
            }
            else if (current_platform == PLATFORM_PS)
            {
                sprintf(dev_env, _("%s Edition / %s"),
                                 TITLE_PLATFORM_PS, _("Developer Mode"));
                sprintf(os_env, _("%s Edition"),
                                TITLE_PLATFORM_PS);
            }
            else if (current_platform == PLATFORM_STEAMDECK)
            {
                sprintf(dev_env, _("%s Edition / %s"),
                                 TITLE_PLATFORM_STEAMDECK, _("Developer Mode"));
                sprintf(os_env, _("%s Edition"),
                                TITLE_PLATFORM_STEAMDECK);
            }
            else if (current_platform == PLATFORM_SWITCH)
            {
                sprintf(dev_env, _("%s Edition / %s"), TITLE_PLATFORM_SWITCH, _("Developer Mode"));
                sprintf(os_env, _("%s Edition"), TITLE_PLATFORM_SWITCH);
            }
            else if (current_platform == PLATFORM_WII)
            {
                sprintf(dev_env, _("%s Edition / %s"), TITLE_PLATFORM_WII, _("Developer Mode"));
                sprintf(os_env, _("%s Edition"), TITLE_PLATFORM_WII);
            }
            else if (current_platform == PLATFORM_WIIU)
            {
                sprintf(dev_env, _("%s Edition / %s"), TITLE_PLATFORM_WIIU, _("Developer Mode"));
                sprintf(os_env, _("%s Edition"), TITLE_PLATFORM_WIIU);
            }
#endif

#if ENABLE_RFD==1
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(dev_env, MAXSTR, _("RFD Edition / %s"),
                                       _("Developer Mode"));
            sprintf_s(os_env, MAXSTR, _("Recipes for Disaster Edition"));
#else
            sprintf(dev_env, _("RFD Edition / %s"), _("Developer Mode"));
            sprintf(os_env, _("Recipes for Disaster Edition"));
#endif
#endif

#elif NB_HAVE_PB_BOTH==1
            if (server_policy_get_d(SERVER_POLICY_EDITION) == 10002)
            {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(dev_env, MAXSTR, _("Server Datacenter / %s"),
                                           _("Developer Mode"));
                sprintf_s(os_env, MAXSTR, _("Server Datacenter Edition"));
#else
                sprintf(dev_env, _("Server Datacenter / %s"), _("Developer Mode"));
                sprintf(os_env, _("Server Datacenter Edition"));
#endif
            }
            else if (server_policy_get_d(SERVER_POLICY_EDITION) == 10001)
            {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(dev_env, MAXSTR, _("Server Standard / %s"),
                                           _("Developer Mode"));
                sprintf_s(os_env, MAXSTR, _("Server Standard Edition"));
#else
                sprintf(dev_env, _("Server Standard / %s"), _("Developer Mode"));
                sprintf(os_env, _("Server Standard Edition"));
#endif
            }
            else if (server_policy_get_d(SERVER_POLICY_EDITION) == 10000)
            {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(dev_env, MAXSTR, _("Server Essential / %s"),
                                           _("Developer Mode"));
                sprintf_s(os_env, MAXSTR, _("Server Essential Edition"));
#else
                sprintf(dev_env, _("Server Essential / %s"), _("Developer Mode"));
                sprintf(os_env, _("Server Essential Edition"));
#endif
            }
            else if (server_policy_get_d(SERVER_POLICY_EDITION) == 3)
            {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(dev_env, MAXSTR, _("Edu Edition / %s"),
                                           _("Developer Mode"));
                sprintf_s(os_env, MAXSTR, _("Edu Edition"));
#else
                sprintf(dev_env, _("Edu Edition / %s"), _("Developer Mode"));
                sprintf(os_env, _("Edu Edition"));
#endif
            }
            else if (server_policy_get_d(SERVER_POLICY_EDITION) == 2)
            {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(dev_env, MAXSTR, _("Enterprise Edition / %s"),
                                           _("Developer Mode"));
                sprintf_s(os_env, MAXSTR, _("Enterprise Edition"));
#else
                sprintf(dev_env, _("Enterprise Edition / %s"), _("Developer Mode"));
                sprintf(os_env, _("Enterprise Edition"));
#endif
            }
            else if (server_policy_get_d(SERVER_POLICY_EDITION) == 1)
            {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(dev_env, MAXSTR, _("Pro Edition / %s"),
                                           _("Developer Mode"));
                sprintf_s(os_env, MAXSTR, _("Pro Edition"));
#else
                sprintf(dev_env, _("Pro Edition / %s"), _("Developer Mode"));
                sprintf(os_env, _("Pro Edition"));
#endif
            }
            else if (server_policy_get_d(SERVER_POLICY_EDITION) == 0)
            {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(dev_env, MAXSTR, _("Home Edition / %s"),
                                           _("Developer Mode"));
                sprintf_s(os_env, MAXSTR, _("Home Edition"));
#else
                sprintf(dev_env, _("Home Edition / %s"), _("Developer Mode"));
                sprintf(os_env, _("Home Edition"));
#endif
            }
#endif

#if ENABLE_RFD==1
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(dev_env, MAXSTR, _("RFD Edition / %s"),
                     _("Developer Mode"));
            sprintf_s(os_env, MAXSTR, _("Recipes for Disaster Edition"));
#else
            sprintf(dev_env, _("RFD Edition / %s"), _("Developer Mode"));
            sprintf(os_env, _("Recipes for Disaster Edition"));
#endif
#endif

#if NB_STEAM_API==0 && NB_EOS_SDK==0
            if (strcmp(queue, keyphrase) == 0)
            {
                config_set_cheat();
                gui_set_label(play_id, gt_prefix("menu^Cheat"));
                gui_pulse(play_id, 1.2f);
                if (edition_id) gui_set_label(edition_id, dev_env);
            }
            else if (config_cheat())
            {
                glSetWireframe_(0);
                config_clr_cheat();
                gui_set_label(play_id, gt_prefix("menu^Play"));
                gui_pulse(play_id, 1.2f);
                if (edition_id) gui_set_label(edition_id, os_env);
            }
#endif
            break;
#endif
#endif
    }

    return 1;
}

static int title_gui(void)
{
    int root_id, id, jd, kd;

    /* Build the title GUI. */

    if ((root_id = gui_root()))
    {
        if ((id = gui_vstack(root_id)))
        {
#ifdef SWITCHBALL_TITLE_BTN_V2
            gui_space(id);
#endif

            char os_env[MAXSTR], dev_env[MAXSTR];
#if NB_HAVE_PB_BOTH==1 && \
    (NEVERBALL_FAMILY_API != NEVERBALL_PC_FAMILY_API || !defined(TITLE_USE_DVD_BOX))
#ifndef __EMSCRIPTEN__
#if ENABLE_HMD
            sprintf(dev_env, _(editions_developer[EDITION_CURRENT]),
                    "OpenHMD", _("Developer Mode"));
            sprintf(os_env, _(editions_common[EDITION_CURRENT]), "OpenHMD");
#elif defined(COMMUNITY_EDITION)
            sprintf(dev_env, _("%s Edition / %s"), _("Community"),
                    _("Developer Mode"));
            sprintf(os_env, _("%s Edition"), _("Community"));
#else
            if (current_platform == PLATFORM_PC)
            {
#if defined(__linux__)
                sprintf(dev_env, _("%s Edition / %s"),
                        TITLE_PLATFORM_CINMAMON, _("Developer Mode"));
                sprintf(os_env, _("%s Edition"), TITLE_PLATFORM_CINMAMON);
#else
                sprintf(dev_env, _(editions_developer[EDITION_CURRENT]), 
                        TITLE_PLATFORM_WINDOWS, _("Developer Mode"));
                sprintf(os_env, _(editions_common[EDITION_CURRENT]), TITLE_PLATFORM_WINDOWS);
#endif
            }
            else if (current_platform == PLATFORM_XBOX)
            {
#if NEVERBALL_FAMILY_API == NEVERBALL_XBOX_360_FAMILY_API
                sprintf(dev_env, _("%s Edition / %s"),
                        TITLE_PLATFORM_XBOX_360, _("Developer Mode"));
                sprintf(os_env, _("%s Edition"), TITLE_PLATFORM_XBOX_360);
#else
                sprintf(dev_env, _("%s Edition / %s"),
                        TITLE_PLATFORM_XBOX_ONE, _("Developer Mode"));
                sprintf(os_env, _("%s Edition"), TITLE_PLATFORM_XBOX_ONE);
#endif
            }
            else if (current_platform == PLATFORM_PS)
            {
                sprintf(dev_env, _("%s Edition / %s"),
                        TITLE_PLATFORM_PS, _("Developer Mode"));
                sprintf(os_env, _("%s Edition"), TITLE_PLATFORM_PS);
            }
            else if (current_platform == PLATFORM_STEAMDECK)
            {
                sprintf(dev_env, _("%s Edition / %s"),
                        TITLE_PLATFORM_STEAMDECK, _("Developer Mode"));
                sprintf(os_env, _("%s Edition"), TITLE_PLATFORM_STEAMDECK);
            }
            else if (current_platform == PLATFORM_SWITCH)
            {
                sprintf(dev_env, _("%s Edition / %s"), 
                        TITLE_PLATFORM_SWITCH, _("Developer Mode"));
                sprintf(os_env, _("%s Edition"), TITLE_PLATFORM_SWITCH);
            }
            else if (current_platform == PLATFORM_WII)
            {
                sprintf(dev_env, _("%s Edition / %s"),
                        TITLE_PLATFORM_WII, _("Developer Mode"));
                sprintf(os_env, _("%s Edition"), TITLE_PLATFORM_WII);
            }
            else if (current_platform == PLATFORM_WIIU)
            {
                sprintf(dev_env, _("%s Edition / %s"),
                        TITLE_PLATFORM_WIIU, _("Developer Mode"));
                sprintf(os_env, _("%s Edition"), TITLE_PLATFORM_WIIU);
            }
#endif

#if NB_STEAM_API==0 && NB_EOS_SDK==0
            if (config_cheat())
            {
                if ((jd = gui_vstack(id)))
                {
                    gui_title_header(jd, video.aspect_ratio < 1.0f ? "Neverball" :
                                                                     "  Neverball  ", 
                                         video.aspect_ratio < 1.0f ? GUI_MED :
                                                                     GUI_LRG,
                                                                     0, 0);
#if NB_STEAM_API==1
                    edition_id = gui_label(jd, _("Steam Valve Edition"),
                                               GUI_SML, GUI_COLOR_WHT);
#elif NB_EOS_SDK==1
                    edition_id = gui_label(jd, _("Epic Games Edition"),
                                               GUI_SML, GUI_COLOR_WHT);
#else
                    edition_id = gui_label(jd, os_env,
                                               GUI_SML, GUI_COLOR_WHT);
#endif
                    gui_set_rect(jd, GUI_ALL);
                }
            }
            else
#endif
#endif
            {
                if ((jd = gui_vstack(id)))
                {
                    /* Use with edition below title */
                    gui_title_header(jd, video.aspect_ratio < 1.0f ? "Neverball" :
                                                                     "  Neverball  ",
                                         video.aspect_ratio < 1.0f ? GUI_MED :
                                                                     GUI_LRG,
                                                                     0, 0);
#if NB_STEAM_API==1
                    edition_id = gui_label(jd, _("Steam Valve Edition"), GUI_SML, GUI_COLOR_WHT);
#elif NB_EOS_SDK==1
                    edition_id = gui_label(jd, _("Epic Games Edition"), GUI_SML, GUI_COLOR_WHT);
#elif defined(__EMSCRIPTEN__)
                    edition_id = gui_label(jd, _("WebGL Edition"), GUI_SML, GUI_COLOR_WHT);
#else
                    edition_id = gui_label(jd, os_env, GUI_SML, GUI_COLOR_WHT);
#endif
                    gui_set_rect(jd, GUI_ALL);
                }
            }

#if ENABLE_RFD==1
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(dev_env, MAXSTR, _("RFD Edition / %s"),
                      _("Developer Mode"));
            sprintf_s(os_env, MAXSTR, _("Recipes for Disaster Edition"));
#else
            sprintf(dev_env, _("RFD Edition / %s"), _("Developer Mode"));
            sprintf(os_env, _("Recipes for Disaster Edition"));
#endif
#endif
#elif NB_HAVE_PB_BOTH==1
            if (server_policy_get_d(SERVER_POLICY_EDITION) == 10002)
            {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(dev_env, MAXSTR, _("Server Datacenter / %s"),
                          _("Developer Mode"));
                sprintf_s(os_env, MAXSTR, _("Server Datacenter Edition"));
#else
                sprintf(dev_env, _("Server Datacenter / %s"), _("Developer Mode"));
                sprintf(os_env, _("Server Datacenter Edition"));
#endif
            }
            else if (server_policy_get_d(SERVER_POLICY_EDITION) == 10001)
            {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(dev_env, MAXSTR, _("Server Standard / %s"),
                          _("Developer Mode"));
                sprintf_s(os_env, MAXSTR, _("Server Standard Edition"));
#else
                sprintf(dev_env, _("Server Standard / %s"), _("Developer Mode"));
                sprintf(os_env, _("Server Standard Edition"));
#endif
            }
            else if (server_policy_get_d(SERVER_POLICY_EDITION) == 10000)
            {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(dev_env, MAXSTR, _("Server Essential / %s"),
                          _("Developer Mode"));
                sprintf_s(os_env, MAXSTR, _("Server Essential Edition"));
#else
                sprintf(dev_env, _("Server Essential / %s"), _("Developer Mode"));
                sprintf(os_env, _("Server Essential Edition"));
#endif
            }
            else if (server_policy_get_d(SERVER_POLICY_EDITION) == 3)
            {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(dev_env, MAXSTR, _("Edu Edition / %s"),
                          _("Developer Mode"));
                sprintf_s(os_env, MAXSTR, _("Edu Edition"));
#else
                sprintf(dev_env, _("Edu Edition / %s"), _("Developer Mode"));
                sprintf(os_env, _("Edu Edition"));
#endif
            }
            else if (server_policy_get_d(SERVER_POLICY_EDITION) == 2)
            {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(dev_env, MAXSTR, _("Enterprise Edition / %s"),
                          _("Developer Mode"));
                sprintf_s(os_env, MAXSTR, _("Enterprise Edition"));
#else
                sprintf(dev_env, _("Enterprise Edition / %s"), _("Developer Mode"));
                sprintf(os_env, _("Enterprise Edition"));
#endif
            }
            else if (server_policy_get_d(SERVER_POLICY_EDITION) == 1)
            {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(dev_env, MAXSTR, _("Pro Edition / %s"),
                          _("Developer Mode"));
                sprintf_s(os_env, MAXSTR, _("Pro Edition"));
#else
                sprintf(dev_env, _("Pro Edition / %s"), _("Developer Mode"));
                sprintf(os_env, _("Pro Edition"));
#endif
            }
            else if (server_policy_get_d(SERVER_POLICY_EDITION) == 0)
            {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(dev_env, MAXSTR, _("Home Edition / %s"),
                          _("Developer Mode"));
                sprintf_s(os_env, MAXSTR, _("Home Edition"));
#else
                sprintf(dev_env, _("Home Edition / %s"), _("Developer Mode"));
                sprintf(os_env, _("Home Edition"));
#endif
            }

#if ENABLE_RFD==1
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(dev_env, MAXSTR, _("RFD Edition / %s"),
                      _("Developer Mode"));
            sprintf_s(os_env, MAXSTR, _("Recipes for Disaster Edition"));
#else
            sprintf(dev_env, _("RFD Edition / %s"), _("Developer Mode"));
            sprintf(os_env, _("Recipes for Disaster Edition"));
#endif
#endif

            if ((jd = gui_vstack(id)))
            {
                gui_title_header(jd, video.aspect_ratio < 1.0f ? "Neverball" : "  Neverball  ", 
                                     video.aspect_ratio < 1.0f ? GUI_MED : 
                                                                 GUI_LRG,
                                                                 0, 0);
                if (server_policy_get_d(SERVER_POLICY_EDITION) > -1)
                    edition_id = gui_label(jd, config_cheat() ? dev_env : os_env,
                                               GUI_SML, GUI_COLOR_WHT);
                gui_set_rect(jd, GUI_ALL);
            }
#else
            gui_title_header(id, video.aspect_ratio < 1.0f ? "Neverball" :
                                                             "  Neverball  ",
                                 video.aspect_ratio < 1.0f ? GUI_MED :
                                                             GUI_LRG, 0, 0);
#endif

#ifndef SWITCHBALL_TITLE_BTN_V2
            if (!title_lockscreen)
            {
                gui_space(id);

                if ((jd = gui_hstack(id)))
                {
                    int btn_size = video.aspect_ratio < 1.0f ? GUI_SML : GUI_MED;

                    gui_filler(jd);

                    if ((kd = gui_varray(jd)))
                    {
#if NB_STEAM_API==0 && NB_EOS_SDK==0
                        if (config_cheat())
                            play_id = gui_start(kd, gt_prefix("menu^Cheat"),
                                                    btn_size, TITLE_PLAY, 0);
                        else
#endif
                            play_id = gui_start(kd, gt_prefix("menu^Play"),
                                                    btn_size, TITLE_PLAY, 0);

                        /* Hilight the start button. */
                        gui_set_hilite(play_id, 1);

#ifdef CONFIG_INCLUDES_ACCOUNT
                        if (server_policy_get_d(SERVER_POLICY_SHOP_ENABLED))
                            gui_state(kd, gt_prefix("menu^Shop"),
                                          btn_size, TITLE_SHOP, 0);
#endif
                        gui_state(kd, gt_prefix("menu^Replay"),
                                      btn_size, TITLE_DEMO, 0);

                        gui_state(kd, gt_prefix("menu^Help"),
                                      btn_size, TITLE_HELP, 0);

                        gui_state(kd, gt_prefix("menu^Options"),
                                      btn_size, TITLE_CONF, 0);

#ifndef __EMSCRIPTEN__
                        /* Have some full screens? (Pressing ALT+F4 is not recommended) */

                        if (support_exit && config_get_d(CONFIG_FULLSCREEN))
                            gui_state(kd, gt_prefix("menu^Exit"),
                                          btn_size, GUI_BACK, 0);
#endif
                    }

                    gui_filler(jd);
                }
            }

            gui_layout(id, 0, 0);
#else
            gui_layout(id, 0, title_lockscreen ? 0 : +1);
#endif
        }

        if (!title_lockscreen)
        {
#ifdef SWITCHBALL_TITLE_BTN_V2
            if ((id = gui_varray(root_id)))
            {
                int btn_size = GUI_TCH;

#if NB_STEAM_API==0 && NB_EOS_SDK==0
                if (config_cheat())
                    play_id = gui_start(id, gt_prefix("menu^Cheat"),
                                            btn_size, TITLE_PLAY, 0);
                else
#endif
                    play_id = gui_start(id, gt_prefix("menu^Play"),
                                            btn_size, TITLE_PLAY, 0);

                /* Hilight the start button. */
                gui_set_hilite(play_id, 1);

#ifdef CONFIG_INCLUDES_ACCOUNT
                if (server_policy_get_d(SERVER_POLICY_SHOP_ENABLED))
                    gui_state(id, gt_prefix("menu^Shop"),
                                  btn_size, TITLE_SHOP, 0);
#endif
                gui_state(id, gt_prefix("menu^Replay"),
                              btn_size, TITLE_DEMO, 0);

                gui_state(id, gt_prefix("menu^Help"),
                              btn_size, TITLE_HELP, 0);

                gui_state(id, gt_prefix("menu^Options"),
                              btn_size, TITLE_CONF, 0);

#ifndef __EMSCRIPTEN__
                /* Have some full screens? (Pressing ALT+F4 is not recommended) */

                if (support_exit && config_get_d(CONFIG_FULLSCREEN))
                    gui_state(id, gt_prefix("menu^Exit"),
                                  btn_size, GUI_BACK, 0);
#endif

                gui_space(id);
                gui_space(id);

                gui_layout(id, 0, -1);
            }
#endif

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
            if ((id = gui_vstack(root_id)))
            {
                if ((jd = gui_hstack(id)))
                {
#if NB_HAVE_PB_BOTH==1
                    const char title_social_image_paths[3][MAXSTR] =
                    {
                        "gui/social/ic_gitea.jpg",
                        "gui/social/ic_github.jpg",
                        "gui/social/ic_discord.jpg",
                    };
#else
                    const char title_social_image_paths[2][MAXSTR] =
                    {
                        "gui/social/ic_github.jpg",
                        "gui/social/ic_discord.jpg",
                    };
#endif

                    for (int i = (ARRAYSIZE(title_social_image_paths) - 1);
                         i >= 0; i--)
                    {
                        int btn_social = gui_image(jd, title_social_image_paths[i],
                                                       video.device_h / 16,
                                                       video.device_h / 16);

                        gui_set_state(btn_social, TITLE_SOCIAL, i);

                        gui_clr_rect(btn_social);
                    }

                    gui_space(jd);
                }

                gui_space(id);

                gui_layout(id, -1, -1);
            }
#endif

#if NB_HAVE_PB_BOTH==1
#if ENABLE_VERSION
            if ((id = gui_label(root_id, "Neverball " VERSION, GUI_TNY, GUI_COLOR_WHT2)))
            {
                gui_clr_rect(id);
                gui_layout(id, -1, -1);
            }
#endif

#ifdef CONFIG_INCLUDES_ACCOUNT
            char account_coinsattr[MAXSTR], account_gemsattr[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(account_gemsattr, MAXSTR, "%s %d", GUI_DIAMOND,
                      account_get_d(ACCOUNT_DATA_WALLET_GEMS));
            sprintf_s(account_coinsattr, MAXSTR, "%s %d", GUI_COIN,
                      account_get_d(ACCOUNT_DATA_WALLET_COINS));
#else
            sprintf(account_gemsattr, "%s %d", GUI_DIAMOND,
                    account_get_d(ACCOUNT_DATA_WALLET_GEMS));
            sprintf(account_coinsattr, "%s %d", GUI_COIN,
                    account_get_d(ACCOUNT_DATA_WALLET_COINS));
#endif

            if (server_policy_get_d(SERVER_POLICY_SHOP_ENABLED))
            {
                if ((id = gui_vstack(root_id)))
                {
                    gui_space(id);

                    if ((jd = gui_hstack(id)))
                    {
                        gui_space(jd);

                        if (!CHECK_ACCOUNT_BANKRUPT)
                        {
                            gui_state(jd, "+", GUI_XS, TITLE_SHOP, 1);
                            gui_label(jd, account_gemsattr, GUI_XS, gui_wht, gui_cya);
                        }

                        gui_label(jd, account_coinsattr, GUI_XS, gui_wht, gui_yel);
                    }

                    gui_layout(id, +1, +1);
                }
            }
#endif
#else
#if ENABLE_VERSION
            if ((id = gui_label(root_id, "Neverball " VERSION, GUI_TNY, GUI_COLOR_WHT2)))
            {
                gui_clr_rect(id);
                gui_layout(id, -1, -1);
            }
#endif

            if ((id = gui_vstack(root_id)))
            {
#if ENABLE_FETCH!=0 && \
    !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__)
                if ((jd = gui_hstack(id)))
                {
                    gui_space(jd);
                    gui_state(jd, _("Addons"), GUI_SML, TITLE_PACKAGES, 0);
                }
#endif

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
                if ((jd = gui_hstack(id)))
                {
                    gui_state(jd, _("Unlock full game"),
                                  GUI_SML, TITLE_UNLOCK_FULL_GAME, 0);
                }
#endif

                gui_layout(id, +1, -1);
            }
#endif
        }
        else
        {
            if ((id = gui_vstack(root_id)))
            {
                char presstostart_pc_attr[MAXSTR];

                if ((jd = gui_hstack(id)))
                {
                    SAFECPY(presstostart_pc_attr, N_("Press to start"));

                    int text_height = gui_measure(_(presstostart_pc_attr), GUI_SML).h;

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
                    if (current_platform == PLATFORM_PC)
#endif
                    {
                        float upscaled = text_height * 1.2f;
                        gui_image(jd, "gui/lockscr/mouse.png", upscaled, upscaled);

                        gui_space(jd);
                    }

                    gui_label(jd, _(presstostart_pc_attr), GUI_SML, GUI_COLOR_WHT);

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
                    if (current_platform != PLATFORM_PC)
                    {
                        gui_space(jd);
                        console_gui_create_start_button(jd, config_get_d(CONFIG_JOYSTICK_BUTTON_START));
                    }
#endif
                    gui_set_rect(jd, GUI_ALL);
                }

                gui_space(id);
                gui_layout(id, 0, -1);
            }
        }
    }

    return root_id;
}

static int filter_cmd(const union cmd *cmd)
{
    return (cmd ? cmd->type != CMD_SOUND : 1);
}

static int title_enter(struct state *st, struct state *prev)
{
    activity_services_group(AS_GROUP_NONE);

#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT)
    if (prev != &st_title)
        account_wgcl_restart_attempt();
#endif

    if (title_load_lockscreen)
        title_load_lockscreen = 0;

    title_lockscreen = title_can_unlock;

    if (prev == &st_title)
        return title_gui();

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    support_exit = (current_platform != PLATFORM_PS &&
                    current_platform != PLATFORM_STEAMDECK &&
                    current_platform != PLATFORM_SWITCH) &&
                   (current_platform == PLATFORM_PC);
#endif

    progress_exit();
    progress_init(MODE_NONE);

    title_freeze_all = 0;

    game_proxy_filter(filter_cmd);

    /* Start the title screen music. */

    audio_music_fade_to(0.5f, switchball_useable() ? "bgm/title-switchball.ogg" :
                                                     BGM_TITLE_CONF_LANGUAGE, 1);

    /* Initialize the build-in nor title level for display. */

    game_fade_color(0.0f, 0.0f, 0.0f);

    if (switchball_useable() && load_title_background())
        mode = TITLE_MODE_LEVEL;
    else if (config_get_d(CONFIG_MAINMENU_PANONLY) && load_title_background())
        mode = TITLE_MODE_LEVEL;
#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT)
    else if (!CHECK_ACCOUNT_BANKRUPT &&
             progress_replay_full("gui/title/title-l.nbr", 0, 0, 0, 0, 0, 0))
#else
    else if (progress_replay_full("gui/title/title-l.nbr", 0, 0, 0, 0, 0, 0))
#endif
        TITLE_BG_DEMO_INIT(TITLE_MODE_DEMO, 1);
    else if (load_title_background())
        mode = TITLE_MODE_LEVEL;
    else
        mode = TITLE_MODE_NONE;

    real_time = 0.0f;

    return title_gui();
}

static void title_leave(struct state *st, struct state *next, int id)
{
    if (title_lockscreen && next != &st_null) return;

    if (next == &st_title)
    {
        gui_delete(id);
        return;
    }

    demo_replay_stop(0);

    if (items)
    {
        demo_dir_free(items);
        items = NULL;
    }

    game_proxy_filter(NULL);

    if (next == &st_null ||
        next == &st_conf)
    {
        game_client_free(NULL);

        if (next == &st_null)
            game_server_free(NULL);
    }

    progress_exit();

    gui_delete(id);
}

static void title_paint(int id, float t)
{
    game_client_draw(0, t);

    gui_paint(id);

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    console_gui_title_paint();
#endif
}

static void title_timer(int id, float dt)
{
    geom_step(dt);

    if (title_freeze_all)
    {
        gui_timer(id, dt);
        return;
    }

    static const char *demo = NULL;

    real_time += dt;

    switch (mode)
    {
        case TITLE_MODE_LEVEL: /* Pan across title level. */

            if (real_time <= 20.0f || title_prequit)
                game_client_fly(fcosf(V_PI * real_time / 20.0f));
            else if (!title_prequit)
            {
                game_fade(+1.0f);
                real_time = 0.0f;
                mode = TITLE_MODE_LEVEL_FADE;
            }
            break;

        case TITLE_MODE_LEVEL_FADE: /* Fade out.  Load demo level. */

            if (real_time > 1.0f && !title_prequit && !st_global_animating())
            {
                if (items)
                {
                    demo_dir_free(items);
                    items = NULL;
                }

                items = demo_dir_scan();

                if ((demo = pick_demo(items)))
                {
                    if (progress_replay_full(demo, 0, 0, 0, 0, 0, 0))
                        TITLE_BG_DEMO_INIT(TITLE_MODE_DEMO, 1);
#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT)
                    else if (!CHECK_ACCOUNT_BANKRUPT &&
                             !config_get_d(CONFIG_MAINMENU_PANONLY) &&
                             progress_replay_full(left_handed ? "gui/title/title-l.nbr" :
                                                                "gui/title/title-r.nbr", 0, 0, 0, 0, 0, 0))
#else
                    else if (!config_get_d(CONFIG_MAINMENU_PANONLY) && 
                             progress_replay_full(left_handed ? "gui/title/title-l.nbr" :
                                                                "gui/title/title-r.nbr", 0, 0, 0, 0, 0, 0))
#endif
                        TITLE_BG_DEMO_INIT(TITLE_MODE_DEMO, 1);
                    else if (load_title_background())
                        mode = TITLE_MODE_LEVEL;
                }
#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT)
                else if (!CHECK_ACCOUNT_BANKRUPT &&
                         !config_get_d(CONFIG_MAINMENU_PANONLY) &&
                         progress_replay_full(left_handed ? "gui/title/title-l.nbr" :
                                                            "gui/title/title-r.nbr", 0, 0, 0, 0, 0, 0))
#else
                else if (!config_get_d(CONFIG_MAINMENU_PANONLY) &&
                         progress_replay_full(left_handed ? "gui/title/title-l.nbr" :
                                                            "gui/title/title-r.nbr", 0, 0, 0, 0, 0, 0))
#endif
                {
                    game_fade(-1.0f);
                    TITLE_BG_DEMO_INIT(TITLE_MODE_DEMO, 1);
                }
                else if (load_title_background())
                    mode = TITLE_MODE_LEVEL;
                else
                    mode = TITLE_MODE_NONE;
            }
            break;

        case TITLE_MODE_DEMO: /* Run demo. */

            if (!demo_replay_step(dt) && !title_prequit)
            {
                left_handed = !left_handed;
                demo_replay_stop(0);
                game_fade(+1.0f);
                real_time = 0.0f;
                if (!title_prequit) mode = TITLE_MODE_DEMO_FADE;
            }
            else if (!title_prequit)
                game_client_blend(demo_replay_blend());

            break;

        case TITLE_MODE_DEMO_FADE: /* Fade out.  Load build-in demo level or load title level. */

            if (real_time > 1.0f && !title_prequit && !st_global_animating())
            {
                real_time = 0.0f;

                if (switchball_useable() && load_title_background())
                    mode = TITLE_MODE_LEVEL;
#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT)
                else if (!CHECK_ACCOUNT_BANKRUPT &&
                         !config_get_d(CONFIG_MAINMENU_PANONLY) &&
                         progress_replay_full(left_handed ? "gui/title/title-l.nbr" :
                                                            "gui/title/title-r.nbr", 0, 0, 0, 0, 0, 0))
#else
                else if (!config_get_d(CONFIG_MAINMENU_PANONLY) &&
                         progress_replay_full(left_handed ? "gui/title/title-l.nbr" :
                                                            "gui/title/title-r.nbr", 0, 0, 0, 0, 0, 0))
#endif
                    TITLE_BG_DEMO_INIT(TITLE_MODE_DEMO, 1);
                else if (load_title_background())
                    mode = TITLE_MODE_LEVEL;
                else
                    mode = TITLE_MODE_NONE;
            }
            break;
    }

    gui_timer(id, dt);
    game_step_fade(dt);
}

static void title_point(int id, int x, int y, int dx, int dy)
{
    int jd;

#ifndef __EMSCRIPTEN__
    console_gui_toggle(0);
#endif

    if ((jd = gui_point(id, x, y)))
        gui_pulse(jd, 1.2f);
}

static void title_stick(int id, int a, float v, int bump)
{
    if (title_lockscreen) return;

    int jd;

#ifndef __EMSCRIPTEN__
    console_gui_toggle(1);
#endif

    if ((jd = gui_stick(id, a, v, bump)))
        gui_pulse(jd, 1.2f);
}

static int title_click(int b, int d)
{
    if (title_lockscreen && title_can_unlock &&
        b == SDL_BUTTON_LEFT && d)
    {
        title_can_unlock = 0;
        return goto_state(&st_title);
    }
    else if (gui_click(b, d) && !title_lockscreen)
        return st_buttn(config_get_d(CONFIG_JOYSTICK_BUTTON_A), 1);

    return 1;
}

static int title_keybd(int c, int d)
{
    if (title_lockscreen) return 1;

#ifndef __EMSCRIPTEN__
    console_gui_toggle(0);
#endif

    if (d)
    {
#ifndef __EMSCRIPTEN__
        if (c == KEY_EXIT && support_exit)
        {
            title_prequit = 1;

            /* bye! */

            return 0;
        }
#endif

#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
        if (c >= SDLK_a && c <= SDLK_z)
            return title_action(GUI_CHAR, c);
#endif
    }
    return 1;
}

static int title_buttn(int b, int d)
{
    /* Lockscreen menu */

    if (title_lockscreen)
    {
        if (d && config_tst_d(CONFIG_JOYSTICK_BUTTON_START, b) && title_can_unlock)
        {
            title_can_unlock = 0;
            goto_state(&st_title);
        }

        return 1;
    }

    /* Main menu */

    else if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return title_action(gui_token(active), gui_value(active));
#ifndef __EMSCRIPTEN__
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b) && support_exit)
        {
            title_prequit = 1;

            /* bye! */

            return 0;
        }
#endif
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

struct state st_title = {
    title_enter,
    title_leave,
    title_paint,  /* Default: shared_paint */
    title_timer,
    title_point,
    title_stick,
    shared_angle,
    title_click,
    title_keybd,
    title_buttn
};
