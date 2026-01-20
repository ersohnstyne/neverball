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

 /*
 * HACK: Used with console version
 */
#include "console_control_gui.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "account_wgcl.h"

#include "glext.h"
#include "config.h"
#include "audio.h"
#include "video.h"
#include "geom.h"
#include "lang.h"
#include "gui.h"
#include "transition.h"
#include "text.h"
#include "fetch.h"
#include "networking.h"

#include "st_common.h"

#if _WIN32 && _MSC_VER && NB_HAVE_PB_BOTH==1
#include "st_wgcl.h"
#endif
#include "st_setup.h"
#include "st_transfer.h"

#if _DEBUG && _MSC_VER
#ifndef _CRTDBG_MAP_ALLOC
#pragma message(__FILE__": Missing _CRT_MAP_ALLOC, recreate: _CRTDBG_MAP_ALLOC + crtdbg.h")
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

#define AUD_MENU     "snd/menu.ogg"
#define AUD_BACK     "snd/back.ogg"
#define AUD_DISABLED "snd/disabled.ogg"

//#define ENABLE_SOFTWARE_UPDATE 1

 /* Macros helps with the action game menu. */

#define GENERIC_GAMEMENU_ACTION                      \
        if (st_global_animating()) {                 \
            audio_play(AUD_DISABLED, 1.0f);          \
            return 1;                                \
        } else audio_play(GUI_BACK == tok ?          \
                          AUD_BACK :                 \
                          (GUI_NONE == tok ?         \
                           AUD_DISABLED : AUD_MENU), \
                          1.0f)

#define GAMEPAD_GAMEMENU_ACTION_SCROLL(tok1, tok2, itemstep) \
        if (st_global_animating()) {                         \
            audio_play(AUD_DISABLED, 1.0f);                  \
            return 1;                                        \
        } else if (tok == tok1 || tok == tok2) {             \
            if (tok == tok1)                                 \
                audio_play(setup_langs_first > 1 ?           \
                           AUD_MENU : AUD_DISABLED, 1.0f);   \
            if (tok == tok2)                                 \
                audio_play(setup_langs_first + itemstep < total ?\
                           AUD_MENU : AUD_DISABLED, 1.0f);   \
        } else GENERIC_GAMEMENU_ACTION

#define SETUP_LANG_STEP 7

#define SETUP_CONTROL_SET_PRESET_KEYS(cam_tgl, cam1, cam2, \
                                      cam3, camL, camR, axYP, \
                                      axXN, axYN, axXP)  \
    do {                                                 \
        config_set_d(CONFIG_KEY_CAMERA_TOGGLE, cam_tgl); \
        config_set_d(CONFIG_KEY_CAMERA_1, cam1);         \
        config_set_d(CONFIG_KEY_CAMERA_2, cam2);         \
        config_set_d(CONFIG_KEY_CAMERA_3, cam3);         \
        config_set_d(CONFIG_KEY_CAMERA_L, camL);         \
        config_set_d(CONFIG_KEY_CAMERA_R, camR);         \
        config_set_d(CONFIG_KEY_FORWARD, axYP);          \
        config_set_d(CONFIG_KEY_LEFT, axXN);             \
        config_set_d(CONFIG_KEY_BACKWARD, axYN);         \
        config_set_d(CONFIG_KEY_RIGHT, axXP);            \
    } while (0)

/*---------------------------------------------------------------------------*/

#define TERMS_CARD_COUNT 3

struct state st_game_setup;

struct state *st_continue;
static int  (*fn_continue) (struct state *st_continue);

static int  (*goto_name_fn) (struct state *finish,
                             int         (*finish_fn) (struct state *));
static int  (*goto_ball_fn) (struct state *finish);

static int setup_page       = 0;
static int setup_process    = 0;
static int setup_login_wgcl = 1;

static int setup_btn_confirm_id = 0;

static int setup_langs_first = 0;

static int setup_terms_tgl_ids[TERMS_CARD_COUNT];
static int setup_terms_master_tgl_id = 0;
static int setup_terms_accepted[TERMS_CARD_COUNT];
static int setup_terms_all_accepted = 0;

static int setup_update_must_install = 0;

static int update_install_dnldprog_id;
static int update_install_dnld_eta_id;

static int setup_update_version_finished = 0;

int goto_game_setup(struct state *start_state, int (*start_fn)(struct state *),
                    int (*new_goto_name_fn) (struct state *,
                                             int         (*new_finish_fn) (struct state *)),
                    int (*new_goto_ball_fn) (struct state *))
{
    st_continue   = start_state;
    fn_continue   = start_fn;

#if ENABLE_NLS==1 || _MSC_VER
    setup_page    = 0;
#else
    setup_page    = 1;
#endif
    setup_process = 1;

    setup_langs_first = 0;

    for (int i = 0; i < TERMS_CARD_COUNT; i++)
        setup_terms_accepted[i] = 0;

    setup_terms_all_accepted = 0;

    setup_update_must_install = 0;

    goto_name_fn = new_goto_name_fn;
    goto_ball_fn = new_goto_ball_fn;

    fs_mkdir("GameUpdate");

    return goto_state(&st_game_setup);
}

int goto_game_setup_finish(struct state *start_state)
{
#if _WIN32 && ENABLE_FETCH!=0 && ENABLE_SOFTWARE_UPDATE!=0
    setup_page++;
#else
    setup_page = 7;
#endif

    return goto_state(&st_game_setup);
}

static int title_check_playername(const char *regname)
{
    for (int i = 0; i < text_length(regname); i++)
    {
        if (regname[i] == '\\' || regname[i] == '/' || regname[i] == ':'  ||
            regname[i] == '*'  || regname[i] == '?' || regname[i] == '"'  ||
            regname[i] == '<'  || regname[i] == '>' || regname[i] == '|')
            return 0;
    }

    return text_length(regname) >= 3;
}

