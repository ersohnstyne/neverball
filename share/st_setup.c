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
#include "text.h"
#include "fetch.h"

#include "st_common.h"

#include "st_setup.h"

#if _DEBUG && _MSC_VER
#ifndef _CRTDBG_MAP_ALLOC
#pragma message(__FILE__": Missing CRT-Debugger include header, recreate: crtdbg.h")
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

#define AUD_MENU     "snd/menu.ogg"
#define AUD_BACK     "snd/back.ogg"
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

static int    (*goto_name_fn) (struct state *finish,
                               int         (*finish_fn) (struct state *));
static int    (*goto_ball_fn) (struct state *finish);

static int    setup_page = 0;
static int    setup_process = 0;

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

int goto_game_setup(struct state *start_state,
                    int (*new_goto_name_fn) (struct state *,
                                             int         (*new_finish_fn) (struct state *)),
                    int (*new_goto_ball_fn) (struct state *))
{
    st_continue   = start_state;
    setup_page    = 0;
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
#if _WIN32 && ENABLE_FETCH==1
    setup_page++;
#else
    setup_page = 6;
#endif

    return goto_state(&st_game_setup);
}

static int title_check_playername(const char *regname)
{
    for (int i = 0; i < text_length(regname); i++)
    {
        if (regname[i] == '\\' ||
            regname[i] == '/' ||
            regname[i] == ':' ||
            regname[i] == '*' ||
            regname[i] == '?' ||
            regname[i] == '"' ||
            regname[i] == '<' ||
            regname[i] == '>' ||
            regname[i] == '|')
            return 0;
    }

    return text_length(regname) >= 3;
}

/*
 * Check whether has meet fullfilled folder requirements
 */
int check_game_setup(void)
{
    const char *curr_write_dir = fs_get_write_dir();

    return dir_exists(path_join(curr_write_dir, "Accounts"))    &&
           dir_exists(path_join(curr_write_dir, "Campaign"))    &&
           dir_exists(path_join(curr_write_dir, "Replays"))     &&
           dir_exists(path_join(curr_write_dir, "Scores"))      &&
           dir_exists(path_join(curr_write_dir, "Screenshots"));
}

/*
 * Check whether setup is in process
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

    SETUP_FINISHED
};

static int setup_terms_card(int pd, const char *name, int idx, int readmore_disabled)
{
    int id, jd;

    if ((id = gui_hstack(pd)))
    {
        if ((jd = gui_state(id, _("Read More"), GUI_SML, SETUP_TERMS_READMORE, idx)))
        {
            if (readmore_disabled)
            {
                gui_set_color(jd, GUI_COLOR_GRY);
                gui_set_state(jd, GUI_NONE, idx);
            }
        }

        gui_space(id);

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

#define NB_CURRDOMAIN_PREMIUM "play.neverball.org"

static const char *get_updated_url(const char *filename)
{
    if (filename && *filename)
    {
        static char url[MAXSTR];
        memset(url, 0, sizeof(url));

#if NB_HAVE_PB_BOTH==1
        SAFECPY(url, "https://" NB_CURRDOMAIN_PREMIUM "/update/");
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

        memset(path, 0, sizeof(path));

        SAFECPY(path, "GameUpdate/");
        SAFECAT(path, filename);

        return path;
    }
    return NULL;
}

static int load_updated_version(void)
{
    fs_file fp = fs_open_read(get_updated_path("version.txt"));

    if (fp)
    {
        char line[MAXSTR] = "";

        int curr_major = 0,
            curr_minor = 0,
            curr_patch = 0,
            curr_revision = 0,
            curr_build = 0;

        int new_major = 0,
            new_minor = 0,
            new_patch = 0,
            new_revision = 0,
            new_build = 0;

        while (fs_gets(line, sizeof(line), fp))
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

    return 0;
}

/*---------------------------------------------------------------------------*/

