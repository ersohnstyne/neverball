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

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <string.h>

#if NB_HAVE_PB_BOTH==1
#include "networking.h"

#ifndef __EMSCRIPTEN__
#include "console_control_gui.h"
#endif

#include "campaign.h" // New: Campaign
#include "account.h"
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
#include "progress.h"
#include "text.h"

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
#include "st_package.h"
#include "st_play.h"

#define TITLE_USE_DVD_BOX_OR_EMAIL

#define WINDOWS_SIMPLIFIED
#define SWITCHBALL_TITLE

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

static int init_title_level(void)
{
#if NB_HAVE_PB_BOTH
    game_client_toggle_show_balls(!CHECK_ACCOUNT_BANKRUPT);
#else
    game_client_toggle_show_balls(1);
#endif

    if (switchball_useable())
    {
        if (game_client_init("gui/switchball-title.sol"))
        {
            union cmd cmd;

            cmd.type = CMD_GOAL_OPEN;
            game_proxy_enq(&cmd);
            game_client_sync(NULL);

            game_client_fly(1.0f);

            return 1;
        }
    }
    else if (game_client_init("gui/title.sol"))
    {
        union cmd cmd;

        cmd.type = CMD_GOAL_OPEN;
        game_proxy_enq(&cmd);
        game_client_sync(NULL);

        game_client_fly(1.0f);

        return 1;
    }

    return 0;
}

static const char *check_unlocked_demo(struct demo *rawdemo)
{
    if (!rawdemo)
    {
        /* Nope, only replays are valid! */
        return NULL;
    }

    int limit = config_get_d(CONFIG_ACCOUNT_LOAD);
    int max = 0;

    if (rawdemo->status == 3) {
        max = 3;
    }
    else if (rawdemo->status == 1 || rawdemo->status == 0) {
        max = 2;
    }
    else if (rawdemo->status == 2) {
        max = 1;
    }

    /* Can access into the replay? */
    return (max <= limit) ? rawdemo->path : NULL;
}

static int demo_is_loaded;

static const char *pick_demo(Array items)
{
    struct dir_item *item;

#pragma region ReplaySuperComplexityCode
    /* With super complexity code in replay */
    if (config_get_d(CONFIG_ACCOUNT_LOAD) != 3)
    {
        int total = array_len(items);
        int selectedDemo = rand_between(0, total - 1);

        if (!demo_is_loaded)
        {
            demo_is_loaded = 1;
            demo_dir_load(items, 0, total - 1);
        }

        struct demo *demoplayable;
        demoplayable = ((struct demo *) ((struct dir_item *) array_get(items, selectedDemo))->data);
#if defined(COVID_HIGH_RISK)
        const char *demopath = check_unlocked_demo(demoplayable);
        if (demopath)
            return demopath;
        else
        {
            log_errorf("Replay files deleted due covid high risks!: %s", 
                       demoplayable->path);
            demo_is_loaded = 0;
            fs_remove(demoplayable->path);
            demo_dir_free(items);
            items = NULL;
            return NULL;
        }
#else
        return check_unlocked_demo(demoplayable);
#endif
    }
#pragma endregion

    /* All replay status */

    if ((item = (struct dir_item *) array_rnd(items)))
    {
        //trg_item = item;
        return item->path;
    }
    return NULL;
}

/*---------------------------------------------------------------------------*/

enum
{
    TITLE_MODE_NONE,
    TITLE_MODE_LEVEL,
    TITLE_MODE_LEVEL_FADE,
    TITLE_MODE_DEMO,
    TITLE_MODE_DEMO_FADE,
    TITLE_MODE_BUILD_IN
};

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
    TITLE_PACKAGES
};

int edition_id;
int system_version_build_id;
int copyright_id;

static int title_check_playername(const char *regname)
{
    for (int i = 0; i < strlen(regname); i++)
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
        {
            log_errorf("Can't accept other charsets!\n", regname[i]);
            return 0;
        }
    }

    return 1;
}