/*
 * Check whether has meet fullfilled folder requirements
 * in the user data folder.
 *
 * Returns TRUE to launch the game normally, unless go to the game setup.
 */
int check_game_setup(void)
{
    const char *curr_write_dir = fs_get_write_dir();

    char *path_accounts = path_join(curr_write_dir, "Accounts"),
         *path_demo     = path_join(curr_write_dir, "Replays"),
         *path_scores   = path_join(curr_write_dir, "Scores"),
         *path_shots    = path_join(curr_write_dir, "Screenshots");

    int skip_setup = dir_exists(path_accounts) &&
                     dir_exists(path_demo)     &&
                     dir_exists(path_scores)   &&
                     dir_exists(path_shots);

    free(path_accounts); path_accounts = NULL;
    free(path_demo);     path_demo     = NULL;
    free(path_scores);   path_scores   = NULL;
    free(path_shots);    path_shots    = NULL;

    return skip_setup;
}

/*
 * Check whether setup is in process.
 */
int game_setup_process(void)
{
    return setup_process;
}

/*---------------------------------------------------------------------------*/

enum
{
    SETUP_LANG_DEFAULT = GUI_LAST,
    SETUP_LANG_SELECT,

    SETUP_CONTROLS_SELECT,

    SETUP_TERMS_TOGGLE,
    SETUP_TERMS_TOGGLE_ALL,
    SETUP_TERMS_READMORE,

    SETUP_UPDATE_START,
    SETUP_UPDATE_SKIP,

    SETUP_TRANSFER,
    SETUP_FINISHED
};

static int setup_gamedeps_card(int pd,
                               const int install_id,
                               const char *install_path_classic,
                               const char *install_path_epic,
                               const char *install_path_steam)
{
    int id;

    const int h = video.device_h;

    const char *s0 = N_("Not installed");
    const char *s1 = N_("Installed");

    char program_path[MAXSTR];
    char gameapp_path_classic[MAXSTR];
    char gameapp_path_epic[MAXSTR];
    char gameapp_path_steam[MAXSTR];

    char img_path_installed[MAXSTR];
    char img_path_uninstalled[MAXSTR];

    switch (install_id)
    {
        case 0:
            SAFECPY(img_path_uninstalled, "gui/setup/game_switchball_0.png");
            SAFECPY(img_path_installed,   "gui/setup/game_switchball_1.png");
            break;
    }

#if _WIN32
    SAFECPY(program_path, "C:\\Program Files\\");
#else
    SAFECPY(program_path, "/");
#endif
    
    SAFECPY(gameapp_path_classic, program_path);
    SAFECPY(gameapp_path_epic,    program_path);
    SAFECPY(gameapp_path_steam,   program_path);

    SAFECAT(gameapp_path_classic, install_path_classic);

    SAFECAT(gameapp_path_epic, "Epic Games\\");
    SAFECAT(gameapp_path_epic, install_path_epic);

    SAFECAT(gameapp_path_steam, "Steam\\steamapps\\common\\");
    SAFECAT(gameapp_path_steam, install_path_steam);

    if ((id = gui_vstack(pd)))
    {
        const int is_installed = file_exists(gameapp_path_classic) ||
                                 file_exists(gameapp_path_epic) ||
                                 file_exists(gameapp_path_steam);

        gui_image(id, is_installed ? img_path_installed :
                                     img_path_uninstalled,
                      h / 5, h / 5);

        gui_label(id, _(is_installed ? s1 : s0), GUI_TNY, GUI_COLOR_WHT);

        gui_set_rect(id, GUI_ALL);
    }

    return id;
}

static int setup_terms_card(int pd, const char *name, int idx, int readmore_disabled)
{
    int id, jd;

    if ((id = gui_hstack(pd)))
    {
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
        if ((jd = gui_state(id, _("Read More"), GUI_SML, SETUP_TERMS_READMORE, idx)))
        {
            if (readmore_disabled)
            {
                gui_set_color(jd, GUI_COLOR_GRY);
                gui_set_state(jd, GUI_NONE, idx);
            }
        }

        gui_space(id);
#endif

        int tmp_lbl_id = gui_label(id, "XXXXXXXXXXXXXXXXXXXXXXXX", GUI_SML, GUI_COLOR_WHT);
        gui_set_label(tmp_lbl_id, name);
        gui_set_trunc(tmp_lbl_id, TRUNC_TAIL);

        gui_space(id);

        if ((setup_terms_tgl_ids[idx] = gui_label(id, GUI_CROSS, GUI_SML, GUI_COLOR_RED)))
        {
            if (setup_terms_accepted[idx])
            {
                gui_set_label(setup_terms_tgl_ids[idx], GUI_CHECKMARK);
                gui_set_color(setup_terms_tgl_ids[idx], GUI_COLOR_GRN);
            }

            gui_set_state(setup_terms_tgl_ids[idx], SETUP_TERMS_TOGGLE, idx);
        }
    }

    return id;
}

