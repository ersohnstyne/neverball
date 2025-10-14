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

#include "gui.h"
#include "transition.h"
#include "audio.h"
#if NB_HAVE_PB_BOTH==1
#include "account.h"
#include "config_wgcl.h"
#endif
#include "config.h"
#include "networking.h"
#include "common.h"
#include "package.h"
#include "geom.h"
#include "lang.h"
#include "ball.h"

#include "st_package.h"
#include "st_common.h"

#if NB_HAVE_PB_BOTH==1
#include "st_wgcl.h"
#endif

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
                audio_play(first > 1 ?                       \
                           AUD_MENU : AUD_DISABLED, 1.0f);   \
            if (tok == tok2)                                 \
                audio_play(first + itemstep < total ?        \
                           AUD_MENU : AUD_DISABLED, 1.0f);   \
        } else GENERIC_GAMEMENU_ACTION

/*---------------------------------------------------------------------------*/

struct state st_package_manage;

#define PACKAGE_STEP 4

static int package_manual_hotreload = 0;

static int total    = 0;
static int first    = 0;
static int selected = 0;

static int shot_id;
static int desc_id;
static int type_id;
static int title_id;
static int install_id;
static int install_status_id;
static int install_label_id;

#if NB_HAVE_PB_BOTH==1
static int category_id = PACKAGE_CATEGORY_LEVELSET;
#endif
static int do_init     = 1;
static int do_download = 0;

static struct state *package_back;

static int  button_ids[PACKAGE_STEP] = {0};
static int *status_ids = NULL;
static int *name_ids   = NULL;

/* === PREFIX/SUFFIX CONVERTER === */

/* This prefix enum "PACKAGE_*" will be replaced into "ADDONS_*". */
#define PACKAGE_INSTALL     ADDONS_INSTALL

/* This prefix enum "PACKAGE_*" will be replaced into "ADDONS_*". */
#define PACKAGE_UNINSTALL   ADDONS_UNINSTALL

/* This prefix enum "PACKAGE_*" will be replaced into "ADDONS_*". */
#define PACKAGE_SELECT      ADDONS_SELECT

/* This prefix enum "PACKAGE_*" will be replaced into "ADDONS_*". */
#define PACKAGE_CHANGEGROUP ADDONS_CHANGEGROUP

/* === END PREFIX/SUFFIX CONVERTER === */

enum
{
    PACKAGE_INSTALL = GUI_LAST,
    PACKAGE_UNINSTALL,
    PACKAGE_SELECT,
    PACKAGE_CHANGEGROUP,
};

struct download_info
{
    char *package_id;
    char  label[32];
};

static int (*installed_action)(int pi);

/*---------------------------------------------------------------------------*/

/* === PREFIX/SUFFIX CONVERTER === */

/* This prefix function "package_*" will be replaced into "addons_*". */
#define package_start_download addons_start_download

/* This prefix function "package_*" will be replaced into "addons_*". */
#define package_action     addons_action

/* This prefix function "package_*" will be replaced into "addons_*". */
#define package_gui        addons_gui

/* This prefix function "package_*" will be replaced into "addons_*". */
#define package_select     addons_select

/* This prefix function "package_*" will be replaced into "addons_*". */
#define gui_package        gui_addons

/* This prefix function "package_*" will be replaced into "addons_*". */
#define gui_package_button gui_addons_button

/* This prefix function "package_*" will be replaced into "addons_*". */
#define package_enter      addons_enter

/* This prefix function "package_*" will be replaced into "addons_*". */
#define package_leave      addons_leave

/* This prefix function "package_*" will be replaced into "addons_*". */
#define package_paint      addons_paint

/* This prefix function "package_*" will be replaced into "addons_*". */
#define package_timer      addons_timer

/* This prefix function "package_*" will be replaced into "addons_*". */
#define package_point      addons_point

/* This prefix function "package_*" will be replaced into "addons_*". */
#define package_stick      addons_stick

/* This prefix function "package_*" will be replaced into "addons_*". */
#define package_keybd      addons_keybd

