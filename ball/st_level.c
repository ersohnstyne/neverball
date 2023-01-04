/*
 * Copyright (C) 2022 Microsoft / Neverball authors
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

/* Uncomment, if complex game mode is implemented */
#define REQUIRES_COMPLEX_MODE

#include <stdio.h>
#include <assert.h>

#if NB_HAVE_PB_BOTH==1
#include "networking.h"

#ifndef __EMSCRIPTEN__
#include "console_control_gui.h"
#endif

#include "campaign.h" // New: Campaign
#include "checkpoints.h" // New: Checkpoints
#include "mediation.h"
#include "boost_rush.h"
#include "account.h"
#endif

#include "lang_switchball.h"

#include "gui.h"
#include "hud.h"
#include "set.h"
#include "progress.h"
#include "audio.h"
#include "config.h"
#include "demo.h"
#include "geom.h"

#ifdef CONFIG_INCLUDES_ACCOUNT
#include "powerup.h"
#endif

#include "game_draw.h"
#include "game_server.h"
#include "game_client.h"
#include "game_common.h"

#include "st_pause.h"
#include "st_level.h"
#include "st_play.h"
#include "st_start.h"
#include "st_set.h"
#include "st_over.h"
#include "st_done.h"
#include "st_shared.h"
#include "st_title.h"
#if defined(SWITCHBALL_HAVE_TIP_AND_TUTORIAL)
#include "st_tutorial.h"
#endif
#include "st_name.h"

/*---------------------------------------------------------------------------*/

struct state st_poser;
struct state st_level_signin_required;

/*---------------------------------------------------------------------------*/

static int level_check_playername(const char *regname)
{
    for (int i = 0; i < strlen(regname); i++)
    {
        if (regname[i] == '\\' || regname[i] == '/' || regname[i] == ':' || regname[i] == '*' || regname[i] == '?' || regname[i] == '"' || regname[i] == '<' || regname[i] == '>' || regname[i] == '|')
        {
            log_errorf("Can't accept other charsets!\n", regname[i]);
            return 0;
        }
    }

    return 1;
}

/*---------------------------------------------------------------------------*/

static int show_info = 0;

static int demo_warn_only_once = 0;
static int check_nodemo = 1;

static int evalue;
static int fvalue;
static int svalue;

enum {
    START_LEVEL_POWERUP = GUI_LAST
};

#if defined(ENABLE_POWERUP) && defined(CONFIG_INCLUDES_ACCOUNT)
static int level_action(int tok, int val)
{
    audio_play(AUD_MENU, 1.0f);

    switch (tok)
    {
    case START_LEVEL_POWERUP:
        if (val == 3) {
            audio_play("snd/speedifier.ogg", 1.0f);
            init_speedifier();
        } else if (val == 2) {
            audio_play("snd/floatifier.ogg", 1.0f);
            init_floatifier();
        } else if (val == 1) {
            audio_play("snd/earninator.ogg", 1.0f);
            init_earninator();
        }
        show_info = 0;

#ifdef SWITCHBALL_HAVE_TIP_AND_TUTORIAL
        if (!tutorial_check())
        {
            if (!hint_check())
                return goto_state(&st_play_ready);
        }
#else
        return goto_state(&st_play_ready);
#endif
        break;
    }
    return 1;
}
#endif

