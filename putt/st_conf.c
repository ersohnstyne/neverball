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
 * HACK: Used with console version
 */
#include "console_control_gui.h"

#include "gui.h"
#include "transition.h"
#include "hud.h"
#include "geom.h"
#include "ball.h"
#include "part.h"
#include "game.h"
#include "audio.h"
#include "config.h"
#include "video.h"
#ifndef VERSION
#include "version.h"
#endif
#include "lang.h"

#include "st_conf.h"
#include "st_all.h"
#include "st_common.h"

/*---------------------------------------------------------------------------*/

enum
{
    CONF_VIDEO = 1,
    CONF_LANG,
    CONF_PLAYER,
    CONF_BACK
};

static int changed;

static int master_id[11];

static int music_id[11];
static int sound_id[11];
static int narrator_id[11];

static int is_value_changed(void)
{
    return changed;
}

static void set_changed_value(void)
{
    config_save();
    changed = 0;
}

static void init_changed_value(void)
{
    changed = 0;
}

static int conf_action(int i)
{
    int master   = config_get_d(CONFIG_MASTER_VOLUME);
    int snd      = config_get_d(CONFIG_SOUND_VOLUME);
    int mus      = config_get_d(CONFIG_MUSIC_VOLUME);
    int narrator = config_get_d(CONFIG_NARRATOR_VOLUME);
    int r = 1;

    audio_play(CONF_BACK == i ? AUD_BACK : AUD_MENU, 1.0f);

    switch (i)
    {
        case CONF_BACK:
            exit_state(&st_title);
            break;

        case CONF_VIDEO:
            goto_video(&st_conf);
            break;

        case CONF_LANG:
            goto_state(&st_lang);
            break;

        /*case CONF_PLAYER:
            goto_name(&st_conf, &st_conf, 1);
            break;*/

        default:
            if (100 <= i && i <= 110)
            {
                int n = i - 100;

                config_set_d(CONFIG_MASTER_VOLUME, n);
                audio_volume(n, snd, mus, narrator);

                gui_toggle(master_id[n]);
                gui_toggle(master_id[master]);

                set_changed_value();
            }
            if (200 <= i && i <= 210)
            {
                int n = i - 200;

                config_set_d(CONFIG_MUSIC_VOLUME, n);
                audio_volume(master, snd, n, narrator);

                gui_toggle(music_id[n]);
                gui_toggle(music_id[mus]);

                set_changed_value();
            }
            if (300 <= i && i <= 310)
            {
                int n = i - 300;

                config_set_d(CONFIG_SOUND_VOLUME, n);
                audio_volume(master, n, mus, narrator);
                audio_play(AUD_NEXTTURN, 1.f);

                gui_toggle(sound_id[n]);
                gui_toggle(sound_id[snd]);

                set_changed_value();
            }
            if (400 <= i && i <= 410)
            {
                int n = i - 400;

                config_set_d(CONFIG_NARRATOR_VOLUME, n);
                audio_volume(master, snd, mus, n);

                gui_toggle(narrator_id[n]);
                gui_toggle(narrator_id[narrator]);

                set_changed_value();
            }
    }

    return r;
}