static int title_action(int tok, int val)
{
    static const char keyphrase[] = "msxdev";
    static char queue[sizeof (keyphrase)] = "";

    size_t queue_len = strlen(queue);

    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
    case GUI_BACK:
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        if (current_platform != PLATFORM_SWITCH
         && current_platform != PLATFORM_STEAMDECK)
#endif
        {
            title_prequit = 1;
            game_fade(+4.0);
            goto_state_full(&st_null, 0, 0, 0); // bye!

            game_server_free(NULL);
            game_client_free(NULL);
            game_base_free(NULL);
            return 0;
        }
        break;

    case TITLE_PLAY:
        if (text_length(config_get_s(CONFIG_PLAYER)) < 3 ||
            !title_check_playername(config_get_s(CONFIG_PLAYER)))
            return goto_playgame_register();
        else
            return goto_playgame();
        break;

#if defined(LEVELGROUPS_INCLUDES_CAMPAIGN) && defined(CONFIG_INCLUDES_ACCOUNT)
    case TITLE_SHOP:
        campaign_init();
        goto_state_full(&st_shop, 0, GUI_ANIMATION_N_CURVE, 0);
        campaign_quit();
        break;
#elif defined(CONFIG_INCLUDES_ACCOUNT)
    case TITLE_SHOP:
        goto_state_full(&st_shop, 0, GUI_ANIMATION_N_CURVE, 0);
        break;
#endif
    case TITLE_HELP: return goto_state(&st_help); break;
    case TITLE_DEMO: return goto_state(&st_demo); break;
    case TITLE_CONF:
        game_fade(+4.0);
        return goto_conf(&st_title, 0, 0);
        break;
#if NB_HAVE_PB_BOTH!=1
    case TITLE_PACKAGES: return goto_state(&st_packages); break;
#endif
#ifndef __EMSCRIPTEN__
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD
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
#if  NEVERBALL_FAMILY_API != NEVERBALL_PC_FAMILY_API || !defined(TITLE_USE_DVD_BOX_OR_EMAIL)
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
#endif

#if ENABLE_RFD==1
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(dev_env, dstSize, _("RFD Edition / %s"),
                 _("Developer Mode"));
        sprintf_s(os_env, dstSize, _("Recipes for Disaster Edition"));
#else
        sprintf(dev_env, _("RFD Edition / %s"), _("Developer Mode"));
        sprintf(os_env, _("Recipes for Disaster Edition"));
#endif
#endif

#elif NB_HAVE_PB_BOTH==1
        if (server_policy_get_d(SERVER_POLICY_EDITION) == 10002)
        {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(dev_env, dstSize, _("Server Datacenter / %s"),
                     _("Developer Mode"));
            sprintf_s(os_env, dstSize, _("Server Datacenter Edition"));
#else
            sprintf(dev_env, _("Server Datacenter / %s"), _("Developer Mode"));
            sprintf(os_env, _("Server Datacenter Edition"));
#endif
        }
        else if (server_policy_get_d(SERVER_POLICY_EDITION) == 10001)
        {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(dev_env, dstSize, _("Server Standard / %s"),
                      _("Developer Mode"));
            sprintf_s(os_env, dstSize, _("Server Standard Edition"));
#else
            sprintf(dev_env, _("Server Standard / %s"), _("Developer Mode"));
            sprintf(os_env, _("Server Standard Edition"));
#endif
        }
        else if (server_policy_get_d(SERVER_POLICY_EDITION) == 10000)
        {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(dev_env, dstSize, _("Server Essential / %s"),
                      _("Developer Mode"));
            sprintf_s(os_env, dstSize, _("Server Essential Edition"));
#else
            sprintf(dev_env, _("Server Essential / %s"), _("Developer Mode"));
            sprintf(os_env, _("Server Essential Edition"));
#endif
        }
        else if (server_policy_get_d(SERVER_POLICY_EDITION) == 3)
        {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(dev_env, dstSize, _("Edu Edition / %s"),
                      _("Developer Mode"));
            sprintf_s(os_env, dstSize, _("Edu Edition"));
#else
            sprintf(dev_env, _("Edu Edition / %s"), _("Developer Mode"));
            sprintf(os_env, _("Edu Edition"));
#endif
        }
        else if (server_policy_get_d(SERVER_POLICY_EDITION) == 2)
        {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(dev_env, dstSize, _("Enterprise Edition / %s"), 
                      _("Developer Mode"));
            sprintf_s(os_env, dstSize, _("Enterprise Edition"));
#else
            sprintf(dev_env, _("Enterprise Edition / %s"), _("Developer Mode"));
            sprintf(os_env, _("Enterprise Edition"));
#endif
        }
        else if (server_policy_get_d(SERVER_POLICY_EDITION) == 1)
        {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(dev_env, dstSize, _("Pro Edition / %s"),
                      _("Developer Mode"));
            sprintf_s(os_env, dstSize, _("Pro Edition"));
#else
            sprintf(dev_env, _("Pro Edition / %s"), _("Developer Mode"));
            sprintf(os_env, _("Pro Edition"));
#endif
        }
        else if (server_policy_get_d(SERVER_POLICY_EDITION) == 0)
        {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(dev_env, dstSize, _("Home Edition / %s"),
                      _("Developer Mode"));
            sprintf_s(os_env, dstSize, _("Home Edition"));
#else
            sprintf(dev_env, _("Home Edition / %s"), _("Developer Mode"));
            sprintf(os_env, _("Home Edition"));
#endif
        }
#endif

#if ENABLE_RFD==1
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(dev_env, dstSize, _("RFD Edition / %s"),
                 _("Developer Mode"));
        sprintf_s(os_env, dstSize, _("Recipes for Disaster Edition"));
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
            config_clr_cheat();
            video_set_wire(0);
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

static void title_create_versions(void)
{
    const char *gameversion = VERSION;

    system_version_build_id = gui_label(0, gameversion,
                                           GUI_SML, gui_wht, gui_wht);
    copyright_id = gui_label(0, "© PennyGames", GUI_SML, gui_wht, gui_wht);

    gui_set_rect(system_version_build_id, GUI_NW);
    gui_layout(system_version_build_id, 1, -1);
    gui_set_rect(copyright_id, GUI_NE);
    gui_layout(copyright_id, -1, -1);
}

static int title_gui(void)
{
    int root_id, id, jd, kd;

    /* Build the title GUI. */

    if ((root_id = gui_root()))
    {
        if ((id = gui_vstack(root_id)))
        {
            char os_env[MAXSTR], dev_env[MAXSTR];
#if NEVERBALL_FAMILY_API != NEVERBALL_PC_FAMILY_API || !defined(TITLE_USE_DVD_BOX_OR_EMAIL)
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
                                               GUI_SML, gui_wht, gui_wht);
#elif NB_EOS_SDK==1
                    edition_id = gui_label(jd, _("Epic Games Edition"),
                                               GUI_SML, gui_wht, gui_wht);
#else
                    edition_id = gui_label(jd, os_env,
                                               GUI_SML, gui_wht, gui_wht);
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
                    edition_id = gui_label(jd, _("Steam Valve Edition"), GUI_SML, gui_wht, gui_wht);
#elif NB_EOS_SDK==1
                    edition_id = gui_label(jd, _("Epic Games Edition"), GUI_SML, gui_wht, gui_wht);
#elif defined(__EMSCRIPTEN__)
                    edition_id = gui_label(jd, _("WebGL Edition"), GUI_SML, gui_wht, gui_wht);
#else
                    edition_id = gui_label(jd, os_env, GUI_SML, gui_wht, gui_wht);
#endif
                    gui_set_rect(jd, GUI_ALL);
                }
            }

#if ENABLE_RFD==1
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(dev_env, dstSize, _("RFD Edition / %s"),
                      _("Developer Mode"));
            sprintf_s(os_env, dstSize, _("Recipes for Disaster Edition"));
#else
            sprintf(dev_env, _("RFD Edition / %s"), _("Developer Mode"));
            sprintf(os_env, _("Recipes for Disaster Edition"));
#endif
#endif

#elif NB_HAVE_PB_BOTH==1
            if (server_policy_get_d(SERVER_POLICY_EDITION) == 10002)
            {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(dev_env, dstSize, _("Server Datacenter / %s"),
                          _("Developer Mode"));
                sprintf_s(os_env, dstSize, _("Server Datacenter Edition"));
#else
                sprintf(dev_env, _("Server Datacenter / %s"), _("Developer Mode"));
                sprintf(os_env, _("Server Datacenter Edition"));
#endif
            }
            else if (server_policy_get_d(SERVER_POLICY_EDITION) == 10001)
            {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(dev_env, dstSize, _("Server Standard / %s"),
                          _("Developer Mode"));
                sprintf_s(os_env, dstSize, _("Server Standard Edition"));
#else
                sprintf(dev_env, _("Server Standard / %s"), _("Developer Mode"));
                sprintf(os_env, _("Server Standard Edition"));
#endif
            }
            else if (server_policy_get_d(SERVER_POLICY_EDITION) == 10000)
            {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(dev_env, dstSize, _("Server Essential / %s"),
                          _("Developer Mode"));
                sprintf_s(os_env, dstSize, _("Server Essential Edition"));
#else
                sprintf(dev_env, _("Server Essential / %s"), _("Developer Mode"));
                sprintf(os_env, _("Server Essential Edition"));
#endif
            }
            else if (server_policy_get_d(SERVER_POLICY_EDITION) == 3)
            {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(dev_env, dstSize, _("Edu Edition / %s"),
                          _("Developer Mode"));
                sprintf_s(os_env, dstSize, _("Edu Edition"));
#else
                sprintf(dev_env, _("Edu Edition / %s"), _("Developer Mode"));
                sprintf(os_env, _("Edu Edition"));
#endif
            }
            else if (server_policy_get_d(SERVER_POLICY_EDITION) == 2)
            {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(dev_env, dstSize, _("Enterprise Edition / %s"),
                          _("Developer Mode"));
                sprintf_s(os_env, dstSize, _("Enterprise Edition"));
#else
                sprintf(dev_env, _("Enterprise Edition / %s"), _("Developer Mode"));
                sprintf(os_env, _("Enterprise Edition"));
#endif
            }
            else if (server_policy_get_d(SERVER_POLICY_EDITION) == 1)
            {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(dev_env, dstSize, _("Pro Edition / %s"),
                          _("Developer Mode"));
                sprintf_s(os_env, dstSize, _("Pro Edition"));
#else
                sprintf(dev_env, _("Pro Edition / %s"), _("Developer Mode"));
                sprintf(os_env, _("Pro Edition"));
#endif
            }
            else if (server_policy_get_d(SERVER_POLICY_EDITION) == 0)
            {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(dev_env, dstSize, _("Home Edition / %s"),
                          _("Developer Mode"));
                sprintf_s(os_env, dstSize, _("Home Edition"));
#else
                sprintf(dev_env, _("Home Edition / %s"), _("Developer Mode"));
                sprintf(os_env, _("Home Edition"));
#endif
            }

#if ENABLE_RFD==1
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(dev_env, dstSize, _("RFD Edition / %s"),
                      _("Developer Mode"));
            sprintf_s(os_env, dstSize, _("Recipes for Disaster Edition"));
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
                if (server_policy_get_d(SERVER_POLICY_EDITION) != -1)
                    edition_id = gui_label(jd, config_cheat() ? dev_env : os_env,
                                               GUI_SML, gui_wht, gui_wht);
                gui_set_rect(jd, GUI_ALL);
            }
