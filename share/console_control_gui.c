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

#include "console_control_gui.h"

#if NB_HAVE_PB_BOTH==1
#include "networking.h"
#endif

#include "gui.h"
#include "audio.h"
#include "config.h"
#include "video.h"
#include "cmd.h"

//#define DISABLE_ONLINE_DATA

#if !defined(DISABLE_ONLINE_DATA)
#include "account.h"
#endif

/*---------------------------------------------------------------------------*/

enum console_platforms current_platform;

static int show_control_gui = 0;


/* Shared */

static int xbox_control_title_id = 0;
static int xbox_control_list_id = 0;
static int xbox_control_paused_id = 0;


/* Generic */

static int xbox_control_desc_id = 0;
static int xbox_control_preparation_id = 0;
static int xbox_control_replay_id = 0;
static int xbox_control_replay_eof_id = 0;
static int xbox_control_shop_id = 0;
static int xbox_control_shop_getcoins_id = 0;
static int xbox_control_model_id = 0;
static int xbox_control_beam_style_id = 0;
static int xbox_control_death_id = 0;


/* Putt */

static int xbox_control_putt_stroke_id = 0;
static int xbox_control_putt_stop_id = 0;
static int xbox_control_putt_scores_id = 0;

/*---------------------------------------------------------------------------*/

void init_controller_type(enum console_platforms new_platforms)
{
    current_platform = new_platforms;
}

void xbox_control_gui_set_alpha(float alpha)
{
    if (current_platform == PLATFORM_PC) return;

    /* Shared */

    gui_set_alpha(xbox_control_title_id, alpha, GUI_ANIMATION_S_LINEAR);
    gui_set_alpha(xbox_control_list_id, alpha, GUI_ANIMATION_S_LINEAR);
    gui_set_alpha(xbox_control_paused_id, alpha, GUI_ANIMATION_S_LINEAR);


    /* Generic */

    gui_set_alpha(xbox_control_desc_id, alpha, GUI_ANIMATION_S_LINEAR);
    gui_set_alpha(xbox_control_preparation_id, alpha, GUI_ANIMATION_S_LINEAR);
    gui_set_alpha(xbox_control_replay_id, alpha, GUI_ANIMATION_S_LINEAR);
    gui_set_alpha(xbox_control_replay_eof_id, alpha, GUI_ANIMATION_S_LINEAR);
    gui_set_alpha(xbox_control_shop_id, alpha, GUI_ANIMATION_S_LINEAR);
    gui_set_alpha(xbox_control_shop_getcoins_id, alpha, GUI_ANIMATION_S_LINEAR);
    gui_set_alpha(xbox_control_model_id, alpha, GUI_ANIMATION_S_LINEAR);
    gui_set_alpha(xbox_control_beam_style_id, alpha, GUI_ANIMATION_S_LINEAR);
    gui_set_alpha(xbox_control_death_id, alpha, GUI_ANIMATION_S_LINEAR);


    /* Putt */

    gui_set_alpha(xbox_control_putt_stroke_id, alpha, GUI_ANIMATION_S_LINEAR);
    gui_set_alpha(xbox_control_putt_stop_id, alpha, GUI_ANIMATION_S_LINEAR);
    gui_set_alpha(xbox_control_putt_scores_id, alpha, GUI_ANIMATION_S_LINEAR);
}

/*---------------------------------------------------------------------------*/

void create_controller_spacer(int space_id)
{
    for (int i = 0; i < 1; i++)
        gui_space(space_id);
}

/*---------------------------------------------------------------------------*/

#pragma region Console Platforms

/* Etihad Handset controllers */

void create_handset_a_button(int a_id)
{
    gui_label(a_id, HANDSET_A_BUTTON, GUI_SML, gui_red, gui_red);
}

void create_handset_b_button(int b_id)
{
    gui_label(b_id, HANDSET_B_BUTTON, GUI_SML, gui_yel, gui_yel);
}

void create_handset_x_button(int x_id)
{
    gui_label(x_id, HANDSET_X_BUTTON, GUI_SML, gui_blu, gui_blu);
}

void create_handset_y_button(int y_id)
{
    gui_label(y_id, HANDSET_Y_BUTTON, GUI_SML, gui_grn, gui_grn);
}