/* === END PREFIX/SUFFIX CONVERTER === */

static struct download_info *create_download_info(const char *package_id)
{
    struct download_info *dli = calloc(sizeof (*dli), 1);

    if (dli)
        dli->package_id = strdup(package_id);

    return dli;
}

static void free_download_info(struct download_info *dli)
{
    if (dli)
    {
        if (dli->package_id)
        {
            free(dli->package_id);
            dli->package_id = NULL;
        }

        free(dli);
        dli = NULL;
    }
}

static void download_progress(void *data1, void *data2)
{
    struct download_info  *dli = data1;
    struct fetch_progress *pr = data2;

    if (dli)
    {
        /* Sets may change places, so we can't keep set index around. */
        int pi = package_search_id(dli->package_id);

        if (status_ids && pi >= 0 && pi < total)
        {
            int id = status_ids[pi];

            if (id)
            {
                char label[32] = GUI_ELLIPSIS;

                if (pr->total > 0)
                    sprintf(label, "%3d%%",
                                   (int) (pr->now * 100.0 / pr->total) % 1000);

                /* Compare against current label so we're not calling GL constantly. */
                /* TODO: just do this in gui_set_label. */

                if (strcmp(label, dli->label) != 0)
                {
                    SAFECPY(dli->label, label);
                    gui_set_label(id, label);
                }
            }
        }
    }
}

static void package_select(int);

static void download_done(void *data1, void *data2)
{
    struct download_info *dli = data1;
    struct fetch_done    *dn = data2;

    if (dli)
    {
        int pi = package_search_id(dli->package_id);

        if (status_ids && pi >= 0 && pi < total)
        {
            int id = status_ids[pi];

            if (selected == pi)
            {
                /* Update GUI. */
                package_select(pi);
            }

            if (id)
            {
                if (dn->success)
                {
                    gui_set_label(id, GUI_CHECKMARK);
                    gui_set_color(id, gui_grn, gui_grn);

                    if (name_ids && name_ids[pi])
                    {
                        gui_set_label(name_ids[pi], package_get_name(pi));
                        gui_pulse(name_ids[pi], 1.2f);
                    }
                }
                else
                {
                    audio_play("snd/uierror.ogg", 1.0f);
                    gui_set_label(id, "!");
                    gui_set_color(id, gui_red, gui_red);

#if NB_HAVE_PB_BOTH==1
                    goto_state(&st_wgcl_error_offline);
#endif
                }
            }
        }

        free_download_info(dli);
        dli = NULL;
    }
}

/*---------------------------------------------------------------------------*/

struct image_download_info
{
    int pi;
};

static struct image_download_info *create_idi(int pi)
{
    struct image_download_info *idi = calloc(sizeof (*idi), 1);

    if (idi)
        idi->pi = pi;

    return idi;
}

static void free_idi(struct image_download_info **idi)
{
    if (idi && *idi)
    {
        free(*idi);
        *idi = NULL;
    }
}

static void image_download_done(void *data, void *extra_data)
{
    struct image_download_info *idi = data;
    struct fetch_done *fd = extra_data;

    if (idi)
    {
        if (fd && fd->success)
        {
            if (idi->pi == selected)
                gui_set_image(shot_id, package_get_shot_filename(selected));
        }

        free_idi(&idi);
    }
}

static void fetch_package_images(void)
{
    int pi;

    for (pi = first; pi < first + PACKAGE_STEP && pi < total; ++pi)
    {
        struct fetch_callback      callback = {0};
        struct image_download_info *idi = create_idi(pi);

        callback.done = image_download_done;
        callback.data = idi;

        if (!package_fetch_image(pi, callback))
        {
            free_idi(&idi);
            callback.data = NULL;
        }
    }
}

/*---------------------------------------------------------------------------*/

static int package_manage_can_start = 0;
static int package_manage_can_equip = 0;
static int package_manage_selected  = 0;

