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

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#if _MSC_VER
#define NOMINMAX
#include <Windows.h> /* FindClose(), FindFirstFile(), FindNextFile() */
#else
#include <dirent.h> /* opendir(), readdir(), closedir() */
#endif

#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "common.h"
#include "fs.h"
#include "dir.h"
#include "array.h"
#include "audio.h"
#include "config.h"
#include "package.h"
#include "state.h"

#include "st_common.h"
#include "st_package.h"

#include "config_wgcl.h"

#if ENABLE_OPENDRIVEAPI!=0
 /*
  * https://gitea.stynegame.de/StyneGameHamburg/opendrivepi
  */
#include <opendriveapi.h>
#elif _WIN32
#if !defined(_MSC_VER)
#error Security compilation error: This was already done with FindClose, \
       FindFirstFileA, FindNextFile and GetFileAttributesA or using OpenDriveAPI. \
       Install Visual Studio 2022 Community or later version to build it there. \
       Or download the OpenDriveAPI project: \
       https://gitea.stynegame.de/StyneGameHamburg/opendrivepi
#else
#pragma message(__FILE__ ": Using code compilation: Microsoft Visual Studio")
#endif
#else
#pragma message(__FILE__ ": Using code compilation: GCC + G++")
#endif

/*
 * Should be set the preset keys as well?
 */
#define CONF_CONTROL_SET_PRESET_KEYS(cam_tgl, cam1, cam2,    \
                                     cam3, camL, camR, axYP, \
                                     axXN, axYN, axXP)       \
    do {                                                     \
        config_set_d(CONFIG_KEY_CAMERA_TOGGLE, cam_tgl);     \
        config_set_d(CONFIG_KEY_CAMERA_1,      cam1);        \
        config_set_d(CONFIG_KEY_CAMERA_2,      cam2);        \
        config_set_d(CONFIG_KEY_CAMERA_3,      cam3);        \
        config_set_d(CONFIG_KEY_CAMERA_L,      camL);        \
        config_set_d(CONFIG_KEY_CAMERA_R,      camR);        \
        config_set_d(CONFIG_KEY_FORWARD,       axYP);        \
        config_set_d(CONFIG_KEY_LEFT,          axXN);        \
        config_set_d(CONFIG_KEY_BACKWARD,      axYN);        \
        config_set_d(CONFIG_KEY_RIGHT,         axXP);        \
    } while (0)

/*---------------------------------------------------------------------------*/

static int control_get_input(void)
{
    const SDL_Keycode k_auto = config_get_d(CONFIG_KEY_CAMERA_TOGGLE);
    const SDL_Keycode k_cam1 = config_get_d(CONFIG_KEY_CAMERA_1);
    const SDL_Keycode k_cam2 = config_get_d(CONFIG_KEY_CAMERA_2);
    const SDL_Keycode k_cam3 = config_get_d(CONFIG_KEY_CAMERA_3);
    const SDL_Keycode k_caml = config_get_d(CONFIG_KEY_CAMERA_L);
    const SDL_Keycode k_camr = config_get_d(CONFIG_KEY_CAMERA_R);

    SDL_Keycode k_arrowkey[4] = { 0, 0, 0, 0 };
    k_arrowkey[0] = config_get_d(CONFIG_KEY_FORWARD);
    k_arrowkey[1] = config_get_d(CONFIG_KEY_LEFT);
    k_arrowkey[2] = config_get_d(CONFIG_KEY_BACKWARD);
    k_arrowkey[3] = config_get_d(CONFIG_KEY_RIGHT);

    if (k_auto == SDLK_c && k_cam1 == SDLK_3 && k_cam2 == SDLK_1 && k_cam3 == SDLK_2
        && k_caml == SDLK_RIGHT && k_camr == SDLK_LEFT
        && k_arrowkey[0] == SDLK_w && k_arrowkey[1] == SDLK_a && k_arrowkey[2] == SDLK_s && k_arrowkey[3] == SDLK_d)
        return 3;
    if (k_auto == SDLK_c && k_cam1 == SDLK_3 && k_cam2 == SDLK_1 && k_cam3 == SDLK_2
        && k_caml == SDLK_d && k_camr == SDLK_a
        && k_arrowkey[0] == SDLK_UP && k_arrowkey[1] == SDLK_LEFT && k_arrowkey[2] == SDLK_DOWN && k_arrowkey[3] == SDLK_RIGHT)
        return 2;
    else if (k_auto == SDLK_e && k_cam1 == SDLK_1 && k_cam2 == SDLK_2 && k_cam3 == SDLK_3
        && k_caml == SDLK_s && k_camr == SDLK_d
        && k_arrowkey[0] == SDLK_UP && k_arrowkey[1] == SDLK_LEFT && k_arrowkey[2] == SDLK_DOWN && k_arrowkey[3] == SDLK_RIGHT)
        return 1;

    return 4;
}