#else
            gui_title_header(id, video.aspect_ratio < 1.0f ? "Neverball" :
                                                             "  Neverball  ",
                                 video.aspect_ratio < 1.0f ? GUI_MED :
                                                             GUI_LRG, 0, 0);
#endif

            gui_space(id);

            /* Use modern or vertical */
            if ((jd = gui_hstack(id)))
            {
                gui_filler(jd);

                if ((kd = gui_varray(jd)))
                {
#if NB_STEAM_API==0 && NB_EOS_SDK==0
                    if (config_cheat())
                        play_id = gui_start(kd, gt_prefix("menu^Cheat"),
                                                video.aspect_ratio < 1.0f ? GUI_SML :
                                                                            GUI_MED,
                                                TITLE_PLAY, 0);
                    else
#endif
                        play_id = gui_start(kd, gt_prefix("menu^Play"),
                                                video.aspect_ratio < 1.0f ? GUI_SML :
                                                                            GUI_MED,
                                                TITLE_PLAY, 0);

                    /* Hilight the start button. */
                    gui_set_hilite(play_id, 1);

                    /* If defined, sort the button */
#ifdef SWITCHBALL_TITLE
#ifdef CONFIG_INCLUDES_ACCOUNT
                    if (server_policy_get_d(SERVER_POLICY_SHOP_ENABLED))
                        gui_state(kd, gt_prefix("menu^Shop"),
                                      video.aspect_ratio < 1.0f ? GUI_SML : 
                                                                  GUI_MED,
                                      TITLE_SHOP, 0);
#endif

                    gui_state(kd, gt_prefix("menu^Options"),
                                  video.aspect_ratio < 1.0f ? GUI_SML :
                                                              GUI_MED,
                                  TITLE_CONF, 0);
                    gui_state(kd, gt_prefix("menu^Replay"),
                                  video.aspect_ratio < 1.0f ? GUI_SML :
                                                              GUI_MED,
                                  TITLE_DEMO, 0);
                    gui_state(kd, gt_prefix("menu^Help"),
                                  video.aspect_ratio < 1.0f ? GUI_SML :
                                                              GUI_MED,
                                  TITLE_HELP, 0);
#else
#ifdef CONFIG_INCLUDES_ACCOUNT
                    if (server_policy_get_d(SERVER_POLICY_SHOP_ENABLED))
                        gui_state(kd, gt_prefix("menu^Shop"),
                                      video.aspect_ratio < 1.0f ? GUI_SML :
                                                                  GUI_MED,
                                      TITLE_SHOP, 0);
#endif
                    gui_state(kd, gt_prefix("menu^Replay"),
                                  video.aspect_ratio < 1.0f ? GUI_SML :
                                                              GUI_MED,
                                  TITLE_DEMO, 0);
                    gui_state(kd, gt_prefix("menu^Help"),
                                  video.aspect_ratio < 1.0f ? GUI_SML :
                                                              GUI_MED,
                                  TITLE_HELP, 0);
                    gui_state(kd, gt_prefix("menu^Options"),
                                  video.aspect_ratio < 1.0f ? GUI_SML :
                                                              GUI_MED,
                                  TITLE_CONF, 0);
#endif

#ifndef __EMSCRIPTEN__
                    /* Have some full screens? (Pressing ALT+F4 is not recommended) */
                    /*if (support_exit && config_get_d(CONFIG_FULLSCREEN) && current_platform == PLATFORM_PC)
                        gui_state(kd, gt_prefix("menu^Exit"),
                                      GUI_MED, GUI_BACK, 0);*/
#endif
                }

                gui_filler(jd);
            }
            gui_layout(id, 0, 0);
        }

