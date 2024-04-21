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

#include "gui.h"
#include "audio.h"
#if NB_HAVE_PB_BOTH==1
#include "account.h"
#endif
#include "config.h"
#include "common.h"
#include "package.h"
#include "geom.h"

#include "st_package.h"
#include "st_common.h"

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
                           AUD_DISABLED : AUD_MENU, 1.0f);   \
            if (tok == tok2)                                 \
                audio_play(first + itemstep < total ?        \
                           AUD_DISABLED : AUD_MENU, 1.0f);   \
        } else GENERIC_GAMEMENU_ACTION

/*---------------------------------------------------------------------------*/

struct state st_package;
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

/*---------------------------------------------------------------------------*/

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
                if (dn->finished)
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
                    gui_set_label(id, "!");
                    gui_set_color(id, gui_red, gui_red);
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
        if (fd && fd->finished)
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

static void package_start_download(int id)
{
    do_download = 0;

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

    /*
     * Issue #352: This gamepad features has been fixed, which it has
     * found on out of bounds bug, so let Mojang doing this.
     */

    GAMEPAD_GAMEMENU_ACTION_SCROLL(GUI_PREV, GUI_NEXT, PACKAGE_STEP);

    switch (tok)
    {
        case GUI_BACK:
            return goto_state(package_back);
            break;

        case GUI_PREV:
            first = MAX(first - PACKAGE_STEP, 0);

            do_init = 0;
            return goto_state(&st_package);

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
                return goto_state(&st_package_manage);
            else if (status == PACKAGE_AVAILABLE ||
                     status == PACKAGE_ERROR)
            {
                package_start_download(selected);

                return 1;
            }
            else if (status == PACKAGE_DOWNLOADING)
                return 1;
            break;

        case PACKAGE_CHANGEGROUP:
#if NB_HAVE_PB_BOTH==1
            category_id = val;
#endif
            break;
    }

    return 1;
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

        gui_set_font(status_id, GUI_FACE);

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
    int id, jd, kd;

    int i;

    if (total <= 0)
    {
        if ((id = gui_vstack(0)))
        {
            gui_label (id, _("No addons found"), GUI_SML, 0, 0);
#if defined(CONFIG_INCLUDES_ACCOUNT) && ENABLE_FETCH
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

        if ((jd = gui_harray(id)))
        {
            const int ww = MIN(w, h) * 5 / 12;
            const int hh = ww / 4 * 3;

            if ((kd = gui_varray(jd)))
            {
                for (i = first; i < first + PACKAGE_STEP; i++)
                {
                    int button_id = gui_package(kd, i);

                    button_ids[i % PACKAGE_STEP] = button_id;
                }

                gui_set_fill(kd);
            }

            shot_id = gui_image(jd, package_get_shot_filename(first), ww, hh);
        }

        gui_space(id);

        if ((jd = gui_vstack(id)))
        {
            title_id = gui_label(jd, package_get_name(first),
                                     GUI_SML, gui_yel, gui_wht);

            gui_space(jd);

            desc_id = gui_multi(jd, " \n \n \n \n \n", GUI_SML, gui_yel, gui_wht);

            gui_set_rect(jd, GUI_ALL);
        }

        gui_space(id);

        if ((jd = gui_hstack(id)))
        {
            if ((kd = gui_hstack(jd)))
            {
                install_status_id = gui_label(kd, GUI_ARROW_DN,
                                                  GUI_SML, gui_grn, gui_grn);
                install_label_id  = gui_label(kd, _("Install"),
                                                  GUI_SML, gui_wht, gui_wht);

                gui_set_font(install_status_id, GUI_FACE);

                gui_set_rect(kd, GUI_ALL);
                gui_set_state(kd, PACKAGE_INSTALL, 0);

                install_id = kd;
            }

            gui_filler(jd);

            type_id = gui_label(jd, "XXXXXXX", GUI_SML, GUI_COLOR_DEFAULT);
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

    if (status == PACKAGE_UPDATE)
        gui_set_label(install_label_id, _("Update"));
    else
        gui_set_label(install_label_id, _("Install"));

    if (package_get_status(pi) == PACKAGE_INSTALLED)
    {
        /* HACK: Mojang made done this. */

        //gui_set_color(install_status_id, gui_gry, gui_gry);
        //gui_set_color(install_label_id, gui_gry, gui_gry);

        gui_set_color(install_status_id, gui_wht, gui_wht);
        gui_set_color(install_label_id,  gui_wht, gui_wht);
    }
    else
    {
        gui_set_color(install_status_id, gui_grn, gui_grn);
        gui_set_color(install_label_id,  gui_wht, gui_wht);
    }
}

static int package_enter(struct state *st, struct state *prev)
{
    common_init(package_action);

    back_init("back/gui.png");

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

    return package_gui();
}

static void package_leave(struct state *st, struct state *next, int id)
{
    gui_delete(id);

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
}

static void package_paint(int id, float st)
{
    video_set_perspective((float) config_get_d(CONFIG_VIEW_FOV), 0.1f, FAR_DIST);
    back_draw_easy();

    gui_paint(id);
}

static void package_timer(int id, float dt)
{
    if (do_download && time_state() > 0.5f)
        package_start_download(selected);

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
    PACKAGE_MANAGE_EXPLORER = GUI_LAST,
    PACKAGE_MANAGE_UPDATE,
    PACKAGE_MANAGE_DELETE
};

enum package_confirm_action
{
    PACKAGE_CONFIRM_NONE = 0,

    PACKAGE_CONFIRM_DELETE,
    PACKAGE_CONFIRM_RENAME,

    PACKAGE_CONFIRM_MAX
};

static enum package_confirm_action curr_confirm_action = PACKAGE_CONFIRM_NONE;

static int package_manage_action(int tok, int val)
{
    switch (curr_confirm_action)
    {
        case PACKAGE_CONFIRM_DELETE:
        {
            switch (tok)
            {
                case PACKAGE_MANAGE_DELETE:
                    curr_confirm_action = PACKAGE_CONFIRM_NONE;
                    fs_remove(package_get_files(selected));
                    return goto_state(&st_package);

                case GUI_BACK:
                    curr_confirm_action = PACKAGE_CONFIRM_NONE;
                    return goto_state(&st_package);
            }
        }
        break;

        default:
        {
            switch (tok)
            {
                case PACKAGE_MANAGE_UPDATE:
                    selected = val;
                    do_download = 1;
                    return goto_state(&st_package);

                case PACKAGE_MANAGE_DELETE:
                    curr_confirm_action = PACKAGE_CONFIRM_DELETE;
                    return goto_state(&st_package_manage);

                case GUI_BACK:
                    return goto_state(&st_package);
            }
        }
        break;
    }

    return 1;
}

static int package_manage_delete_gui(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        char desc[MAXSTR];
        sprintf(desc, _("Seriously, this action cannot be undone!:\n%s"),
                      package_get_name(selected));

        gui_title_header(id, _("Delete package?"), GUI_MED, gui_red, gui_red);

        gui_space(id);

        gui_multi(id, desc, GUI_SML, gui_wht, gui_wht);

        gui_space(id);

        if ((jd = gui_harray(id)))
        {
            gui_state(jd, _("Cancel"), GUI_SML, GUI_BACK, 0);
            gui_start(jd, _("Delete"), GUI_SML, PACKAGE_MANAGE_DELETE, selected);
        }
    }

    return id;
}

static int package_manage_gui(void)
{
    int id, btn_id;

    if ((id = gui_vstack(0)))
    {
        char desc[MAXSTR];
        sprintf(desc, _("Installed package path:\n%s"),
                         package_get_files(selected));

        gui_multi(id, desc, GUI_SML, gui_wht, gui_wht);

        gui_space(id);

        if ((btn_id = gui_state(id, _("Update"),
                                    GUI_SML, PACKAGE_MANAGE_UPDATE, selected)))
            gui_set_color(btn_id, gui_wht, gui_wht);
        if ((btn_id = gui_state(id, _("Delete"),
                                    GUI_SML, PACKAGE_MANAGE_DELETE, selected)))
            gui_set_color(btn_id, gui_red, gui_red);

        gui_space(id);

        gui_back_button(id);
    }

    return id;
}

static int package_manage_enter(struct state *st, struct state *prev)
{
    common_init(package_manage_action);

    return curr_confirm_action == PACKAGE_CONFIRM_DELETE ?
           package_manage_delete_gui() : package_manage_gui();
}

static void package_manage_leave(struct state *st, struct state *next, int id)
{
    gui_delete(id);
}

/*---------------------------------------------------------------------------*/

void goto_package(int package_id, struct state *back_state)
{
    /* Initialize the state. */

    goto_state(&st_package);

    package_back = back_state;

    /* Navigate to the page. */

    first = (package_id / PACKAGE_STEP) * PACKAGE_STEP;
    do_init = 0;
    goto_state(&st_package);

    /* Finally, select the package. */

    package_select(package_id);
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
    common_timer,
    package_point,
    package_stick,
    NULL,
    common_click,
    common_keybd,
    common_buttn
};
