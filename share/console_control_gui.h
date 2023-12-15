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

#ifndef CONSOLE_CONTROL_GUI_H
#define CONSOLE_CONTROL_GUI_H

/* Supported Platforms */
enum console_platforms {
    PLATFORM_PC,
    PLATFORM_XBOX,      /* Xbox Controllers        */
    PLATFORM_PS,        /* PlayStation Controllers */
    PLATFORM_SWITCH,    /* Switch Controllers      */
    PLATFORM_STEAMDECK, /* Steam Deck Controllers  */
    PLATFORM_HANDSET    /* Etihad Handset          */
};


/* Etihad Handset */

#define HANDSET_A_BUTTON  "A"  /* Red A          */
#define HANDSET_B_BUTTON  "B"  /* Yellow B       */
#define HANDSET_X_BUTTON  "X"  /* Blue X         */
#define HANDSET_Y_BUTTON  "Y"  /* Green Y        */
#define HANDSET_LB_BUTTON "L"  /* Left Shoulder  */
#define HANDSET_RB_BUTTON "R"  /* Right Shoulder */


/* Xbox One / 360 */

#define XBOX_A_BUTTON      "A"     /* Green A        */
#define XBOX_B_BUTTON      "B"     /* Red B          */
#define XBOX_X_BUTTON      "X"     /* Blue X         */
#define XBOX_Y_BUTTON      "Y"     /* Yellow Y       */
#define XBOX_LB_BUTTON     "LB"    /* Left Shoulder  */
#define XBOX_RB_BUTTON     "RB"    /* Right Shoulder */
#define XBOX_LT_BUTTON     "LT"    /* Left Trigger   */
#define XBOX_RT_BUTTON     "RT"    /* Right Trigger  */
#define XBOX_LS_BUTTON     "LS"    /* Left Stick     */
#define XBOX_RS_BUTTON     "RS"    /* Right Stick    */
#define XBOX_START_BUTTON  "◀️"     /* Start */
#define XBOX_SELECT_BUTTON "▶️"     /* Select */


/* PlayStation 4 */

#define PS4_A_BUTTON      "×"     /* Blue cross     */
#define PS4_B_BUTTON      "○"     /* Red circle     */
#define PS4_X_BUTTON      "◻"     /* Pink square    */
#define PS4_Y_BUTTON      "△"     /* Cyan triangle  */
#define PS4_LB_BUTTON     "L1"    /* Left Shoulder  */
#define PS4_RB_BUTTON     "R1"    /* Right Shoulder */
#define PS4_LT_BUTTON     "L2"    /* Left Trigger   */
#define PS4_RT_BUTTON     "R2"    /* Right Trigger  */
#define PS4_LS_BUTTON     "L3"    /* Left Stick     */
#define PS4_RS_BUTTON     "R3"    /* Right Stick    */
#define PS4_START_BUTTON  "◀️"     /* Start */
#define PS4_SELECT_BUTTON "▶️"     /* Select */


/* Steam Deck */

#define STEAMDECK_A_BUTTON      "A"     /* Blue cross     */
#define STEAMDECK_B_BUTTON      "B"     /* Red circle     */
#define STEAMDECK_X_BUTTON      "X"     /* Pink square    */
#define STEAMDECK_Y_BUTTON      "Y"     /* Cyan triangle  */
#define STEAMDECK_LB_BUTTON     "L1"    /* Left Shoulder  */
#define STEAMDECK_RB_BUTTON     "R1"    /* Right Shoulder */
#define STEAMDECK_LT_BUTTON     "L2"    /* Left Trigger   */
#define STEAMDECK_RT_BUTTON     "R2"    /* Right Trigger  */
#define STEAMDECK_LS_BUTTON     "L3"    /* Left Stick     */
#define STEAMDECK_RS_BUTTON     "R3"    /* Right Stick    */
#define STEAMDECK_START_BUTTON  "-"     /* Start */
#define STEAMDECK_SELECT_BUTTON "+"     /* Select */


/* Wii U / Nintendo Switch */

#define SWITCH_A_BUTTON      "A"
#define SWITCH_B_BUTTON      "B"
#define SWITCH_X_BUTTON      "X"
#define SWITCH_Y_BUTTON      "Y"
#define SWITCH_LB_BUTTON     "L"
#define SWITCH_RB_BUTTON     "R"
#define SWITCH_LT_BUTTON     "ZL"
#define SWITCH_RT_BUTTON     "ZR"
#define SWITCH_LS_BUTTON     "LS"
#define SWITCH_RS_BUTTON     "RS"
#define SWITCH_START_BUTTON  "+"
#define SWITCH_SELECT_BUTTON "-"

/*---------------------------------------------------------------------------*/

extern enum console_platforms current_platform;

void init_controller_type(enum console_platforms);
void xbox_control_gui_set_alpha(float);

void create_a_button(int, int);
void create_b_button(int, int);
void create_x_button(int, int);
void create_y_button(int, int);
void create_lb_button(int, int);
void create_rb_button(int, int);
void create_lt_button(int, int);
void create_rt_button(int, int);
void create_ls_button(int, int);
void create_rs_button(int, int);
void create_start_button(int, int);
void create_select_button(int, int);

int xbox_show_gui(void);

void xbox_toggle_gui(int);

void xbox_control_gui_init(void);
void xbox_control_gui_free(void);


/* Shared */

void xbox_control_title_gui_paint(void);
void xbox_control_list_gui_paint(void);
void xbox_control_paused_gui_paint(void);


/* Generic */

void xbox_control_desc_gui_paint(void);
void xbox_control_preparation_gui_paint(void);
void xbox_control_replay_paint(void);
void xbox_control_replay_eof_paint(void);
void xbox_control_shop_gui_paint(void);
void xbox_control_shop_getcoins_gui_paint(void);
void xbox_control_model_gui_paint(void);
void xbox_control_beam_style_gui_paint(void);
void xbox_control_death_gui_paint(void);


/* Putt */

void xbox_control_putt_stroke_gui_paint(void);
void xbox_control_putt_stop_gui_paint(void);
void xbox_control_putt_scores_gui_paint(void);

#endif