static void package_start_download(int id)
{
    do_download = 0;
    selected = id;

    struct fetch_callback callback = {0};

    callback.progress = download_progress;
    callback.done     = download_done;
    callback.data     = create_download_info(package_get_id(id));

#if NB_HAVE_PB_BOTH==1
    if (!package_fetch(id, callback, category_id))
#else
    if (!package_fetch(id, callback, 0))
#endif
    {
        free_download_info(callback.data);
        callback.data = NULL;
    }
    else if (status_ids && status_ids[id])
    {
        gui_set_label(status_ids[id], GUI_ELLIPSIS);
        gui_set_color(status_ids[id], gui_grn, gui_grn);
    }
}

static int package_action(int tok, int val)
{
    enum package_status status;

    int r = 1;

    /*
     * Issue #352: This gamepad features has been fixed, which it has
     * found on out of bounds bug, so let Mojang doing this.
     */

    GAMEPAD_GAMEMENU_ACTION_SCROLL(GUI_PREV, GUI_NEXT, PACKAGE_STEP);

    switch (tok)
    {
        case GUI_BACK:
            package_manage_selected = -1;

#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_WGCL_GAME_OPTIONS)
            if (package_manage_selected == -1 && WGCL_GameOptions_Exists)
                WGCL_BackToGameOptions("gameoptions_st_data");
#endif

            r = exit_state(package_back);

            if (package_manage_selected == -1)
                package_back = 0;

            return r;
            break;

        case GUI_PREV:
            first = MAX(first - PACKAGE_STEP, 0);

            do_init = 0;
            return exit_state(&st_package);

            break;

        case GUI_NEXT:
            first = MIN(first + PACKAGE_STEP, total - 1);

            do_init = 0;
            return goto_state(&st_package);

            break;

        case PACKAGE_SELECT:
            package_select(val);
            break;

        case PACKAGE_UNINSTALL:
        case PACKAGE_INSTALL:
            status = package_get_status(selected);

            if (status == PACKAGE_INSTALLED ||
                status == PACKAGE_UPDATE)
            {
                package_manage_can_start = strcmp(package_get_type(selected), "set") == 0;
                package_manage_can_equip = strcmp(package_get_type(selected), "ball") == 0;

                package_manage_selected = selected;

                return goto_state(&st_package_manage);
            }
            else if (status == PACKAGE_AVAILABLE ||
                     status == PACKAGE_ERROR)
            {
                package_start_download(selected);

                return r;
            }
            else if (status == PACKAGE_DOWNLOADING)
                return r;
            break;

        case PACKAGE_CHANGEGROUP:
#if NB_HAVE_PB_BOTH==1
            category_id = val;
#endif
            break;
    }

    return r;
}

static int gui_package_button(int id, int pi)
{
    int jd;

    if ((jd = gui_hstack(id)))
    {
        int status_id, name_id;
        enum package_status status = package_get_status(pi);

        status_id = gui_label(jd, "100%", GUI_SML, gui_grn, gui_grn);

        switch (status)
        {
            case PACKAGE_INSTALLED:   gui_set_label(status_id, GUI_CHECKMARK);    break;
            case PACKAGE_DOWNLOADING: gui_set_label(status_id, GUI_ELLIPSIS);     break;
            case PACKAGE_UPDATE:      gui_set_label(status_id, GUI_CIRCLE_ARROW); break;
            default:                  gui_set_label(status_id, GUI_ARROW_DN);     break;
        }

        gui_set_font(status_id, "ttf/DejaVuSans-Bold.ttf");

        if (status_ids)
            status_ids[pi] = status_id;

        name_id = gui_label(jd, "XXXXXXXXXXXXXXXXX", GUI_SML, gui_wht, gui_wht);

        gui_set_trunc(name_id, TRUNC_TAIL);
        gui_set_label(name_id, package_get_name(pi));
        gui_set_fill (name_id);

        if (name_ids)
            name_ids[pi] = name_id;

        gui_set_state(jd, PACKAGE_SELECT, pi);
        gui_set_rect (jd, GUI_ALL);
    }

    return jd;
}