static int level_gui(void)
{
#if defined(ENABLE_POWERUP) && defined(CONFIG_INCLUDES_ACCOUNT)
    evalue = account_get_d(ACCOUNT_CONSUMEABLE_EARNINATOR);
    fvalue = account_get_d(ACCOUNT_CONSUMEABLE_FLOATIFIER);
    svalue = account_get_d(ACCOUNT_CONSUMEABLE_SPEEDIFIER);
#endif

    int id, jd, kd;

    if ((id = gui_vstack(0)))
    {
#ifdef CONFIG_INCLUDES_ACCOUNT
        if ((curr_mode() == MODE_CHALLENGE || curr_mode() == MODE_BOOST_RUSH) && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_CONSUMABLES))
        {
            char account_coinsattr[MAXSTR], account_gemsattr[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(account_gemsattr, dstSize, "%s: %i", _("Gems"), account_get_d(ACCOUNT_DATA_WALLET_GEMS));
            sprintf_s(account_coinsattr, dstSize, "%s: %i", _("Coins"), account_get_d(ACCOUNT_DATA_WALLET_COINS));
#else
            sprintf(account_gemsattr, "%s: %i", _("Gems"), account_get_d(ACCOUNT_DATA_WALLET_GEMS));
            sprintf(account_coinsattr, "%s: %i", _("Coins"), account_get_d(ACCOUNT_DATA_WALLET_COINS));
#endif
            if ((jd = gui_hstack(id)))
            {
                gui_filler(jd);
                if ((kd = gui_harray(jd)))
                {
                    gui_label(kd, account_gemsattr, GUI_SML, gui_wht, gui_cya);
                    gui_label(kd, account_coinsattr, GUI_SML, gui_wht, gui_yel);
                }
                gui_filler(jd);
            }

            gui_space(id);
        }
#endif

        if ((jd = gui_hstack(id)))
        {
            gui_filler(jd);

            if ((kd = gui_vstack(jd)))
            {
                const char *ln = level_name (curr_level());
                int b          = level_bonus(curr_level());

                char setattr[MAXSTR], lvlattr[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                if (b)
                    sprintf_s(lvlattr, dstSize, _("Bonus Level %s"), ln);
                else if (curr_mode() == MODE_STANDALONE)
                    sprintf_s(lvlattr, dstSize, _("Level ---"));
                else
                    sprintf_s(lvlattr, dstSize, _("Level %s"), ln);

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                if (curr_mode() == MODE_CAMPAIGN)
                    sprintf_s(setattr, dstSize, "%s", mode_to_str(curr_mode(), 1));
                else if (curr_mode() == MODE_HARDCORE)
                    sprintf_s(setattr, dstSize, _(mode_to_str(curr_mode(), 1)));
                else
#endif
                if (curr_mode() == MODE_STANDALONE)
                    sprintf_s(setattr, dstSize, _("Standalone level"));
                else if (curr_mode() == MODE_NORMAL)
                    sprintf_s(setattr, dstSize, "%s", set_name(curr_set()));
                else if (curr_mode() != MODE_NONE)
                    sprintf_s(setattr, dstSize, _("%s: %s"), set_name(curr_set()), mode_to_str(curr_mode(), 1));
#else
                if (b)
                    sprintf(lvlattr, _("Bonus Level %s"), ln);
                else if (curr_mode() == MODE_STANDALONE)
                    sprintf(lvlattr, _("Level ---"));
                else
                    sprintf(lvlattr, _("Level %s"), ln);

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                if (curr_mode() == MODE_CAMPAIGN)
                    sprintf(setattr, "%s", mode_to_str(curr_mode(), 1));
                else if (curr_mode() == MODE_HARDCORE)
                    sprintf(setattr, _(mode_to_str(curr_mode(), 1)));
                else
#endif
                if (curr_mode() == MODE_STANDALONE)
                    sprintf(setattr, _("Standalone level"));
                else if (curr_mode() == MODE_NORMAL)
                    sprintf(setattr, "%s", set_name(curr_set()));
                else if (curr_mode() != MODE_NONE)
                    sprintf(setattr, _("%s: %s"), set_name(curr_set()), mode_to_str(curr_mode(), 1));
#endif
                gui_title_header(kd, lvlattr,
                          b ? GUI_MED : GUI_LRG,
                          b ? gui_wht : 0,
                          b ? gui_grn : 0);
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                if (curr_mode() == MODE_HARDCORE)
                    gui_label(kd, _("Hardcore Mode!"), GUI_SML, gui_red, gui_red);
                else
#endif
                    gui_label(kd, setattr, GUI_SML, gui_wht, gui_wht);

                gui_set_rect(kd, GUI_ALL);
            }
            gui_filler(jd);
        }
        gui_space(id);

#ifdef ENABLE_POWERUP
        if ((curr_mode() == MODE_CHALLENGE || curr_mode() == MODE_BOOST_RUSH) && !show_info && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_CONSUMABLES))
        {
            gui_start(id, _("Start Level"), GUI_SML, START_LEVEL_POWERUP, 0);
            gui_space(id);

            gui_label(id, _("Use special powers"), GUI_SML, gui_wht, gui_wht);

            if ((jd = gui_harray(id)))
            {
                int ced, cfd, csd;
                char pow1attr[MAXSTR], pow2attr[MAXSTR], pow3attr[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(pow3attr, dstSize, _("Speedifier (%i)"), svalue);
#else
                sprintf(pow3attr, _("Speedifier (%i)"), svalue);
#endif
                if ((csd = gui_varray(jd)))
                {
                    gui_label(csd, pow3attr, GUI_SML,
                        svalue > 0 ? gui_grn : gui_gry,
                        svalue > 0 ? gui_wht : gui_gry);
                    if (svalue > 0)
                        gui_set_state(csd, START_LEVEL_POWERUP, 3);
                    else
                        gui_set_state(csd, GUI_NONE, 0);
                }

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(pow2attr, dstSize, _("Floatifier (%i)"), fvalue);
#else
                sprintf(pow2attr, _("Floatifier (%i)"), fvalue);
#endif
                if ((cfd = gui_varray(jd)))
                {
                    gui_label(cfd, pow2attr, GUI_SML,
                        fvalue > 0 && curr_mode() != MODE_BOOST_RUSH ? gui_blu : gui_gry,
                        fvalue > 0 && curr_mode() != MODE_BOOST_RUSH ? gui_wht : gui_gry);
                    if (fvalue > 0 && curr_mode() != MODE_BOOST_RUSH)
                        gui_set_state(cfd, START_LEVEL_POWERUP, 2);
                    else
                        gui_set_state(cfd, GUI_NONE, 0);
                }

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(pow1attr, dstSize, _("Earninator (%i)"), evalue);
#else
                sprintf(pow1attr, _("Earninator (%i)"), evalue);
#endif
                if ((ced = gui_varray(jd)))
                {
#ifndef LEVELGROUPS_INCLUDES_CAMPAIGN
                    gui_label(ced, pow1attr, GUI_SML,
                        evalue > 0 ? gui_red : gui_gry,
                        evalue > 0 ? gui_wht : gui_gry);
#else
                    gui_label(ced, pow1attr, GUI_SML,
                        evalue > 0 && curr_mode() != MODE_HARDCORE ? gui_red : gui_gry,
                        evalue > 0 && curr_mode() != MODE_HARDCORE ? gui_wht : gui_gry);
#endif
                    if (evalue > 0)
                        gui_set_state(ced, START_LEVEL_POWERUP, 1);
                    else
                        gui_set_state(ced, GUI_NONE, 0);
                }
            }
        }
        else
#endif
        {
#ifdef MAPC_INCLUDES_CHKP
            gui_multi(id, last_active ? _("The checkpoint is in the\\last position as last time.\\ \\Click to continue.") : level_msg(curr_level()), GUI_SML, gui_wht, gui_wht);
#else
            gui_multi(id, level_msg(curr_level()), GUI_SML, gui_wht, gui_wht);
#endif
        }

        gui_layout(id, 0, 0);
    }

    return id;
}

static int level_enter(struct state *st, struct state *prev)
{
    game_lerp_pose_point_reset();
    game_client_fly(1.0f);
    
    demo_warn_only_once = prev == &st_nodemo;
    check_nodemo = 1;

    if (prev != &st_level)
        show_info = 0;

    /* New: Checkpoints */
    game_client_sync(!campaign_hardcore_norecordings() && curr_mode() != MODE_NONE ? demo_fp : NULL);
    game_client_fly(1.0f);
    hud_update(0, 0.0f);

    return level_gui();
}

static void level_paint(int id, float t)
{
    game_client_draw(0, t);

    gui_paint(id);
#ifndef __EMSCRIPTEN__
    if (xbox_show_gui())
    {
        hud_cam_paint();
        xbox_control_desc_gui_paint();
    }
    else
#endif
        hud_paint();
}

static void level_timer(int id, float dt)
{
    /* HACK: This shouldn't have a bug. This has been fixed. */

    if (!demo_warn_only_once && config_get_d(CONFIG_ACCOUNT_SAVE) > 0)
    {
        if (strlen(config_get_s(CONFIG_PLAYER)) < 3 || !level_check_playername(config_get_s(CONFIG_PLAYER)))
        {
            goto_state(&st_level_signin_required);
            return;
        }

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        if (check_nodemo == 1 && !demo_fp && !campaign_hardcore_norecordings() && curr_mode() != MODE_NONE)
#else
        if (check_nodemo == 1 && !demo_fp && curr_mode() != MODE_NONE)
#endif
        {
            goto_state(&st_nodemo);
            return;
        }
    }

    geom_step(dt);
    hud_timer(dt);
    gui_timer(id, dt);
    game_step_fade(dt);
    game_lerp_pose_point_tick(dt);
}

static int level_click(int b, int d)
{
    if (b == SDL_BUTTON_LEFT && d == 1)
    {
#if defined(ENABLE_POWERUP) && defined(CONFIG_INCLUDES_ACCOUNT)
        if ((curr_mode() == MODE_CHALLENGE || curr_mode() == MODE_BOOST_RUSH) && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_CONSUMABLES))
        {
            int active = gui_active();
            return level_action(gui_token(active), gui_value(active));
        }
        else
#endif
        {
#if defined(SWITCHBALL_HAVE_TIP_AND_TUTORIAL)
            return tutorial_check() ? 1 : goto_state(&st_play_ready);
#else
            return goto_state(&st_play_ready);
#endif
        }
    }
    return 1;
}