static void game_setup_terms_checkmark_update_all(void)
{
    setup_terms_all_accepted = 1;

    for (int i = 0; i < TERMS_CARD_COUNT; i++)
    {
        if (!setup_terms_accepted[i])
        {
            gui_set_label(setup_terms_tgl_ids[i], GUI_CROSS);
            gui_set_color(setup_terms_tgl_ids[i], GUI_COLOR_RED);
            setup_terms_all_accepted = 0;
        }
        else
        {
            gui_set_label(setup_terms_tgl_ids[i], GUI_CHECKMARK);
            gui_set_color(setup_terms_tgl_ids[i], GUI_COLOR_GRN);
        }
    }

    const GLubyte *master_checkmark_color = setup_terms_all_accepted ? gui_grn : gui_red;
    const GLubyte *btn_color              = setup_terms_all_accepted ? gui_wht : gui_gry;

    gui_set_label(setup_terms_master_tgl_id, setup_terms_all_accepted ? GUI_CHECKMARK : GUI_CROSS);
    gui_set_color(setup_terms_master_tgl_id, master_checkmark_color, master_checkmark_color);

    gui_set_state(setup_btn_confirm_id, setup_terms_all_accepted ? GUI_NEXT : GUI_NONE, 0);
    gui_set_color(setup_btn_confirm_id, btn_color, btn_color);
}

/*---------------------------------------------------------------------------*/

/*
 * Premium: appdownload.stynegame.de
 * Legacy downloads: play.neverball.org
 */
#define NB_CURRDOMAIN_PREMIUM "appdownload.stynegame.de"

static const char *get_updated_url(const char *filename)
{
    if (filename && *filename)
    {
        static char url[MAXSTR];
        memset(url, 0, sizeof (url));

#if NB_HAVE_PB_BOTH==1
        SAFECPY(url, "https://" NB_CURRDOMAIN_PREMIUM "/app/");
#else
        SAFECPY(url, "https://play.neverball.org/update/");
#endif
        SAFECAT(url, filename);

        return url;
    }

    return NULL;
}

static const char *get_updated_path(const char *filename)
{
    if (filename && *filename)
    {
        static char path[MAXSTR];

        memset(path, 0, sizeof (path));

        SAFECPY(path, "GameUpdate/");
        SAFECAT(path, filename);

        return path;
    }
    return NULL;
}

static int load_updated_version(void)
{
#if ENABLE_SOFTWARE_UPDATE!=0
    fs_file fp = fs_open_read(get_updated_path("version.txt"));

    if (fp)
    {
        char line[MAXSTR] = "";

        int curr_major    = 0,
            curr_minor    = 0,
            curr_patch    = 0,
            curr_revision = 0,
            curr_build    = 0;

        int new_major    = 0,
            new_minor    = 0,
            new_patch    = 0,
            new_revision = 0,
            new_build    = 0;

        while (fs_gets(line, sizeof (line), fp))
        {
            strip_newline(line);

#if _MSC_VER && !_CRT_SECURE_NO_WARNINGS
            if (sscanf_s(line, "%d.%d.%d.%d.%d",
                               &new_major,
                               &new_minor,
                               &new_patch,
                               &new_revision,
                               &new_build) == 5)
#else
            if (sscanf(line, "%d.%d.%d.%d.%d",
                             &new_major,
                             &new_minor,
                             &new_patch,
                             &new_revision,
                             &new_build) == 5)
#endif
            {
#if _MSC_VER && !_CRT_SECURE_NO_WARNINGS
                sscanf_s(VERSION, "%d.%d.%d",
                                &curr_major,
                                &curr_minor,
                                &curr_patch);
#else
                sscanf(VERSION, "%d.%d.%d",
                                &curr_major,
                                &curr_minor,
                                &curr_patch);
#endif

                if (new_major != curr_major ||
                    new_minor != curr_minor ||
                    new_patch != curr_patch)
                    setup_update_must_install = 1;
            }
        }

        fs_close(fp);
        fp = NULL;

        return 1;
    }
#endif

    return 0;
}

/*---------------------------------------------------------------------------*/

static int setup_is_replay(struct dir_item *item)
{
    return str_ends_with(item->path, str_ends_with(item->path, ".nbrx") ? ".nbrx" : ".nbr");
}

static int setup_is_score_file(struct dir_item *item)
{
    return str_starts_with(item->path, "neverballhs-");
}