/*---------------------------------------------------------------------------*/

int WGCL_GameOptions_Exists;

static struct state *st_returnable;

void WGCL_InitGameOptions(struct state *returnable)
{
#ifdef __EMSCRIPTEN__
    if (WGCL_GameOptions_Exists) return;

    WGCL_GameOptions_Exists = 1;

    audio_music_fade_to(0.5f, "bgm/inter.ogg", 1);

    EM_ASM({
        gameoptions_packages_available = navigator.onLine && $0;
        CoreLauncherOptions_GameOptions_Init();
    }, config_get_d(CONFIG_ONLINE));

    if (st_returnable == NULL)
        st_returnable = returnable;
#endif
}

void WGCL_QuitGameOptions(void)
{
#ifdef __EMSCRIPTEN__
    if (!WGCL_GameOptions_Exists) return;

    if (st_returnable)
    {
        goto_state(st_returnable);
        st_returnable = NULL;
    }

    WGCL_GameOptions_Exists = 0;
#endif
}

void WGCL_BackToGameOptions(const char *st_class_name)
{
#ifdef __EMSCRIPTEN__
    if (WGCL_GameOptions_Exists)
        EM_ASM({ CoreLauncherOptions_GameOptions_Init(UTF8ToString($0)); }, st_class_name);
#endif
}

/*---------------------------------------------------------------------------*/

static void wgcl_gameoptions_refresh_packages_done(void *data1, void *data2)
{
    goto_package(0, &st_null);
}

void WGCL_CallClassicPackagesUI(void)
{
    if (WGCL_GameOptions_Exists)
    {
        package_change_category(0);

        struct fetch_callback callback = { 0 };

        callback.data = NULL;
        callback.done = wgcl_gameoptions_refresh_packages_done;

        package_refresh(callback);
    }
}

/*---------------------------------------------------------------------------*/

struct WGCL_Campaign_Level
{
    int time_hundred[3];
    int coins[3];

    char player_name[3][MAXSTR];
};

struct WGCL_Campaign_WorldCard
{
    struct WGCL_Campaign_Level levels_time[6];
    struct WGCL_Campaign_Level levels_goal[6];
    struct WGCL_Campaign_Level levels_coin[6];

    int locked[6];
    int completed[6];

    int time_hundred_total;
};

static struct WGCL_Campaign_WorldCard world_cards[5];

static Array wgcloptions_file_items = NULL;

static int hs_scan_item(struct dir_item *item)
{
    return str_ends_with(item->path, ".txt");
}

static int hs_cmp_items(const void *A, const void *B)
{
    const struct dir_item *a = A, *b = B;

    return strcmp(a->path, b->path);
}