void create_handset_lb_button(int lb_id)
{
    gui_label(lb_id, HANDSET_LB_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_handset_rb_button(int rb_id)
{
    gui_label(rb_id, HANDSET_RB_BUTTON, GUI_SML, gui_gry, gui_wht);
}

/*---------------------------------------------------------------------------*/

/* Xbox controllers */

void create_xbox_a_button(int a_id)
{
    gui_label(a_id, XBOX_A_BUTTON, GUI_SML, gui_grn, gui_grn);
}

void create_xbox_b_button(int b_id)
{
    gui_label(b_id, XBOX_B_BUTTON, GUI_SML, gui_red, gui_red);
}

void create_xbox_x_button(int x_id)
{
    gui_label(x_id, XBOX_X_BUTTON, GUI_SML, gui_blu, gui_blu);
}

void create_xbox_y_button(int y_id)
{
    gui_label(y_id, XBOX_Y_BUTTON, GUI_SML, gui_yel, gui_yel);
}

void create_xbox_lb_button(int lb_id)
{
    gui_label(lb_id, XBOX_LB_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_xbox_rb_button(int rb_id)
{
    gui_label(rb_id, XBOX_RB_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_xbox_lt_button(int lb_id)
{
    gui_label(lb_id, XBOX_LT_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_xbox_rt_button(int rb_id)
{
    gui_label(rb_id, XBOX_RT_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_xbox_ls_button(int ls_id)
{
    gui_label(ls_id, XBOX_LS_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_xbox_rs_button(int rs_id)
{
    gui_label(rs_id, XBOX_RS_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_xbox_start_button(int start_id)
{
    gui_label(start_id, XBOX_START_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_xbox_select_button(int select_id)
{
    gui_label(select_id, XBOX_SELECT_BUTTON, GUI_SML, gui_gry, gui_wht);
}

/*---------------------------------------------------------------------------*/

/* PlayStation Controllers */

void create_ps4_a_button(int a_id)
{
    gui_label(a_id, PS4_A_BUTTON, GUI_SML, gui_blu, gui_blu);
}

void create_ps4_b_button(int b_id)
{
    gui_label(b_id, PS4_B_BUTTON, GUI_SML, gui_red, gui_red);
}

void create_ps4_x_button(int x_id)
{
    gui_label(x_id, PS4_X_BUTTON, GUI_SML, gui_pnk, gui_pnk);
}

void create_ps4_y_button(int y_id)
{
    gui_label(y_id, PS4_Y_BUTTON, GUI_SML, gui_cya, gui_cya);
}

void create_ps4_lb_button(int lb_id)
{
    gui_label(lb_id, PS4_LB_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_ps4_rb_button(int rb_id)
{
    gui_label(rb_id, PS4_RB_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_ps4_lt_button(int lb_id)
{
    gui_label(lb_id, PS4_LT_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_ps4_rt_button(int rb_id)
{
    gui_label(rb_id, PS4_RT_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_ps4_ls_button(int ls_id)
{
    gui_label(ls_id, PS4_LS_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_ps4_rs_button(int rs_id)
{
    gui_label(rs_id, PS4_RS_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_ps4_start_button(int start_id)
{
    gui_label(start_id, PS4_START_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_ps4_select_button(int select_id)
{
    gui_label(select_id, PS4_SELECT_BUTTON, GUI_SML, gui_gry, gui_wht);
}

/*---------------------------------------------------------------------------*/

void create_steamdeck_a_button(int a_id)
{
    gui_label(a_id, STEAMDECK_A_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_steamdeck_b_button(int b_id)
{
    gui_label(b_id, STEAMDECK_B_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_steamdeck_x_button(int x_id)
{
    gui_label(x_id, STEAMDECK_X_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_steamdeck_y_button(int y_id)
{
    gui_label(y_id, STEAMDECK_Y_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_steamdeck_lb_button(int lb_id)
{
    gui_label(lb_id, STEAMDECK_LB_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_steamdeck_rb_button(int rb_id)
{
    gui_label(rb_id, STEAMDECK_RB_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_steamdeck_lt_button(int lb_id)
{
    gui_label(lb_id, STEAMDECK_LT_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_steamdeck_rt_button(int rb_id)
{
    gui_label(rb_id, STEAMDECK_RT_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_steamdeck_ls_button(int ls_id)
{
    gui_label(ls_id, STEAMDECK_LS_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_steamdeck_rs_button(int rs_id)
{
    gui_label(rs_id, STEAMDECK_RS_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_steamdeck_start_button(int start_id)
{
    gui_label(start_id, STEAMDECK_START_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_steamdeck_select_button(int select_id)
{
    gui_label(select_id, STEAMDECK_SELECT_BUTTON, GUI_SML, gui_gry, gui_wht);
}

/*---------------------------------------------------------------------------*/

/* Nintendo Controllers */

void create_switch_a_button(int a_id)
{
    gui_label(a_id, SWITCH_A_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_switch_b_button(int b_id)
{
    gui_label(b_id, SWITCH_B_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_switch_x_button(int x_id)
{
    gui_label(x_id, SWITCH_X_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_switch_y_button(int y_id)
{
    gui_label(y_id, SWITCH_Y_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_switch_lb_button(int lb_id)
{
    gui_label(lb_id, SWITCH_LB_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_switch_rb_button(int rb_id)
{
    gui_label(rb_id, SWITCH_RB_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_switch_lt_button(int lb_id)
{
    gui_label(lb_id, SWITCH_LT_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_switch_rt_button(int rb_id)
{
    gui_label(rb_id, SWITCH_RT_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_switch_ls_button(int ls_id)
{
    gui_label(ls_id, SWITCH_LS_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_switch_rs_button(int rs_id)
{
    gui_label(rs_id, SWITCH_RS_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_switch_start_button(int start_id)
{
    gui_label(start_id, SWITCH_START_BUTTON, GUI_SML, gui_gry, gui_wht);
}

void create_switch_select_button(int select_id)
{
    gui_label(select_id, SWITCH_SELECT_BUTTON, GUI_SML, gui_gry, gui_wht);
}

/*---------------------------------------------------------------------------*/

/* Generic Controllers */

void create_a_button(int gui_id, int btn_id)
{
    switch (btn_id)
    {
        case 1:
        create_b_button(gui_id, btn_id);
        return;
        case 2:
        create_x_button(gui_id, btn_id);
        return;
        case 3:
        create_y_button(gui_id, btn_id);
        return;
        case 4:
        create_lb_button(gui_id, btn_id);
        return;
        case 5:
        create_lt_button(gui_id, btn_id);
        return;
        case 6:
        create_rb_button(gui_id, btn_id);
        return;
        case 7:
        create_rt_button(gui_id, btn_id);
        return;
        case 8:
        create_start_button(gui_id, btn_id);
        return;
        case 9:
        create_select_button(gui_id, btn_id);
        return;
        case 13:
        create_ls_button(gui_id, btn_id);
        return;
        case 15:
        create_rs_button(gui_id, btn_id);
        return;
    }

    switch (current_platform)
    {
        case PLATFORM_HANDSET:
        create_handset_y_button(gui_id);
        break;
        case PLATFORM_XBOX:
        create_xbox_a_button(gui_id);
        break;
        case PLATFORM_PS:
        create_ps4_a_button(gui_id);
        break;
        case PLATFORM_STEAMDECK:
        create_steamdeck_a_button(gui_id);
        break;
        case PLATFORM_SWITCH:
        create_switch_a_button(gui_id);
        break;
    }
}

void create_b_button(int gui_id, int btn_id)
{
    switch (btn_id)
    {
        case 0:
        create_a_button(gui_id, btn_id);
        return;
        case 2:
        create_x_button(gui_id, btn_id);
        return;
        case 3:
        create_y_button(gui_id, btn_id);
        return;
        case 4:
        create_lb_button(gui_id, btn_id);
        return;
        case 5:
        create_lt_button(gui_id, btn_id);
        return;
        case 6:
        create_rb_button(gui_id, btn_id);
        return;
        case 7:
        create_rt_button(gui_id, btn_id);
        return;
        case 8:
        create_start_button(gui_id, btn_id);
        return;
        case 9:
        create_select_button(gui_id, btn_id);
        return;
        case 13:
        create_ls_button(gui_id, btn_id);
        return;
        case 15:
        create_rs_button(gui_id, btn_id);
        return;
    }

    switch (current_platform)
    {
        case PLATFORM_HANDSET:
        create_handset_x_button(gui_id);
        break;
        case PLATFORM_XBOX:
        create_xbox_b_button(gui_id);
        break;
        case PLATFORM_PS:
        create_ps4_b_button(gui_id);
        break;
        case PLATFORM_STEAMDECK:
        create_steamdeck_b_button(gui_id);
        break;
        case PLATFORM_SWITCH:
        create_switch_b_button(gui_id);
        break;
    }
}

void create_x_button(int gui_id, int btn_id)
{
    switch (btn_id)
    {
        case 0:
        create_a_button(gui_id, btn_id);
        return;
        case 1:
        create_b_button(gui_id, btn_id);
        return;
        case 3:
        create_y_button(gui_id, btn_id);
        return;
        case 4:
        create_lb_button(gui_id, btn_id);
        return;
        case 5:
        create_lt_button(gui_id, btn_id);
        return;
        case 6:
        create_rb_button(gui_id, btn_id);
        return;
        case 7:
        create_rt_button(gui_id, btn_id);
        return;
        case 8:
        create_start_button(gui_id, btn_id);
        return;
        case 9:
        create_select_button(gui_id, btn_id);
        return;
        case 13:
        create_ls_button(gui_id, btn_id);
        return;
        case 15:
        create_rs_button(gui_id, btn_id);
        return;
    }

    switch (current_platform)
    {
        case PLATFORM_HANDSET:
        create_handset_b_button(gui_id);
        break;
        case PLATFORM_XBOX:
        create_xbox_x_button(gui_id);
        break;
        case PLATFORM_PS:
        create_ps4_x_button(gui_id);
        break;
        case PLATFORM_STEAMDECK:
        create_steamdeck_x_button(gui_id);
        break;
        case PLATFORM_SWITCH:
        create_switch_x_button(gui_id);
        break;
    }
}

void create_y_button(int gui_id, int btn_id)
{
    switch (btn_id)
    {
        case 0:
        create_a_button(gui_id, btn_id);
        return;
        case 1:
        create_b_button(gui_id, btn_id);
        return;
        case 2:
        create_x_button(gui_id, btn_id);
        return;
        case 4:
        create_lb_button(gui_id, btn_id);
        return;
        case 5:
        create_lt_button(gui_id, btn_id);
        return;
        case 6:
        create_rb_button(gui_id, btn_id);
        return;
        case 7:
        create_rt_button(gui_id, btn_id);
        return;
        case 8:
        create_start_button(gui_id, btn_id);
        return;
        case 9:
        create_select_button(gui_id, btn_id);
        return;
        case 13:
        create_ls_button(gui_id, btn_id);
        return;
        case 15:
        create_rs_button(gui_id, btn_id);
        return;
    }

    switch (current_platform)
    {
        case PLATFORM_HANDSET:
        create_handset_a_button(gui_id);
        break;
        case PLATFORM_XBOX:
        create_xbox_y_button(gui_id);
        break;
        case PLATFORM_PS:
        create_ps4_y_button(gui_id);
        break;
        case PLATFORM_STEAMDECK:
        create_steamdeck_y_button(gui_id);
        break;
        case PLATFORM_SWITCH:
        create_switch_y_button(gui_id);
        break;
    }
}

void create_lb_button(int gui_id, int btn_id)
{
    switch (btn_id)
    {
        case 0:
        create_a_button(gui_id, btn_id);
        return;
        case 1:
        create_b_button(gui_id, btn_id);
        return;
        case 2:
        create_x_button(gui_id, btn_id);
        return;
        case 3:
        create_y_button(gui_id, btn_id);
        return;
        case 5:
        create_lt_button(gui_id, btn_id);
        return;
        case 6:
        create_rb_button(gui_id, btn_id);
        return;
        case 7:
        create_rt_button(gui_id, btn_id);
        return;
        case 8:
        create_start_button(gui_id, btn_id);
        return;
        case 9:
        create_select_button(gui_id, btn_id);
        return;
        case 13:
        create_ls_button(gui_id, btn_id);
        return;
        case 15:
        create_rs_button(gui_id, btn_id);
        return;
    }

    switch (current_platform)
    {
        case PLATFORM_HANDSET:
        create_handset_lb_button(gui_id);
        break;
        case PLATFORM_XBOX:
        create_xbox_lb_button(gui_id);
        break;
        case PLATFORM_PS:
        create_ps4_lb_button(gui_id);
        break;
        case PLATFORM_STEAMDECK:
        create_steamdeck_lb_button(gui_id);
        break;
        case PLATFORM_SWITCH:
        create_switch_lb_button(gui_id);
        break;
    }
}

void create_rb_button(int gui_id, int btn_id)
{
    switch (btn_id)
    {
        case 0:
        create_a_button(gui_id, btn_id);
        return;
        case 1:
        create_b_button(gui_id, btn_id);
        return;
        case 2:
        create_x_button(gui_id, btn_id);
        return;
        case 3:
        create_y_button(gui_id, btn_id);
        return;
        case 4:
        create_lb_button(gui_id, btn_id);
        return;
        case 5:
        create_lt_button(gui_id, btn_id);
        return;
        case 7:
        create_rt_button(gui_id, btn_id);
        return;
        case 8:
        create_start_button(gui_id, btn_id);
        return;
        case 9:
        create_select_button(gui_id, btn_id);
        return;
        case 13:
        create_ls_button(gui_id, btn_id);
        return;
        case 15:
        create_rs_button(gui_id, btn_id);
        return;
    }

    switch (current_platform)
    {
        case PLATFORM_HANDSET:
        create_handset_rb_button(gui_id);
        break;
        case PLATFORM_XBOX:
        create_xbox_rb_button(gui_id);
        break;
        case PLATFORM_PS:
        create_ps4_rb_button(gui_id);
        break;
        case PLATFORM_STEAMDECK:
        create_steamdeck_rb_button(gui_id);
        break;
        case PLATFORM_SWITCH:
        create_switch_rb_button(gui_id);
        break;
    }
}

void create_lt_button(int gui_id, int btn_id)
{
    switch (btn_id)
    {
        case 0:
        create_a_button(gui_id, btn_id);
        return;
        case 1:
        create_b_button(gui_id, btn_id);
        return;
        case 2:
        create_x_button(gui_id, btn_id);
        return;
        case 3:
        create_y_button(gui_id, btn_id);
        return;
        case 4:
        create_lb_button(gui_id, btn_id);
        return;
        case 6:
        create_rb_button(gui_id, btn_id);
        return;
        case 7:
        create_rt_button(gui_id, btn_id);
        return;
        case 8:
        create_start_button(gui_id, btn_id);
        return;
        case 9:
        create_select_button(gui_id, btn_id);
        return;
        case 13:
        create_ls_button(gui_id, btn_id);
        return;
        case 15:
        create_rs_button(gui_id, btn_id);
        return;
    }

    switch (current_platform)
    {
        case PLATFORM_XBOX:
        create_xbox_lt_button(gui_id);
        break;
        case PLATFORM_PS:
        create_ps4_lt_button(gui_id);
        break;
        case PLATFORM_STEAMDECK:
        create_steamdeck_lt_button(gui_id);
        break;
        case PLATFORM_SWITCH:
        create_switch_lt_button(gui_id);
        break;
    }
}

void create_rt_button(int gui_id, int btn_id)
{
    switch (btn_id)
    {
        case 0:
        create_a_button(gui_id, btn_id);
        return;
        case 1:
        create_b_button(gui_id, btn_id);
        return;
        case 2:
        create_x_button(gui_id, btn_id);
        return;
        case 3:
        create_y_button(gui_id, btn_id);
        return;
        case 4:
        create_lb_button(gui_id, btn_id);
        return;
        case 5:
        create_lt_button(gui_id, btn_id);
        return;
        case 6:
        create_rb_button(gui_id, btn_id);
        return;
        case 8:
        create_start_button(gui_id, btn_id);
        return;
        case 9:
        create_select_button(gui_id, btn_id);
        return;
        case 13:
        create_ls_button(gui_id, btn_id);
        return;
        case 15:
        create_rs_button(gui_id, btn_id);
        return;
    }

    switch (current_platform)
    {
        case PLATFORM_XBOX:
        create_xbox_rt_button(gui_id);
        break;
        case PLATFORM_PS:
        create_ps4_rt_button(gui_id);
        break;
        case PLATFORM_STEAMDECK:
        create_steamdeck_rt_button(gui_id);
        break;
        case PLATFORM_SWITCH:
        create_switch_rt_button(gui_id);
        break;
    }
}

void create_ls_button(int gui_id, int btn_id)
{
    switch (btn_id)
    {
        case 0:
        create_a_button(gui_id, btn_id);
        return;
        case 1:
        create_b_button(gui_id, btn_id);
        return;
        case 2:
        create_x_button(gui_id, btn_id);
        return;
        case 3:
        create_y_button(gui_id, btn_id);
        return;
        case 4:
        create_lb_button(gui_id, btn_id);
        return;
        case 5:
        create_lt_button(gui_id, btn_id);
        return;
        case 6:
        create_rb_button(gui_id, btn_id);
        return;
        case 7:
        create_rt_button(gui_id, btn_id);
        return;
        case 8:
        create_start_button(gui_id, btn_id);
        return;
        case 9:
        create_select_button(gui_id, btn_id);
        return;
        case 15:
        create_rs_button(gui_id, btn_id);
        return;
    }

    switch (current_platform)
    {
        case PLATFORM_XBOX:
        create_xbox_ls_button(gui_id);
        break;
        case PLATFORM_PS:
        create_ps4_ls_button(gui_id);
        break;
        case PLATFORM_STEAMDECK:
        create_steamdeck_ls_button(gui_id);
        break;
        case PLATFORM_SWITCH:
        create_switch_ls_button(gui_id);
        break;
    }
}

void create_rs_button(int gui_id, int btn_id)
{
    switch (btn_id)
    {
        case 0:
        create_a_button(gui_id, btn_id);
        return;
        case 1:
        create_b_button(gui_id, btn_id);
        return;
        case 2:
        create_x_button(gui_id, btn_id);
        return;
        case 3:
        create_y_button(gui_id, btn_id);
        return;
        case 4:
        create_lb_button(gui_id, btn_id);
        return;
        case 5:
        create_lt_button(gui_id, btn_id);
        return;
        case 6:
        create_rb_button(gui_id, btn_id);
        return;
        case 7:
        create_rt_button(gui_id, btn_id);
        return;
        case 8:
        create_start_button(gui_id, btn_id);
        return;
        case 9:
        create_select_button(gui_id, btn_id);
        return;
        case 13:
        create_ls_button(gui_id, btn_id);
        return;
    }

    switch (current_platform)
    {
        case PLATFORM_XBOX:
        create_xbox_rs_button(gui_id);
        break;
        case PLATFORM_PS:
        create_ps4_rs_button(gui_id);
        break;
        case PLATFORM_STEAMDECK:
        create_steamdeck_rs_button(gui_id);
        break;
        case PLATFORM_SWITCH:
        create_switch_rs_button(gui_id);
        break;
    }
}

void create_start_button(int gui_id, int btn_id)
{
    switch (btn_id)
    {
        case 0:
        create_a_button(gui_id, btn_id);
        return;
        case 1:
        create_b_button(gui_id, btn_id);
        return;
        case 2:
        create_x_button(gui_id, btn_id);
        return;
        case 3:
        create_y_button(gui_id, btn_id);
        return;
        case 4:
        create_lb_button(gui_id, btn_id);
        return;
        case 5:
        create_rb_button(gui_id, btn_id);
        return;
        case 7:
        create_select_button(gui_id, btn_id);
        return;
    }

    switch (current_platform)
    {
        case PLATFORM_XBOX:
        create_xbox_start_button(gui_id);
        break;
        case PLATFORM_PS:
        create_ps4_start_button(gui_id);
        break;
        case PLATFORM_STEAMDECK:
        create_steamdeck_start_button(gui_id);
        break;
        case PLATFORM_SWITCH:
        create_switch_start_button(gui_id);
        break;
    }
}

void create_select_button(int gui_id, int btn_id)
{
    switch (btn_id)
    {
        case 0:
        create_a_button(gui_id, btn_id);
        return;
        case 1:
        create_b_button(gui_id, btn_id);
        return;
        case 2:
        create_x_button(gui_id, btn_id);
        return;
        case 3:
        create_y_button(gui_id, btn_id);
        return;
        case 4:
        create_lb_button(gui_id, btn_id);
        return;
        case 5:
        create_rb_button(gui_id, btn_id);
        return;
        case 6:
        create_start_button(gui_id, btn_id);
        return;
    }

    switch (current_platform)
    {
        case PLATFORM_XBOX:
        create_xbox_select_button(gui_id);
        break;
        case PLATFORM_PS:
        create_ps4_select_button(gui_id);
        break;
        case PLATFORM_STEAMDECK:
        create_steamdeck_select_button(gui_id);
        break;
        case PLATFORM_SWITCH:
        create_switch_select_button(gui_id);
        break;
    }
}

#pragma endregion

/*---------------------------------------------------------------------------*/

/* Shared */

void init_xbox_title(void)
{
    if ((xbox_control_title_id = gui_hstack(0)))
    {
        /* Nintendo Switch has close software features,
         * so we don't need that */
        if (current_platform != PLATFORM_SWITCH
            && current_platform != PLATFORM_STEAMDECK)
        {
            gui_label(xbox_control_title_id, _("Exit"),
                      GUI_SML, gui_wht, gui_wht);

            create_b_button(xbox_control_title_id,
                            config_get_d(CONFIG_JOYSTICK_BUTTON_B));

            create_controller_spacer(xbox_control_title_id);
        }

        gui_label(xbox_control_title_id, _("Select"),
                  GUI_SML, gui_wht, gui_wht);

        create_a_button(xbox_control_title_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_A));

        gui_set_rect(xbox_control_title_id, GUI_TOP);
        gui_layout(xbox_control_title_id, 0, -1);
    }
}

void init_xbox_list(void)
{
    if ((xbox_control_list_id = gui_hstack(0)))
    {
        gui_label(xbox_control_list_id, _("Back"),
                  GUI_SML, gui_wht, gui_wht);

        create_b_button(xbox_control_list_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_B));

        create_controller_spacer(xbox_control_list_id);

        gui_label(xbox_control_list_id, _("Select"),
                  GUI_SML, gui_wht, gui_wht);

        create_a_button(xbox_control_list_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_A));

        gui_set_rect(xbox_control_list_id, GUI_TOP);
        gui_layout(xbox_control_list_id, 0, -1);
    }
}

void init_xbox_paused(void)
{
    if ((xbox_control_paused_id = gui_hstack(0)))
    {
        gui_label(xbox_control_paused_id, _("Continue"), 
                  GUI_SML, gui_wht, gui_wht);

        create_b_button(xbox_control_paused_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_B));

        create_controller_spacer(xbox_control_paused_id);

        gui_label(xbox_control_paused_id, _("Select"),
                  GUI_SML, gui_wht, gui_wht);

        create_a_button(xbox_control_paused_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_A));

        gui_set_rect(xbox_control_paused_id, GUI_TOP);
        gui_layout(xbox_control_paused_id, 0, -1);
    }
}

/* Generic */

void init_xbox_desc(void)
{
    if ((xbox_control_desc_id = gui_hstack(0)))
    {
        gui_label(xbox_control_desc_id, _("Show description"),
                  GUI_SML, gui_wht, gui_wht);

        create_x_button(xbox_control_desc_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_X));

        create_controller_spacer(xbox_control_desc_id);

        gui_label(xbox_control_desc_id, _("Start Level"),
                  GUI_SML, gui_wht, gui_wht);

        create_a_button(xbox_control_desc_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_A));

        gui_set_rect(xbox_control_desc_id, GUI_TOP);
        gui_layout(xbox_control_desc_id, 0, -1);
    }
}

void init_xbox_preparation(void)
{
    if ((xbox_control_preparation_id = gui_hstack(0)))
    {
        gui_label(xbox_control_preparation_id, _("Cycle Camera Mode"),
                  GUI_SML, gui_wht, gui_wht);

        create_x_button(xbox_control_preparation_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_X));

        create_controller_spacer(xbox_control_preparation_id);

        gui_label(xbox_control_preparation_id, _("Skip"),
                  GUI_SML, gui_wht, gui_wht);

        create_a_button(xbox_control_preparation_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_A));

        gui_set_rect(xbox_control_preparation_id, GUI_TOP);
        gui_layout(xbox_control_preparation_id, 0, -1);
    }
}

void init_xbox_replay(void)
{
    if ((xbox_control_replay_id = gui_hstack(0)))
    {
        gui_label(xbox_control_replay_id, _("Pause"),
                  GUI_SML, gui_wht, gui_wht);

        create_b_button(xbox_control_replay_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_A));

        gui_set_rect(xbox_control_replay_id, GUI_TOP);
        gui_layout(xbox_control_replay_id, 0, -1);
    }
}

void init_xbox_replay_eof(void)
{
    if ((xbox_control_replay_eof_id = gui_hstack(0)))
    {
        gui_label(xbox_control_replay_eof_id, _("Quit"),
                  GUI_SML, gui_wht, gui_wht);

        create_b_button(xbox_control_replay_eof_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_B));

        create_controller_spacer(xbox_control_replay_eof_id);

        gui_label(xbox_control_replay_eof_id, _("Select"),
                  GUI_SML, gui_wht, gui_wht);

        create_a_button(xbox_control_replay_eof_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_A));

        gui_set_rect(xbox_control_replay_eof_id, GUI_TOP);
        gui_layout(xbox_control_replay_eof_id, 0, -1);
    }
}

void init_xbox_shop(void)
{
    if ((xbox_control_shop_id = gui_hstack(0)))
    {
        gui_label(xbox_control_shop_id, _("Get Coins"),
                  GUI_SML, gui_wht, gui_wht);

        create_y_button(xbox_control_shop_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_Y));

        create_controller_spacer(xbox_control_shop_id);

        gui_label(xbox_control_shop_id, _("Back"),
                  GUI_SML, gui_wht, gui_wht);

        create_b_button(xbox_control_shop_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_B));

        create_controller_spacer(xbox_control_shop_id);

        gui_label(xbox_control_shop_id, _("Buy"),
                  GUI_SML, gui_wht, gui_wht);

        create_a_button(xbox_control_shop_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_A));

        gui_set_rect(xbox_control_shop_id, GUI_TOP);
        gui_layout(xbox_control_shop_id, 0, -1);
    }
}

void init_xbox_shop_getcoins(void)
{
    if ((xbox_control_shop_getcoins_id = gui_hstack(0)))
    {
        gui_label(xbox_control_shop_getcoins_id, _("Back"),
                  GUI_SML, gui_wht, gui_wht);

        create_b_button(xbox_control_shop_getcoins_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_B));

        create_controller_spacer(xbox_control_shop_getcoins_id);

        gui_label(xbox_control_shop_getcoins_id, _("Purchase"),
                  GUI_SML, gui_wht, gui_wht);

        create_a_button(xbox_control_shop_getcoins_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_A));

        gui_set_rect(xbox_control_shop_getcoins_id, GUI_TOP);
        gui_layout(xbox_control_shop_getcoins_id, 0, -1);
    }
}

void init_xbox_model(void)
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
            create_y_button(xbox_control_model_id,
                            config_get_d(CONFIG_JOYSTICK_BUTTON_Y));

            create_controller_spacer(xbox_control_model_id);
        }
#endif

        gui_label(xbox_control_model_id, _("Change Model"),
                  GUI_SML, gui_wht, gui_wht);

        create_rb_button(xbox_control_model_id,
                         config_get_d(CONFIG_JOYSTICK_BUTTON_L1));
        create_lb_button(xbox_control_model_id,
                         config_get_d(CONFIG_JOYSTICK_BUTTON_L2));

        create_controller_spacer(xbox_control_model_id);

        gui_label(xbox_control_model_id, _("Back"),
                  GUI_SML, gui_wht, gui_wht);

        create_b_button(xbox_control_model_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_B));

        gui_set_rect(xbox_control_model_id, GUI_TOP);
        gui_layout(xbox_control_model_id, 0, -1);
    }
}

void init_xbox_beam_style(void)
{
    if ((xbox_control_beam_style_id = gui_hstack(0)))
    {
        gui_label(xbox_control_beam_style_id, _("Change Style"), 
                  GUI_SML, gui_wht, gui_wht);

        create_rb_button(xbox_control_model_id,
                         config_get_d(CONFIG_JOYSTICK_BUTTON_L1));
        create_lb_button(xbox_control_model_id,
                         config_get_d(CONFIG_JOYSTICK_BUTTON_L2));

        create_controller_spacer(xbox_control_beam_style_id);

        gui_label(xbox_control_beam_style_id, _("Back"),
                  GUI_SML, gui_wht, gui_wht);

        create_b_button(xbox_control_model_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_B));

        gui_set_rect(xbox_control_beam_style_id, GUI_TOP);
        gui_layout(xbox_control_beam_style_id, 0, -1);
    }
}

void init_xbox_death()
{
    if ((xbox_control_death_id = gui_hstack(0)))
    {
        gui_label(xbox_control_death_id, _("Select"),
                  GUI_SML, gui_wht, gui_wht);

        create_a_button(xbox_control_death_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_A));

        gui_set_rect(xbox_control_death_id, GUI_TOP);
        gui_layout(xbox_control_death_id, 0, -1);
    }
}

/* Putt */
void init_xbox_putt_stroke()
{
    if ((xbox_control_putt_stroke_id = gui_hstack(0)))
    {
        gui_label(xbox_control_putt_stroke_id, _("Change club"),
                  GUI_SML, gui_wht, gui_wht);

        create_lt_button(xbox_control_putt_stroke_id,
                         config_get_d(CONFIG_JOYSTICK_BUTTON_L2));
        create_lb_button(xbox_control_putt_stroke_id,
                         config_get_d(CONFIG_JOYSTICK_BUTTON_L1));

        gui_label(xbox_control_putt_stroke_id, _("Shot"),
                  GUI_SML, gui_wht, gui_wht);

        create_a_button(xbox_control_putt_stroke_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_A));

        create_controller_spacer(xbox_control_putt_stroke_id);

        gui_label(xbox_control_putt_stroke_id, _("Fine aim"),
                  GUI_SML, gui_wht, gui_wht);

        create_x_button(xbox_control_putt_stroke_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_X));

        gui_set_rect(xbox_control_putt_stroke_id, GUI_TOP);
        gui_layout(xbox_control_putt_stroke_id, 0, -1);
    }
}

void init_xbox_putt_stop()
{
    if ((xbox_control_putt_stop_id = gui_hstack(0)))
    {
        gui_label(xbox_control_putt_stop_id, _("Skip"),
                  GUI_SML, gui_wht, gui_wht);

        create_a_button(xbox_control_putt_stop_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_A));

        gui_set_rect(xbox_control_putt_stop_id, GUI_TOP);
        gui_layout(xbox_control_putt_stop_id, 0, -1);
    }
}

void init_xbox_putt_scores()
{
    if ((xbox_control_putt_scores_id = gui_hstack(0)))
    {
        gui_label(xbox_control_putt_scores_id, _("Continue"),
                  GUI_SML, gui_wht, gui_wht);

        create_a_button(xbox_control_putt_scores_id,
                        config_get_d(CONFIG_JOYSTICK_BUTTON_A));

        gui_set_rect(xbox_control_putt_scores_id, GUI_TOP);
        gui_layout(xbox_control_putt_scores_id, 0, -1);
    }
}

/*---------------------------------------------------------------------------*/

int xbox_show_gui(void)
{
    return show_control_gui;
}

void xbox_toggle_gui(int active)
{
#if NEVERBALL_FAMILY_API != NEVERBALL_PC_FAMILY_API
    show_control_gui = 1;
#else
    show_control_gui = 0;
#endif
}

void xbox_control_gui_init(void)
{
    if (current_platform == PLATFORM_PC)
        return;

    /* Shared */
    init_xbox_title();
    init_xbox_list();
    init_xbox_paused();

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

void xbox_control_gui_free(void)
{
    if (current_platform == PLATFORM_PC)
        return;

    /* Shared */
    gui_delete(xbox_control_title_id);
    gui_delete(xbox_control_list_id);
    gui_delete(xbox_control_paused_id);

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
}

/*---------------------------------------------------------------------------*/

/* Shared */

void xbox_control_title_gui_paint(void)
{
    if (show_control_gui && current_platform != PLATFORM_PC)
        gui_paint(xbox_control_title_id);
}

void xbox_control_list_gui_paint(void)
{
    if (show_control_gui && current_platform != PLATFORM_PC)
        gui_paint(xbox_control_list_id);
}

void xbox_control_paused_gui_paint(void)
{
    if (show_control_gui && current_platform != PLATFORM_PC)
        gui_paint(xbox_control_paused_id);
}

/* Generic */

void xbox_control_desc_gui_paint(void)
{
    gui_paint(xbox_control_desc_id);
}

void xbox_control_preparation_gui_paint(void)
{
    if (show_control_gui && current_platform != PLATFORM_PC)
        gui_paint(xbox_control_preparation_id);
}

void xbox_control_replay_paint(void)
{
    if (show_control_gui && current_platform != PLATFORM_PC)
        gui_paint(xbox_control_replay_id);
}

void xbox_control_replay_eof_paint(void)
{
    if (show_control_gui && current_platform != PLATFORM_PC)
        gui_paint(xbox_control_replay_eof_id);
}

void xbox_control_shop_gui_paint(void)
{
    if (show_control_gui && current_platform != PLATFORM_PC)
        gui_paint(xbox_control_shop_id);
}

void xbox_control_shop_getcoins_gui_paint(void)
{
    if (show_control_gui && current_platform != PLATFORM_PC)
        gui_paint(xbox_control_shop_getcoins_id);
}

void xbox_control_model_gui_paint(void)
{
    if (show_control_gui && current_platform != PLATFORM_PC)
        gui_paint(xbox_control_model_id);
}

void xbox_control_beam_style_gui_paint(void)
{
    if (show_control_gui && current_platform != PLATFORM_PC)
        gui_paint(xbox_control_beam_style_id);
}

void xbox_control_death_gui_paint(void)
{
    if (show_control_gui && current_platform != PLATFORM_PC)
        gui_paint(xbox_control_death_id);
}

/* Putt */
void xbox_control_putt_stroke_gui_paint(void)
{
    if (show_control_gui && current_platform != PLATFORM_PC)
        gui_paint(xbox_control_putt_stroke_id);
}

void xbox_control_putt_stop_gui_paint(void)
{
    if (show_control_gui && current_platform != PLATFORM_PC)
        gui_paint(xbox_control_putt_stop_id);
}

void xbox_control_putt_scores_gui_paint(void)
{
    if (show_control_gui && current_platform != PLATFORM_PC)
        gui_paint(xbox_control_putt_scores_id);
}