static int level_keybd(int c, int d)
{
    if (d)
    {
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
        if (c == KEY_EXIT && current_platform == PLATFORM_PC)
            return goto_pause(curr_state());
        if (c == KEY_POSE && current_platform == PLATFORM_PC)
            return goto_state(&st_poser);
#else
        if (c == KEY_EXIT)
            return goto_pause(curr_state());
        if (c == KEY_POSE)
            return goto_state(&st_poser);
#endif
        if (config_tst_d(CONFIG_KEY_SCORE_NEXT, c))
        {
            if ((curr_mode() == MODE_CHALLENGE || curr_mode() == MODE_BOOST_RUSH))
            {
                show_info = show_info == 0 ? 1 : 0;
                goto_state(&st_level);
            }
        }
    }
    return 1;
}

static int level_buttn(int b, int d)
{
    if (d)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
        {
#if defined(ENABLE_POWERUP) && defined(CONFIG_INCLUDES_ACCOUNT)
            if ((curr_mode() == MODE_CHALLENGE || curr_mode() == MODE_BOOST_RUSH) && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_CONSUMABLES))
            {
                int active = gui_active();
                return level_action(gui_token(active), gui_value(active));
            }
            else
#endif
            {
#ifdef SWITCHBALL_HAVE_TIP_AND_TUTORIAL
                return tutorial_check() ? 1 : goto_state(&st_play_ready);
#else
                return goto_state(&st_play_ready);
#endif
            }
        }
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return goto_pause(curr_state());
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_X, b))
        {
            if ((curr_mode() == MODE_CHALLENGE || curr_mode() == MODE_BOOST_RUSH))
            {
                show_info = show_info == 0 ? 1 : 0;
                goto_state(&st_level);
            }
        }
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static void poser_paint(int id, float t)
{
#if NB_STEAM_API==0 && NB_EOS_SDK==0
    game_client_draw(config_cheat() ? 0 : POSE_LEVEL, t);
#else
    game_client_draw(0, t);
#endif
}