#if !defined(SWITCHBALL_GUI) && ENABLE_FETCH==1
        /*if ((id = gui_vstack(root_id)))
        {
            if ((jd = gui_hstack(id)))
            {
                gui_space(jd);
                gui_state(jd, _("Packages"), GUI_SML, TITLE_PACKAGES, 0);
            }
            gui_space(id);

            // HACK: Buttons overlaps by the game version
            gui_layout(id, +1, 0);
        }*/
#endif
    }

    return root_id;
}

static int filter_cmd(const union cmd *cmd)
{
    return (cmd ? cmd->type != CMD_SOUND : 1);
}

static int title_enter(struct state *st, struct state *prev)
{
#if NB_HAVE_PB_BOTH==1
    support_exit =
        current_platform != PLATFORM_STEAMDECK
     && current_platform != PLATFORM_SWITCH;
#endif
    progress_init(MODE_NONE);

    title_freeze_all = 0;

    game_proxy_filter(filter_cmd);

    /* Start the title screen music. */

    audio_music_fade_to(0.5f, switchball_useable() ? "bgm/title-switchball.ogg" :
                                                     "bgm/title.ogg");

    /* Initialize the build-in nor title level for display. */

    game_fade_color(0.0f, 0.0f, 0.0f);

    if (switchball_useable() && init_title_level())
        mode = TITLE_MODE_LEVEL;
    else if (demo_replay_init("gui/title-l.nbr",
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL))
        mode = TITLE_MODE_BUILD_IN;
    else if (init_title_level())
        mode = TITLE_MODE_LEVEL;
    else
        mode = TITLE_MODE_NONE;

    real_time = 0.0f;

    title_create_versions();
    return title_gui();
}