static void setup_mkdir_migrate(void)
{
#if NB_HAVE_PB_BOTH==1
    Array items;
    int i;

    const char *src;
    char *dst;

    fs_mkdir("Accounts");
    fs_mkdir("Campaign");

    if (fs_mkdir("Replays"))
    {
        if ((items = fs_dir_scan("", setup_is_replay)))
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
        if ((items = fs_dir_scan("", setup_is_score_file)))
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
#endif
}

/*---------------------------------------------------------------------------*/

enum setup_update_status
{
    SETUP_UPDATE_STATUS_NONE = 0,
    SETUP_UPDATE_STATUS_AVAILABLE,
    SETUP_UPDATE_STATUS_DOWNLOADING,
    SETUP_UPDATE_STATUS_INSTALLED,
    SETUP_UPDATE_STATUS_ERROR
};

/*struct setup_update_version
{
    char current_version[64];
    char newer_version  [64];

    enum setup_update_status status;
};

struct setup_update_version_info
{
    struct fetch_callback        callback;
    char                        *temp_filename;
    char                        *dest_filename;
    struct setup_update_version *version;
};*/

struct setup_update_list_info
{
    struct fetch_callback callback;
};

static struct setup_update_list_info *create_suli(struct fetch_callback nested_callback)
{
    struct setup_update_list_info *suli = calloc(sizeof (*suli), 1);

    if (suli)
        suli->callback = nested_callback;

    return suli;
}

static void free_suli(struct setup_update_list_info **suli)
{
    if (suli && *suli)
    {
        free(*suli);
        *suli = NULL;
    }
}

static void available_updates_done(void *data, void *extra_data)
{
    struct setup_update_list_info *pli = data;
    struct fetch_done             *fd  = extra_data;

    if (fd && fd->success)
    {
        const char *filename = get_updated_path("version.txt");

        if (filename)
            load_updated_version();
        else
            setup_update_must_install = -1;

        goto_state(&st_game_setup);
    }
}

static unsigned int fetch_available_updates(int gdrive_support)
{
    unsigned int fetch_id = 0;
    const char  *filename = get_updated_path("version.txt");
    const char  *url      = get_updated_url("version.txt");

#if _WIN32 && ENABLE_FETCH<3 && ENABLE_SOFTWARE_UPDATE!=0
    if (url && filename)
    {
        struct fetch_callback          callback = {0};
        struct setup_update_list_info *suli     = create_suli(callback);

        callback.data = suli;
        callback.done = available_updates_done;

        fetch_id = fetch_file(url, filename, callback);

        if (!fetch_id)
        {
            free_suli(&suli);
            callback.data = NULL;
        }
    }
#endif

    return fetch_id;
}

/*---------------------------------------------------------------------------*/

static Array setup_langs;
static int   setup_langs_first;

static void game_setup_terms_openlink(const char *link)
{
    char buf[MAXSTR];

#ifdef __EMSCRIPTEN__
    EM_ASM({ window.open(UTF8ToString($0)); }, link);
#elif _WIN32
    SAFECPY(buf, "explorer ");
#elif defined(__APPLE__)
    SAFECPY(buf, "open ");
#elif defined(__linux__)
    SAFECPY(buf, "x-www-browser ");
#endif

    SAFECAT(buf, link);

#ifndef __EMSCRIPTEN__
    system(buf);
#endif
}

static int game_setup_action(int tok, int val)
{
    if (tok == GUI_BACK)
    {
        audio_play(AUD_DISABLED, 1.0f);
        return 1;
    }

    if (setup_page == 0)
    {
        struct lang_desc *desc;

        int total = array_len(setup_langs);

        GAMEPAD_GAMEMENU_ACTION_SCROLL(GUI_PREV, GUI_NEXT, SETUP_LANG_STEP);

        switch (tok)
        {
            case GUI_PREV:
                setup_langs_first = MAX(setup_langs_first - SETUP_LANG_STEP, 0);

                return exit_state(&st_game_setup);

                break;

            case GUI_NEXT:
                setup_langs_first = MIN(setup_langs_first + SETUP_LANG_STEP, total - 1);

                return goto_state(&st_game_setup);

                break;

#if ENABLE_NLS==1 || _MSC_VER
            case SETUP_LANG_DEFAULT:
                goto_state(&st_null);
                config_set_s(CONFIG_LANGUAGE, "");
                lang_init();
                setup_page++;
                lang_dir_free(setup_langs);
                setup_langs = NULL;
                return goto_state(&st_game_setup);

            case SETUP_LANG_SELECT:
                desc = LANG_GET(setup_langs, val);
                goto_state(&st_null);
                config_set_s(CONFIG_LANGUAGE, desc->code);
                lang_init();
                setup_page++;
                lang_dir_free(setup_langs);
                setup_langs = NULL;
                return goto_state(&st_game_setup);
#endif
        }
    }
    else
    {
        GENERIC_GAMEMENU_ACTION;

        int i = 0;

        switch (setup_page)
        {
            case 1:
                {
                    switch (tok)
                    {
                        case SETUP_CONTROLS_SELECT:
                            if (val == 2)
                                SETUP_CONTROL_SET_PRESET_KEYS(SDLK_c, SDLK_3, SDLK_1, SDLK_2,
                                                              SDLK_RIGHT, SDLK_LEFT,
                                                              SDLK_w, SDLK_a, SDLK_s, SDLK_d);
                            else if (val == 1)
                                SETUP_CONTROL_SET_PRESET_KEYS(SDLK_c, SDLK_3, SDLK_1, SDLK_2,
                                                              SDLK_d, SDLK_a,
                                                              SDLK_UP, SDLK_LEFT, SDLK_DOWN, SDLK_RIGHT);

#if _WIN32
                            if (val != 0)
                                setup_page++;
                            else
#endif
                                setup_page = 3;

                            return goto_state(&st_game_setup);
                    }
                }
                break;

            case 2:
                {
                    switch (tok)
                    {
                        case GUI_NEXT:
                            setup_page++;

                            return goto_state(&st_game_setup);
                    }
                }
                break;

            case 3:
                {
                    switch (tok)
                    {
                        case GUI_NEXT:
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
#if _WIN32 && ENABLE_FETCH==1 && ENABLE_SOFTWARE_UPDATE!=0
                            setup_page++;
#else
                            setup_page = 7;
#if NB_HAVE_PB_BOTH==1
                            if (account_wgcl_reload())
                                /* Let WGCL choose, how to load player datas. */;
                            else if (server_policy_get_d(SERVER_POLICY_EDITION) != 0)
                            {
#if NB_EOS_SDK==0 || NB_STEAM_API==0
                                if (goto_name_fn)
                                    return goto_name_fn(&st_game_setup, goto_ball_fn);
#else
                                if (goto_ball_fn)
                                    return goto_ball_fn(&st_game_setup);
#endif
                            }
#endif
#endif
#else
                            setup_page = 7;
#endif

#if _WIN32 && _MSC_VER && NB_HAVE_PB_BOTH==1 && \
    !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__)
                            if (setup_page == 7 &&
                                !account_wgcl_name_read_only())
                                return goto_wgcl_login(&st_game_setup, 0,
                                                       &st_game_setup, goto_ball_fn ? goto_ball_fn : 0);
#endif

                            return goto_state(&st_game_setup);

                        case SETUP_TERMS_TOGGLE:
                            setup_terms_accepted[val] = setup_terms_accepted[val] == 0;
                            game_setup_terms_checkmark_update_all();
                            break;

                        case SETUP_TERMS_READMORE:
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
                            if (val == 0)
                                game_setup_terms_openlink("https://pennyball.stynegame.de/terms");
                            else if (val == 1)
                                game_setup_terms_openlink("https://discord.com/terms");
                            else if (val == 2)
                                game_setup_terms_openlink("https://aka.ms/servicesagreement");
#endif
                            break;

                        case SETUP_TERMS_TOGGLE_ALL:
                            setup_terms_all_accepted = setup_terms_all_accepted == 0;

                            for (i = 0; i < TERMS_CARD_COUNT; i++)
                                setup_terms_accepted[i] = setup_terms_all_accepted;

                            game_setup_terms_checkmark_update_all();
                            break;
                    }
                }
                break;
            case 4:
            case 5:
                {
                    switch (tok)
                    {
                        case SETUP_UPDATE_START:
                            setup_page++;
                            config_save();
                            return goto_state(&st_game_setup);

                        case SETUP_UPDATE_SKIP:
                            setup_page = 7;
#if _WIN32 && _MSC_VER && NB_HAVE_PB_BOTH==1 && \
    !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
                            if (setup_page == 7)
                            {
                                if (account_wgcl_reload())
                                    /* Let WGCL choose, how to load player datas. */;
                                else if (server_policy_get_d(SERVER_POLICY_EDITION) != 0)
                                {
#if NB_EOS_SDK==0 || NB_STEAM_API==0
                                    if (goto_name_fn)
                                        return goto_name_fn(&st_game_setup, goto_ball_fn);
#else
                                    if (goto_ball_fn)
                                        return goto_ball_fn(&st_game_setup);
#endif
                                }
                                else if (!account_wgcl_name_read_only())
                                    return goto_wgcl_login(&st_game_setup, 0,
                                                           &st_game_setup, goto_ball_fn ? goto_ball_fn : 0);
                            }
#endif
                            return goto_state(&st_game_setup);
                    }
                }
            case 7:
                {
                    switch (tok)
                    {
#if ENABLE_GAME_TRANSFER==1 && defined(GAME_TRANSFER_TARGET)
                        case SETUP_TRANSFER:
                            setup_mkdir_migrate();
                            setup_process = 0;
                            config_save();
                            return st_continue ? goto_game_transfer(st_continue) : 0;
#endif
                        case SETUP_FINISHED:
                            setup_mkdir_migrate();
                            setup_process = 0;
                            config_save();
                            return fn_continue ? fn_continue(st_continue) :
                                                 st_continue ? goto_state(st_continue) : 0;
                    }
                }
                break;
        }
    }

    return 1;
}