static void poser_timer(int id, float dt)
{
    float fix_deltatime = CLAMP(0.001f, dt, config_get_d(CONFIG_SMOOTH_FIX) ? MIN(1/60, dt) : MIN(100.f, dt));
    geom_step(dt);
    game_lerp_pose_point_tick(dt);
}

static void poser_point(int id, int x, int y, int dx, int dy)
{
    game_lerp_pose_point(dx, dy);
}

static int poser_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT || c == KEY_POSE)
            return goto_state(&st_level);
    }
    return 1;
}

static int poser_buttn(int c, int d)
{
    if (d && config_tst_d(CONFIG_JOYSTICK_BUTTON_B, c))
        return goto_state(&st_level);

    return 1;
}

/*---------------------------------------------------------------------------*/

static int nodemo_gui(void)
{
    int id;

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("Warning!"), GUI_MED, gui_red, gui_red);
        gui_space(id);
        gui_multi(id, _("A replay file could not be opened for writing.\\"
                        "This game will not be recorded.\\"),
                  GUI_SML, gui_wht, gui_wht);

        gui_layout(id, 0, 0);
    }

    return id;
}

static int nodemo_enter(struct state *st, struct state *prev)
{
    check_nodemo = 0;

    return nodemo_gui();
}

static void nodemo_timer(int id, float dt)
{
    geom_step(dt);
    game_step_fade(dt);
    gui_timer(id, dt);
}

static int nodemo_keybd(int c, int d)
{
    if (d)
    {
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
        if (c == KEY_EXIT && current_platform == PLATFORM_PC)
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            return goto_state(campaign_used() ? &st_play_ready : &st_level);
#else
            return goto_state(&st_level);
#endif
#else
        if (c == KEY_EXIT)
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            return goto_state(campaign_used() ? &st_play_ready : &st_level);
#else
            return goto_state(&st_level);
#endif
#endif
    }
    return 1;
}

static int nodemo_buttn(int b, int d)
{
    if (d)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            return goto_state(campaign_used() ? &st_play_ready : &st_level);
#else
            return goto_state(&st_play_ready);
#endif
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            return goto_state(campaign_used() ? &st_play_ready : &st_level);
#else
            return goto_state(&st_play_ready);
#endif
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int level_signin_required_gui(void)
{
    int id;

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("Sign in required!"), GUI_MED, gui_red, gui_red);
        gui_space(id);
        gui_multi(id, _("This account must be signed in,\\before you play this levels!"),
            GUI_SML, gui_wht, gui_wht);

        gui_layout(id, 0, 0);
    }

    return id;
}