static void title_leave(struct state *st, struct state *next, int id)
{
    if (items)
    {
        demo_dir_free(items);
        items = NULL;
        demo_is_loaded = 0;
    }

    demo_replay_stop(0);
    game_proxy_filter(NULL);
    gui_delete(id);
}

static void title_paint(int id, float t)
{
    game_client_draw(0, t);

    gui_paint(id);
    gui_paint(system_version_build_id);
    gui_paint(copyright_id);

#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
    xbox_control_title_gui_paint();
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
        else
        {
            game_fade(+1.0f);
            real_time = 0.0f;
            mode = TITLE_MODE_LEVEL_FADE;
        }
        break;

    case TITLE_MODE_LEVEL_FADE: /* Fade out.  Load demo level. */

        if (real_time > 1.0f && !title_prequit)
        {
            int title_gamemode;
            if (!items)
                items = demo_dir_scan();

            if ((demo = pick_demo(items)))
            {
                if (demo_replay_init(demo, NULL, &title_gamemode, NULL, NULL, NULL, NULL))
                {
                    progress_init(title_gamemode);
                    game_client_fly(0.0f);
                    real_time = 0.0f;
                    mode = TITLE_MODE_DEMO;
                }
                else if (demo_replay_init(left_handed ? "gui/title/title-l.nbr" :
                                                        "gui/title/title-r.nbr",
                                          NULL, &title_gamemode, NULL, NULL, NULL, NULL))
                {
                    progress_init(title_gamemode);
                    game_client_fly(0.0f);
                    real_time = 0.0f;
                    mode = TITLE_MODE_BUILD_IN;
                }
                else
                {
                    init_title_level();
                    mode = TITLE_MODE_LEVEL;
                }
            }
            else if (demo_replay_init(left_handed ? "gui/title/title-l.nbr" :
                                                    "gui/title/title-r.nbr",
                                      NULL, &title_gamemode, NULL, NULL, NULL, NULL))
            {
                progress_init(title_gamemode);
                game_fade(-1.0f);
                real_time = 0.0f;
                mode = TITLE_MODE_BUILD_IN;
            }
        }
        break;

    case TITLE_MODE_DEMO: /* Run demo. */

        if (!demo_replay_step(dt))
        {
            demo_replay_stop(0);
            game_fade(+1.0f);
            real_time = 0.0f;
            if (!title_prequit) mode = TITLE_MODE_DEMO_FADE;
        }
        else
            game_client_blend(demo_replay_blend());

        break;

    case TITLE_MODE_DEMO_FADE: /* Fade out.  Load build-in demo level or load title level. */

        if (real_time > 1.0f && !title_prequit)
        {
            int title_gamemode;
            real_time = 0.0f;

            if (switchball_useable())
            {
                init_title_level();
                mode = TITLE_MODE_LEVEL;
            }
            else if (demo_replay_init(left_handed ? "gui/title/title-l.nbr" :
                                                    "gui/title/title-r.nbr",
                                      NULL, &title_gamemode, NULL, NULL, NULL, NULL))
            {
                progress_init(title_gamemode);
                game_client_fly(0.0f);
                real_time = 0.0f;
                mode = TITLE_MODE_BUILD_IN;
            }
            else
            {
                init_title_level();
                mode = TITLE_MODE_LEVEL;
            }
        }
        break;

    case TITLE_MODE_BUILD_IN: /* Run build-in demo. */

        if (!demo_replay_step(dt))
        {
            left_handed = left_handed ? 0 : 1;
            demo_replay_stop(0);
            game_fade(+1.0f);
            real_time = 0.0f;
            mode = TITLE_MODE_LEVEL_FADE;
        }
        else
            game_client_blend(demo_replay_blend());

    break;
    }

    gui_timer(id, dt);
    game_step_fade(dt);
}