/*
 * Setup Page 1: Language and Locales
 */
static int game_setup_lang_gui(void)
{
    const int step = (setup_langs_first == 0 ? SETUP_LANG_STEP - 1 : SETUP_LANG_STEP);

    int id, jd;
    int i;

    if ((id = gui_vstack(0)))
    {
        if ((jd = gui_hstack(id)))
        {
            gui_label(jd, _("Language"), GUI_SML, 0, 0);
            gui_space(jd);
            gui_filler(jd);
            gui_navig_full(jd, array_len(setup_langs), setup_langs_first, SETUP_LANG_STEP, 1);
        }

        gui_space(id);

        if (step < SETUP_LANG_STEP)
        {
            int default_id;
            default_id = gui_state(id, _("Default"), GUI_SML, SETUP_LANG_DEFAULT, 0);
            gui_set_hilite(default_id, !*config_get_s(CONFIG_LANGUAGE));
        }

        for (i = (step < SETUP_LANG_STEP ? setup_langs_first : setup_langs_first - 1);
            i < (step < SETUP_LANG_STEP ? setup_langs_first : setup_langs_first - 1) + step;
            i++)
        {
            if (i < array_len(setup_langs))
            {
                struct lang_desc *desc = LANG_GET(setup_langs, i);

#if NB_HAVE_PB_BOTH==1
                int lang_root_id, lang_top_id, lang_bot_id;

                if ((lang_root_id = gui_vstack(id)))
                {
                    lang_top_id = gui_label(lang_root_id, "XXXXXXXXXXXXXXXXXXXXXXXXXXXX", GUI_SML, GUI_COLOR_WHT);
                    lang_bot_id = gui_label(lang_root_id, "XXXXXXXXXXXXXXXXXXXXXXXXXXXX", GUI_TNY, GUI_COLOR_WHT);
                    gui_set_label(lang_top_id, " ");
                    gui_set_label(lang_bot_id, " ");
                    gui_set_trunc(lang_top_id, TRUNC_TAIL);
                    gui_set_trunc(lang_bot_id, TRUNC_TAIL);

                    gui_set_rect(lang_root_id, GUI_ALL);

                    /* Set font and rebuild texture. */

                    gui_set_font(lang_top_id, desc->font);
                    gui_set_label(lang_top_id, lang_name(desc));
                    gui_set_label(lang_bot_id, desc->name1);

                    gui_set_hilite(lang_root_id, (strcmp(config_get_s(CONFIG_LANGUAGE),
                                                         desc->code) == 0));
                    gui_set_state(lang_root_id, SETUP_LANG_SELECT, i);
                }
#else
                int lang_id;

                lang_id = gui_state(id, "XXXXXXXXXXXXXXXXXXXXXXXXXXXX",
                                    GUI_SML, SETUP_LANG_SELECT, i);
                gui_set_label(lang_id, " ");
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
#endif
            }
            else
            {
#if NB_HAVE_PB_BOTH==1
                int lang_root_id;

                if ((lang_root_id = gui_vstack(id)))
                {
                    gui_label(lang_root_id, " ", GUI_SML, 0, 0);
                    gui_label(lang_root_id, " ", GUI_TNY, 0, 0);
                    gui_set_rect(lang_root_id, GUI_ALL);
                }
#else
                gui_label(id, " ", GUI_SML, 0, 0);
#endif
            }
        }

        gui_layout(id, 0, 0);
    }

    return id;
}