static int campaign_get_score(fs_file fp, struct WGCL_Campaign_Level *l)
{
    char line[MAXSTR];
    int i;

    for (i = 0; i <= 2; i++)
    {
        int n = -1;

        if (!fs_gets(line, sizeof (line), fp))
            return 0;

        strip_newline(line);

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        if (sscanf_s(line,
#else
        if (sscanf(line,
#endif
                   "%d %d %n", &l->time_hundred[i], &l->coins[i], &n) < 2)
            return 0;

        if (n < 0)
            return 0;

        SAFECPY(l->player_name[i], line + n);
    }

    return 1;
}

static void campaign_hs_read_lines(fs_file fp, char *buf, int size)
{
    int block_hs_total_time = 1;

    int i = -1, j = 6;

    struct WGCL_Campaign_Level tmp_classic;

    while (fs_gets(buf, size, fp))
    {
        int campaign_version = 0;
        int flags            = 0;
        int n                = 0;

        strip_newline(buf);

        if (strncmp(buf, "campaign", 8) == 0)
        {
            campaign_get_score(fp, &tmp_classic);
            campaign_get_score(fp, &tmp_classic);
        }
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        else if (sscanf_s(buf,
#else
        else if (sscanf(buf,
#endif
                 "level %d %d %n", &flags, &campaign_version, &n) >= 2)
        {
            block_hs_total_time = 0;

            if (j > 5) {
                i++;
                j = 0;
            } else j++;
        }

        if (!block_hs_total_time)
        {
            /* Same as locked levels in the level set. */

            world_cards[i].locked[j]    = !!(flags & 1);
            world_cards[i].completed[j] = !!(flags & 2);

            campaign_get_score(fp, &world_cards[i].levels_time[j]);
            campaign_get_score(fp, &world_cards[i].levels_goal[j]);
            campaign_get_score(fp, &world_cards[i].levels_coin[j]);

            j++;
        }
    }
}

static Array wgcl_gameoptions_levelset_hs_dir_scan(void)
{
    Array items;

    if ((items = fs_dir_scan("Scores", hs_scan_item)))
        array_sort(items, hs_cmp_items);

    return items;
}

static void set_hs_read_lines(fs_file fp, char *buf, int size)
{
    int total_g  = 0;
    int total_xt = 0;
    int total_xf = 0;

    while (fs_gets(buf, size, fp))
    {
        strip_newline(buf);

        if (strncmp(buf, "stats ", 6) == 0)
        {
            int additive_g  = 0;
            int additive_xt = 0;
            int additive_xf = 0;

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            if (sscanf_s(buf, "stats %d %d %d", &additive_g,
                                                &additive_xt,
                                                &additive_xf) == 3)
#else
            if (sscanf(buf, "stats %d %d %d", &additive_g,
                                              &additive_xt,
                                              &additive_xf) == 3)
#endif
            {
                total_g  += additive_g;
                total_xt += additive_xt;
                total_xf += additive_xt;
            }
        }
    }

    if (total_g != 0 ||
        total_xt != 0 || total_xf != 0)
    {
#ifdef __EMSCRIPTEN__
        EM_ASM({
            CoreLauncherOptions_GameOptions_SaveData_LevelSet_AddToHSFileCardContent_Stats($0, $1, $2);
        }, total_g, total_xt, total_xf);
#endif
    }
}

static void campaign_load_hs(void)
{
    fs_file fp;

    if ((fp = fs_open_read("Campaign/time-trial.txt")))
    {
        for (int i = 0; i < 5; i++)
            for (int j = 0; j < 6; j++)
                for (int k = 0; k < 3; k++)
                {
                    world_cards[i].levels_coin[j].time_hundred[k] = 360000;
                    world_cards[i].levels_coin[j].coins[k] = 0;

                    SAFECPY(world_cards[i].levels_time[j].player_name[k], "");
                }

        char buf[MAXSTR];

        if (fs_gets(buf, sizeof(buf), fp))
        {
            strip_newline(buf);

            campaign_hs_read_lines(fp, buf, sizeof(buf));
        }

        fs_close(fp);
    }
}

static void set_load_hs(const char *filename)
{
    fs_file fp;

    if ((fp = fs_open_read(filename)))
    {
        char buf[MAXSTR];

        if (fs_gets(buf, sizeof (buf), fp))
        {
            strip_newline(buf);

            set_hs_read_lines(fp, buf, sizeof(buf));
        }

        fs_close(fp);
    }
}

void WGCL_CallSaveDataUI(void)
{
#ifdef __EMSCRIPTEN__
    int campaign_available = 0;
    int levelset_available = 0;

    /* === Campaign inside WGCL's game options === */

    campaign_available = fs_exists("campaign.txt") &&
                         fs_exists("Campaign/time-trial.txt");

    /* === Level set inside WGCL's game options === */

    char userdata_levelset_path[128];
    sprintf(userdata_levelset_path, "%s/%s", fs_get_write_dir(), "Scores");

    levelset_available = dir_exists(userdata_levelset_path);

    EM_ASM({
        gameoptions_campaign_available = $0;
        gameoptions_levelset_available = $1;
    }, campaign_available, levelset_available);
#endif
}

void WGCL_CallHighscoreDataUI_Campaign(void)
{
    /* HACK: Just read it first, then check whether first campaign level is unlocked. */

    campaign_load_hs();

    for (int i = 0; i < 5; i++)
    {
        int time_hundred_total = 0;

        int medals[3] = { 0, 0, 0 };

        for (int j = 0; j < 6; j++)
        {
            if (world_cards[i].locked[0])
            {
#ifdef __EMSCRIPTEN__
                EM_ASM({
                    CoreLauncherOptions_GameOptions_SaveData_Campaign_AddToWorldMedalCard(false, 0, false, 360000);
                });
#endif
                break;
            }

            else if (strcmp(world_cards[i].levels_time[j].player_name[0], "Hard")   != 0)
            {
                time_hundred_total += world_cards[i].levels_time[j].time_hundred[0];
                medals[0]++;
                medals[1]++;
                medals[2]++;
            }
            else if (strcmp(world_cards[i].levels_time[j].player_name[1], "Medium") != 0)
            {
                time_hundred_total += world_cards[i].levels_time[j].time_hundred[1];
                medals[0]++;
                medals[1]++;
            }
            else if (strcmp(world_cards[i].levels_time[j].player_name[2], "Easy")   != 0)
            {
                time_hundred_total += world_cards[i].levels_time[j].time_hundred[2];
                medals[0]++;
            }
        }

        if (!world_cards[i].locked[0])
        {
            int levelnum_progress = 0, levelnum_completed = 0;

            for (int j = 0; j < 6; j++)
            {
                if (!world_cards[i].locked[j])
                    levelnum_progress++;
                if (!world_cards[i].completed[j])
                    levelnum_completed++;
            }
#ifdef __EMSCRIPTEN__
            EM_ASM({
                CoreLauncherOptions_GameOptions_SaveData_Campaign_AddToWorldMedalCard(true, $0, $1 == 6, $2, $3, $4, $5);
            }, levelnum_progress, levelnum_completed, time_hundred_total, medals[0], medals[1], medals[2]);
#endif
        }
    }

#ifdef __EMSCRIPTEN__
    EM_ASM({ CoreLauncherOptions_GameOptions_SaveData_Campaign_RenderWorldCard(); });
#endif
}

void WGCL_CallHighscoreDataUI_LevelSet(void)
{
#ifdef __EMSCRIPTEN__
    EM_ASM({ CoreLauncherOptions_GameOptions_SaveData_LevelSet_ClearHSFileCards(); });
#endif

    wgcloptions_file_items = wgcl_gameoptions_levelset_hs_dir_scan();
    const int file_count = array_len(wgcloptions_file_items);

    for (int i = 0; i < file_count; i++)
    {
        struct dir_item *out_item = array_get(wgcloptions_file_items, i);

        if (out_item &&
            str_ends_with(out_item->path, ".txt") &&
            !str_ends_with(out_item->path, "-cheat.txt"))
        {
            int filesize = fs_size(out_item->path);

#ifdef __EMSCRIPTEN__
            EM_ASM({
                CoreLauncherOptions_GameOptions_SaveData_LevelSet_AddToHSFileCard(UTF8ToString($0), $1);
            }, out_item->path, filesize);
#endif
            set_load_hs(out_item->path);
        }
    }

    /*
     * Note that array will be freed automatically upon succesfully scanned
     * by the WGCL's game options (faster, but improvement).
     */

    array_free(wgcloptions_file_items);

#ifdef __EMSCRIPTEN__
    EM_ASM({
        CoreLauncherOptions_GameOptions_SaveData_LevelSet_RenderHSFileCard(1);
    });
#endif
}

void WGCL_TryEraseHighscoreFile_Campaign(void)
{
    fs_remove("Campaign/time-trial.txt");

    WGCL_CallHighscoreDataUI_Campaign();
}

void WGCL_TryEraseHighscoreFile_LevelSet(const char *filename)
{
    char file_path[64];
    sprintf(file_path, "Scores/%s.txt", filename);

    fs_remove(file_path);

    WGCL_CallHighscoreDataUI_LevelSet();
}

/*---------------------------------------------------------------------------*/

/*
 * This maps mouse_sense 300 (default) to the 7th of an 11 button
 * series. Effectively there are more options for a lower-than-default
 * sensitivity than for a higher one.
 */

#define MOUSE_RANGE_MIN  100
#define MOUSE_RANGE_INC  50
#define MOUSE_RANGE_MAX (MOUSE_RANGE_MIN + (MOUSE_RANGE_INC * 10))

 /*
  * Map mouse_sense values to [0, 10]. A higher mouse_sense value means
  * lower sensitivity, thus counter-intuitively, 0 maps to the higher
  * value.
  */

#define MOUSE_RANGE_MAP(m) \
    CLAMP(0, (MOUSE_RANGE_MAX - m) / MOUSE_RANGE_INC, 10)

#define MOUSE_RANGE_UNMAP(i) \
    (MOUSE_RANGE_MAX - (i * MOUSE_RANGE_INC))

void WGCL_LoadGameSystemSettings(void)
{
    /* FIXME: Just send EM_ASM to the WGCL System Settings. */

#ifdef __EMSCRIPTEN__
    EM_ASM({ systemsettings_conf_game_tutorial         = $0; }, config_get_d(CONFIG_ACCOUNT_TUTORIAL));
    EM_ASM({ systemsettings_conf_game_hint             = $0; }, config_get_d(CONFIG_ACCOUNT_HINT));
    EM_ASM({ systemsettings_conf_screenanimations      = $0; }, config_get_d(CONFIG_SCREEN_ANIMATIONS));
    EM_ASM({ systemsettings_conf_gfx_textures          = $0; }, config_get_d(CONFIG_TEXTURES));
    EM_ASM({ systemsettings_conf_gfx_smoothfix_flags   = $0; }, config_get_d(CONFIG_SMOOTH_FIX));
    EM_ASM({ systemsettings_conf_gfx_shadows           = $0; }, config_get_d(CONFIG_SHADOW));
    EM_ASM({ systemsettings_conf_gfx_background        = $0; }, config_get_d(CONFIG_BACKGROUND));
    EM_ASM({ systemsettings_conf_audio_volume_master   = $0; }, config_get_d(CONFIG_MASTER_VOLUME));
    EM_ASM({ systemsettings_conf_audio_volume_sfx      = $0; }, config_get_d(CONFIG_SOUND_VOLUME));
    EM_ASM({ systemsettings_conf_audio_volume_music    = $0; }, config_get_d(CONFIG_MUSIC_VOLUME));
    EM_ASM({ systemsettings_conf_audio_volume_narrator = $0; }, config_get_d(CONFIG_NARRATOR_VOLUME));
    EM_ASM({ systemsettings_conf_replay_controls_save  = $0; }, config_get_d(CONFIG_ACCOUNT_SAVE));
    EM_ASM({ systemsettings_conf_replay_controls_load  = $0; }, config_get_d(CONFIG_ACCOUNT_LOAD));
    EM_ASM({ systemsettings_conf_input_sensitivity     = $0; }, MOUSE_RANGE_MAP(config_get_d(CONFIG_MOUSE_SENSE)));
    EM_ASM({ systemsettings_conf_input_camrotation     = $0; }, config_get_d(CONFIG_CAMERA_ROTATE_MODE));

    EM_ASM({ systemsettings_conf_wgclworkers_notifications_chkp       = $0; }, config_get_d(CONFIG_NOTIFICATION_CHKP));
    EM_ASM({ systemsettings_conf_wgclworkers_notifications_extraballs = $0; }, config_get_d(CONFIG_NOTIFICATION_REWARD));
    EM_ASM({ systemsettings_conf_wgclworkers_notifications_shop       = $0; }, config_get_d(CONFIG_NOTIFICATION_SHOP));

    EM_ASM({ systemsettings_conf_input_preset = $0; }, control_get_input());
#endif
}

void WGCL_SaveGameSystemSettings(void)
{
    /* FIXME: Just call EM_ASM_INT from the WGCL System Settings. */

#ifdef __EMSCRIPTEN__
    config_set_d(CONFIG_ACCOUNT_TUTORIAL,    EM_ASM_INT({ return systemsettings_conf_game_tutorial;                        }));
    config_set_d(CONFIG_ACCOUNT_HINT,        EM_ASM_INT({ return systemsettings_conf_game_hint;                            }));
    config_set_d(CONFIG_SCREEN_ANIMATIONS,   EM_ASM_INT({ return systemsettings_conf_screenanimations;                     }));
    config_set_d(CONFIG_TEXTURES,            EM_ASM_INT({ return systemsettings_conf_gfx_textures;                         }));
    config_set_d(CONFIG_SMOOTH_FIX,          EM_ASM_INT({ return systemsettings_conf_gfx_smoothfix_flags;                  }));
    config_set_d(CONFIG_SHADOW,              EM_ASM_INT({ return systemsettings_conf_gfx_shadows;                          }));
    config_set_d(CONFIG_BACKGROUND,          EM_ASM_INT({ return systemsettings_conf_gfx_background;                       }));
    config_set_d(CONFIG_MASTER_VOLUME,       EM_ASM_INT({ return systemsettings_conf_audio_volume_master;                  }));
    config_set_d(CONFIG_SOUND_VOLUME,        EM_ASM_INT({ return systemsettings_conf_audio_volume_sfx;                     }));
    config_set_d(CONFIG_MUSIC_VOLUME,        EM_ASM_INT({ return systemsettings_conf_audio_volume_music;                   }));
    config_set_d(CONFIG_NARRATOR_VOLUME,     EM_ASM_INT({ return systemsettings_conf_audio_volume_narrator;                }));
    config_set_d(CONFIG_ACCOUNT_SAVE,        EM_ASM_INT({ return systemsettings_conf_replay_controls_save;                 }));
    config_set_d(CONFIG_ACCOUNT_LOAD,        EM_ASM_INT({ return systemsettings_conf_replay_controls_load;                 }));
    config_set_d(CONFIG_MOUSE_SENSE,         MOUSE_RANGE_UNMAP(EM_ASM_INT({ return systemsettings_conf_input_sensitivity; })));
    config_set_d(CONFIG_CAMERA_ROTATE_MODE,  EM_ASM_INT({ return systemsettings_conf_input_camrotation;                    }));
    config_set_d(CONFIG_NOTIFICATION_CHKP,   EM_ASM_INT({ return systemsettings_conf_wgclworkers_notifications_chkp;       }));
    config_set_d(CONFIG_NOTIFICATION_REWARD, EM_ASM_INT({ return systemsettings_conf_wgclworkers_notifications_extraballs; }));
    config_set_d(CONFIG_NOTIFICATION_SHOP,   EM_ASM_INT({ return systemsettings_conf_wgclworkers_notifications_shop;       }));

    switch (EM_ASM_INT({ return systemsettings_conf_input_preset; }))
    {
        case 1:
            CONF_CONTROL_SET_PRESET_KEYS(SDLK_e, SDLK_1, SDLK_2, SDLK_3,
                                         SDLK_s, SDLK_d,
                                         SDLK_UP, SDLK_LEFT, SDLK_DOWN, SDLK_RIGHT);
            break;
        case 2:
            CONF_CONTROL_SET_PRESET_KEYS(SDLK_c, SDLK_3, SDLK_1, SDLK_2,
                                         SDLK_d, SDLK_a,
                                         SDLK_UP, SDLK_LEFT, SDLK_DOWN, SDLK_RIGHT);
            break;
        case 3:
            CONF_CONTROL_SET_PRESET_KEYS(SDLK_c, SDLK_3, SDLK_1, SDLK_2,
                                         SDLK_RIGHT, SDLK_LEFT,
                                         SDLK_w, SDLK_a, SDLK_s, SDLK_d);
            break;
    }

    audio_volume(CLAMP(0, config_get_d(CONFIG_MASTER_VOLUME),   10),
                 CLAMP(0, config_get_d(CONFIG_SOUND_VOLUME),    10),
                 CLAMP(0, config_get_d(CONFIG_MUSIC_VOLUME),    10),
                 CLAMP(0, config_get_d(CONFIG_NARRATOR_VOLUME), 10));
#endif
}

/*---------------------------------------------------------------------------*/

void WGCL_TryFormatUserData(void)
{
#ifdef __EMSCRIPTEN__
    /* First things first, quit package first. */

    package_quit();

    const char path_name[5][256] = {
        "Campaign",
        "DLC",
        "Replays"
        "Scores",
        "Screenshots"
    };

    for (int i = 0; i < 5; i++)
    {
        char userdir_path[256];
        char file_attr[256];

#if ENABLE_OPENDRIVEAPI!=0
        sprintf_s(userdir_path, 256, "%s/%s", fs_get_write_dir(), path_name[i]);

        OpenDriveAPI_PDIR          dir;
        struct OpenDriveAPI_DirEnt ent;

        if ((dir = opendriveapi_opendir(userdir_path, &ent)))
        {
            while ((opendriveapi_readdir(dir, &ent)) != 0)
            {
                if (strcmp(ent.dir_ent_d_name, ".") == 0 || strcmp(ent.dir_ent_d_name, "..") == 0)
                    continue;

#if _WIN32 && _MSC_VER
                sprintf_s(file_attr, 256, "%s\\%s", path_name[i], ent.dir_ent_d_name);
#else
                sprintf(file_attr, "%s/%s", path_name[i], ent.dir_ent_d_name);
#endif
                fs_remove(file_attr);
            }
            opendriveapi_closedir(dir);
        }
#elif _WIN32 && _MSC_VER
        sprintf_s(userdir_path, 256, "%s\\%s\\*", fs_get_write_dir(), path_name[i]);

        WIN32_FIND_DATAA find_data;
        HANDLE hFind;

        if ((hFind = FindFirstFileA(userdir_path, &find_data)) != INVALID_HANDLE_VALUE)
        {
            do {
                if (strcmp(find_data.cFileName, ".") == 0 ||
                    strcmp(find_data.cFileName, "..") == 0)
                    continue;

                sprintf_s(file_attr, 256, "%s\\%s", path_name[i], find_data.cFileName);
                fs_remove(file_attr);
            } while (FindNextFileA(hFind, &find_data));

            FindClose(hFind);
            hFind = 0;
        }
#else
        sprintf(userdir_path, "%s/%s", fs_get_write_dir(), path_name[i]);

        DIR *dir;
        if ((dir = opendir(userdir_path)))
        {
            struct dirent *ent;

            while ((ent = readdir(dir)))
            {
                if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
                    continue;

                sprintf(file_attr, "%s/%s", path_name[i], ent->d_name);
                fs_remove(file_attr);
            }
            closedir(dir);
        }
#endif
    }

    EM_ASM({ CoreLauncherOptions_SystemSettings_OnFormatCompleted(); });
#endif
}