static int gui_package(int id, int pi)
{
    if (pi >= 0 && pi < package_count())
        return gui_package_button(id, pi);
    else
        return gui_label(id, "", GUI_SML, 0, 0);
}

static int package_gui(void)
{
    int w = video.device_w;
    int h = video.device_h;
    int id, jd, kd, ld;

    int i;

    if (total <= 0)
    {
        if ((id = gui_vstack(0)))
        {
            gui_title_header(id, _("No addons found"), GUI_MED, 0, 0);
#if defined(CONFIG_INCLUDES_ACCOUNT) && ENABLE_FETCH!=0
#if (NB_STEAM_API!=1 && NB_EOS_SDK!=1)
            gui_space (id);
            gui_multi (id, _("Use the web shop or mobile\nto buy new addons."),
                           GUI_SML, gui_wht, gui_wht);
#elif NB_EOS_SDK==1
            gui_space (id);
            gui_multi (id, _("Use the Epic Games Store launcher\nto buy new addons."),
                           GUI_SML, gui_wht, gui_wht);
#elif NB_STEAM_API==1
            gui_space (id);
            gui_multi (id, _("Use the Steam launcher\nto buy new addons."),
                          GUI_SML, gui_wht, gui_wht);
#endif
#endif
            gui_space (id);
            gui_state (id, _("Back"), GUI_SML, GUI_BACK, 0);
            gui_layout(id, 0, 0);
        }

        return id;
    }

    if ((id = gui_vstack(0)))
    {
        if ((jd = gui_hstack(id)))
        {
            gui_label (jd, _("Addons"), GUI_SML, 0, 0);
            gui_filler(jd);
            gui_navig (jd, total, first, PACKAGE_STEP);
        }

        gui_space(id);

        if ((jd = gui_vstack(id)))
        {
            if ((kd = gui_hstack(jd)))
            {
                const int ww = MIN(w, h) * 5 / 12;
                const int hh = ww / 4 * 3;

                if ((ld = gui_varray(kd)))
                {
                    for (i = first; i < first + PACKAGE_STEP; i++)
                    {
                        int button_id = gui_package(ld, i);

                        button_ids[i % PACKAGE_STEP] = button_id;
                    }

                    gui_set_fill(ld);
                }

                shot_id = gui_image(kd, package_get_shot_filename(first), ww, hh);
            }

            gui_space(jd);

            if ((kd = gui_vstack(jd)))
            {
                title_id = gui_label(kd, package_get_name(first), GUI_SML, gui_yel, gui_wht);

                gui_space(kd);

                desc_id = gui_multi(kd, " \n \n \n \n \n", GUI_SML, gui_yel, gui_wht);

                gui_set_rect(kd, GUI_ALL);
            }

            gui_space(jd);

            if ((kd = gui_hstack(jd)))
            {
                if ((ld = gui_hstack(kd)))
                {
                    install_status_id = gui_label(ld, GUI_ARROW_DN,
                                                      GUI_SML, gui_grn, gui_grn);
                    install_label_id = gui_label(ld, "XXXXXXXXXXXX",
                                                     GUI_SML, gui_wht, gui_wht);

                    gui_set_label(install_label_id, _("Install"));

                    gui_set_font(install_status_id, "ttf/DejaVuSans-Bold.ttf");

                    gui_set_rect(ld, GUI_ALL);
                    gui_set_state(ld, PACKAGE_INSTALL, 0);

                    install_id = ld;
                }

                gui_filler(kd);

                type_id = gui_label(kd, "XXXXXXX", GUI_SML, GUI_COLOR_DEFAULT);
            }
        }

        gui_layout(id, 0, 0);

        gui_set_label(type_id, package_get_formatted_type(first));
    }

    package_select(selected);

    return id;
}

