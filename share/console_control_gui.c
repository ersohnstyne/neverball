/*
 * Copyright (C) 2026 Microsoft / Neverball authors / Jānis Rūcis
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

#include "console_control_gui.h"

#if NB_HAVE_PB_BOTH==1
#include "networking.h"
#endif

#include "gui.h"
#include "audio.h"
#include "config.h"
#include "video.h"
#include "cmd.h"
#include "lang.h"
#include "log.h"

#if !defined(DISABLE_ONLINE_DATA)
#include "account.h"
#endif

#include "joy.h"

/*---------------------------------------------------------------------------*/

#define GAMEPAD_MAX_BTN_GUI_IDS 64

enum console_platforms current_platform;
static int show_control_gui;

static int gamepad_btn_universal;

/*
 * HACK: CONTROLE SCHEME LAYOUTS:
 *
 * * XBOX: gui/controllers/btn_gamepad_%BTN_NAME%_%CONTROLLER_NAME%_xbox.png
 * * PLAYSTATION: gui/controllers/btn_gamepad_%BTN_NAME%_%CONTROLLER_NAME%_playstation.png
 * * NINTENDO SWITCH: gui/controllers/btn_gamepad_%BTN_NAME%_%CONTROLLER_NAME%_switch.png
 * * UNIVERSAL: gui/controllers/btn_gamepad_%BTN_NAME%_%CONTROLLER_NAME%_switch_universal.png
 */
struct gamepad_btn_gui_id {
    int gui_id;
    int btn_id;
};

static struct gamepad_btn_gui_id gamepad_btn_gui_ids_a[GAMEPAD_MAX_BTN_GUI_IDS];
static struct gamepad_btn_gui_id gamepad_btn_gui_ids_b[GAMEPAD_MAX_BTN_GUI_IDS];
static struct gamepad_btn_gui_id gamepad_btn_gui_ids_x[GAMEPAD_MAX_BTN_GUI_IDS];
static struct gamepad_btn_gui_id gamepad_btn_gui_ids_y[GAMEPAD_MAX_BTN_GUI_IDS];
static struct gamepad_btn_gui_id gamepad_btn_gui_ids_lb[GAMEPAD_MAX_BTN_GUI_IDS];
static struct gamepad_btn_gui_id gamepad_btn_gui_ids_rb[GAMEPAD_MAX_BTN_GUI_IDS];
static struct gamepad_btn_gui_id gamepad_btn_gui_ids_lt[GAMEPAD_MAX_BTN_GUI_IDS];
static struct gamepad_btn_gui_id gamepad_btn_gui_ids_rt[GAMEPAD_MAX_BTN_GUI_IDS];
static struct gamepad_btn_gui_id gamepad_btn_gui_ids_ls[GAMEPAD_MAX_BTN_GUI_IDS];
static struct gamepad_btn_gui_id gamepad_btn_gui_ids_rs[GAMEPAD_MAX_BTN_GUI_IDS];
static struct gamepad_btn_gui_id gamepad_btn_gui_ids_start[GAMEPAD_MAX_BTN_GUI_IDS];
static struct gamepad_btn_gui_id gamepad_btn_gui_ids_select[GAMEPAD_MAX_BTN_GUI_IDS];
static struct gamepad_btn_gui_id gamepad_btn_gui_ids_dpads[4][GAMEPAD_MAX_BTN_GUI_IDS];

/* Shared */
static int xbox_control_title_id               = 0;
static int xbox_control_keybd_id               = 0;
static int xbox_control_list_id                = 0;
static int xbox_control_levelopt_id            = 0;
static int xbox_control_paused_id              = 0;
static int xbox_control_package_installable_id = 0;
static int xbox_control_package_updateable_id  = 0;
static int xbox_control_package_manageable_id  = 0;
static int xbox_control_package_equipable_id   = 0;
static int xbox_control_package_startable_id   = 0;

/* Generic */
static int xbox_control_desc_id          = 0;
static int xbox_control_preparation_id   = 0;
static int xbox_control_replay_id        = 0;
static int xbox_control_replay_eof_id    = 0;
static int xbox_control_shop_id          = 0;
static int xbox_control_shop_getcoins_id = 0;
static int xbox_control_model_id         = 0;
static int xbox_control_beam_style_id    = 0;
static int xbox_control_death_id         = 0;

/* Putt */
static int xbox_control_putt_stroke_id = 0;
static int xbox_control_putt_stop_id   = 0;
static int xbox_control_putt_scores_id = 0;

/*---------------------------------------------------------------------------*/

void console_init_controller_type(const enum console_platforms new_platforms)
{
    current_platform = new_platforms;

    for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS; i++)
        for (int j = 0; j < 4; j++) {
            memset(&gamepad_btn_gui_ids_a[i],        0, sizeof (gamepad_btn_gui_ids_a[i]));
            memset(&gamepad_btn_gui_ids_b[i],        0, sizeof (gamepad_btn_gui_ids_b[i]));
            memset(&gamepad_btn_gui_ids_x[i],        0, sizeof (gamepad_btn_gui_ids_x[i]));
            memset(&gamepad_btn_gui_ids_y[i],        0, sizeof (gamepad_btn_gui_ids_y[i]));
            memset(&gamepad_btn_gui_ids_lb[i],       0, sizeof (gamepad_btn_gui_ids_lb[i]));
            memset(&gamepad_btn_gui_ids_rb[i],       0, sizeof (gamepad_btn_gui_ids_rb[i]));
            memset(&gamepad_btn_gui_ids_lt[i],       0, sizeof (gamepad_btn_gui_ids_lt[i]));
            memset(&gamepad_btn_gui_ids_rt[i],       0, sizeof (gamepad_btn_gui_ids_rt[i]));
            memset(&gamepad_btn_gui_ids_ls[i],       0, sizeof (gamepad_btn_gui_ids_ls[i]));
            memset(&gamepad_btn_gui_ids_rs[i],       0, sizeof (gamepad_btn_gui_ids_rs[i]));
            memset(&gamepad_btn_gui_ids_dpads[j][i], 0, sizeof (gamepad_btn_gui_ids_dpads[j][i]));

            gamepad_btn_gui_ids_a[i].btn_id        = -1;
            gamepad_btn_gui_ids_b[i].btn_id        = -1;
            gamepad_btn_gui_ids_x[i].btn_id        = -1;
            gamepad_btn_gui_ids_y[i].btn_id        = -1;
            gamepad_btn_gui_ids_lb[i].btn_id       = -1;
            gamepad_btn_gui_ids_rb[i].btn_id       = -1;
            gamepad_btn_gui_ids_lt[i].btn_id       = -1;
            gamepad_btn_gui_ids_rt[i].btn_id       = -1;
            gamepad_btn_gui_ids_ls[i].btn_id       = -1;
            gamepad_btn_gui_ids_rs[i].btn_id       = -1;
            gamepad_btn_gui_ids_dpads[j][i].btn_id = -1;

            gamepad_btn_gui_ids_a[i].gui_id        = -1;
            gamepad_btn_gui_ids_b[i].gui_id        = -1;
            gamepad_btn_gui_ids_x[i].gui_id        = -1;
            gamepad_btn_gui_ids_y[i].gui_id        = -1;
            gamepad_btn_gui_ids_lb[i].gui_id       = -1;
            gamepad_btn_gui_ids_rb[i].gui_id       = -1;
            gamepad_btn_gui_ids_lt[i].gui_id       = -1;
            gamepad_btn_gui_ids_rt[i].gui_id       = -1;
            gamepad_btn_gui_ids_ls[i].gui_id       = -1;
            gamepad_btn_gui_ids_rs[i].gui_id       = -1;
            gamepad_btn_gui_ids_dpads[j][i].gui_id = -1;
        }
}

void console_gui_set_alpha(float alpha)
{
    /* Shared */

    gui_set_alpha(xbox_control_title_id,    alpha, GUI_ANIMATION_S_LINEAR);
    gui_set_alpha(xbox_control_keybd_id,    alpha, GUI_ANIMATION_S_LINEAR);
    gui_set_alpha(xbox_control_list_id,     alpha, GUI_ANIMATION_S_LINEAR);
    gui_set_alpha(xbox_control_levelopt_id, alpha, GUI_ANIMATION_S_LINEAR);
    gui_set_alpha(xbox_control_paused_id,   alpha, GUI_ANIMATION_S_LINEAR);


    /* Generic */

    gui_set_alpha(xbox_control_desc_id,          alpha, GUI_ANIMATION_S_LINEAR);
    gui_set_alpha(xbox_control_preparation_id,   alpha, GUI_ANIMATION_S_LINEAR);
    gui_set_alpha(xbox_control_replay_id,        alpha, GUI_ANIMATION_S_LINEAR);
    gui_set_alpha(xbox_control_replay_eof_id,    alpha, GUI_ANIMATION_S_LINEAR);
    gui_set_alpha(xbox_control_shop_id,          alpha, GUI_ANIMATION_S_LINEAR);
    gui_set_alpha(xbox_control_shop_getcoins_id, alpha, GUI_ANIMATION_S_LINEAR);
    gui_set_alpha(xbox_control_model_id,         alpha, GUI_ANIMATION_S_LINEAR);
    gui_set_alpha(xbox_control_beam_style_id,    alpha, GUI_ANIMATION_S_LINEAR);
    gui_set_alpha(xbox_control_death_id,         alpha, GUI_ANIMATION_S_LINEAR);


    /* Putt */

    gui_set_alpha(xbox_control_putt_stroke_id, alpha, GUI_ANIMATION_S_LINEAR);
    gui_set_alpha(xbox_control_putt_stop_id,   alpha, GUI_ANIMATION_S_LINEAR);
    gui_set_alpha(xbox_control_putt_scores_id, alpha, GUI_ANIMATION_S_LINEAR);
}

/*---------------------------------------------------------------------------*/

static void create_controller_spacer(int space_id)
{
    for (int i = 0; i < 1; i++)
        gui_space(space_id);
}

/*---------------------------------------------------------------------------*/

#pragma region Console Platforms

/* Etihad Handset controllers */

static void create_handset_a_button(int a_id, int liveconfig)
{
    gui_label(a_id, HANDSET_A_BUTTON, GUI_SML, gui_red, gui_red);
}

static void create_handset_b_button(int b_id, int liveconfig)
{
    gui_label(b_id, HANDSET_B_BUTTON, GUI_SML, gui_yel, gui_yel);
}

static void create_handset_x_button(int x_id, int liveconfig)
{
    gui_label(x_id, HANDSET_X_BUTTON, GUI_SML, gui_blu, gui_blu);
}