static void title_point(int id, int x, int y, int dx, int dy)
{
    int jd;

#ifndef __EMSCRIPTEN__
    xbox_toggle_gui(0);
#endif

    if ((jd = gui_point(id, x, y)))
        gui_pulse(jd, 1.2f);
}

void title_stick(int id, int a, float v, int bump)
{
    int jd;

#ifndef __EMSCRIPTEN__
    xbox_toggle_gui(1);
#endif

    if ((jd = gui_stick(id, a, v, bump)))
        gui_pulse(jd, 1.2f);
}

static int title_keybd(int c, int d)
{
#ifndef __EMSCRIPTEN__
    xbox_toggle_gui(0);
#endif

    if (d)
    {
#ifndef __EMSCRIPTEN__
        if (c == KEY_EXIT && support_exit)
            return title_action(GUI_BACK, 0);
#endif

#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD
        if (c >= SDLK_a && c <= SDLK_z)
            return title_action(GUI_CHAR, c);
#endif
    }
    return 1;
}

static int title_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return title_action(gui_token(active), gui_value(active));
#ifndef __EMSCRIPTEN__
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b) && support_exit)
            return title_action(GUI_BACK, 0);
#endif
    }
    return 1;
}

void title_fade(float alpha)
{
    gui_set_alpha(system_version_build_id, alpha, 0);
    gui_set_alpha(copyright_id, alpha, 0);
}

/*---------------------------------------------------------------------------*/

struct state st_title = {
    title_enter,
    title_leave,
    title_paint, // Default: shared_paint
    title_timer,
    title_point,
    title_stick,
    shared_angle,
    shared_click,
    title_keybd,
    title_buttn,
    NULL,
    NULL,
    title_fade
};