static void package_select(int pi)
{
    enum package_status status;

    if (pi < first || pi >= MIN(first + PACKAGE_STEP, total))
        return;

    status = package_get_status(pi);

    gui_set_hilite(button_ids[selected % PACKAGE_STEP], 0);
    selected = pi;
    gui_set_hilite(button_ids[selected % PACKAGE_STEP], 1);

    gui_set_image(shot_id,  package_get_shot_filename(pi));
    gui_set_multi(desc_id,  package_get_desc(pi));
    gui_set_label(type_id,  package_get_formatted_type(pi));
    gui_set_label(title_id, package_get_name(pi));

    if      (status == PACKAGE_UPDATE)
    {
        gui_set_label(install_status_id, GUI_CIRCLE_ARROW);
        gui_set_label(install_label_id, _("Update"));
    }
    else if (status == PACKAGE_INSTALLED)
    {
        gui_set_label(install_status_id, "");
        gui_set_label(install_label_id, _("Manage"));
    }
    else
    {
        gui_set_label(install_status_id, GUI_ARROW_DN);
        gui_set_label(install_label_id, _("Install"));
    }

    gui_set_font(install_status_id, "ttf/DejaVuSans-Bold.ttf");

    if (status == PACKAGE_INSTALLED)
    {
        /* HACK: Mojang made done this. */

        gui_set_color(install_status_id, gui_gry, gui_gry);
        gui_set_color(install_label_id,  gui_wht, gui_wht);
    }
    else
    {
        gui_set_color(install_status_id, gui_grn, gui_grn);
        gui_set_color(install_label_id,  gui_wht, gui_wht);
    }
}

static int package_enter(struct state *st, struct state *prev, int intent)
{
    common_init(package_action);

    back_init("back/gui.png");

    if (package_back == 0)
        package_back = prev;

    if (do_init || package_manual_hotreload)
    {
        if (package_manual_hotreload)
            first = 0;

        total = package_count();
        first = MIN(first, (total - 1) - ((total - 1) % PACKAGE_STEP));

        audio_music_fade_to(0.5f, "bgm/inter.ogg", 1);

        package_manual_hotreload = 0;
    }
    else do_init = 1;

    selected = first;

    if (status_ids)
    {
        free(status_ids);
        status_ids = NULL;
    }

    status_ids = calloc(total, sizeof (*status_ids));

    if (name_ids)
    {
        free(name_ids);
        name_ids = NULL;
    }

    name_ids = calloc(total, sizeof (*name_ids));

    fetch_package_images();

    if (prev == &st_package)
        return transition_page(package_gui(), 1, intent);

    return transition_slide(package_gui(), 1, intent);
}

static int package_leave(struct state *st, struct state *next, int id, int intent)
{
    if (status_ids)
    {
        free(status_ids);
        status_ids = NULL;
    }

    if (name_ids)
    {
        free(name_ids);
        name_ids = NULL;
    }

    if (next == &st_package)
        return transition_page(id, 0, intent);

    return transition_slide(id, 0, intent);
}

static void package_paint(int id, float st)
{
    video_set_perspective((float) config_get_d(CONFIG_VIEW_FOV), 0.1f, FAR_DIST);
    back_draw_easy();

    gui_paint(id);
}

static void package_timer(int id, float dt)
{
    if (do_download &&
        package_manage_selected >= 0 &&
        time_state() > 0.5f)
        package_start_download(package_manage_selected);

    gui_timer(id, dt);
}

static void package_point(int id, int x, int y, int dx, int dy)
{
    int jd = gui_point(id, x, y);

    if (jd)
        gui_pulse(jd, 1.2f);
}

static void package_stick(int id, int a, float v, int bump)
{
    int jd = gui_stick(id, a, v, bump);

    if (id)
        gui_pulse(jd, 1.2f);
}

static int package_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT)
            return package_action(GUI_BACK, 0);
        else if (c == KEY_LOOKAROUND)
        {
            package_manual_hotreload = 1;
            return goto_state(&st_package);
        }
    }

    return 1;
}

/*---------------------------------------------------------------------------*/

/*
 * HACK: This is how Mojang UI's file manager works.
 * - Ersohn Styne
 */