static int conf_enter(struct state *st, struct state *prev, int intent)
{
    if (prev == &st_title)
        init_changed_value();

    int root_id;

    back_init("back/gui.png");

    /* Initialize the configuration GUI. */

    if ((root_id = gui_root()))
    {
        int id, jd, kd;
        int i;

        if ((id = gui_vstack(root_id)))
        {
            gui_space(id);

            if ((jd = gui_harray(id)))
            {
                gui_label(jd, _("Options"), GUI_SML, 0, 0);
                gui_space(jd);
                gui_start(jd, _("Back"), GUI_SML, CONF_BACK, 0);
            }

            gui_space(id);

            if ((jd = gui_harray(id)) &&
                (kd = gui_harray(jd)))
            {
                gui_state(kd, _("Configure"), GUI_SML, CONF_VIDEO, 0);

                gui_label(jd, _("Graphics"), GUI_SML, 0, 0);
            }

            gui_space(id);

            if ((jd = gui_harray(id)) &&
                (kd = gui_harray(jd)))
            {
                /* A series of empty buttons forms the sound volume control. */

                int ms = config_get_d(CONFIG_MASTER_VOLUME);

                for (i = 10; i >= 0; i--)
                {
                    master_id[i] = gui_state(kd, NULL, GUI_SML, 100 + i, 0);
                    gui_set_hilite(master_id[i], (ms == i));
                }

                gui_label(jd, _("Master Volume"), GUI_SML, 0, 0);
            }

            gui_space(id);

            if ((jd = gui_harray(id)) &&
                (kd = gui_harray(jd)))
            {
                /* A series of empty buttons forms the music volume control. */

                int mus = config_get_d(CONFIG_MUSIC_VOLUME);

                for (i = 10; i >= 0; i--)
                {
                    music_id[i] = gui_state(kd, NULL, GUI_SML, 200 + i, 0);
                    gui_set_hilite(music_id[i], (mus == i));
                }

                gui_label(jd, _("Music Volume"), GUI_SML, 0, 0);
            }

            if ((jd = gui_harray(id)) &&
                (kd = gui_harray(jd)))
            {
                /* A series of empty buttons forms the sound volume control. */

                int snd = config_get_d(CONFIG_SOUND_VOLUME);

                for (i = 10; i >= 0; i--)
                {
                    sound_id[i] = gui_state(kd, NULL, GUI_SML, 300 + i, 0);
                    gui_set_hilite(sound_id[i], (snd == i));
                }

                gui_label(jd, _("Sound Volume"), GUI_SML, 0, 0);
            }

            if ((jd = gui_harray(id)) &&
                (kd = gui_harray(jd)))
            {
                /* A series of empty buttons forms the sound volume control. */

                int narrator = config_get_d(CONFIG_NARRATOR_VOLUME);

                for (i = 10; i >= 0; i--)
                {
                    narrator_id[i] = gui_state(kd, NULL, GUI_SML, 400 + i, 0);
                    gui_set_hilite(narrator_id[i], (narrator == i));
                }

                gui_label(jd, _("Narrator Volume"), GUI_SML, 0, 0);
            }

            gui_space(id);

            if ((jd = gui_harray(id)) &&
                (kd = gui_harray(jd)))
            {
#if ENABLE_NLS==1 || _WIN32
                int lang_id = gui_state(kd, "                            ", GUI_SML, CONF_LANG, 0);
                gui_label(jd, _("Language"), GUI_SML, 0, 0);

                gui_set_trunc(lang_id, TRUNC_TAIL);

                if (*config_get_s(CONFIG_LANGUAGE))
                    gui_set_label(lang_id, lang_name(&curr_lang));
                else
                    gui_set_label(lang_id, _("Default"));
#endif
            }

            gui_layout(id, 0, +1);
        }

        if ((id = gui_vstack(root_id)))
        {
            gui_label(id, "Neverputt " VERSION " (High)", GUI_TNY, GUI_COLOR_WHT);
            gui_multi(id, _("Copyright © 2025 Neverball authors\n"
                            "Neverball is free software available under the terms of GPL v2 or later."),
                          GUI_TNY, GUI_COLOR_WHT);
            gui_clr_rect(id);
            gui_layout(id, 0, -1);
        }
    }

#if NB_HAVE_PB_BOTH==1
    audio_music_fade_to(0.5f, _("bgm/title.ogg"), 1);
#else
    audio_music_fade_to(0.5f, "gui/bgm/inter.ogg", 1);
#endif

    return transition_slide(root_id, 1, intent);
}

static int conf_leave(struct state *st, struct state *next, int id, int intent)
{
    back_free();
    return transition_slide(id, 0, intent);
}

static void conf_paint(int id, float st)
{
    video_set_perspective((float) config_get_d(CONFIG_VIEW_FOV), 0.1f, FAR_DIST);
    back_draw_easy();

    gui_paint(id);
}

static void conf_timer(int id, float dt)
{
    gui_timer(id, dt);
}

static void conf_point(int id, int x, int y, int dx, int dy)
{
    gui_pulse(gui_point(id, x, y), 1.2f);
}

static void conf_stick(int id, int a, float v, int bump)
{
    gui_pulse(gui_stick(id, a, v, bump), 1.2f);
}

static int conf_click(int b, int d)
{
    if (config_tst_d(CONFIG_MOUSE_CANCEL_MENU, b))
        return conf_action(CONF_BACK);

    if (gui_click(b, d))
        return conf_action(gui_token(gui_active()));

    return 1;
}

static int conf_keybd(int c, int d)
{
    return (d && c == SDLK_ESCAPE) ? conf_action(CONF_BACK) : 1;
}

static int conf_buttn(int b, int d)
{
    if (d)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return conf_action(gui_token(gui_active()));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return conf_action(CONF_BACK);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int null_enter(struct state *st, struct state *prev, int intent)
{
#if ENABLE_MOTIONBLUR!=0
    video_motionblur_quit();
#endif

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    xbox_control_gui_free();
#endif
    transition_quit();
    gui_free();
    geom_free();
    ball_free();
    shad_free();
    mtrl_free_objects();

    return 0;
}

static int null_leave(struct state *st, struct state *next, int id, int intent)
{
    mtrl_load_objects();
    shad_init();
    ball_init();
    geom_init();
    gui_init();
    transition_init();
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    xbox_control_gui_init();
#endif
#if ENABLE_MOTIONBLUR!=0
    video_motionblur_init();
#endif
    return 0;
}

/*---------------------------------------------------------------------------*/

struct state st_conf = {
    conf_enter,
    conf_leave,
    conf_paint,
    conf_timer,
    conf_point,
    conf_stick,
    NULL,
    conf_click,
    conf_keybd,
    conf_buttn
};

struct state st_null = {
    null_enter,
    null_leave
};