/*
 * Setup Page 2: Preset controls
 */
static int game_setup_controls_gui(void)
{
    int root_id, id, jd, kd;

    if ((root_id = gui_root()))
    {
        if ((id = gui_vstack(root_id)))
        {
            gui_space(id);

            if ((jd = gui_vstack(id)))
            {
                gui_title_header(jd, _("Three input options"), GUI_MED, GUI_COLOR_DEFAULT);
                gui_label(jd, _("How would you like to get started?"), GUI_SML, GUI_COLOR_WHT);

                gui_set_rect(jd, GUI_ALL);
            }

            gui_layout(id, 0, +1);
        }

        if ((id = gui_hstack(root_id)))
        {
            if ((jd = gui_vstack(id)))
            {
                if ((kd = gui_vstack(jd)))
                {
                    gui_label(kd, _("Switchball HD"), GUI_SML, gui_wht, gui_cya);
                    gui_label(kd, _("Same as original Switchball"), GUI_TNY, GUI_COLOR_WHT);

                    gui_set_rect(kd, GUI_ALL);
                }

                gui_set_state(jd, SETUP_CONTROLS_SELECT, 2);
            }

            gui_space(id);

            if ((jd = gui_vstack(id)))
            {
                if ((kd = gui_vstack(jd)))
                {
                    gui_label(kd, _("Switchball"), GUI_SML, gui_wht, gui_cya);
                    gui_label(kd, _("Modern keybinding"), GUI_TNY, GUI_COLOR_WHT);

                    gui_set_rect(kd, GUI_ALL);
                }

                gui_set_state(jd, SETUP_CONTROLS_SELECT, 1);
            }

            gui_space(id);

            if ((jd = gui_vstack(id)))
            {
                if ((kd = gui_vstack(jd)))
                {
                    gui_label(kd, _("Neverball"), GUI_SML, gui_wht, gui_cya);
                    gui_label(kd, _("Classic keybinding"), GUI_TNY, GUI_COLOR_WHT);

                    gui_set_rect(kd, GUI_ALL);
                }

                gui_set_state(jd, SETUP_CONTROLS_SELECT, 0);
            }

            gui_layout(id, 0, 0);
        }
    }

    return root_id;
}

/*
 * Setup Page 3: Pre-existing installs
 */
static int game_setup_preexisting_gui(void)
{
    int root_id, id, jd;

    if ((root_id = gui_root()))
    {
        if ((id = gui_vstack(root_id)))
        {
            gui_space(id);

            if ((jd = gui_vstack(id)))
            {
#if NB_HAVE_PB_BOTH==1
                gui_title_header(jd, _("Pennyball is all you need"), GUI_MED, GUI_COLOR_DEFAULT);
#else
                gui_title_header(jd, _("Neverball is all you need"), GUI_MED, GUI_COLOR_DEFAULT);
#endif
                gui_multi(jd, _("You can simply install and run another games.\n"
                                "When you're ready, move on to the next step."),
                              GUI_SML, GUI_COLOR_WHT);

                gui_set_rect(jd, GUI_ALL);
            }

            gui_layout(id, 0, +1);
        }

        if ((id = gui_hstack(root_id)))
        {
            setup_gamedeps_card(id, 0,
                                "Switchball\\switchball.exe",
                                "SwitchballHD\\switchball.exe",
                                "Switchball HD\\switchball.exe");

            gui_layout(id, 0, 0);
        }

        if ((id = gui_vstack(root_id)))
        {
            if ((jd = gui_hstack(id)))
            {
                gui_label(jd, GUI_TRIANGLE_RIGHT, GUI_SML, GUI_COLOR_GRN);
                gui_label(jd, _("Next"), GUI_SML, GUI_COLOR_WHT);

                gui_set_state(jd, GUI_NEXT, 0);
                gui_set_rect(jd, GUI_ALL);
                gui_focus(jd);
            }

            gui_space(id);

            gui_layout(id, 0, -1);
        }
    }

    return root_id;
}

/*
 * Setup Page 4: PB+NB Terms & Conditions, Discord TOS, MSA
 */