static int setup_is_replay(struct dir_item *item)
{
    return str_ends_with(item->path, ".nbr");
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

    if (fd && fd->finished)
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

#if _WIN32 && ENABLE_FETCH>=2
    if (filename)
    {
        struct fetch_callback          callback = {0};
        struct setup_update_list_info *suli     = create_suli(callback);

        callback.data = suli;
        callback.done = available_updates_done;

        fetch_id = fetch_gdrive("1RCfauN7HxxDXbSxltz82IC4BHeGZC9cH", filename, callback);

        if (!fetch_id)
        {
            free_suli(&suli);
            callback.data = NULL;
        }
    }

    if (fetch_id) return fetch_id;
#endif

    const char *url = get_updated_url("version.txt");

#if _WIN32 && ENABLE_FETCH<3
    if (url && filename)
    {
        struct fetch_callback          callback = {0};
        struct setup_update_list_info *suli     = create_suli(callback);

        callback.data = suli;
        callback.done = available_updates_done;

        fetch_id = fetch_url(url, filename, callback);

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
    EM_ASM({ window.open($0); }, link);
#elif _WIN32
    SAFECPY(buf, "start msedge ");
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
        audio_play(AUD_DISABLED, 1.f);
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

                return goto_state(&st_game_setup);

                break;

            case GUI_NEXT:
                setup_langs_first = MIN(setup_langs_first + SETUP_LANG_STEP, total - 1);

                return goto_state(&st_game_setup);

                break;

            case SETUP_LANG_DEFAULT:
                goto_state(&st_null);
                config_set_s(CONFIG_LANGUAGE, "");
                lang_init();
                setup_page++;
                return goto_state(&st_game_setup);

            case SETUP_LANG_SELECT:
                desc = LANG_GET(setup_langs, val);
                goto_state(&st_null);
                config_set_s(CONFIG_LANGUAGE, desc->code);
                lang_init();
                setup_page++;
                return goto_state(&st_game_setup);
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

                            setup_page++;

                            return goto_state(&st_game_setup);
                    }
                }
                break;
            case 2:
                {
                    switch (tok)
                    {
                        case GUI_NEXT:
                            if (goto_name_fn)
                                return goto_name_fn(&st_game_setup, goto_ball_fn);
                            else
#if _WIN32 && ENABLE_FETCH==1
                                setup_page++;
#else
                                setup_page = 6;
#endif

                            return goto_state(&st_game_setup);
                            break;
                        case SETUP_TERMS_TOGGLE:
                            setup_terms_accepted[val] = setup_terms_accepted[val] == 0;
                            game_setup_terms_checkmark_update_all();
                            break;

                        case SETUP_TERMS_READMORE:
                            if (val == 1)
                                game_setup_terms_openlink("https://discord.com/terms");
                            else if (val == 2)
                                game_setup_terms_openlink("https://aka.ms/servicesagreement");
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
            case 3:
            case 4:
                {
                    switch (tok)
                    {
                        case SETUP_UPDATE_START:
                            setup_page++;
                            config_save();
                            return goto_state(&st_game_setup);

                        case SETUP_UPDATE_SKIP:
                            setup_page = 6;
                            return goto_state(&st_game_setup);
                    }
                }
            case 6:
                {
                    switch (tok)
                    {
                        case SETUP_FINISHED:
                            lang_dir_free(setup_langs);
                            setup_langs = NULL;
                            setup_process = 0;
                            config_save();
                            return st_continue ? goto_state(st_continue) : 0;
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
                gui_title_header(jd, _("Three simple options"), GUI_MED, GUI_COLOR_DEFAULT);
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
                    gui_label(kd, _("Quick and Easy"), GUI_TNY, GUI_COLOR_WHT);

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
 * Setup Page 3: PB+NB Terms & Conditions, Discord TOS, MSA
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
                gui_title_header(jd, _("PB+NB Terms & Conditions, Discord TOS, MSA"), GUI_MED, GUI_COLOR_DEFAULT);
                gui_multi(jd, _("To proceed with setup, you must agree to the PB+NB Terms & Conditions and acknowledge\n"
                                "that you have read and understood the Discord TOS and MSA by selecting \"OK\" below.\n"
                                "To learn more, click \"Read more\". You represent that you are a legally consenting adult\n"
                                "and are authorised to consent for all users of this Game, including minors."),
                              GUI_TNY, GUI_COLOR_WHT);

                gui_set_rect(jd, GUI_ALL);
            }

            gui_layout(id, 0, +1);
        }

        if ((id = gui_vstack(root_id)))
        {
            setup_terms_card(id, _("PB+NB Terms and Conditions"),   0, 1);
            setup_terms_card(id, _("Discord Terms of Service"),     1, 0);
            setup_terms_card(id, _("Microsoft Services Agreement"), 2, 0);

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

/*
 * Setup Page 4: Game updates
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
 * Setup Page 5: Confirm install updates
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
                update_install_dnld_eta_id = gui_label(id, _("XXXXXXXXXXXXXXXXXXXXXXXXXXXXX"), GUI_SML, GUI_COLOR_GRN);
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
                    gui_start(jd, _("Now"), GUI_SML, SETUP_UPDATE_START, 0);
                }

                gui_space(id);

                gui_layout(id, 0, -1);
            }
        }
        else
        {
            if ((id = gui_vstack(root_id)))
            {
                gui_label(id, _("Your game is up to date."), GUI_SML, GUI_COLOR_WHT);
                gui_space(id);
                gui_label(id, _("Select \"OK\", the game is ready to start!"),
                              GUI_TNY, GUI_COLOR_WHT);

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
 * Setup Page 6: Install updates
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
            update_install_dnld_eta_id = gui_label(id, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXX", GUI_SML, GUI_COLOR_GRN);
            gui_space(id);
            gui_multi(id, _("Once updated, the game will itself saved\n"
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

/*
 * Setup Page 7: Install updates
 */
static int game_setup_welcome_gui(void)
{
    int root_id, id, jd;

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
            gui_label(id, _("Select \"Start\", the game is ready to start!"),
                            GUI_TNY, GUI_COLOR_WHT);

            gui_layout(id, 0, 0);
        }

        if ((id = gui_vstack(root_id)))
        {
            if ((jd = gui_hstack(id)))
            {
                gui_label(jd, GUI_TRIANGLE_RIGHT, GUI_SML, GUI_COLOR_GRN);
                gui_label(jd, _("Start"), GUI_SML, GUI_COLOR_WHT);

                gui_set_state(jd, SETUP_FINISHED, 0);
                gui_set_rect(jd, GUI_ALL);
                gui_focus(jd);
            }

            gui_space(id);

            gui_layout(id, 0, -1);
        }
    }

    return root_id;
}

static int game_setup_enter(struct state *st, struct state *prev)
{
    if (!setup_langs)
    {
        setup_langs = lang_dir_scan();
        setup_langs_first = 0;
    }

    common_init(game_setup_action);
    audio_music_fade_to(.5f, "bgm/setup.ogg");
    back_init("back/gui.png");

    if (setup_page == 3)
        fetch_available_updates(1);

    if (setup_page == 6)
        setup_mkdir_migrate();

    switch (setup_page)
    {
        case 0: return game_setup_lang_gui();
        case 1: return game_setup_controls_gui();
        case 2: return game_setup_terms_gui();
        case 3: return game_setup_update_gui();
        case 4: return game_setup_install_confirm_gui();
        case 5: return game_setup_install_gui();
        case 6: return game_setup_welcome_gui();
        default: return 0;
    }
}

static void game_setup_leave(struct state *st, struct state *next, int id)
{
    back_free();

    gui_delete(id);

    if (next == &st_null &&
        setup_page == 5)
        setup_mkdir_migrate();
}

static void game_setup_paint(int id, float t)
{
    video_push_persp((float) config_get_d(CONFIG_VIEW_FOV), 0.1f, FAR_DIST);
    back_draw_easy();
    video_pop_matrix();

    gui_paint(id);
}

static void game_setup_timer(int id, float dt)
{
    gui_timer(id, dt);

    if (setup_page == 5 && setup_update_version_finished &&
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