static int level_signin_required_enter(struct state *st, struct state *prev)
{
    return level_signin_required_gui();
}

static int level_signin_required_keybd(int c, int d)
{
    if (d)
    {
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
        if (c == KEY_EXIT && current_platform == PLATFORM_PC)
            return goto_pause(curr_state());
#else
        if (c == KEY_EXIT)
            return goto_pause(curr_state());
#endif
    }
    return 1;
}

static int level_signin_required_buttn(int b, int d)
{
    if (d)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return goto_name(&st_level, &st_level_signin_required, 0, 0, 0);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return goto_pause(curr_state());
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

int goto_exit(void)
{
    if (curr_mode() != MODE_NONE)
        audio_play(AUD_QUITGAME, 1.0f);

    struct state *curr = curr_state();

    show_info = 0;

#ifdef LEVELGROUPS_INCLUDES_ZEN
    mediation_stop();
#endif

#ifdef ENABLE_POWERUP
    powerup_stop();
#endif

#ifdef MAPC_INCLUDES_CHKP
    checkpoints_stop();
#endif

    progress_stop();
    progress_exit();

    struct state *dst = NULL;

    video_clr_grab();

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    if (campaign_used())
    {
        //dst = &st_campaign;

        /* End of the level. */

        if (progress_done() && !progress_dead())
        {
            game_fade_color(0.0f, 0.25f, 0.0f);
            game_fade(+0.333f);
        }

        /*
         * If it manages to reach in there,
         * it should be containing the coordinates.
         */

        else if (campaign_hardcore())
        {
            if (progress_dead() && !progress_done())
            {
                game_fade_color(0.25f, 0.0f, 0.0f);
                game_fade(+0.333f);
            }
            else if (progress_done())
                dst = curr_times() > 0 && !progress_dead() ? &st_done : &st_over;
        }
        else
            return goto_playmenu(curr_mode());
    }
    else
#endif
    if (progress_done() && !progress_dead())
    {
        game_fade_color(0.0f, 0.25f, 0.0f);
        game_fade(+0.333f);
        dst = &st_done;
    }
#ifdef REQUIRES_COMPLEX_MODE
    else if (curr_mode() == MODE_BOOST_RUSH)
    {
        boost_rush_stop();
        return goto_playmenu(curr_mode());
    }
#endif
    else if (curr_mode() == MODE_CHALLENGE)
    {
        /*
         * Player wants to save and open the Leaderboard in Challenge mode,
         * but the game causes a crash.
         */

        if (progress_dead() && !campaign_used())
        {
            game_fade_color(0.25f, 0.0f, 0.0f);
            game_fade(+0.333f);
        }
        
        dst = curr_times() > 0 && progress_dead() ? &st_over : &st_start;
    }
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    else if (curr_mode() != MODE_NONE && !campaign_used())
        dst = &st_start;
    else
        dst = &st_title;
#else
    else dst = &st_start;
#endif

    if (dst)
    {
        /* Visit the auxilliary screen or exit to level selection. */

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        goto_state(dst != curr ? dst : &st_start);
#else
        goto_state(&st_start);
#endif
    }
    else
    {
        /* Quit the game. */

        SDL_Event e = { SDL_QUIT };
        SDL_PushEvent(&e);
    }

    return 1;
}

/*---------------------------------------------------------------------------*/

struct state st_level = {
    level_enter,
    shared_leave,
    level_paint,
    level_timer,
    shared_point, // Can hover on: point
    shared_stick, // Can hover on: stick
    shared_angle, // Can hover on: angle
    level_click,
    level_keybd,
    level_buttn,
    NULL,
    NULL
};

struct state st_poser = {
    NULL,
    NULL,
    poser_paint,
    poser_timer,
    poser_point,
    NULL,
    NULL,
    NULL,
    poser_keybd,
    poser_buttn,
    NULL,
    NULL
};

struct state st_nodemo = {
    nodemo_enter,
    shared_leave,
    shared_paint,
    nodemo_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click_basic,
    nodemo_keybd,
    nodemo_buttn,
    NULL,
    NULL
};

struct state st_level_signin_required = {
    level_signin_required_enter,
    shared_leave,
    shared_paint,
    nodemo_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click_basic,
    level_signin_required_keybd,
    level_signin_required_buttn,
    NULL,
    NULL
};