static void create_handset_y_button(int y_id, int liveconfig)
{
    gui_label(y_id, HANDSET_Y_BUTTON, GUI_SML, gui_grn, gui_grn);
}

static void create_handset_lb_button(int lb_id, int liveconfig)
{
    gui_label(lb_id, HANDSET_LB_BUTTON, GUI_SML, gui_gry, gui_wht);
}

static void create_handset_rb_button(int rb_id, int liveconfig)
{
    gui_label(rb_id, HANDSET_RB_BUTTON, GUI_SML, gui_gry, gui_wht);
}

/*---------------------------------------------------------------------------*/

/* Energize Lab Maticontroller controllers */

static void create_maticontroller_a_button(int a_id, int liveconfig)
{
    gui_label(a_id, MATICONTROLLER_A_BUTTON, GUI_SML, gui_cya, gui_cya);
}

static void create_maticontroller_b_button(int b_id, int liveconfig)
{
    gui_label(b_id, MATICONTROLLER_B_BUTTON, GUI_SML, gui_cya, gui_cya);
}

static void create_maticontroller_x_button(int x_id, int liveconfig)
{
    gui_label(x_id, MATICONTROLLER_X_BUTTON, GUI_SML, gui_cya, gui_cya);
}

static void create_maticontroller_y_button(int y_id, int liveconfig)
{
    gui_label(y_id, MATICONTROLLER_Y_BUTTON, GUI_SML, gui_cya, gui_cya);
}

static void create_maticontroller_lb_button(int lb_id, int liveconfig)
{
    gui_label(lb_id, MATICONTROLLER_LB_BUTTON, GUI_SML, gui_cya, gui_wht);
}

static void create_maticontroller_rb_button(int rb_id, int liveconfig)
{
    gui_label(rb_id, MATICONTROLLER_RB_BUTTON, GUI_SML, gui_cya, gui_wht);
}

static void create_maticontroller_lt_button(int lb_id, int liveconfig)
{
    gui_label(lb_id, MATICONTROLLER_LT_BUTTON, GUI_SML, gui_cya, gui_wht);
}

static void create_maticontroller_rt_button(int rb_id, int liveconfig)
{
    gui_label(rb_id, MATICONTROLLER_RT_BUTTON, GUI_SML, gui_cya, gui_wht);
}

static void create_maticontroller_ls_button(int lb_id, int liveconfig)
{
    gui_label(lb_id, MATICONTROLLER_LS_BUTTON, GUI_SML, gui_cya, gui_wht);
}

static void create_maticontroller_rs_button(int rb_id, int liveconfig)
{
    gui_label(rb_id, MATICONTROLLER_RS_BUTTON, GUI_SML, gui_cya, gui_wht);
}

/*---------------------------------------------------------------------------*/

/* Xbox controllers */

static void create_xbox_a_button(int a_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_a[i].gui_id == -1) {
                gamepad_btn_gui_ids_a[i].gui_id = gui_image(a_id, "gui/controllers/btn_gamepad_a_xbox.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_a[i].btn_id = 0;
                found = 1;
            }

            if (found) break;
        }

        if (!found) log_errorf("Out of controller button slots! - ID: %d\n", a_id);

        return;
    }

    gui_image(a_id, "gui/controllers/btn_gamepad_a_xbox.png", video.device_h / 22, video.device_h / 26);
}

static void create_xbox_b_button(int b_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_b[i].gui_id == -1) {
                gamepad_btn_gui_ids_b[i].gui_id = gui_image(b_id, "gui/controllers/btn_gamepad_b_xbox.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_b[i].btn_id = 1;
                found = 1;
            }

            if (found) break;
        }

        if (!found) log_errorf("Out of controller button slots! - ID: %d\n", b_id);

        return;
    }

    gui_image(b_id, "gui/controllers/btn_gamepad_b_xbox.png", video.device_h / 22, video.device_h / 26);
}

static void create_xbox_x_button(int x_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_x[i].gui_id == -1) {
                gamepad_btn_gui_ids_x[i].gui_id = gui_image(x_id, "gui/controllers/btn_gamepad_x_xbox.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_x[i].btn_id = 2;
                found = 1;
            }

            if (found) break;
        }

        if (!found) log_errorf("Out of controller button slots! - ID: %d\n", x_id);

        return;
    }

    gui_image(x_id, "gui/controllers/btn_gamepad_x_xbox.png", video.device_h / 22, video.device_h / 26);
}

static void create_xbox_y_button(int y_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_y[i].gui_id == -1) {
                gamepad_btn_gui_ids_y[i].gui_id = gui_image(y_id, "gui/controllers/btn_gamepad_y_xbox.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_y[i].btn_id = 3;
                found = 1;
            }

            if (found) break;
        }

        if (!found) log_errorf("Out of controller button slots! - ID: %d\n", y_id);

        return;
    }

    gui_image(y_id, "gui/controllers/btn_gamepad_y_xbox.png", video.device_h / 22, video.device_h / 26);
}

static void create_xbox_lb_button(int lb_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_lb[i].gui_id == -1) {
                gamepad_btn_gui_ids_lb[i].gui_id = gui_image(lb_id, "gui/controllers/btn_gamepad_lb_xbox.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_lb[i].btn_id = 4;
                found = 1;
            }

            if (found) break;
        }

        if (!found) log_errorf("Out of controller button slots! - ID: %d\n", lb_id);

        return;
    }

    gui_image(lb_id, "gui/controllers/btn_gamepad_lb_xbox.png", video.device_h / 22, video.device_h / 26);
}

static void create_xbox_rb_button(int rb_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_rb[i].gui_id == -1) {
                gamepad_btn_gui_ids_rb[i].gui_id = gui_image(rb_id, "gui/controllers/btn_gamepad_rb_xbox.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_rb[i].btn_id = 5;
                found = 1;
            }

            if (found) break;
        }

        if (!found) log_errorf("Out of controller button slots! - ID: %d\n", rb_id);

        return;
    }

    gui_image(rb_id, "gui/controllers/btn_gamepad_rb_xbox.png", video.device_h / 22, video.device_h / 26);
}

static void create_xbox_lt_button(int lt_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_lt[i].gui_id == -1) {
                gamepad_btn_gui_ids_lt[i].gui_id = gui_image(lt_id, "gui/controllers/btn_gamepad_lt_xbox.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_lt[i].btn_id = 6;
                found = 1;
            }

            if (found) break;
        }

        if (!found) log_errorf("Out of controller button slots! - ID: %d\n", lt_id);

        return;
    }

    gui_image(lt_id, "gui/controllers/btn_gamepad_lt_xbox.png", video.device_h / 22, video.device_h / 26);
}

static void create_xbox_rt_button(int rt_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_rt[i].gui_id == -1) {
                gamepad_btn_gui_ids_rt[i].gui_id = gui_image(rt_id, "gui/controllers/btn_gamepad_rt_xbox.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_rt[i].btn_id = 7;
                found = 1;
            }

            if (found) break;
        }

        if (!found) log_errorf("Out of controller button slots! - ID: %d\n", rt_id);

        return;
    }

    gui_image(rt_id, "gui/controllers/btn_gamepad_rt_xbox.png", video.device_h / 22, video.device_h / 26);
}

static void create_xbox_ls_button(int ls_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_ls[i].gui_id == -1) {
                gamepad_btn_gui_ids_ls[i].gui_id = gui_image(ls_id, "gui/controllers/btn_gamepad_ls_xbox.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_ls[i].btn_id = 13;
                found = 1;
            }

            if (found) break;
        }

        if (!found) log_errorf("Out of controller button slots! - ID: %d\n", ls_id);

        return;
    }

    gui_image(ls_id, "gui/controllers/btn_gamepad_ls_xbox.png", video.device_h / 22, video.device_h / 26);
}

static void create_xbox_rs_button(int rs_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_rs[i].gui_id == -1) {
                gamepad_btn_gui_ids_rs[i].gui_id = gui_image(rs_id, "gui/controllers/btn_gamepad_rs_xbox.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_rs[i].btn_id = 15;
                found = 1;
            }

            if (found) break;
        }

        if (!found) log_errorf("Out of controller button slots! - ID: %d\n", rs_id);

        return;
    }

    gui_image(rs_id, "gui/controllers/btn_gamepad_rs_xbox.png", video.device_h / 22, video.device_h / 26);
}

static void create_xbox_start_button(int start_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_start[i].gui_id == -1) {
                gamepad_btn_gui_ids_start[i].gui_id = gui_image(start_id, "gui/controllers/btn_gamepad_start_xbox.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_start[i].btn_id = 9;
                found = 1;
            }

            if (found) break;
        }

        if (!found) log_errorf("Out of controller button slots! - ID: %d\n", start_id);

        return;
    }

    gui_image(start_id, "gui/controllers/btn_gamepad_start_xbox.png", video.device_h / 22, video.device_h / 26);
}

static void create_xbox_select_button(int select_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_select[i].gui_id == -1) {
                gamepad_btn_gui_ids_select[i].gui_id = gui_image(select_id, "gui/controllers/btn_gamepad_select_xbox.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_select[i].btn_id = 8;
                found = 1;
            }

            if (found) break;
        }

        if (!found) log_errorf("Out of controller button slots! - ID: %d\n", select_id);

        return;
    }

    gui_image(select_id, "gui/controllers/btn_gamepad_select_xbox.png", video.device_h / 22, video.device_h / 26);
}

/*---------------------------------------------------------------------------*/

/* PlayStation Controllers */