static int game_setup_terms_gui(void)
{
    int root_id, id, jd;

    if ((root_id = gui_root()))
    {
        if ((id = gui_vstack(root_id)))
        {
            gui_space(id);

            if ((jd = gui_vstack(id)))
            {
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
                gui_title_header(jd, _("PB+NB, Discord ToS, MSA"), GUI_MED, GUI_COLOR_DEFAULT);
                gui_multi(jd, _("To proceed with setup, you must agree to the PB+NB Terms and acknowledge\n"
                                "that you have read and understood the Discord TOS and MSA by selecting \"OK\" below.\n"
                                "To learn more, click \"Read more\". You represent that you are a legally consenting adult\n"
                                "and are authorised to consent for all users of this Game, including minors."),
                              GUI_TNY, GUI_COLOR_WHT);
#else
                gui_title_header(jd, _("PB+NB, Discord, Nintendo ToS"), GUI_MED, GUI_COLOR_DEFAULT);
                gui_multi(jd, _("To proceed with setup, you must agree to the PB+NB Terms and acknowledge\n"
                                "that you have read and understood the Discord and Nintendo ToS by selecting \"OK\" below.\n"
                                "You represent that you are a legally consenting adult and are authorised to consent for\n"
                                "all users of this Game, including minors."),
                              GUI_TNY, GUI_COLOR_WHT);
#endif

                gui_set_rect(jd, GUI_ALL);
            }

            gui_layout(id, 0, +1);
        }

        if ((id = gui_vstack(root_id)))
        {
            setup_terms_card(id, _("PB+NB Terms of Service"),       0, 0);
            setup_terms_card(id, _("Discord Terms of Service"),     1, 0);
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
            setup_terms_card(id, _("Microsoft Services Agreement"), 2, 0);
#else
            setup_terms_card(id, _("Nintendo Terms of Service"),    2, 0);
#endif

            gui_layout(id, 0, 0);
        }

        if ((id = gui_vstack(root_id)))
        {
            if ((jd = gui_hstack(id)))
            {
                gui_label(jd, _("I agree to all"), GUI_SML, GUI_COLOR_DEFAULT);
                gui_space(jd);

                if ((setup_terms_master_tgl_id = gui_label(jd, GUI_CROSS, GUI_SML, GUI_COLOR_RED)))
                {
                    if (setup_terms_all_accepted)
                    {
                        gui_set_label(setup_terms_master_tgl_id, GUI_CHECKMARK);
                        gui_set_color(setup_terms_master_tgl_id, GUI_COLOR_GRN);
                    }
                    gui_set_state(setup_terms_master_tgl_id, SETUP_TERMS_TOGGLE_ALL, 0);
                }
            }

            gui_space(id);

            if ((jd = gui_harray(id)))
            {
                setup_btn_confirm_id = gui_label(jd, _("OK"), GUI_SML, GUI_COLOR_GRY);

                if (setup_btn_confirm_id)
                {
                    const GLubyte *tmp_color = setup_terms_all_accepted ? gui_wht : gui_gry;
                    gui_set_color(setup_btn_confirm_id, tmp_color, tmp_color);
                    gui_set_state(setup_btn_confirm_id, setup_terms_all_accepted ? GUI_NEXT : GUI_NONE, 1);
                }
            }

            gui_space(id);

            gui_layout(id, 0, -1);
        }
    }

    return root_id;
}

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
/*
 * Setup Page 5: Game updates
 */
static int game_setup_update_gui(void)
{
    int root_id, id;

    if ((root_id = gui_root()))
    {
        if ((id = gui_vstack(root_id)))
        {
            gui_space(id);
            gui_title_header(id, _("An even better version may be available"), GUI_MED, GUI_COLOR_DEFAULT);

            gui_layout(id, 0, +1);
        }

        if ((id = gui_vstack(root_id)))
        {
            gui_label(id, _("Checking for updates..."), GUI_SML, GUI_COLOR_WHT);

            gui_layout(id, 0, 0);
        }

        if ((id = gui_vstack(root_id)))
        {
            gui_start(id, _("Skip"), GUI_SML, SETUP_UPDATE_SKIP, 0);
            gui_space(id);

            gui_layout(id, 0, -1);
        }
    }

    return root_id;
}

/*
 * Setup Page 6: Confirm install updates
 */
static int game_setup_install_confirm_gui(void)
{
    int root_id, id, jd;

    if ((root_id = gui_root()))
    {
        if ((id = gui_vstack(root_id)))
        {
            gui_space(id);
            gui_title_header(id, _("An even better version may be available"), GUI_MED, GUI_COLOR_DEFAULT);

            gui_layout(id, 0, +1);
        }

        if (setup_update_must_install == 1)
        {
            if ((id = gui_vstack(root_id)))
            {
                gui_label(id, _("Install the latest game update."), GUI_SML, GUI_COLOR_WHT);
                update_install_dnld_eta_id = gui_label(id, _("XXXXXXXXXXXXXXXXXXXXXXXXXXXXX"),
                                                           GUI_TNY, GUI_COLOR_GRN);
                gui_space(id);
                gui_multi(id, _("Select \"Now\" to begin the update straight away.\n"
                                "Or select \"Later\" to install the update next time."),
                                GUI_TNY, GUI_COLOR_WHT);

                gui_set_label(update_install_dnld_eta_id, " ");
                gui_set_trunc(update_install_dnld_eta_id, TRUNC_TAIL);

                gui_set_rect(id, GUI_ALL);
                gui_layout(id, 0, 0);
            }

            if ((id = gui_vstack(root_id)))
            {
                if ((jd = gui_harray(id)))
                {
                    gui_state(jd, _("Later"), GUI_SML, SETUP_UPDATE_SKIP, 0);
                    gui_start(jd, _("Now"),   GUI_SML, SETUP_UPDATE_START, 0);
                }

                gui_space(id);

                gui_layout(id, 0, -1);
            }
        }
        else
        {
            if ((id = gui_vstack(root_id)))
            {
                gui_label(id, _("You have the latest game."), GUI_SML, GUI_COLOR_WHT);
                gui_space(id);
#if NB_HAVE_PB_BOTH==1
                gui_label(id, _("There are no new updates right now.\n"
                                "An update may be available for download at pennyball.stynegame.de"),
                              GUI_TNY, GUI_COLOR_WHT);
#else
                gui_label(id, _("There are no new updates right now.\n"
                                "An update may be available for download at neverball.org"),
                              GUI_TNY, GUI_COLOR_WHT);
#endif

                gui_layout(id, 0, 0);

                gui_set_rect(id, GUI_ALL);
            }

            if ((id = gui_vstack(root_id)))
            {
                gui_start(id, _("OK"), GUI_SML, SETUP_UPDATE_SKIP, 0);
                gui_space(id);

                gui_layout(id, 0, -1);
            }
        }
    }

    return root_id;
}

/*
 * Setup Page 7: Install updates
 */