enum
{
    PACKAGE_MANAGE_START = GUI_LAST,
    PACKAGE_MANAGE_EQUIP,
    PACKAGE_MANAGE_UPDATE
};

enum package_confirm_action
{
    PACKAGE_CONFIRM_NONE = 0,

    PACKAGE_CONFIRM_RENAME,

    PACKAGE_CONFIRM_MAX
};

static int manage_del_confirm_btn_enabled = 0;
static int manage_del_confirm_btn_id      = 0;

static enum package_confirm_action curr_confirm_action = PACKAGE_CONFIRM_NONE;

static int package_check_purchased_extralevels(const char *set_id)
{
    int curr_date_month = 0;

    time_t     curr_date = time(NULL);
    struct tm *curr_date_localtime = localtime(&curr_date);

    if (curr_date_localtime)
        curr_date_month = curr_date_localtime->tm_mon + 1;

    /* HACK: Requires prefix filename "set-"! */

    if (strcmp(package_get_type(selected), "set") == 0)
        return 0;

#if NB_HAVE_PB_BOTH==1
#ifdef CONFIG_INCLUDES_ACCOUNT
    /* Add more level sets when products bought. */

    if ((str_starts_with(set_id, "set-easy")   &&
         str_starts_with(set_id, "set-medium") &&
         str_starts_with(set_id, "set-hard")   &&
         str_starts_with(set_id, "set-mym")    &&
         str_starts_with(set_id, "set-mym2")   &&
         str_starts_with(set_id, "set-fwp")    &&
         str_starts_with(set_id, "set-tones")  &&
         str_starts_with(set_id, "set-misc")) &&
        (!account_get_d(ACCOUNT_PRODUCT_LEVELS) &&
         !server_policy_get_d(SERVER_POLICY_LEVELSET_ENABLED_CUSTOMSET)))
        return 0;
#else
    if (str_starts_with(set_id, "set-easy")   &&
        str_starts_with(set_id, "set-medium") &&
        str_starts_with(set_id, "set-hard")   &&
        str_starts_with(set_id, "set-mym")    &&
        str_starts_with(set_id, "set-mym2")   &&
        str_starts_with(set_id, "set-fwp")    &&
        str_starts_with(set_id, "set-tones")  &&
        str_starts_with(set_id, "set-misc")   &&
        !server_policy_get_d(SERVER_POLICY_LEVELSET_ENABLED_CUSTOMSET))
        return 0;
#endif

    /* Limited offered region only */

    if (str_starts_with(set_id, "set-anime") &&
        !config_cheat())
    {
        if (server_policy_get_d(SERVER_POLICY_EDITION) < 3)
            return 0;

        if (config_get_s(CONFIG_LANGUAGE) &&
            (strcmp(config_get_s(CONFIG_LANGUAGE), "ja") != 0 &&
             strcmp(config_get_s(CONFIG_LANGUAGE), "jp") != 0))
        {
#ifdef __EMSCRIPTEN__
            if (EM_ASM_INT({ return Neverball.gamecore_geolocation_checkisjapan() || navigator.language.startsWith("ja") || navigator.language.startsWith("jp") ? 0 : 1; }))
#endif
                return 0;
        }
#ifdef __EMSCRIPTEN__
        else if (EM_ASM_INT({ return Neverball.gamecore_geolocation_checkisjapan() || navigator.language.startsWith("ja") || navigator.language.startsWith("jp") ? 0 : 1; }))
            return 0;
#endif
    }

    /* Limited special offers only */

    if (str_starts_with(set_id, "set-valentine") &&
        curr_date_month != 2 &&
        !config_cheat())
        return 0;

    if (str_starts_with(set_id, "set-freeland") &&
        curr_date_month != 5 &&
        !config_cheat())
        return 0;

    if (str_starts_with(set_id, "set-halloween") &&
        curr_date_month != 10 &&
        !config_cheat())
        return 0;

    if (str_starts_with(set_id, "set-christmas") &&
        curr_date_month != 12 &&
        !config_cheat())
        return 0;
#endif

    return 1;
}