static void create_ps4_a_button(int a_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_a[i].gui_id == -1) {
                gamepad_btn_gui_ids_a[i].gui_id = gui_image(a_id, "gui/controllers/btn_gamepad_a_playstation.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_a[i].btn_id = 0;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(a_id, "gui/controllers/btn_gamepad_a_playstation.png", video.device_h / 22, video.device_h / 26);
}

static void create_ps4_b_button(int b_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_b[i].gui_id == -1) {
                gamepad_btn_gui_ids_b[i].gui_id = gui_image(b_id, "gui/controllers/btn_gamepad_b_playstation.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_b[i].btn_id = 1;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(b_id, "gui/controllers/btn_gamepad_b_playstation.png", video.device_h / 22, video.device_h / 26);
}

static void create_ps4_x_button(int x_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_x[i].gui_id == -1) {
                gamepad_btn_gui_ids_x[i].gui_id = gui_image(x_id, "gui/controllers/btn_gamepad_x_playstation.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_x[i].btn_id = 2;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(x_id, "gui/controllers/btn_gamepad_x_playstation.png", video.device_h / 22, video.device_h / 26);
}

static void create_ps4_y_button(int y_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_y[i].gui_id == -1) {
                gamepad_btn_gui_ids_y[i].gui_id = gui_image(y_id, "gui/controllers/btn_gamepad_y_playstation.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_y[i].btn_id = 3;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(y_id, "gui/controllers/btn_gamepad_y_playstation.png", video.device_h / 22, video.device_h / 26);
}

static void create_ps4_lb_button(int lb_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_lb[i].gui_id == -1) {
                gamepad_btn_gui_ids_lb[i].gui_id = gui_image(lb_id, "gui/controllers/btn_gamepad_lb_playstation.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_lb[i].btn_id = 4;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(lb_id, "gui/controllers/btn_gamepad_lb_playstation.png", video.device_h / 22, video.device_h / 26);
}

static void create_ps4_rb_button(int rb_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_rb[i].gui_id == -1) {
                gamepad_btn_gui_ids_rb[i].gui_id = gui_image(rb_id, "gui/controllers/btn_gamepad_rb_playstation.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_rb[i].btn_id = 5;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(rb_id, "gui/controllers/btn_gamepad_rb_playstation.png", video.device_h / 22, video.device_h / 26);
}

static void create_ps4_lt_button(int lt_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_lt[i].gui_id == -1) {
                gamepad_btn_gui_ids_lt[i].gui_id = gui_image(lt_id, "gui/controllers/btn_gamepad_lt_playstation.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_lt[i].btn_id = 6;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(lt_id, "gui/controllers/btn_gamepad_lt_playstation.png", video.device_h / 22, video.device_h / 26);
}

static void create_ps4_rt_button(int rt_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_rt[i].gui_id == -1) {
                gamepad_btn_gui_ids_rt[i].gui_id = gui_image(rt_id, "gui/controllers/btn_gamepad_rt_playstation.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_rt[i].btn_id = 7;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(rt_id, "gui/controllers/btn_gamepad_rt_playstation.png", video.device_h / 22, video.device_h / 26);
}

static void create_ps4_ls_button(int ls_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_ls[i].gui_id == -1) {
                gamepad_btn_gui_ids_ls[i].gui_id = gui_image(ls_id, "gui/controllers/btn_gamepad_ls_playstation.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_ls[i].btn_id = 13;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(ls_id, "gui/controllers/btn_gamepad_ls_playstation.png", video.device_h / 22, video.device_h / 26);
}

static void create_ps4_rs_button(int rs_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_rs[i].gui_id == -1) {
                gamepad_btn_gui_ids_rs[i].gui_id = gui_image(rs_id, "gui/controllers/btn_gamepad_rs_playstation.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_rs[i].btn_id = 15;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(rs_id, "gui/controllers/btn_gamepad_rs_playstation.png", video.device_h / 22, video.device_h / 26);
}

static void create_ps4_start_button(int start_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_start[i].gui_id == -1) {
                gamepad_btn_gui_ids_start[i].gui_id = gui_image(start_id, "gui/controllers/btn_gamepad_start_playstation.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_start[i].btn_id = 9;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(start_id, "gui/controllers/btn_gamepad_start_playstation.png", video.device_h / 22, video.device_h / 26);
}

static void create_ps4_select_button(int select_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_select[i].gui_id == -1) {
                gamepad_btn_gui_ids_select[i].gui_id = gui_image(select_id, "gui/controllers/btn_gamepad_select_playstation.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_select[i].btn_id = 8;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(select_id, "gui/controllers/btn_gamepad_select_playstation.png", video.device_h / 22, video.device_h / 26);
}

/*---------------------------------------------------------------------------*/

/* Steam Controllers */

static void create_steamdeck_a_button(int a_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_a[i].gui_id == -1) {
                gamepad_btn_gui_ids_a[i].gui_id = gui_image(a_id, "gui/controllers/btn_gamepad_a_switch.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_a[i].btn_id = 0;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(a_id, "gui/controllers/btn_gamepad_a_switch.png", video.device_h / 22, video.device_h / 26);
}

static void create_steamdeck_b_button(int b_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_b[i].gui_id == -1) {
                gamepad_btn_gui_ids_b[i].gui_id = gui_image(b_id, "gui/controllers/btn_gamepad_b_switch.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_b[i].btn_id = 1;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(b_id, "gui/controllers/btn_gamepad_b_switch.png", video.device_h / 22, video.device_h / 26);
}

static void create_steamdeck_x_button(int x_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_x[i].gui_id == -1) {
                gamepad_btn_gui_ids_x[i].gui_id = gui_image(x_id, "gui/controllers/btn_gamepad_x_switch.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_x[i].btn_id = 2;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(x_id, "gui/controllers/btn_gamepad_x_switch.png", video.device_h / 22, video.device_h / 26);
}

static void create_steamdeck_y_button(int y_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_y[i].gui_id == -1) {
                gamepad_btn_gui_ids_y[i].gui_id = gui_image(y_id, "gui/controllers/btn_gamepad_y_switch.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_y[i].btn_id = 3;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(y_id, "gui/controllers/btn_gamepad_y_switch.png", video.device_h / 22, video.device_h / 26);
}

static void create_steamdeck_lb_button(int lb_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_lb[i].gui_id == -1) {
                gamepad_btn_gui_ids_lb[i].gui_id = gui_image(lb_id, "gui/controllers/btn_gamepad_lb_switch.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_lb[i].btn_id = 4;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(lb_id, "gui/controllers/btn_gamepad_lb_switch.png", video.device_h / 22, video.device_h / 26);
}

static void create_steamdeck_rb_button(int rb_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_rb[i].gui_id == -1) {
                gamepad_btn_gui_ids_rb[i].gui_id = gui_image(rb_id, "gui/controllers/btn_gamepad_rb_switch.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_rb[i].btn_id = 5;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(rb_id, "gui/controllers/btn_gamepad_rb_switch.png", video.device_h / 22, video.device_h / 26);
}

static void create_steamdeck_lt_button(int lt_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_lt[i].gui_id == -1) {
                gamepad_btn_gui_ids_lt[i].gui_id = gui_image(lt_id, "gui/controllers/btn_gamepad_lt_switch.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_lt[i].btn_id = 6;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(lt_id, "gui/controllers/btn_gamepad_lt_switch.png", video.device_h / 22, video.device_h / 26);
}

static void create_steamdeck_rt_button(int rt_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_rt[i].gui_id == -1) {
                gamepad_btn_gui_ids_rt[i].gui_id = gui_image(rt_id, "gui/controllers/btn_gamepad_rt_switch.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_rt[i].btn_id = 7;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(rt_id, "gui/controllers/btn_gamepad_rt_switch.png", video.device_h / 22, video.device_h / 26);
}

static void create_steamdeck_ls_button(int ls_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_ls[i].gui_id == -1) {
                gamepad_btn_gui_ids_ls[i].gui_id = gui_image(ls_id, "gui/controllers/btn_gamepad_ls_switch.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_ls[i].btn_id = 13;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(ls_id, "gui/controllers/btn_gamepad_ls_switch.png", video.device_h / 22, video.device_h / 26);
}

static void create_steamdeck_rs_button(int rs_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_rs[i].gui_id == -1) {
                gamepad_btn_gui_ids_rs[i].gui_id = gui_image(rs_id, "gui/controllers/btn_gamepad_rs_switch.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_rs[i].btn_id = 15;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(rs_id, "gui/controllers/btn_gamepad_rs_switch.png", video.device_h / 22, video.device_h / 26);
}

static void create_steamdeck_start_button(int start_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_start[i].gui_id == -1) {
                gamepad_btn_gui_ids_start[i].gui_id = gui_image(start_id, "gui/controllers/btn_gamepad_start_xbox.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_start[i].btn_id = 9;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(start_id, "gui/controllers/btn_gamepad_start_xbox.png", video.device_h / 22, video.device_h / 26);
}

static void create_steamdeck_select_button(int select_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_select[i].gui_id == -1) {
                gamepad_btn_gui_ids_select[i].gui_id = gui_image(select_id, "gui/controllers/btn_gamepad_select_xbox.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_select[i].btn_id = 8;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(select_id, "gui/controllers/btn_gamepad_select_xbox.png", video.device_h / 22, video.device_h / 26);
}

/*---------------------------------------------------------------------------*/

/* Nintendo Controllers */

static void create_switch_a_button(int a_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_a[i].gui_id == -1) {
                gamepad_btn_gui_ids_a[i].gui_id = gui_image(a_id, "gui/controllers/btn_gamepad_a_switch.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_a[i].btn_id = 0;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(a_id, "gui/controllers/btn_gamepad_a_switch.png", video.device_h / 22, video.device_h / 26);
}

static void create_switch_b_button(int b_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_b[i].gui_id == -1) {
                gamepad_btn_gui_ids_b[i].gui_id = gui_image(b_id, "gui/controllers/btn_gamepad_b_switch.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_b[i].btn_id = 1;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(b_id, "gui/controllers/btn_gamepad_b_switch.png", video.device_h / 22, video.device_h / 26);
}

static void create_switch_x_button(int x_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_x[i].gui_id == -1) {
                gamepad_btn_gui_ids_x[i].gui_id = gui_image(x_id, "gui/controllers/btn_gamepad_x_switch.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_x[i].btn_id = 2;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(x_id, "gui/controllers/btn_gamepad_x_switch.png", video.device_h / 22, video.device_h / 26);
}

static void create_switch_y_button(int y_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_y[i].gui_id == -1) {
                gamepad_btn_gui_ids_y[i].gui_id = gui_image(y_id, "gui/controllers/btn_gamepad_y_switch.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_y[i].btn_id = 3;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(y_id, "gui/controllers/btn_gamepad_y_switch.png", video.device_h / 22, video.device_h / 26);
}

static void create_switch_lb_button(int lb_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_lb[i].gui_id == -1) {
                gamepad_btn_gui_ids_lb[i].gui_id = gui_image(lb_id, "gui/controllers/btn_gamepad_lb_switch.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_lb[i].btn_id = 4;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(lb_id, "gui/controllers/btn_gamepad_lb_switch.png", video.device_h / 22, video.device_h / 26);
}

static void create_switch_rb_button(int rb_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_rb[i].gui_id == -1) {
                gamepad_btn_gui_ids_rb[i].gui_id = gui_image(rb_id, "gui/controllers/btn_gamepad_rb_switch.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_rb[i].btn_id = 5;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(rb_id, "gui/controllers/btn_gamepad_rb_switch.png", video.device_h / 22, video.device_h / 26);
}

static void create_switch_lt_button(int lt_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_lt[i].gui_id == -1) {
                gamepad_btn_gui_ids_lt[i].gui_id = gui_image(lt_id, "gui/controllers/btn_gamepad_lt_switch.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_lt[i].btn_id = 6;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(lt_id, "gui/controllers/btn_gamepad_lt_switch.png", video.device_h / 22, video.device_h / 26);
}

static void create_switch_rt_button(int rt_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_rt[i].gui_id == -1) {
                gamepad_btn_gui_ids_rt[i].gui_id = gui_image(rt_id, "gui/controllers/btn_gamepad_rt_switch.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_rt[i].btn_id = 7;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(rt_id, "gui/controllers/btn_gamepad_rt_switch.png", video.device_h / 22, video.device_h / 26);
}

static void create_switch_ls_button(int ls_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_ls[i].gui_id == -1) {
                gamepad_btn_gui_ids_ls[i].gui_id = gui_image(ls_id, "gui/controllers/btn_gamepad_ls_switch.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_ls[i].btn_id = 13;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(ls_id, "gui/controllers/btn_gamepad_ls_switch.png", video.device_h / 22, video.device_h / 26);
}

static void create_switch_rs_button(int rs_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_rs[i].gui_id == -1) {
                gamepad_btn_gui_ids_rs[i].gui_id = gui_image(rs_id, "gui/controllers/btn_gamepad_rs_switch.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_rs[i].btn_id = 15;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(rs_id, "gui/controllers/btn_gamepad_rs_switch.png", video.device_h / 22, video.device_h / 26);
}

static void create_switch_start_button(int start_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_start[i].gui_id == -1) {
                gamepad_btn_gui_ids_start[i].gui_id = gui_image(start_id, "gui/controllers/btn_gamepad_start_switch.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_start[i].btn_id = 9;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(start_id, "gui/controllers/btn_gamepad_start_switch.png", video.device_h / 22, video.device_h / 26);
}

static void create_switch_select_button(int select_id, int liveconfig)
{
    if (liveconfig) {
        int found = 0;
        for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS && !found; i++) {
            if (gamepad_btn_gui_ids_select[i].gui_id == -1) {
                gamepad_btn_gui_ids_select[i].gui_id = gui_image(select_id, "gui/controllers/btn_gamepad_select_switch.png", video.device_h / 22, video.device_h / 26);
                gamepad_btn_gui_ids_select[i].btn_id = 8;
                found = 1;
            }

            if (found) break;
        }
        return;
    }

    gui_image(select_id, "gui/controllers/btn_gamepad_select_switch.png", video.device_h / 22, video.device_h / 26);
}

/*---------------------------------------------------------------------------*/

/* Generic Controllers */

void console_gui_create_a_button(int gui_id, int btn_id, int liveconfig)
{
    switch (btn_id)
    {
        //case 0: console_gui_create_a_button(gui_id, btn_id, liveconfig); return;
        case 1: console_gui_create_b_button(gui_id, btn_id, liveconfig); return;
        case 2: console_gui_create_x_button(gui_id, btn_id, liveconfig); return;
        case 3: console_gui_create_y_button(gui_id, btn_id, liveconfig); return;
        case 4: console_gui_create_lb_button(gui_id, btn_id, liveconfig); return;
        case 6: console_gui_create_lt_button(gui_id, btn_id, liveconfig); return;
        case 5: console_gui_create_rb_button(gui_id, btn_id, liveconfig); return;
        case 7: console_gui_create_rt_button(gui_id, btn_id, liveconfig); return;
        case 9: console_gui_create_start_button(gui_id, btn_id, liveconfig); return;
        case 8: console_gui_create_select_button(gui_id, btn_id, liveconfig); return;
        case 13: console_gui_create_ls_button(gui_id, btn_id, liveconfig); return;
        case 15: console_gui_create_rs_button(gui_id, btn_id, liveconfig); return;
    }

    switch (current_platform)
    {
        case PLATFORM_HANDSET:
        create_handset_y_button(gui_id, liveconfig);
        break;
        case PLATFORM_PS:
        create_ps4_a_button(gui_id, liveconfig);
        break;
        case PLATFORM_STEAMDECK:
        create_steamdeck_a_button(gui_id, liveconfig);
        break;
        case PLATFORM_SWITCH:
        create_switch_a_button(gui_id, liveconfig);
        break;
        case PLATFORM_ENERGIZELAB:
        create_maticontroller_a_button(gui_id, liveconfig);
        break;
        default:
        create_xbox_a_button(gui_id, liveconfig);
    }
}

void console_gui_create_b_button(int gui_id, int btn_id, int liveconfig)
{
    switch (btn_id)
    {
        case 0: console_gui_create_a_button(gui_id, btn_id, liveconfig); return;
        //case 1: console_gui_create_b_button(gui_id, btn_id, liveconfig); return;
        case 2: console_gui_create_x_button(gui_id, btn_id, liveconfig); return;
        case 3: console_gui_create_y_button(gui_id, btn_id, liveconfig); return;
        case 4: console_gui_create_lb_button(gui_id, btn_id, liveconfig); return;
        case 6: console_gui_create_lt_button(gui_id, btn_id, liveconfig); return;
        case 5: console_gui_create_rb_button(gui_id, btn_id, liveconfig); return;
        case 7: console_gui_create_rt_button(gui_id, btn_id, liveconfig); return;
        case 9: console_gui_create_start_button(gui_id, btn_id, liveconfig); return;
        case 8: console_gui_create_select_button(gui_id, btn_id, liveconfig); return;
        case 13: console_gui_create_ls_button(gui_id, btn_id, liveconfig); return;
        case 15: console_gui_create_rs_button(gui_id, btn_id, liveconfig); return;
    }

    switch (current_platform)
    {
        case PLATFORM_HANDSET:
        create_handset_x_button(gui_id, liveconfig);
        break;
        case PLATFORM_PS:
        create_ps4_b_button(gui_id, liveconfig);
        break;
        case PLATFORM_STEAMDECK:
        create_steamdeck_b_button(gui_id, liveconfig);
        break;
        case PLATFORM_SWITCH:
        create_switch_b_button(gui_id, liveconfig);
        break;
        case PLATFORM_ENERGIZELAB:
        create_maticontroller_b_button(gui_id, liveconfig);
        break;
        default:
        create_xbox_b_button(gui_id, liveconfig);
    }
}

void console_gui_create_x_button(int gui_id, int btn_id, int liveconfig)
{
    switch (btn_id)
    {
        case 0: console_gui_create_a_button(gui_id, btn_id, liveconfig); return;
        case 1: console_gui_create_b_button(gui_id, btn_id, liveconfig); return;
        //case 2: console_gui_create_x_button(gui_id, btn_id, liveconfig); return;
        case 3: console_gui_create_y_button(gui_id, btn_id, liveconfig); return;
        case 4: console_gui_create_lb_button(gui_id, btn_id, liveconfig); return;
        case 6: console_gui_create_lt_button(gui_id, btn_id, liveconfig); return;
        case 5: console_gui_create_rb_button(gui_id, btn_id, liveconfig); return;
        case 7: console_gui_create_rt_button(gui_id, btn_id, liveconfig); return;
        case 9: console_gui_create_start_button(gui_id, btn_id, liveconfig); return;
        case 8: console_gui_create_select_button(gui_id, btn_id, liveconfig); return;
        case 13: console_gui_create_ls_button(gui_id, btn_id, liveconfig); return;
        case 15: console_gui_create_rs_button(gui_id, btn_id, liveconfig); return;
    }

    switch (current_platform)
    {
        case PLATFORM_HANDSET:
        create_handset_b_button(gui_id, liveconfig);
        break;
        case PLATFORM_PS:
        create_ps4_x_button(gui_id, liveconfig);
        break;
        case PLATFORM_STEAMDECK:
        create_steamdeck_x_button(gui_id, liveconfig);
        break;
        case PLATFORM_SWITCH:
        create_switch_x_button(gui_id, liveconfig);
        break;
        case PLATFORM_ENERGIZELAB:
        create_maticontroller_x_button(gui_id, liveconfig);
        break;
        default:
        create_xbox_x_button(gui_id, liveconfig);
    }
}

void console_gui_create_y_button(int gui_id, int btn_id, int liveconfig)
{
    switch (btn_id)
    {
        case 0: console_gui_create_a_button(gui_id, btn_id, liveconfig); return;
        case 1: console_gui_create_b_button(gui_id, btn_id, liveconfig); return;
        case 2: console_gui_create_x_button(gui_id, btn_id, liveconfig); return;
        //case 3: console_gui_create_y_button(gui_id, btn_id, liveconfig); return;
        case 4: console_gui_create_lb_button(gui_id, btn_id, liveconfig); return;
        case 6: console_gui_create_lt_button(gui_id, btn_id, liveconfig); return;
        case 5: console_gui_create_rb_button(gui_id, btn_id, liveconfig); return;
        case 7: console_gui_create_rt_button(gui_id, btn_id, liveconfig); return;
        case 9: console_gui_create_start_button(gui_id, btn_id, liveconfig); return;
        case 8: console_gui_create_select_button(gui_id, btn_id, liveconfig); return;
        case 13: console_gui_create_ls_button(gui_id, btn_id, liveconfig); return;
        case 15: console_gui_create_rs_button(gui_id, btn_id, liveconfig); return;
    }

    switch (current_platform)
    {
        case PLATFORM_HANDSET:
        create_handset_a_button(gui_id, liveconfig);
        break;
        case PLATFORM_PS:
        create_ps4_y_button(gui_id, liveconfig);
        break;
        case PLATFORM_STEAMDECK:
        create_steamdeck_y_button(gui_id, liveconfig);
        break;
        case PLATFORM_SWITCH:
        create_switch_y_button(gui_id, liveconfig);
        break;
        case PLATFORM_ENERGIZELAB:
        create_maticontroller_y_button(gui_id, liveconfig);
        break;
        default:
        create_xbox_y_button(gui_id, liveconfig);
    }
}

void console_gui_create_lb_button(int gui_id, int btn_id, int liveconfig)
{
    switch (btn_id)
    {
        case 0: console_gui_create_a_button(gui_id, btn_id, liveconfig); return;
        case 1: console_gui_create_b_button(gui_id, btn_id, liveconfig); return;
        case 2: console_gui_create_x_button(gui_id, btn_id, liveconfig); return;
        case 3: console_gui_create_y_button(gui_id, btn_id, liveconfig); return;
        //case 4: console_gui_create_lb_button(gui_id, btn_id, liveconfig); return;
        case 6: console_gui_create_lt_button(gui_id, btn_id, liveconfig); return;
        case 5: console_gui_create_rb_button(gui_id, btn_id, liveconfig); return;
        case 7: console_gui_create_rt_button(gui_id, btn_id, liveconfig); return;
        case 9: console_gui_create_start_button(gui_id, btn_id, liveconfig); return;
        case 8: console_gui_create_select_button(gui_id, btn_id, liveconfig); return;
        case 13: console_gui_create_ls_button(gui_id, btn_id, liveconfig); return;
        case 15: console_gui_create_rs_button(gui_id, btn_id, liveconfig); return;
    }

    switch (current_platform)
    {
        case PLATFORM_HANDSET:
        create_handset_lb_button(gui_id, liveconfig);
        break;
        case PLATFORM_PS:
        create_ps4_lb_button(gui_id, liveconfig);
        break;
        case PLATFORM_STEAMDECK:
        create_steamdeck_lb_button(gui_id, liveconfig);
        break;
        case PLATFORM_SWITCH:
        create_switch_lb_button(gui_id, liveconfig);
        break;
        case PLATFORM_ENERGIZELAB:
        create_maticontroller_lb_button(gui_id, liveconfig);
        break;
        default:
        create_xbox_lb_button(gui_id, liveconfig);
    }
}

void console_gui_create_rb_button(int gui_id, int btn_id, int liveconfig)
{
    switch (btn_id)
    {
        case 0: console_gui_create_a_button(gui_id, btn_id, liveconfig); return;
        case 1: console_gui_create_b_button(gui_id, btn_id, liveconfig); return;
        case 2: console_gui_create_x_button(gui_id, btn_id, liveconfig); return;
        case 3: console_gui_create_y_button(gui_id, btn_id, liveconfig); return;
        case 4: console_gui_create_lb_button(gui_id, btn_id, liveconfig); return;
        case 6: console_gui_create_lt_button(gui_id, btn_id, liveconfig); return;
        //case 5: console_gui_create_rb_button(gui_id, btn_id, liveconfig); return;
        case 7: console_gui_create_rt_button(gui_id, btn_id, liveconfig); return;
        case 9: console_gui_create_start_button(gui_id, btn_id, liveconfig); return;
        case 8: console_gui_create_select_button(gui_id, btn_id, liveconfig); return;
        case 13: console_gui_create_ls_button(gui_id, btn_id, liveconfig); return;
        case 15: console_gui_create_rs_button(gui_id, btn_id, liveconfig); return;
    }

    switch (current_platform)
    {
        case PLATFORM_HANDSET:
        create_handset_rb_button(gui_id, liveconfig);
        break;
        case PLATFORM_PS:
        create_ps4_rb_button(gui_id, liveconfig);
        break;
        case PLATFORM_STEAMDECK:
        create_steamdeck_rb_button(gui_id, liveconfig);
        break;
        case PLATFORM_SWITCH:
        create_switch_rb_button(gui_id, liveconfig);
        break;
        case PLATFORM_ENERGIZELAB:
        create_maticontroller_rb_button(gui_id, liveconfig);
        break;
        default:
        create_xbox_rb_button(gui_id, liveconfig);
    }
}

void console_gui_create_lt_button(int gui_id, int btn_id, int liveconfig)
{
    if (current_platform == PLATFORM_HANDSET)
    {
        /* Not available in handset */
        return;
    }

    switch (btn_id)
    {
        case 0: console_gui_create_a_button(gui_id, btn_id, liveconfig); return;
        case 1: console_gui_create_b_button(gui_id, btn_id, liveconfig); return;
        case 2: console_gui_create_x_button(gui_id, btn_id, liveconfig); return;
        case 3: console_gui_create_y_button(gui_id, btn_id, liveconfig); return;
        case 4: console_gui_create_lb_button(gui_id, btn_id, liveconfig); return;
        //case 6: console_gui_create_lt_button(gui_id, btn_id, liveconfig); return;
        case 5: console_gui_create_rb_button(gui_id, btn_id, liveconfig); return;
        case 7: console_gui_create_rt_button(gui_id, btn_id, liveconfig); return;
        case 9: console_gui_create_start_button(gui_id, btn_id, liveconfig); return;
        case 8: console_gui_create_select_button(gui_id, btn_id, liveconfig); return;
        case 13: console_gui_create_ls_button(gui_id, btn_id, liveconfig); return;
        case 15: console_gui_create_rs_button(gui_id, btn_id, liveconfig); return;
    }

    switch (current_platform)
    {
        case PLATFORM_PS:
        create_ps4_lt_button(gui_id, liveconfig);
        break;
        case PLATFORM_STEAMDECK:
        create_steamdeck_lt_button(gui_id, liveconfig);
        break;
        case PLATFORM_SWITCH:
        create_switch_lt_button(gui_id, liveconfig);
        break;
        case PLATFORM_ENERGIZELAB:
        create_maticontroller_lt_button(gui_id, liveconfig);
        break;
        default:
        create_xbox_lt_button(gui_id, liveconfig);
    }
}

void console_gui_create_rt_button(int gui_id, int btn_id, int liveconfig)
{
    if (current_platform == PLATFORM_HANDSET)
    {
        /* Not available in handset */
        return;
    }

    switch (btn_id)
    {
        case 0: console_gui_create_a_button(gui_id, btn_id, liveconfig); return;
        case 1: console_gui_create_b_button(gui_id, btn_id, liveconfig); return;
        case 2: console_gui_create_x_button(gui_id, btn_id, liveconfig); return;
        case 3: console_gui_create_y_button(gui_id, btn_id, liveconfig); return;
        case 4: console_gui_create_lb_button(gui_id, btn_id, liveconfig); return;
        case 6: console_gui_create_lt_button(gui_id, btn_id, liveconfig); return;
        case 5: console_gui_create_rb_button(gui_id, btn_id, liveconfig); return;
        //case 7: console_gui_create_rt_button(gui_id, btn_id, liveconfig); return;
        case 9: console_gui_create_start_button(gui_id, btn_id, liveconfig); return;
        case 8: console_gui_create_select_button(gui_id, btn_id, liveconfig); return;
        case 13: console_gui_create_ls_button(gui_id, btn_id, liveconfig); return;
        case 15: console_gui_create_rs_button(gui_id, btn_id, liveconfig); return;
    }

    switch (current_platform)
    {
        case PLATFORM_PS:
        create_ps4_rt_button(gui_id, liveconfig);
        break;
        case PLATFORM_STEAMDECK:
        create_steamdeck_rt_button(gui_id, liveconfig);
        break;
        case PLATFORM_SWITCH:
        create_switch_rt_button(gui_id, liveconfig);
        break;
        case PLATFORM_ENERGIZELAB:
        create_maticontroller_rt_button(gui_id, liveconfig);
        break;
        default:
        create_xbox_rt_button(gui_id, liveconfig);
    }
}

void console_gui_create_ls_button(int gui_id, int btn_id, int liveconfig)
{
    if (current_platform == PLATFORM_HANDSET ||
        current_platform == PLATFORM_ENERGIZELAB)
    {
        /* Not available in handset or Energize Lab Maticontroller */
        return;
    }

    switch (btn_id)
    {
        case 0: console_gui_create_a_button(gui_id, btn_id, liveconfig); return;
        case 1: console_gui_create_b_button(gui_id, btn_id, liveconfig); return;
        case 2: console_gui_create_x_button(gui_id, btn_id, liveconfig); return;
        case 3: console_gui_create_y_button(gui_id, btn_id, liveconfig); return;
        case 4: console_gui_create_lb_button(gui_id, btn_id, liveconfig); return;
        case 6: console_gui_create_lt_button(gui_id, btn_id, liveconfig); return;
        case 5: console_gui_create_rb_button(gui_id, btn_id, liveconfig); return;
        case 7: console_gui_create_rt_button(gui_id, btn_id, liveconfig); return;
        case 9: console_gui_create_start_button(gui_id, btn_id, liveconfig); return;
        case 8: console_gui_create_select_button(gui_id, btn_id, liveconfig); return;
        //case 13: console_gui_create_ls_button(gui_id, btn_id, liveconfig); return;
        case 15: console_gui_create_rs_button(gui_id, btn_id, liveconfig); return;
    }

    switch (current_platform)
    {
        case PLATFORM_PS:
        create_ps4_ls_button(gui_id, liveconfig);
        break;
        case PLATFORM_STEAMDECK:
        create_steamdeck_ls_button(gui_id, liveconfig);
        break;
        case PLATFORM_SWITCH:
        create_switch_ls_button(gui_id, liveconfig);
        break;
        default:
        create_xbox_ls_button(gui_id, liveconfig);
    }
}

void console_gui_create_rs_button(int gui_id, int btn_id, int liveconfig)
{
    if (current_platform == PLATFORM_HANDSET ||
        current_platform == PLATFORM_ENERGIZELAB)
    {
        /* Not available in handset or Energize Lab Maticontroller */
        return;
    }

    switch (btn_id)
    {
        case 0: console_gui_create_a_button(gui_id, btn_id, liveconfig); return;
        case 1: console_gui_create_b_button(gui_id, btn_id, liveconfig); return;
        case 2: console_gui_create_x_button(gui_id, btn_id, liveconfig); return;
        case 3: console_gui_create_y_button(gui_id, btn_id, liveconfig); return;
        case 4: console_gui_create_lb_button(gui_id, btn_id, liveconfig); return;
        case 6: console_gui_create_lt_button(gui_id, btn_id, liveconfig); return;
        case 5: console_gui_create_rb_button(gui_id, btn_id, liveconfig); return;
        case 7: console_gui_create_rt_button(gui_id, btn_id, liveconfig); return;
        case 9: console_gui_create_start_button(gui_id, btn_id, liveconfig); return;
        case 8: console_gui_create_select_button(gui_id, btn_id, liveconfig); return;
        case 13: console_gui_create_ls_button(gui_id, btn_id, liveconfig); return;
        //case 15: console_gui_create_rs_button(gui_id, btn_id, liveconfig); return;
    }

    switch (current_platform)
    {
        case PLATFORM_PS:
        create_ps4_rs_button(gui_id, liveconfig);
        break;
        case PLATFORM_STEAMDECK:
        create_steamdeck_rs_button(gui_id, liveconfig);
        break;
        case PLATFORM_SWITCH:
        create_switch_rs_button(gui_id, liveconfig);
        break;
        default:
        create_xbox_rs_button(gui_id, liveconfig);
    }
}

void console_gui_create_start_button(int gui_id, int btn_id, int liveconfig)
{
    if (current_platform == PLATFORM_HANDSET ||
        current_platform == PLATFORM_ENERGIZELAB)
    {
        /* Not available in handset or Energize Lab Maticontroller */
        return;
    }

    switch (btn_id)
    {
        case 0: console_gui_create_a_button(gui_id, btn_id, liveconfig); return;
        case 1: console_gui_create_b_button(gui_id, btn_id, liveconfig); return;
        case 2: console_gui_create_x_button(gui_id, btn_id, liveconfig); return;
        case 3: console_gui_create_y_button(gui_id, btn_id, liveconfig); return;
        case 4: console_gui_create_lb_button(gui_id, btn_id, liveconfig); return;
        case 6: console_gui_create_lt_button(gui_id, btn_id, liveconfig); return;
        case 5: console_gui_create_rb_button(gui_id, btn_id, liveconfig); return;
        case 7: console_gui_create_rt_button(gui_id, btn_id, liveconfig); return;
        //case 9: console_gui_create_start_button(gui_id, btn_id, liveconfig); return;
        case 8: console_gui_create_select_button(gui_id, btn_id, liveconfig); return;
        case 13: console_gui_create_ls_button(gui_id, btn_id, liveconfig); return;
        case 15: console_gui_create_rs_button(gui_id, btn_id, liveconfig); return;
    }

    switch (current_platform)
    {
        case PLATFORM_PS:
        create_ps4_start_button(gui_id, liveconfig);
        break;
        case PLATFORM_STEAMDECK:
        create_steamdeck_start_button(gui_id, liveconfig);
        break;
        case PLATFORM_SWITCH:
        create_switch_start_button(gui_id, liveconfig);
        break;
        default:
        create_xbox_start_button(gui_id, liveconfig);
    }
}

void console_gui_create_select_button(int gui_id, int btn_id, int liveconfig)
{
    if (current_platform == PLATFORM_HANDSET ||
        current_platform == PLATFORM_ENERGIZELAB)
    {
        /* Not available in handset or Energize Lab Maticontroller */
        return;
    }

    switch (btn_id)
    {
        case 0: console_gui_create_a_button(gui_id, btn_id, liveconfig); return;
        case 1: console_gui_create_b_button(gui_id, btn_id, liveconfig); return;
        case 2: console_gui_create_x_button(gui_id, btn_id, liveconfig); return;
        case 3: console_gui_create_y_button(gui_id, btn_id, liveconfig); return;
        case 4: console_gui_create_lb_button(gui_id, btn_id, liveconfig); return;
        case 6: console_gui_create_lt_button(gui_id, btn_id, liveconfig); return;
        case 5: console_gui_create_rb_button(gui_id, btn_id, liveconfig); return;
        case 7: console_gui_create_rt_button(gui_id, btn_id, liveconfig); return;
        case 9: console_gui_create_start_button(gui_id, btn_id, liveconfig); return;
        //case 8: console_gui_create_select_button(gui_id, btn_id, liveconfig); return;
        case 13: console_gui_create_ls_button(gui_id, btn_id, liveconfig); return;
        case 15: console_gui_create_rs_button(gui_id, btn_id, liveconfig); return;
    }

    switch (current_platform)
    {
        case PLATFORM_PS:
        create_ps4_select_button(gui_id, liveconfig);
        break;
        case PLATFORM_STEAMDECK:
        create_steamdeck_select_button(gui_id, liveconfig);
        break;
        case PLATFORM_SWITCH:
        create_switch_select_button(gui_id, liveconfig);
        break;
        default:
        create_xbox_select_button(gui_id, liveconfig);
    }
}

#pragma endregion

/*---------------------------------------------------------------------------*/

/* Shared */

static void init_xbox_title(void)
{
    if ((xbox_control_title_id = gui_hstack(0)))
    {
        /* Nintendo Switch has close software features,
         * so we don't need that */
        if (current_platform != PLATFORM_SWITCH
            && current_platform != PLATFORM_STEAMDECK)
        {
#if defined(__WII__)
            gui_label(xbox_control_title_id, _("Wii Menu"),
                      GUI_SML, gui_wht, gui_wht);
#else
            gui_label(xbox_control_title_id, _("Exit"),
                      GUI_SML, gui_wht, gui_wht);
#endif

            console_gui_create_b_button(xbox_control_title_id,
                            config_get_d(CONFIG_JOYSTICK_BUTTON_B), 1);

            create_controller_spacer(xbox_control_title_id);
        }

        gui_label(xbox_control_title_id, _("Select"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_a_button(xbox_control_title_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_A), 1);

        gui_set_rect(xbox_control_title_id, GUI_TOP);
        gui_layout(xbox_control_title_id, 0, -1);
    }
}

static void init_xbox_keybd(void)
{
    if ((xbox_control_keybd_id = gui_hstack(0)))
    {
        gui_label(xbox_control_keybd_id, _("OK"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_x_button(xbox_control_keybd_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_R2), 1);

        create_controller_spacer(xbox_control_list_id);

        gui_label(xbox_control_keybd_id, _("Shift"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_x_button(xbox_control_keybd_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_L2), 1);

        create_controller_spacer(xbox_control_list_id);

        gui_label(xbox_control_keybd_id, _("Backsp"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_x_button(xbox_control_keybd_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_X), 1);

        create_controller_spacer(xbox_control_list_id);

        gui_label(xbox_control_keybd_id, _("Cancel"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_b_button(xbox_control_keybd_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_B), 1);

        gui_set_rect(xbox_control_keybd_id, GUI_TOP);
        gui_layout(xbox_control_keybd_id, 0, -1);
    }
}

static void init_xbox_list(void)
{
    if ((xbox_control_list_id = gui_hstack(0)))
    {
        gui_label(xbox_control_list_id, _("Back"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_b_button(xbox_control_list_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_B), 1);

        create_controller_spacer(xbox_control_list_id);

        gui_label(xbox_control_list_id, _("Select"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_a_button(xbox_control_list_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_A), 1);

        gui_set_rect(xbox_control_list_id, GUI_TOP);
        gui_layout(xbox_control_list_id, 0, -1);
    }
}

static void init_xbox_levelopt(void)
{
    if ((xbox_control_levelopt_id = gui_hstack(0)))
    {
        gui_label(xbox_control_levelopt_id, _("Options"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_y_button(xbox_control_levelopt_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_Y), 1);

        create_controller_spacer(xbox_control_levelopt_id);

        gui_label(xbox_control_levelopt_id, _("Back"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_b_button(xbox_control_levelopt_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_B), 1);

        create_controller_spacer(xbox_control_levelopt_id);

        gui_label(xbox_control_levelopt_id, _("Select"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_a_button(xbox_control_levelopt_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_A), 1);

        gui_set_rect(xbox_control_levelopt_id, GUI_TOP);
        gui_layout(xbox_control_levelopt_id, 0, -1);
    }
}

static void init_xbox_paused(void)
{
    if ((xbox_control_paused_id = gui_hstack(0)))
    {
        gui_label(xbox_control_paused_id, _("Continue"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_b_button(xbox_control_paused_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_B), 1);

        create_controller_spacer(xbox_control_paused_id);

        gui_label(xbox_control_paused_id, _("Select"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_a_button(xbox_control_paused_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_A), 1);

        gui_set_rect(xbox_control_paused_id, GUI_TOP);
        gui_layout(xbox_control_paused_id, 0, -1);
    }
}

static void init_xbox_package_installable()
{
    if ((xbox_control_package_installable_id = gui_hstack(0)))
    {
        gui_label(xbox_control_package_installable_id, _("Back"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_b_button(xbox_control_package_installable_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_B), 1);

        create_controller_spacer(xbox_control_package_installable_id);

        gui_label(xbox_control_package_installable_id, _("Install"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_a_button(xbox_control_package_installable_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_A), 1);

        gui_set_rect(xbox_control_package_installable_id, GUI_TOP);
        gui_layout(xbox_control_package_installable_id, 0, -1);
    }
}

static void init_xbox_package_updateable()
{
    if ((xbox_control_package_updateable_id = gui_hstack(0)))
    {
        gui_label(xbox_control_package_updateable_id, _("Update"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_x_button(xbox_control_package_updateable_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_X), 1);

        create_controller_spacer(xbox_control_package_updateable_id);

        gui_label(xbox_control_package_updateable_id, _("Back"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_b_button(xbox_control_package_updateable_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_B), 1);

        create_controller_spacer(xbox_control_package_updateable_id);

        gui_label(xbox_control_package_updateable_id, _("Select"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_a_button(xbox_control_package_updateable_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_A), 1);

        gui_set_rect(xbox_control_package_updateable_id, GUI_TOP);
        gui_layout(xbox_control_package_updateable_id, 0, -1);
    }
}

static void init_xbox_package_manageable()
{
    if ((xbox_control_package_manageable_id = gui_hstack(0)))
    {
        gui_label(xbox_control_package_manageable_id, _("Manage"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_x_button(xbox_control_package_manageable_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_X), 1);

        create_controller_spacer(xbox_control_package_manageable_id);

        gui_label(xbox_control_package_manageable_id, _("Back"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_b_button(xbox_control_package_manageable_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_B), 1);

        gui_set_rect(xbox_control_package_manageable_id, GUI_TOP);
        gui_layout(xbox_control_package_manageable_id, 0, -1);
    }
}

static void init_xbox_package_equipable()
{
    if ((xbox_control_package_equipable_id = gui_hstack(0)))
    {
        gui_label(xbox_control_package_equipable_id, _("Equip/Manage"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_x_button(xbox_control_package_equipable_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_X), 1);

        create_controller_spacer(xbox_control_package_equipable_id);

        gui_label(xbox_control_package_equipable_id, _("Back"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_b_button(xbox_control_package_equipable_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_B), 1);
        
        gui_set_rect(xbox_control_package_equipable_id, GUI_TOP);
        gui_layout(xbox_control_package_equipable_id, 0, -1);
    }
}

static void init_xbox_package_startable()
{
    if ((xbox_control_package_startable_id = gui_hstack(0)))
    {
        gui_label(xbox_control_package_startable_id, _("Start/Manage"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_x_button(xbox_control_package_startable_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_X), 1);

        create_controller_spacer(xbox_control_package_startable_id);

        gui_label(xbox_control_package_startable_id, _("Back"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_b_button(xbox_control_package_startable_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_B), 1);
        
        gui_set_rect(xbox_control_package_startable_id, GUI_TOP);
        gui_layout(xbox_control_package_startable_id, 0, -1);
    }
}

/* Generic */

static void init_xbox_desc(void)
{
    if ((xbox_control_desc_id = gui_hstack(0)))
    {
        gui_label(xbox_control_desc_id, _("Show description"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_x_button(xbox_control_desc_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_X), 1);

        create_controller_spacer(xbox_control_desc_id);

        gui_label(xbox_control_desc_id, _("Back"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_b_button(xbox_control_desc_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_B), 1);

        create_controller_spacer(xbox_control_desc_id);

        gui_label(xbox_control_desc_id, _("Select"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_a_button(xbox_control_desc_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_A), 1);

        gui_set_rect(xbox_control_desc_id, GUI_TOP);
        gui_layout(xbox_control_desc_id, 0, -1);
    }
}

static void init_xbox_preparation(void)
{
    if ((xbox_control_preparation_id = gui_hstack(0)))
    {
        gui_label(xbox_control_preparation_id, _("Cycle Camera Mode"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_x_button(xbox_control_preparation_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_X), 1);

        create_controller_spacer(xbox_control_preparation_id);

        gui_label(xbox_control_preparation_id, _("Skip"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_a_button(xbox_control_preparation_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_A), 1);

        gui_set_rect(xbox_control_preparation_id, GUI_TOP);
        gui_layout(xbox_control_preparation_id, 0, -1);
    }
}

static void init_xbox_replay(void)
{
    if ((xbox_control_replay_id = gui_hstack(0)))
    {
        gui_label(xbox_control_replay_id, _("Pause"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_b_button(xbox_control_replay_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_B), 1);

        gui_set_rect(xbox_control_replay_id, GUI_TOP);
        gui_layout(xbox_control_replay_id, 0, -1);
    }
}

static void init_xbox_replay_eof(void)
{
    if ((xbox_control_replay_eof_id = gui_hstack(0)))
    {
        gui_label(xbox_control_replay_eof_id, _("Exit"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_b_button(xbox_control_replay_eof_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_B), 1);

        create_controller_spacer(xbox_control_replay_eof_id);

        gui_label(xbox_control_replay_eof_id, _("Select"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_a_button(xbox_control_replay_eof_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_A), 1);

        gui_set_rect(xbox_control_replay_eof_id, GUI_TOP);
        gui_layout(xbox_control_replay_eof_id, 0, -1);
    }
}

static void init_xbox_shop(void)
{
    if ((xbox_control_shop_id = gui_hstack(0)))
    {
        gui_label(xbox_control_shop_id, _("Get Coins"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_y_button(xbox_control_shop_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_Y), 1);

        create_controller_spacer(xbox_control_shop_id);

        gui_label(xbox_control_shop_id, _("Back"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_b_button(xbox_control_shop_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_B), 1);

        create_controller_spacer(xbox_control_shop_id);

        gui_label(xbox_control_shop_id, _("Buy"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_a_button(xbox_control_shop_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_A), 1);

        gui_set_rect(xbox_control_shop_id, GUI_TOP);
        gui_layout(xbox_control_shop_id, 0, -1);
    }
}

static void init_xbox_shop_getcoins(void)
{
    if ((xbox_control_shop_getcoins_id = gui_hstack(0)))
    {
        gui_label(xbox_control_shop_getcoins_id, _("Toggle Type"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_x_button(xbox_control_shop_getcoins_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_X), 1);

        create_controller_spacer(xbox_control_shop_getcoins_id);

        gui_label(xbox_control_shop_getcoins_id, _("Back"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_b_button(xbox_control_shop_getcoins_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_B), 1);

        create_controller_spacer(xbox_control_shop_getcoins_id);

        gui_label(xbox_control_shop_getcoins_id, _("Purchase"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_a_button(xbox_control_shop_getcoins_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_A), 1);
        
        gui_set_rect(xbox_control_shop_getcoins_id, GUI_TOP);
        gui_layout(xbox_control_shop_getcoins_id, 0, -1);
    }
}

static void init_xbox_model(void)
{
    if ((xbox_control_model_id = gui_hstack(0)))
    {
#if !defined(DISABLE_ONLINE_DATA) && NB_HAVE_PB_BOTH==1
        if (account_get_d(ACCOUNT_PRODUCT_BALLS) == 1)
        {
#if NB_STEAM_API==1 || NB_EOS_SDK==1
            const char *more_balls_text = server_policy_get_d(SERVER_POLICY_EDITION) == -1 ?
                                          "Upgrade to Home edition!" :
                                          "Go to Workshop!";
#else
            const char *more_balls_text = server_policy_get_d(SERVER_POLICY_EDITION) == -1 ?
                                          "Upgrade to Home edition!" :
                                          "Get more Balls!";
#endif
            gui_label(xbox_control_model_id, _(more_balls_text),
                      GUI_SML, gui_wht, gui_wht);
            console_gui_create_y_button(xbox_control_model_id,
                            config_get_d(CONFIG_JOYSTICK_BUTTON_Y), 1);

            create_controller_spacer(xbox_control_model_id);
        }
#endif

        gui_label(xbox_control_model_id, _("Change Model"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_rb_button(xbox_control_model_id,
                         config_get_d(CONFIG_JOYSTICK_BUTTON_R1), 1);
        console_gui_create_lb_button(xbox_control_model_id,
                         config_get_d(CONFIG_JOYSTICK_BUTTON_L1), 1);

        create_controller_spacer(xbox_control_model_id);

        gui_label(xbox_control_model_id, _("Back"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_b_button(xbox_control_model_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_B), 1);

        gui_set_rect(xbox_control_model_id, GUI_TOP);
        gui_layout(xbox_control_model_id, 0, -1);
    }
}

static void init_xbox_beam_style(void)
{
    if ((xbox_control_beam_style_id = gui_hstack(0)))
    {
        gui_label(xbox_control_beam_style_id, _("Change Style"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_rb_button(xbox_control_beam_style_id,
                                     config_get_d(CONFIG_JOYSTICK_BUTTON_R1), 1);
        console_gui_create_lb_button(xbox_control_beam_style_id,
                                     config_get_d(CONFIG_JOYSTICK_BUTTON_L1), 1);

        create_controller_spacer(xbox_control_beam_style_id);

        gui_label(xbox_control_beam_style_id, _("Back"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_b_button(xbox_control_beam_style_id,
                                    config_get_d(CONFIG_JOYSTICK_BUTTON_B), 1);

        gui_set_rect(xbox_control_beam_style_id, GUI_TOP);
        gui_layout(xbox_control_beam_style_id, 0, -1);
    }
}

static void init_xbox_death()
{
    if ((xbox_control_death_id = gui_hstack(0)))
    {
        gui_label(xbox_control_death_id, _("Select"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_a_button(xbox_control_death_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_A), 1);

        gui_set_rect(xbox_control_death_id, GUI_TOP);
        gui_layout(xbox_control_death_id, 0, -1);
    }
}

/* Putt */
static void init_xbox_putt_stroke()
{
    if ((xbox_control_putt_stroke_id = gui_hstack(0)))
    {
        gui_label(xbox_control_putt_stroke_id, _("Change club"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_lt_button(xbox_control_putt_stroke_id,
                         config_get_d(CONFIG_JOYSTICK_BUTTON_L2), 1);
        console_gui_create_lb_button(xbox_control_putt_stroke_id,
                         config_get_d(CONFIG_JOYSTICK_BUTTON_L1), 1);

        gui_label(xbox_control_putt_stroke_id, _("Shot"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_a_button(xbox_control_putt_stroke_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_A), 1);

        create_controller_spacer(xbox_control_putt_stroke_id);

        gui_label(xbox_control_putt_stroke_id, _("Fine aim"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_x_button(xbox_control_putt_stroke_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_X), 1);

        gui_set_rect(xbox_control_putt_stroke_id, GUI_TOP);
        gui_layout(xbox_control_putt_stroke_id, 0, -1);
    }
}

static void init_xbox_putt_stop()
{
    if ((xbox_control_putt_stop_id = gui_hstack(0)))
    {
        gui_label(xbox_control_putt_stop_id, _("Skip"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_a_button(xbox_control_putt_stop_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_A), 1);

        gui_set_rect(xbox_control_putt_stop_id, GUI_TOP);
        gui_layout(xbox_control_putt_stop_id, 0, -1);
    }
}

static void init_xbox_putt_scores()
{
    if ((xbox_control_putt_scores_id = gui_hstack(0)))
    {
        gui_label(xbox_control_putt_scores_id, _("Continue"),
                  GUI_SML, gui_wht, gui_wht);

        console_gui_create_a_button(xbox_control_putt_scores_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_A), 1);

        gui_set_rect(xbox_control_putt_scores_id, GUI_TOP);
        gui_layout(xbox_control_putt_scores_id, 0, -1);
    }
}

/*---------------------------------------------------------------------------*/

int console_gui_shown(void)
{
    return show_control_gui;
}

void console_gui_toggle(int active)
{
    if (show_control_gui == active)
        return;

#if PENNYBALL_FAMILY_API != PENNYBALL_PC_FAMILY_API
    show_control_gui = 1;
#else
    show_control_gui = active;
#endif

    console_gui_slide(GUI_S | GUI_EASE_ELASTIC | (active ? 0 : GUI_BACKWARD));
}

void console_gui_init(void)
{
    /* Shared */
    init_xbox_title();
    init_xbox_keybd();
    init_xbox_list();
    init_xbox_levelopt();
    init_xbox_paused();

    init_xbox_package_installable();
    init_xbox_package_updateable();
    init_xbox_package_manageable();
    init_xbox_package_equipable();
    init_xbox_package_startable();

    /* Generic */
    init_xbox_desc();
    init_xbox_preparation();
    init_xbox_replay();
    init_xbox_replay_eof();

    init_xbox_shop();
    init_xbox_shop_getcoins();

    init_xbox_model();
    init_xbox_beam_style();

    init_xbox_death();

    /* Putt */
    init_xbox_putt_stroke();
    init_xbox_putt_stop();
    init_xbox_putt_scores();

    if (strlen(config_get_s(CONFIG_WIIMOTE_ADDR)) == 17)
        show_control_gui = 1;
}

void console_gui_free(void)
{
    /* Shared */
    gui_delete(xbox_control_title_id);
    gui_delete(xbox_control_keybd_id);
    gui_delete(xbox_control_list_id);
    gui_delete(xbox_control_levelopt_id);
    gui_delete(xbox_control_paused_id);
    gui_delete(xbox_control_package_installable_id);
    gui_delete(xbox_control_package_updateable_id);
    gui_delete(xbox_control_package_manageable_id);
    gui_delete(xbox_control_package_equipable_id);
    gui_delete(xbox_control_package_startable_id);

    /* Generic */
    gui_delete(xbox_control_desc_id);
    gui_delete(xbox_control_preparation_id);
    gui_delete(xbox_control_replay_id);
    gui_delete(xbox_control_replay_eof_id);
    gui_delete(xbox_control_shop_id);
    gui_delete(xbox_control_shop_getcoins_id);
    gui_delete(xbox_control_model_id);
    gui_delete(xbox_control_beam_style_id);
    gui_delete(xbox_control_death_id);

    /* Putt */
    gui_delete(xbox_control_putt_stroke_id);
    gui_delete(xbox_control_putt_stop_id);
    gui_delete(xbox_control_putt_scores_id);

    for (int i = 0; i < GAMEPAD_MAX_BTN_GUI_IDS; i++)
        for (int j = 0; j < 4; j++) {
            memset(&gamepad_btn_gui_ids_a[i],        0, sizeof (gamepad_btn_gui_ids_a[i]));
            memset(&gamepad_btn_gui_ids_b[i],        0, sizeof (gamepad_btn_gui_ids_b[i]));
            memset(&gamepad_btn_gui_ids_x[i],        0, sizeof (gamepad_btn_gui_ids_x[i]));
            memset(&gamepad_btn_gui_ids_y[i],        0, sizeof (gamepad_btn_gui_ids_y[i]));
            memset(&gamepad_btn_gui_ids_lb[i],       0, sizeof (gamepad_btn_gui_ids_lb[i]));
            memset(&gamepad_btn_gui_ids_rb[i],       0, sizeof (gamepad_btn_gui_ids_rb[i]));
            memset(&gamepad_btn_gui_ids_lt[i],       0, sizeof (gamepad_btn_gui_ids_lt[i]));
            memset(&gamepad_btn_gui_ids_rt[i],       0, sizeof (gamepad_btn_gui_ids_rt[i]));
            memset(&gamepad_btn_gui_ids_ls[i],       0, sizeof (gamepad_btn_gui_ids_ls[i]));
            memset(&gamepad_btn_gui_ids_rs[i],       0, sizeof (gamepad_btn_gui_ids_rs[i]));
            memset(&gamepad_btn_gui_ids_dpads[j][i], 0, sizeof (gamepad_btn_gui_ids_dpads[j][i]));

            gamepad_btn_gui_ids_a[i].btn_id        = -1;
            gamepad_btn_gui_ids_b[i].btn_id        = -1;
            gamepad_btn_gui_ids_x[i].btn_id        = -1;
            gamepad_btn_gui_ids_y[i].btn_id        = -1;
            gamepad_btn_gui_ids_lb[i].btn_id       = -1;
            gamepad_btn_gui_ids_rb[i].btn_id       = -1;
            gamepad_btn_gui_ids_lt[i].btn_id       = -1;
            gamepad_btn_gui_ids_rt[i].btn_id       = -1;
            gamepad_btn_gui_ids_ls[i].btn_id       = -1;
            gamepad_btn_gui_ids_rs[i].btn_id       = -1;
            gamepad_btn_gui_ids_dpads[j][i].btn_id = -1;

            gamepad_btn_gui_ids_a[i].gui_id        = -1;
            gamepad_btn_gui_ids_b[i].gui_id        = -1;
            gamepad_btn_gui_ids_x[i].gui_id        = -1;
            gamepad_btn_gui_ids_y[i].gui_id        = -1;
            gamepad_btn_gui_ids_lb[i].gui_id       = -1;
            gamepad_btn_gui_ids_rb[i].gui_id       = -1;
            gamepad_btn_gui_ids_lt[i].gui_id       = -1;
            gamepad_btn_gui_ids_rt[i].gui_id       = -1;
            gamepad_btn_gui_ids_ls[i].gui_id       = -1;
            gamepad_btn_gui_ids_rs[i].gui_id       = -1;
            gamepad_btn_gui_ids_dpads[j][i].gui_id = -1;
        }
}

void console_gui_slide(int flags)
{
    /* Shared */
    gui_slide(xbox_control_title_id,               flags, 0, 0.3f, 0);
    gui_slide(xbox_control_keybd_id,               flags, 0, 0.3f, 0);
    gui_slide(xbox_control_list_id,                flags, 0, 0.3f, 0);
    gui_slide(xbox_control_levelopt_id,            flags, 0, 0.3f, 0);
    gui_slide(xbox_control_paused_id,              flags, 0, 0.3f, 0);
    gui_slide(xbox_control_package_installable_id, flags, 0, 0.3f, 0);
    gui_slide(xbox_control_package_updateable_id,  flags, 0, 0.3f, 0);
    gui_slide(xbox_control_package_manageable_id,  flags, 0, 0.3f, 0);
    gui_slide(xbox_control_package_equipable_id,   flags, 0, 0.3f, 0);
    gui_slide(xbox_control_package_startable_id,   flags, 0, 0.3f, 0);

    /* Generic */
    gui_slide(xbox_control_desc_id,          flags, 0, 0.3f, 0);
    gui_slide(xbox_control_preparation_id,   flags, 0, 0.3f, 0);
    gui_slide(xbox_control_replay_id,        flags, 0, 0.3f, 0);
    gui_slide(xbox_control_replay_eof_id,    flags, 0, 0.3f, 0);
    gui_slide(xbox_control_shop_id,          flags, 0, 0.3f, 0);
    gui_slide(xbox_control_shop_getcoins_id, flags, 0, 0.3f, 0);
    gui_slide(xbox_control_model_id,         flags, 0, 0.3f, 0);
    gui_slide(xbox_control_beam_style_id,    flags, 0, 0.3f, 0);
    gui_slide(xbox_control_death_id,         flags, 0, 0.3f, 0);

    /* Putt */
    gui_slide(xbox_control_putt_stroke_id, flags, 0, 0.3f, 0);
    gui_slide(xbox_control_putt_stop_id,   flags, 0, 0.3f, 0);
    gui_slide(xbox_control_putt_scores_id, flags, 0, 0.3f, 0);
}

void console_gui_timer(float dt)
{
    /* Shared */
    gui_timer(xbox_control_title_id, dt);
    gui_timer(xbox_control_keybd_id, dt);
    gui_timer(xbox_control_list_id, dt);
    gui_timer(xbox_control_levelopt_id, dt);
    gui_timer(xbox_control_paused_id, dt);
    gui_timer(xbox_control_package_installable_id, dt);
    gui_timer(xbox_control_package_updateable_id, dt);
    gui_timer(xbox_control_package_manageable_id, dt);
    gui_timer(xbox_control_package_equipable_id, dt);
    gui_timer(xbox_control_package_startable_id, dt);

    /* Generic */
    gui_timer(xbox_control_desc_id, dt);
    gui_timer(xbox_control_preparation_id, dt);
    gui_timer(xbox_control_replay_id, dt);
    gui_timer(xbox_control_replay_eof_id, dt);
    gui_timer(xbox_control_shop_id, dt);
    gui_timer(xbox_control_shop_getcoins_id, dt);
    gui_timer(xbox_control_model_id, dt);
    gui_timer(xbox_control_beam_style_id, dt);
    gui_timer(xbox_control_death_id, dt);

    /* Putt */
    gui_timer(xbox_control_putt_stroke_id, dt);
    gui_timer(xbox_control_putt_stop_id, dt);
    gui_timer(xbox_control_putt_scores_id, dt);
}

/*---------------------------------------------------------------------------*/

/* Shared */

void console_gui_title_paint(void)
{
    if (show_control_gui || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_paint(xbox_control_title_id);
}

void console_gui_keybd_paint(void)
{
    if (show_control_gui || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_paint(xbox_control_keybd_id);
}

void console_gui_list_paint(void)
{
    if (show_control_gui || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_paint(xbox_control_list_id);
}

void console_gui_levelopt_paint(void)
{
    if (show_control_gui || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_paint(xbox_control_levelopt_id);
}

void console_gui_paused_paint(void)
{
    if (show_control_gui || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_paint(xbox_control_paused_id);
}

void console_gui_package_installable_paint(void)
{
    if (show_control_gui || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_paint(xbox_control_package_installable_id);
}

void console_gui_package_updateable_paint(void)
{
    if (show_control_gui || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_paint(xbox_control_package_updateable_id);
}

void console_gui_package_manageable_paint(void)
{
    if (show_control_gui || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_paint(xbox_control_package_manageable_id);
}

void console_gui_package_equipable_paint(void)
{
    if (show_control_gui || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_paint(xbox_control_package_equipable_id);
}

void console_gui_package_startable_paint(void)
{
    if (show_control_gui || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_paint(xbox_control_package_startable_id);
}

/* Generic */

void console_gui_desc_paint(void)
{
    if (show_control_gui || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_paint(xbox_control_desc_id);
}

void console_gui_preparation_paint(void)
{
    if (show_control_gui || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_paint(xbox_control_preparation_id);
}

void console_gui_replay_paint(void)
{
    if (show_control_gui || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_paint(xbox_control_replay_id);
}

void console_gui_replay_eof_paint(void)
{
    if (show_control_gui || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_paint(xbox_control_replay_eof_id);
}

void console_gui_shop_paint(void)
{
    if (show_control_gui || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_paint(xbox_control_shop_id);
}

void console_gui_shop_getcoins_paint(void)
{
    if (show_control_gui || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_paint(xbox_control_shop_getcoins_id);
}

void console_gui_model_paint(void)
{
    if (show_control_gui || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_paint(xbox_control_model_id);
}

void console_gui_beam_style_paint(void)
{
    if (show_control_gui || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_paint(xbox_control_beam_style_id);
}

void console_gui_death_paint(void)
{
    if (show_control_gui || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_paint(xbox_control_death_id);
}

/* Putt */
void console_gui_putt_stroke_paint(void)
{
    if (show_control_gui || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_paint(xbox_control_putt_stroke_id);
}

void console_gui_putt_stop_paint(void)
{
    if (show_control_gui || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_paint(xbox_control_putt_stop_id);
}

void console_gui_putt_scores_paint(void)
{
    if (show_control_gui || config_get_d(CONFIG_SCREEN_ANIMATIONS))
        gui_paint(xbox_control_putt_scores_id);
}