static int game_setup_install_gui(void)
{
    int root_id, id;

    if ((root_id = gui_root()))
    {
        if ((id = gui_vstack(root_id)))
        {
            gui_space(id);
            gui_title_header(id, _("An even better version may be available"), GUI_MED, GUI_COLOR_DEFAULT);

            gui_layout(id, 0, +1);
        }

        if ((id = gui_vstack(root_id)))
        {
            gui_label(id, _("Downloading..."), GUI_SML, GUI_COLOR_WHT);
            update_install_dnldprog_id = gui_label(id, "XXXXXXX", GUI_MED, GUI_COLOR_GRN);
            update_install_dnld_eta_id = gui_label(id, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXX", GUI_TNY, GUI_COLOR_GRN);
            gui_space(id);
            gui_multi(id, _("Once the update is completed, the game will itself saved\n"
                            "into the user data folder and quit automatically."),
                            GUI_TNY, GUI_COLOR_WHT);

            gui_set_label(update_install_dnldprog_id, " ");
            gui_set_label(update_install_dnld_eta_id, " ");
            gui_set_trunc(update_install_dnld_eta_id, TRUNC_TAIL);

            gui_set_rect(id, GUI_ALL);
            gui_layout(id, 0, 0);
        }
    }

    return root_id;
}

#endif

/*
 * Setup Page 8: Finish setup
 */
static int game_setup_welcome_gui(void)
{
    int root_id, id, jd, kd;

    if ((root_id = gui_root()))
    {
        if ((id = gui_vstack(root_id)))
        {
            gui_space(id);
            gui_title_header(id, _("Welcome to Neverball!"), GUI_MED, GUI_COLOR_DEFAULT);

            gui_layout(id, 0, +1);
        }

        if ((id = gui_vstack(root_id)))
        {
            const int ww = MIN(video.device_w, video.device_h) / 1.5f;
            const int hh = ww / 1.7777f;

            gui_image(id, "png/premium-planet.png", ww, hh);
            gui_space(id);
#if ENABLE_GAME_TRANSFER==1 && defined(GAME_TRANSFER_TARGET)
            gui_multi(id, _("Select \"Start\", the game is ready to start.\n"
                            "Or select \"Transfer\" to transfer from the source game."),
                            GUI_TNY, GUI_COLOR_WHT);
#else
            gui_label(id, _("Select \"Start\", the game is ready to start!"),
                            GUI_TNY, GUI_COLOR_WHT);
#endif

            gui_layout(id, 0, 0);
        }

        if ((id = gui_vstack(root_id)))
        {
#if ENABLE_GAME_TRANSFER==1 && defined(GAME_TRANSFER_TARGET)
            if ((jd = gui_harray(id)))
            {
                gui_state(jd, _("Transfer"), GUI_SML, SETUP_TRANSFER, 0);

                if ((kd = gui_hstack(jd)))
                {
                    gui_label(kd, GUI_TRIANGLE_RIGHT, GUI_SML, GUI_COLOR_GRN);
                    gui_label(kd, _("Start"), GUI_SML, GUI_COLOR_WHT);

                    gui_set_state(kd, SETUP_FINISHED, 0);
                    gui_set_rect(kd, GUI_ALL);
                    gui_focus(kd);
                }
            }
#else
            if ((jd = gui_hstack(id)))
            {
                gui_label(jd, GUI_TRIANGLE_RIGHT, GUI_SML, GUI_COLOR_GRN);
                gui_label(jd, _("Start"), GUI_SML, GUI_COLOR_WHT);

                gui_set_state(jd, SETUP_FINISHED, 0);
                gui_set_rect(jd, GUI_ALL);
                gui_focus(jd);
            }
#endif
            gui_space(id);

            gui_layout(id, 0, -1);
        }
    }

    return root_id;
}

static int game_setup_enter(struct state *st, struct state *prev, int intent)
{
#if ENABLE_NLS==1 || _MSC_VER
    if (!setup_langs)
    {
        setup_langs       = lang_dir_scan();
        setup_langs_first = 0;
    }
#endif

    common_init(game_setup_action);
    audio_music_fade_to(.5f, "bgm/setup.ogg", 1);
    back_init("back/gui.png");

    if (setup_page == 4)
        fetch_available_updates(1);

    switch (setup_page)
    {
        case 0: return transition_slide(game_setup_lang_gui(),        1, intent);
        case 1: return transition_slide(game_setup_controls_gui(),    1, intent);
#if _WIN32
        case 2: return transition_slide(game_setup_preexisting_gui(), 1, intent);
#endif
        case 3: return transition_slide(game_setup_terms_gui(),       1, intent);
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
        case 4: return transition_slide(game_setup_update_gui(),          1, intent);
        case 5: return transition_slide(game_setup_install_confirm_gui(), 1, intent);
        case 6: return transition_slide(game_setup_install_gui(),         1, intent);
#endif
        case 7: return transition_slide(game_setup_welcome_gui(), 1, intent);
        default: return 0;
    }
}

static int game_setup_leave(struct state *st, struct state *next, int id, int intent)
{
    back_free();

    if (next == &st_null &&
        ((setup_page == 6 && setup_update_version_finished) ||
         setup_page == 7))
        setup_mkdir_migrate();

    return transition_slide(id, 0, intent);
}

static void game_setup_paint(int id, float t)
{
    video_set_perspective((float) config_get_d(CONFIG_VIEW_FOV), 0.1f, FAR_DIST);
    back_draw_easy();

    gui_paint(id);
}

static void game_setup_timer(int id, float dt)
{
    gui_timer(id, dt);

    if (setup_page == 6 && setup_update_version_finished &&
        !st_global_animating())
    {
        setup_mkdir_migrate();

        /* bye! */
        SDL_Event e = { SDL_QUIT };
        SDL_PushEvent(&e);
    }
}

/*---------------------------------------------------------------------------*/

struct state st_game_setup = {
    game_setup_enter,
    game_setup_leave,
    game_setup_paint,
    game_setup_timer,
    common_point,
    common_stick,
    NULL,
    common_click,
    common_keybd,
    common_buttn
};