static int package_check_purchased_online_balls(void)
{
#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT)
    return account_get_d(ACCOUNT_PRODUCT_BALLS) == 1;
#else
    return 0;
#endif
}

static int package_manage_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    const char *package_id = package_get_id(package_manage_selected);

    char equip_ball_path[MAXSTR];
    sprintf(equip_ball_path, "ball/%s/%s", package_id ? package_id + 5 : "(null)", package_id ? package_id + 5 : "(null)");

    switch (curr_confirm_action)
    {
        default:
        {
            switch (tok)
            {
                case PACKAGE_MANAGE_START:
                    if (package_manage_can_start)
                        return installed_action ? installed_action(package_manage_selected) : 1;

                case PACKAGE_MANAGE_EQUIP:
                    if (package_manage_can_equip && package_id)
                    {
#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT)
                        account_set_s(ACCOUNT_BALL_FILE, equip_ball_path);
#else
                        config_set_s(BALL_FILE, equip_ball_path);
#endif
                        ball_free();
                        ball_init();
                        return exit_state(&st_package);
                    }

                case PACKAGE_MANAGE_UPDATE:
                    selected = package_manage_selected;
                    do_download = 1;
                    return exit_state(&st_package);

                case GUI_BACK:
                    package_manage_selected = -1;
                    return exit_state(&st_package);
            }
        }
        break;
    }

    return 1;
}

static int package_manage_delete_gui(void)
{
    manage_del_confirm_btn_enabled = 0;

    audio_play("snd/warning.ogg", 1.0f);

    int id, jd;

    if ((id = gui_vstack(0)))
    {
        char desc[MAXSTR];
        sprintf(desc, _("%s\nSeriously, this action cannot be undone!"),
                      package_get_name(package_manage_selected));

        gui_title_header(id, _("Delete addon?"), GUI_MED, gui_red, gui_red);

        gui_space(id);

        gui_multi(id, desc, GUI_SML, gui_wht, gui_wht);

        gui_space(id);

        if ((jd = gui_harray(id)))
        {
            gui_start(jd, _("Cancel"), GUI_SML, GUI_BACK, 0);
            manage_del_confirm_btn_id = gui_state(jd, _("Delete"),
                                                      GUI_SML, GUI_NONE, 0);

            if (manage_del_confirm_btn_id)
                gui_set_color(manage_del_confirm_btn_id, GUI_COLOR_GRY);
        }

        gui_layout(id, 0, 0);
    }

    return id;
}

static int package_manage_gui(void)
{
    int id, btn_id, jd;

    if ((id = gui_vstack(0)))
    {
        const char *package_id   = package_get_id(package_manage_selected);
        const char *package_name = package_get_name(package_manage_selected);

        char desc[MAXSTR];
        sprintf(desc, _("Installed addon ID / Name:\n%s / %s"),
                         package_id   ? package_id   : "(null)",
                         package_name ? package_name : "Unknown addon");

        gui_multi(id, desc, GUI_SML, gui_wht, gui_wht);

        gui_space(id);

        if (package_manage_can_start)
        {
            if ((btn_id = gui_vstack(id)))
            {
                if ((jd = gui_hstack(btn_id)))
                {
                    gui_filler(jd);
                    const int startbtn_id = gui_label(jd, GUI_TRIANGLE_RIGHT, GUI_SML, GUI_COLOR_GRN);
                    gui_label(jd, _("Start"), GUI_SML, GUI_COLOR_DEFAULT);
                    gui_filler(jd);

                    gui_set_font(startbtn_id, "ttf/DejaVuSans-Bold.ttf");
                }

                gui_multi(btn_id, _("Start this current Level Set"),
                                  GUI_SML, GUI_COLOR_WHT);

                gui_set_state(btn_id, PACKAGE_MANAGE_START, package_manage_selected);
                gui_set_rect(btn_id, GUI_ALL);
            }

            gui_space(id);
        }

        if (package_manage_can_equip)
        {
            if ((btn_id = gui_vstack(id)))
            {
                char expected_path[MAXSTR];
                sprintf(expected_path, "ball/%s/%s", package_id ? package_id + 5 : "(null)", package_id ? package_id + 5 : "(null)");

                const int equip_available = 1;
#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT)
                const int equip_same = package_id && strcmp(account_get_s(ACCOUNT_BALL_FILE), expected_path) == 0;
#else
                const int equip_same = package_id && strcmp(config_get_s(CONFIG_BALL_FILE), expected_path) == 0;
#endif

                int equip_btn_id = 0;

                if ((jd = gui_hstack(btn_id)))
                {
                    gui_filler(jd);
                    equip_btn_id = gui_label(jd, _("Equip"), GUI_SML, GUI_COLOR_DEFAULT);
                    gui_filler(jd);
                }

                const int equip_desc_id = gui_multi(btn_id, _("Equip this ball model"),
                                                            GUI_SML, GUI_COLOR_WHT);

                if (!equip_available)
                {
                    gui_set_color(equip_btn_id, gui_red, gui_blk);
                    gui_set_multi(equip_desc_id, _("Addon is not available"));
                }
                else if (equip_same)
                {
                    gui_set_color(equip_btn_id, gui_red, gui_blk);
                    gui_set_multi(equip_desc_id, _("Already equipped"));
                }

                gui_set_state(btn_id, equip_available && !equip_same ? PACKAGE_MANAGE_EQUIP : GUI_NONE, package_manage_selected);
                gui_set_rect(btn_id, GUI_ALL);
            }

            gui_space(id);
        }

        if ((btn_id = gui_vstack(id)))
        {
            if ((jd = gui_hstack(btn_id)))
            {
                gui_filler(jd);
                const int updatebtn_id = gui_label(jd, GUI_CIRCLE_ARROW, GUI_SML, GUI_COLOR_GRN);
                gui_label(jd, _("Update"), GUI_SML, GUI_COLOR_DEFAULT);
                gui_filler(jd);

                gui_set_font(updatebtn_id, "ttf/DejaVuSans-Bold.ttf");
            }

            gui_multi(btn_id, _("Update the new package"),
                              GUI_SML, GUI_COLOR_WHT);

            gui_set_state(btn_id, PACKAGE_MANAGE_UPDATE, package_manage_selected);
            gui_set_rect(btn_id, GUI_ALL);
        }

        gui_space(id);

        gui_start(id, _("Back"), GUI_SML, GUI_BACK, 0);

        gui_layout(id, 0, 0);
    }

    return id;
}

static int package_manage_enter(struct state *st, struct state *prev, int intent)
{
    common_init(package_manage_action);

    return transition_slide(package_manage_gui(), 1, intent);
}

static int package_manage_leave(struct state *st, struct state *next, int id, int intent)
{
    manage_del_confirm_btn_enabled = 0;

    return transition_slide(id, 0, intent);
}

static void package_manage_timer(int id, float dt)
{
    gui_timer(id, dt);
}

/*---------------------------------------------------------------------------*/

void goto_package(int package_id, struct state *back_state)
{
    /* Initialize the state. */

    if (package_back == 0)
        package_back = back_state;

    //goto_state(&st_package);

    /* Navigate to the page. */

    first = (package_id / PACKAGE_STEP) * PACKAGE_STEP;
    do_init = 1;
    goto_state(&st_package);

    /* Finally, select the package. */

    package_select(package_id);
}

void package_set_installed_action(int (*installed_action_fn)(int pi))
{
    installed_action = installed_action_fn;
}

/*---------------------------------------------------------------------------*/

struct state st_package = {
    package_enter,
    package_leave,
    package_paint,
    package_timer,
    package_point,
    package_stick,
    NULL,
    common_click,
    package_keybd,
    common_buttn
};

struct state st_package_manage = {
    package_manage_enter,
    package_manage_leave,
    package_paint,
    package_manage_timer,
    package_point,
    package_stick,
    NULL,
    common_click,
    common_keybd,
    common_buttn
};
