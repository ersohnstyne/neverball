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

#include <assert.h>

#include "solid_chkp.h"

#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
#include "console_control_gui.h"
#endif

#if NB_HAVE_PB_BOTH==1
#include "account.h"
#include "campaign.h" // New: Campaign
#endif

#ifdef MAPC_INCLUDES_CHKP
#include "checkpoints.h" // New: Checkpoints
#endif

#include "util.h"
#include "networking.h"
#include "progress.h"
#include "demo.h"
#include "audio.h"
#include "gui.h"
#include "config.h"
#ifdef CONFIG_INCLUDES_ACCOUNT
#include "powerup.h"
#include "mediation.h"
#endif
#include "video.h"
#include "hud.h"

#include "game_common.h"
#include "game_server.h"
#include "game_client.h"

#include "st_play.h"
#include "st_shop.h"
#include "st_save.h"
#include "st_fail.h"
#include "st_level.h"
#include "st_play.h"
#include "st_shared.h"

/*---------------------------------------------------------------------------*/

#define FAIL_ERROR_REPLAY    _("You can't save new replays anymore!")
#define FAIL_ERROR_RESPAWN_1 _("You can't respawn in under one minute!")
#define FAIL_ERROR_RESPAWN_2 _("You can't respawn from checkpoints anymore!")
#define FAIL_ERROR_RESPAWN_3 _("You can't respawn in hardcore mode!")

#define FAIL_ERROR_REPLAY_COVID_HIGHRISK _("Replays have locked down during high risks!")

#define FAIL_ERROR_SERVER_POLICY_SHOP         _("Shop is disabled with server policy!")
#define FAIL_ERROR_SERVER_POLICY_SHOP_MANAGED _("Managed product is disabled with server policy!")

#define FAIL_UPGRADE_EDITION_1 _("Upgrade to Enterprise edition, to buy Mediation!")
#define FAIL_UPGRADE_EDITION_2 _("Upgrade to Pro edition, to buy more balls!")

#define FAIL_TRANSFER_MEMBER_1 _("Join Neverball Discord, to buy more balls!")

/*---------------------------------------------------------------------------*/

#if NB_HAVE_PB_BOTH==1

/* TODO: Place the following screenstate variables into the header file. */

struct state st_zen_warning;
struct state st_ask_more;
struct state st_ask_more_purchased;

struct state st_dedicated_buyballsqueue;

#endif

/*---------------------------------------------------------------------------*/

enum
{
    /* Some enumerations were FINALLY removed in this future! */
    FAIL_SAME = GUI_LAST,
    FAIL_CHECKPOINT_RESPAWN,
    FAIL_CHECKPOINT_CANCEL,
    FAIL_ZEN_SWITCH,
    FAIL_ASK_MORE,
    FAIL_SAVE, /* We're just reverted back for you! */
    FAIL_OVER,
    FAIL_UPGRADE_EDITION,
    FAIL_TRANSFER_MEMBER
};

enum ask_more_options
{
    ASK_MORE_TIME,
    ASK_MORE_BALLS
};

static int ask_more_target;

static int resume;
static int status;

static int respawnable;
static int balls_bought;

#if defined(__EMSCRIPTEN__) || _MSC_VER
void detect_replay_filters(int exceeded);
#endif

static int fail_action(int tok, int val)
{
    int save = config_get_d(CONFIG_ACCOUNT_SAVE);
    audio_play(GUI_BACK == tok ? AUD_BACK : AUD_MENU, 1.0f);

    /* Some tokens were FINALLY removed in this future! */
    switch (tok)
    {
    case GUI_BACK:
    case FAIL_OVER:
        progress_stop();
        detect_replay_filters(
            (status == GAME_FALL && save < 3) || (status == GAME_TIME && save < 2) ||
            (campaign_hardcore() && campaign_hardcore_norecordings())
        );
        powerup_stop();
        return goto_exit();

    /* We're just reverted back for you! */
    case FAIL_SAVE:
        progress_stop();
        return goto_save(&st_fail, &st_fail);

#ifdef MAPC_INCLUDES_CHKP
    /* New: Checkpoints */
    case FAIL_CHECKPOINT_RESPAWN:
        if (progress_same_avail())
        {
            powerup_stop();
            checkpoints_respawn();
            return progress_same() ? goto_state(campaign_used() ? &st_play_ready : &st_level) : 1;
        }
        break;

    /* Need permanent restart the level? */
    case FAIL_CHECKPOINT_CANCEL:
        powerup_stop();
        respawnable = 0;
        goto_state(&st_fail);
        break;
#endif

#if NB_HAVE_PB_BOTH==1
    case FAIL_ZEN_SWITCH:
        return goto_state(&st_zen_warning);

    case FAIL_ASK_MORE:
        ask_more_target = val;
        return goto_state(&st_ask_more);
        break;
#endif

    case FAIL_SAME:
#ifdef MAPC_INCLUDES_CHKP
        if (progress_same_avail())
            checkpoints_stop();
#endif

        if (progress_same())
        {
            powerup_stop();
            return goto_state(campaign_used() ? &st_play_ready : &st_level);
        }
        break;

    case FAIL_UPGRADE_EDITION:
#if _WIN32
        system("start msedge https://forms.gle/62iaMCNKan4z2SJs5");
#elif __APPLE__
        system("open https://forms.gle/62iaMCNKan4z2SJs5");
#else
        system("x-www-browser https://forms.gle/62iaMCNKan4z2SJs5");
#endif
        break;

    case FAIL_TRANSFER_MEMBER:
#if _WIN32
        system("start msedge https://discord.gg/qnJR263Hm2");
#elif __APPLE__
        system("open https://discord.gg/qnJR263Hm2");
#else
        system("x-www-browser https://discord.gg/qnJR263Hm2");
#endif
        break;
    }

    return 1;
}

void detect_replay_checkpoints(void)
{
    int save = config_get_d(CONFIG_ACCOUNT_SAVE);

#ifdef MAPC_INCLUDES_CHKP
    if (progress_extended() && !last_active)
#else
    if (progress_extended())
#endif
        detect_replay_filters(
            (status == GAME_FALL && save < 3) || (status == GAME_TIME && save < 2) ||
            (campaign_hardcore() && campaign_hardcore_norecordings())
        );
}

void detect_replay_filters(int exceeded)
{
    /* Delete replay permanently with filters (view replay guidelines) */
    if (exceeded) demo_play_stop(1);
}

static int fail_gui(void)
{
    int fid = 0, id = 0, jd = 0, nosaveid = 0;

    const char *label = "";

    if (status == GAME_FALL)
        label = _("Fall-out!");
    else if (status == GAME_TIME)
        label = _("Time's Up!");

    if ((id = gui_vstack(0)))
    {
#ifdef CONFIG_INCLUDES_ACCOUNT
        int save = config_get_d(CONFIG_ACCOUNT_SAVE);
#else
        int save = 2;
#endif

        if ((jd = gui_vstack(id)))
        {
#if NB_HAVE_PB_BOTH==1
            if (balls_bought == 0)
#endif
            {
                fid = gui_title_header(jd, label, GUI_LRG, gui_gry, gui_red);

#if NB_HAVE_PB_BOTH==1
                if (status == GAME_FALL && (save < 3 || campaign_hardcore_norecordings()))
                {
                    audio_music_fade_out(0.f);
                    audio_play(AUD_INTRO_SHATTER, 1.0f);

#if defined(COVID_HIGH_RISK)
                    /* Unsaved replay files dissapear during covid high risks! */
                    demo_play_stop(1);
                    nosaveid = gui_multi(jd, FAIL_ERROR_REPLAY_COVID_HIGHRISK, GUI_SML, gui_red, gui_red);
#else
                    detect_replay_checkpoints();
                    nosaveid = gui_multi(jd, FAIL_ERROR_REPLAY, GUI_SML, gui_red, gui_red);
#endif
                    gui_pulse(nosaveid, 1.2f);

                    if (last_active)
                    {
                        if (((last_time > 60.0f && last_timer_down) || !last_timer_down) && progress_same_avail() && !campaign_hardcore())
                        {
                            audio_play(AUD_RESPAWN, 1.0f);
                            gui_multi(jd, _("Respawn is still available during active!"), GUI_SML, gui_grn, gui_grn);
                        }
                        else if (progress_same_avail() && !campaign_hardcore())
                        {
                            audio_music_fade_out(0.f);
                            gui_multi(jd, FAIL_ERROR_RESPAWN_1, GUI_SML, gui_red, gui_red);
                        }
                        else if (!campaign_hardcore() && progress_dead())
                        {
                            audio_music_fade_out(0.f);
                            gui_multi(jd, FAIL_ERROR_RESPAWN_2, GUI_SML, gui_red, gui_red);
                        }
                        else if (progress_dead())
                        {
                            audio_music_fade_out(0.f);
                            gui_multi(jd, FAIL_ERROR_RESPAWN_3, GUI_SML, gui_red, gui_red);
                        }
                    }

                    if (server_policy_get_d(SERVER_POLICY_SHOP_ENABLED))
                    {
                        if (progress_dead() && server_policy_get_d(SERVER_POLICY_EDITION) == -1)
                            gui_multi(jd, FAIL_UPGRADE_EDITION_2, GUI_SML, gui_red, gui_red);

                        else if (curr_mode() == MODE_NORMAL && progress_extended() && server_policy_get_d(SERVER_POLICY_EDITION) == -1)
                            gui_multi(jd, FAIL_UPGRADE_EDITION_1, GUI_SML, gui_red, gui_red);

                        else if (server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_MANAGED) == 0)
                            gui_multi(jd, FAIL_ERROR_SERVER_POLICY_SHOP_MANAGED, GUI_SML, gui_red, gui_red);
                    }
                    else if (((curr_mode() == MODE_NORMAL && progress_extended()) || progress_dead()) && !server_policy_get_d(SERVER_POLICY_SHOP_ENABLED) && server_policy_get_d(SERVER_POLICY_EDITION) > -1)
                    {
                        audio_music_fade_out(0.f);
                        gui_multi(jd, FAIL_ERROR_SERVER_POLICY_SHOP, GUI_SML, gui_red, gui_red);
                    }
                    else if (server_policy_get_d(SERVER_POLICY_EDITION) == -1)
                    {
                        audio_music_fade_out(0.f);
                        gui_multi(jd, FAIL_UPGRADE_EDITION_2, GUI_SML, gui_red, gui_red);
                    }
                    gui_space(id);
                }
                else if (status == GAME_TIME && (save < 2 || campaign_hardcore_norecordings()))
                {
                    detect_replay_checkpoints();
                    audio_music_fade_out(0.f);
                    audio_play(AUD_INTRO_SHATTER, 1.0f);
                    nosaveid = gui_multi(jd, FAIL_ERROR_REPLAY, GUI_SML, gui_red, gui_red);
                    gui_pulse(nosaveid, 1.2f);

#ifdef MAPC_INCLUDES_CHKP
                    if (last_active)
                    {
                        if (((last_time > 60.0f && last_timer_down) || !last_timer_down) && progress_same_avail() && !campaign_hardcore())
                        {
                            audio_play(AUD_RESPAWN, 1.0f);
                            gui_multi(jd, _("Respawn is still available during active!"), GUI_SML, gui_grn, gui_grn);
                        }
                        else if (progress_same_avail() && !campaign_hardcore())
                        {
                            audio_music_fade_out(0.f);
                            gui_multi(jd, FAIL_ERROR_RESPAWN_1, GUI_SML, gui_red, gui_red);
                        }
                        else if (!campaign_hardcore() && progress_dead())
                        {
                            audio_music_fade_out(0.f);
                            gui_multi(jd, FAIL_ERROR_RESPAWN_2, GUI_SML, gui_red, gui_red);
                        }
                        else if (progress_dead())
                        {
                            audio_music_fade_out(0.f);
                            gui_multi(jd, FAIL_ERROR_RESPAWN_3, GUI_SML, gui_red, gui_red);
                        }
                    }
#endif

                    if (server_policy_get_d(SERVER_POLICY_SHOP_ENABLED))
                    {
                        if (progress_dead() && server_policy_get_d(SERVER_POLICY_EDITION) == -1)
                            gui_multi(jd, FAIL_UPGRADE_EDITION_2, GUI_SML, gui_red, gui_red);

                        else if (curr_mode() == MODE_NORMAL && progress_extended() && server_policy_get_d(SERVER_POLICY_EDITION) == -1)
                            gui_multi(jd, FAIL_UPGRADE_EDITION_1, GUI_SML, gui_red, gui_red);

                        else if (server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_MANAGED) == 0)
                            gui_multi(jd, FAIL_ERROR_SERVER_POLICY_SHOP_MANAGED, GUI_SML, gui_red, gui_red);
                    }
                    else if (((curr_mode() == MODE_NORMAL && progress_extended()) || progress_dead()) && !server_policy_get_d(SERVER_POLICY_SHOP_ENABLED) && server_policy_get_d(SERVER_POLICY_EDITION) > -1)
                    {
                        audio_music_fade_out(0.f);
                        gui_multi(jd, FAIL_ERROR_SERVER_POLICY_SHOP, GUI_SML, gui_red, gui_red);
                    }
                    else if (server_policy_get_d(SERVER_POLICY_EDITION) == -1)
                    {
                        audio_music_fade_out(0.f);
                        gui_multi(jd, FAIL_UPGRADE_EDITION_2, GUI_SML, gui_red, gui_red);
                    }

                    gui_space(id);
                }
                else
                {
                    audio_music_fade_out(0.f);
                    audio_play(AUD_INTRO_SHATTER, 1.0f);
                    nosaveid = gui_multi(jd, _("You can save new replays only once!"), GUI_SML, gui_red, gui_red);
                    gui_pulse(nosaveid, 1.2f);

                    if (last_active)
                    {
                        /* Optional can be save */
                        if (((last_time > 60.0f && last_timer_down) || !last_timer_down) && progress_same_avail() && !campaign_hardcore())
                        {
                            audio_play(AUD_RESPAWN, 1.0f);
                            gui_multi(jd, _("Respawn is still available during active!"), GUI_SML, gui_grn, gui_grn);
                        }
                        else if (progress_same_avail() && !campaign_hardcore())
                        {
                            audio_music_fade_out(0.f);
                            audio_play(AUD_INTRO_SHATTER, 1.0f);
                            gui_multi(jd, FAIL_ERROR_RESPAWN_1, GUI_SML, gui_red, gui_red);
                        }
                        else if (!campaign_hardcore() && progress_dead())
                        {
                            audio_music_fade_out(0.f);
                            audio_play(AUD_INTRO_SHATTER, 1.0f);
                            gui_multi(jd, FAIL_ERROR_RESPAWN_2, GUI_SML, gui_red, gui_red);
                        }
                        else if (progress_dead())
                        {
                            audio_music_fade_out(0.f);
                            audio_play(AUD_INTRO_SHATTER, 1.0f);
                            gui_multi(jd, FAIL_ERROR_RESPAWN_3, GUI_SML, gui_red, gui_red);
                        }

                        if (server_policy_get_d(SERVER_POLICY_SHOP_ENABLED) && server_policy_get_d(SERVER_POLICY_EDITION) > -1)
                        {
                            if (progress_dead() && server_policy_get_d(SERVER_POLICY_EDITION) == -1)
                            {
                                audio_music_fade_out(0.f);
                                audio_play(AUD_INTRO_SHATTER, 1.0f);
                                gui_multi(jd, FAIL_UPGRADE_EDITION_2, GUI_SML, gui_red, gui_red);
                            }
                            else if (curr_mode() == MODE_NORMAL && progress_extended() && server_policy_get_d(SERVER_POLICY_EDITION) == -1)
                            {
                                audio_music_fade_out(0.f);
                                audio_play(AUD_INTRO_SHATTER, 1.0f);
                                gui_multi(jd, FAIL_UPGRADE_EDITION_1, GUI_SML, gui_red, gui_red);
                            }
                            else if (server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_MANAGED) == 0)
                            {
                                audio_music_fade_out(0.f);
                                audio_play(AUD_INTRO_SHATTER, 1.0f);
                                gui_multi(jd, FAIL_ERROR_SERVER_POLICY_SHOP_MANAGED, GUI_SML, gui_red, gui_red);
                            }
                        }
                        else if (((curr_mode() == MODE_NORMAL && progress_extended()) || progress_dead()) && !server_policy_get_d(SERVER_POLICY_SHOP_ENABLED) && server_policy_get_d(SERVER_POLICY_EDITION) > -1)
                        {
                            audio_music_fade_out(0.f);
                            audio_play(AUD_INTRO_SHATTER, 1.0f);
                            gui_multi(jd, FAIL_ERROR_SERVER_POLICY_SHOP, GUI_SML, gui_red, gui_red);
                        }
                        else if (server_policy_get_d(SERVER_POLICY_EDITION) == -1)
                        {
                            audio_music_fade_out(0.f);
                            audio_play(AUD_INTRO_SHATTER, 1.0f);
                            gui_multi(jd, FAIL_UPGRADE_EDITION_2, GUI_SML, gui_red, gui_red);
                        }

                        gui_space(id);
                    }
                }
#else
                if (progress_dead())
                {
                    audio_music_fade_out(0.f);
                    audio_play(AUD_INTRO_SHATTER, 1.0f);
                    gui_multi(jd, FAIL_TRANSFER_MEMBER, GUI_SML, gui_red, gui_red);
                }
#endif            
            }
#if NB_HAVE_PB_BOTH==1
            else
                gui_title_header(jd, _("Purchased!"), GUI_LRG, gui_blu, gui_grn);
#endif

            gui_set_rect(jd, GUI_ALL);
        }

#if defined(CONFIG_INCLUDES_ACCOUNT) && defined(NB_HAVE_PB_BOTH)
        if (progress_same_avail() && !respawnable) {
            if (account_get_d(ACCOUNT_PRODUCT_MEDIATION) == 0 && status == GAME_TIME && curr_mode() == MODE_NORMAL &&
                (server_policy_get_d(SERVER_POLICY_EDITION) > -1 && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED) && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_MANAGED)))
            {
                gui_state(id, _("Ask for more time!"), GUI_SML, FAIL_ASK_MORE, ASK_MORE_TIME);
                gui_space(id);
            }
            else if (curr_mode() == MODE_NORMAL && curr_mode() != MODE_ZEN && status == GAME_TIME && !progress_extended() &&
                (server_policy_get_d(SERVER_POLICY_EDITION) > -1 && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED) && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_MANAGED)))
            {
                gui_state(id, _("Buy Mediation!"), GUI_SML, FAIL_ASK_MORE, ASK_MORE_TIME);
                gui_space(id);
            }
        }
#endif

        if ((jd = gui_harray(id)))
        {
#if NB_HAVE_PB_BOTH==1
#if defined(MAPC_INCLUDES_CHKP) && defined(LEVELGROUPS_INCLUDES_CAMPAIGN)
            if (respawnable && progress_same_avail() && !campaign_hardcore())
            {
                gui_start(jd, _("Cancel"), GUI_SML, FAIL_CHECKPOINT_CANCEL, 0);

                /* New: Checkpoints; An optional can be respawn last location */
                if (progress_same_avail() && last_active && ((last_time > 60.0f && last_timer_down) || !last_timer_down))
                    gui_state(jd, _("Respawn"), GUI_SML, FAIL_CHECKPOINT_RESPAWN, 0);
                else if (!campaign_hardcore() && progress_dead() && server_policy_get_d(SERVER_POLICY_EDITION) > -1 && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED))
                    gui_state(jd, _("Buy more balls!"), GUI_SML, FAIL_ASK_MORE, ASK_MORE_BALLS);
                else if (!campaign_hardcore() && server_policy_get_d(SERVER_POLICY_EDITION) > -1 && progress_dead())
                    gui_state(jd, _("Upgrade edition!"), GUI_SML, FAIL_UPGRADE_EDITION, 0);
            }
            else
#elif defined(MAPC_INCLUDES_CHKP)
            if (respawnable && progress_same_avail())
            {
                gui_start(jd, _("Cancel"), GUI_SML, FAIL_CHECKPOINT_CANCEL, 0);

                /* New: Checkpoints; An optional can be respawn last location */
                if (progress_same_avail() && last_active && ((last_time > 60.0f && last_timer_down) || !last_timer_down))
                    gui_state(jd, _("Respawn"), GUI_SML, FAIL_CHECKPOINT_RESPAWN, 0);
                else if (progress_dead() && server_policy_get_d(SERVER_POLICY_EDITION) > -1 && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED))
                    gui_state(jd, _("Buy more balls!"), GUI_SML, FAIL_ASK_MORE, ASK_MORE_BALLS);
                else if (server_policy_get_d(SERVER_POLICY_EDITION) > -1 && progress_dead())
                    gui_state(jd, _("Upgrade edition!"), GUI_SML, FAIL_UPGRADE_EDITION, 0);
            }
            else
#endif
#endif
            {
                /* Some buttons were FINALLY removed in this future (e.g. death_screen.json in Minecraft Android, iOS or Windows)! */
                gui_start(jd, _("Exit"), GUI_SML, FAIL_OVER, 0);
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                if (progress_same_avail() && !campaign_hardcore())
                    gui_state(jd, _("Retry Level"), GUI_SML, FAIL_SAME, 0);
#else
                if (progress_same_avail())
                    gui_state(jd, _("Retry Level"), GUI_SML, FAIL_SAME, 0);
#ifndef NB_HAVE_PB_BOTH
                else
                    gui_state(jd, _("Join PB"), GUI_SML, FAIL_TRANSFER_MEMBER, 0);
#endif
#endif
#ifdef CONFIG_INCLUDES_ACCOUNT
                else if (!campaign_hardcore() && !respawnable && progress_dead() && server_policy_get_d(SERVER_POLICY_EDITION) > -1 && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED))
                    gui_state(jd, _("Buy more balls!"), GUI_SML, FAIL_ASK_MORE, ASK_MORE_BALLS);
#endif
#if NB_HAVE_PB_BOTH==1
                else if (!campaign_hardcore() && server_policy_get_d(SERVER_POLICY_EDITION) == -1 && progress_dead())
                    gui_state(jd, _("Upgrade edition!"), GUI_SML, FAIL_UPGRADE_EDITION, 0);
#endif

#if defined(CONFIG_INCLUDES_ACCOUNT) && defined(NB_HAVE_PB_BOTH)
                if (account_get_d(ACCOUNT_PRODUCT_MEDIATION) == 1 && status == GAME_TIME && curr_mode() == MODE_NORMAL)
                    gui_state(jd, _("Switch to Zen"), GUI_SML, FAIL_ZEN_SWITCH, 0);
#endif
                /* We're just reverted back for you! */
                if (demo_saved())
                {
                    if (save == 3 && status == GAME_FALL)
                        gui_state(jd, _("Save Replay"), GUI_SML, FAIL_SAVE, 0);

                    if (save >= 2 && status == GAME_TIME)
                        gui_state(jd, _("Save Replay"), GUI_SML, FAIL_SAVE, 0);
                }
            }
        }

        gui_pulse(fid, 1.2f);
        gui_layout(id, 0, 0);
    }

    return id;
}

static int fail_enter(struct state *st, struct state *prev)
{
    audio_music_fade_out(1.0f);
    video_clr_grab();

    /* Check if we came from a known previous state. */

    resume = (prev != &st_play_loop);

    /* Note the current status if we got here from elsewhere. */

    if (!resume)
    {
#ifdef MAPC_INCLUDES_CHKP
        respawnable = last_active;
#endif
        status = curr_status();
        balls_bought = 0;
    }

    return fail_gui();
}

static void fail_paint(int id, float t)
{
    game_client_draw(0, t);
    
    gui_paint(id);
#if !defined(__EMSCRIPTEN__)
    if (xbox_show_gui())
        xbox_control_death_gui_paint();
    if (hud_visibility())
#endif
        hud_paint();

}

static void fail_timer(int id, float dt)
{
    if (status == GAME_FALL)
    {
        /* Uncomment, if you have game "crash balls" implemented. */
        /*
        {
            geom_step(dt);
            game_server_step(dt);

            int record_screenanimations = time_state() < (config_get_d(CONFIG_SCREEN_ANIMATIONS) ? 2.5f : 2.f);
            int record_modes = curr_mode() != MODE_NONE;
            int record_campaign = !campaign_hardcore_norecordings();

            if (!resume)
                game_client_sync(record_screenanimations
                              && record_modes
                              && record_campaign ? demo_fp : NULL);
            else game_client_sync(NULL);
            game_client_blend(game_server_blend());
        }*/
    }

    gui_timer(id, dt);
}

static int fail_keybd(int c, int d)
{
    if (d)
    {
        /*
         * This is revealed in death_screen.json.
         * This methods can't use this: fail_action(GUI_BACK, 0);
         */
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
        if (c == KEY_EXIT && current_platform == PLATFORM_PC)
            return 1;
#else
        if (c == KEY_EXIT)
            return 1;
#endif

        /*
         * There is still available in this checkpoints,
         * but if you permanent restart the default location,
         * all checkpoints will erased.
         */
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1 && defined(LEVELGROUPS_INCLUDES_CAMPAIGN)
        if (config_tst_d(CONFIG_KEY_RESTART, c) && progress_same_avail() && !campaign_hardcore() && !respawnable && current_platform == PLATFORM_PC)
#else
        if (config_tst_d(CONFIG_KEY_RESTART, c) && progress_same_avail())
#endif
        {
            if (progress_same())
            {
#ifdef MAPC_INCLUDES_CHKP
                checkpoints_stop();
#endif
                goto_state(&st_play_ready);
            }
        }
    }
    return 1;
}

static int fail_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return fail_action(gui_token(active), gui_value(active));
        /*if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return fail_action(GUI_BACK, 0);*/
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

#if NB_HAVE_PB_BOTH==1

enum
{
    ZEN_SWITCH_CANCEL = GUI_LAST,
    ZEN_SWITCH_ACCEPT
};

static int zen_warning_action(int tok, int val)
{
    audio_play(AUD_MENU, 1.f);

    switch (tok)
    {
    case ZEN_SWITCH_CANCEL:
        return goto_state(&st_fail);
        break;
    case ZEN_SWITCH_ACCEPT:
#ifdef LEVELGROUPS_INCLUDES_ZEN
        mediation_init();
        progress_init(MODE_ZEN);
        if (progress_same())
        {
            checkpoints_stop();
            return goto_state(campaign_used() ? &st_play_ready : &st_level);
        }
#endif
        break;
    }

    return 1;
}

static int zen_warning_enter(struct state *st, struct state *prev)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("Warning!"), GUI_MED, gui_red, gui_red);
        gui_space(id);
        gui_multi(id, _("If you switch to Zen Mode,\\all Achievements will disabled!\\Are you sure want to DO that?"), GUI_SML, gui_wht, gui_wht);
        gui_space(id);

        if ((jd = gui_harray(id)))
        {
            gui_start(jd, _("Cancel"), GUI_SML, ZEN_SWITCH_CANCEL, 0);
            gui_state(jd, _("Switch"), GUI_SML, ZEN_SWITCH_ACCEPT, 0);
        }
    }

    gui_layout(id, 0, 0);
    return id;
}

static int zen_warning_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT)
            return zen_warning_action(ZEN_SWITCH_CANCEL, 0);
    }
    return 1;
}

static int zen_warning_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return zen_warning_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return zen_warning_action(ZEN_SWITCH_CANCEL, 0);
    }
    return 1;
}

#endif

/*---------------------------------------------------------------------------*/

#if NB_HAVE_PB_BOTH==1

enum
{
    ASK_MORE_CANCEL = GUI_LAST,
    ASK_MORE_GET_COINS,
    ASK_MORE_GET_GEMS,
    ASK_MORE_ACCEPT,
    ASK_MORE_BUY
};

static int ask_more_action(int tok, int val)
{
    audio_play(AUD_MENU, 1.f);

    switch (tok)
    {
    case ASK_MORE_CANCEL:
        return goto_state(&st_fail);
        break;
    case ASK_MORE_GET_COINS:
        return goto_shop_iap(&st_ask_more_purchased, &st_fail, val, 0, 0);
        break;
#if NB_STEAM_API==1 || NB_EOS_SDK==1
    case ASK_MORE_GET_GEMS:
        return goto_shop_iap(&st_ask_more_purchased, &st_fail, val, 1, 0);
        break;
#endif
    case ASK_MORE_ACCEPT:
        video_set_grab(1);
        audio_music_fade_in(0.5f);
        game_extend_time(val);
        progress_extend();
        return goto_state(&st_play_loop);
        break;

    case ASK_MORE_BUY:
#ifdef CONFIG_INCLUDES_ACCOUNT
#ifdef LEVELGROUPS_INCLUDES_ZEN
        if (account_get_d(ACCOUNT_DATA_WALLET_COINS) >= 120 && ask_more_target == ASK_MORE_TIME)
        {
            audio_play("snd/buyproduct.ogg", 1.0f);
            int coinwallet = account_get_d(ACCOUNT_DATA_WALLET_COINS) - 120;
            account_set_d(ACCOUNT_DATA_WALLET_COINS, coinwallet);
            account_set_d(ACCOUNT_PRODUCT_MEDIATION, 1);
            account_save();

            mediation_init();
            progress_init(MODE_ZEN);
            if (progress_same())
            {
                checkpoints_stop();
                return goto_state(campaign_used() ? &st_play_ready : &st_level);
            }
        }
        else if (account_get_d(ACCOUNT_DATA_WALLET_GEMS) >= 15 && ask_more_target == ASK_MORE_TIME)
        {
            audio_play("snd/buyproduct.ogg", 1.0f);
            int gemwallet = account_get_d(ACCOUNT_DATA_WALLET_GEMS) - 50;
            account_set_d(ACCOUNT_DATA_WALLET_GEMS, gemwallet);

            int coinwallet = account_get_d(ACCOUNT_DATA_WALLET_COINS) + 130;
            account_set_d(ACCOUNT_DATA_WALLET_COINS, coinwallet);

            account_set_d(ACCOUNT_PRODUCT_MEDIATION, 1);
            account_save();

            mediation_init();
            progress_init(MODE_ZEN);
            if (progress_same())
            {
                checkpoints_stop();
                return goto_state(campaign_used() ? &st_play_ready : &st_level);
            }
        }
#endif

        else if (account_get_d(ACCOUNT_DATA_WALLET_GEMS) >= 15 && ask_more_target == ASK_MORE_BALLS)
        {
            audio_play("snd/buyproduct.ogg", 1.0f);

#if ENABLE_DEDICATED_SERVER==1
            int net_marker_result = -1;
            net_marker_result = networking_dedicated_buyballs_send(1);

            if (net_marker_result == -1)
            {
                /* Waiting for approval */
            }
#else
            int coinwallet = account_get_d(ACCOUNT_DATA_WALLET_GEMS) - 15;
            account_set_d(ACCOUNT_DATA_WALLET_GEMS, coinwallet);
            account_save();

            progress_buy_balls(1);

            balls_bought = 1;

            if (last_active && status == GAME_FALL) /* Should be respawn, or not! */
                return goto_state(&st_fail); /* Back to respawn state! */
            else if (progress_same())
                return goto_state(campaign_used() ? &st_play_ready : &st_level);
            else
                return goto_state(&st_fail); /* An error occured, because the level is not loaded! */
#endif
        }
#endif
        break;
    }

    return 1;
}

static int ask_more_enter(struct state *st, struct state *prev)
{
#ifdef CONFIG_INCLUDES_ACCOUNT
    int coinwallet = account_get_d(ACCOUNT_DATA_WALLET_COINS);
    int gemswallet = account_get_d(ACCOUNT_DATA_WALLET_GEMS);
#else
    int coinwallet = 0;
    int gemswallet = 0;
#endif

    int topbar;
    int id, jd;

    if ((id = gui_vstack(0)))
    {
#ifdef CONFIG_INCLUDES_ACCOUNT
        if ((topbar = gui_hstack(id)))
        {
            gui_filler(topbar);

            if (curr_mode() == MODE_NORMAL)
            {
                char coinsattr[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(coinsattr, dstSize, "%s: %i", _("Coins"), coinwallet);
#else
                sprintf(coinsattr, "%s: %i", _("Coins"), coinwallet);
#endif
                gui_label(topbar, coinsattr, GUI_SML, gui_wht, gui_yel);
            }

            if (progress_dead())
            {
                char gemsattr[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(gemsattr, dstSize, "%s: %i", _("Gems"), gemswallet);
#else
                sprintf(gemsattr, "%s: %i", _("Gems"), gemswallet);
#endif
                gui_label(topbar, gemsattr, GUI_SML, gui_wht, gui_cya);
            }

            gui_filler(topbar);
        }

        gui_space(id);
#endif

        if (ask_more_target == ASK_MORE_BALLS)
        {
#if NB_STEAM_API==1 || NB_EOS_SDK==1
            if (gemswallet >= 15 || server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_IAP))
#else
            if (gemswallet >= 15)
#endif
                gui_title_header(id, _("Buy more balls?"), GUI_MED, gui_gry, gui_red);
            else
                gui_title_header(id, _("Sorry!"), GUI_MED, gui_gry, gui_red);
        }
        else
        {
            if (server_policy_get_d(SERVER_POLICY_SHOP_ENABLED) && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_MANAGED) && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_IAP) && !progress_extended())
                gui_title_header(id, _("Ask for more time?"), GUI_MED, gui_gry, gui_red);
            else if (coinwallet >= 120 || (server_policy_get_d(SERVER_POLICY_SHOP_ENABLED) && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_MANAGED) && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_IAP)))
                gui_title_header(id, _("Buy Mediation?"), GUI_MED, gui_gry, gui_red);
            else
                gui_title_header(id, _("Sorry!"), GUI_MED, gui_gry, gui_red);
        }

        gui_space(id);

        if (ask_more_target == ASK_MORE_BALLS)
        {
#if NB_STEAM_API==1 || NB_EOS_SDK==1
            if (gemswallet >= 15 || server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_IAP))
#else
            if (gemswallet >= 15)
#endif
            {
                if (last_active)
                    gui_multi(id, _("You want to buy more balls\\and respawn from checkpoint?\\ \\You need 15 gems from your wallet!"), GUI_SML, gui_wht, gui_wht);
                else
                    gui_multi(id, _("You want to buy more balls\\and restart the level?\\ \\You need 15 gems from your wallet!"), GUI_SML, gui_wht, gui_wht);
            }
            else
                gui_multi(id, _("You don't have enough gems\\to buy more balls!"), GUI_SML, gui_wht, gui_wht);
        }
        else
        {
            if (curr_mode() == MODE_NORMAL && coinwallet >= 120 && !progress_extended() &&
                (server_policy_get_d(SERVER_POLICY_SHOP_ENABLED) && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_MANAGED) && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_IAP)))
                gui_multi(id, _("If you want to extend more time, then\\please buy the mediation for 120 coins.\\Once you've bought, you will not be able\\to unlock achievements."), GUI_SML, gui_wht, gui_wht);
            else if (curr_mode() == MODE_NORMAL && coinwallet >= 120 && progress_extended() &&
                (server_policy_get_d(SERVER_POLICY_SHOP_ENABLED) && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_MANAGED) && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_IAP)))
                gui_multi(id, _("Buy the mediation for 120 coins or 50 Gems?\\You will not be able to unlock achievements."), GUI_SML, gui_wht, gui_wht);
            else if (server_policy_get_d(SERVER_POLICY_SHOP_ENABLED) && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_MANAGED) && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_IAP))
                gui_multi(id, _("You need at least 120 coins to buy Mediation,\\but you can purchase from coin shop."), GUI_SML, gui_wht, gui_wht);
            else if (!progress_extended())
                gui_multi(id, _("Are you sure want to extend\\more time to finish the level?"), GUI_SML, gui_wht, gui_wht);
            else
                gui_multi(id, _("You don't have enough coins\\to buy Mediation!"), GUI_SML, gui_wht, gui_wht);
        }

        gui_space(id);

        if (ask_more_target == ASK_MORE_BALLS)
        {
            if ((jd = gui_harray(id)))
            {
                if (gemswallet >= 15)
                {
                    gui_start(jd, _("No, thanks!"), GUI_SML, ASK_MORE_CANCEL, 0);
                    gui_state(jd, _("Buy now!"), GUI_SML, ASK_MORE_BUY, 0);
                }
                else if (server_policy_get_d(SERVER_POLICY_SHOP_ENABLED) && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_IAP))
                {
                    gui_start(jd, _("No, thanks!"), GUI_SML, ASK_MORE_CANCEL, 0);
                    gui_state(jd, _("Get Gems"), GUI_SML, ASK_MORE_GET_GEMS, 15);
                }
                else
                    gui_start(jd, _("OK"), GUI_SML, ASK_MORE_CANCEL, 0);
            }
        }
        else
        {
            if ((jd = gui_harray(id)))
            {
                int extendvalue = 10;
                char approveattr[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(approveattr, dstSize, _("Extend (+%ds)"), extendvalue);
#else
                sprintf(approveattr, _("Extend (+%ds)"), extendvalue);
#endif

                if (curr_mode() == MODE_NORMAL)
                {
                    if (coinwallet >= 120)
                    {
                        gui_start(jd, _("No, thanks!"), GUI_SML, ASK_MORE_CANCEL, 0);
                        if (!progress_extended())
                            gui_state(jd, approveattr, GUI_SML, ASK_MORE_ACCEPT, extendvalue);
                        gui_state(jd, _("Purchase"), GUI_SML, ASK_MORE_BUY, 0);
                    }
                    if (gemswallet >= 50)
                    {
                        gui_start(jd, _("No, thanks!"), GUI_SML, ASK_MORE_CANCEL, 0);
                        if (!progress_extended())
                            gui_state(jd, approveattr, GUI_SML, ASK_MORE_ACCEPT, extendvalue);
                        gui_state(jd, _("Purchase"), GUI_SML, ASK_MORE_BUY, 0);

                        if (server_policy_get_d(SERVER_POLICY_SHOP_ENABLED) && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_IAP))
                            gui_state(jd, _("Get Coins"), GUI_SML, ASK_MORE_GET_COINS, 120);
                    }
#if NB_STEAM_API==1 || NB_EOS_SDK==1
                    else if (server_policy_get_d(SERVER_POLICY_SHOP_ENABLED) && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_IAP))
#else
                    else if (server_policy_get_d(SERVER_POLICY_SHOP_ENABLED) && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_IAP) && !progress_extended())
#endif
                    {
                        gui_start(jd, _("No, thanks!"), GUI_SML, ASK_MORE_CANCEL, 0);
                        if (!progress_extended())
                            gui_state(jd, approveattr, GUI_SML, ASK_MORE_ACCEPT, extendvalue);
#if NB_STEAM_API==1 || NB_EOS_SDK==1
                        gui_state(jd, _("Get Gems"), GUI_SML, ASK_MORE_GET_GEMS, 50);
#endif
                    }
                    else
                        gui_start(jd, _("OK"), GUI_SML, ASK_MORE_CANCEL, 0);
                }
                else if (!progress_extended())
                {
                    gui_start(jd, _("No, thanks!"), GUI_SML, ASK_MORE_CANCEL, 0);
                    gui_state(jd, approveattr, GUI_SML, ASK_MORE_ACCEPT, extendvalue);
                }
                else
                {
                    assert(0 && "Unknown state");
                    gui_start(jd, _("OK"), GUI_SML, ASK_MORE_CANCEL, 0);
                }
            }
        }
    }

    gui_layout(id, 0, 0);
    return id;
}

static int ask_more_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT)
            return ask_more_action(ASK_MORE_CANCEL, 0);

    }
    return 1;
}

static int ask_more_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return ask_more_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return ask_more_action(ASK_MORE_CANCEL, 0);
    }
    return 1;
}

#endif

/*---------------------------------------------------------------------------*/

#if UNUSED

int buyballs_was_connected;

int ask_more_dedicated_buyballsqueue_gui(void)
{
    int id;

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("Purchase sent"), GUI_MED, gui_gry, gui_red);
        gui_space(id);
        gui_multi(id, _("Your purchase was sent\\and is waiting for delivery."), GUI_SML, gui_wht, gui_wht);
    }

    return id;
}

int ask_more_dedicated_buyballsqueue_enter(struct state *st, struct state *prev)
{
    return ask_more_dedicated_buyballsqueue_gui();
}

void ask_more_dedicated_buyballsqueue_timer(int id, float dt)
{
    buyballs_was_connected=networking_connected();
    if (!networking_connected())
    {
        /* Go back, if offline. */
        goto_state(&st_fail);
    }

    gui_timer(id, dt);
}

#endif

/*---------------------------------------------------------------------------*/

int ask_more_purchased_enter(struct state *st, struct state *prev)
{
    audio_play("snd/buyproduct.ogg", 1.0f);

#ifdef CONFIG_INCLUDES_ACCOUNT
    if (ask_more_target == ASK_MORE_BALLS)
    {
        int gemswallet = account_get_d(ACCOUNT_DATA_WALLET_GEMS);

        while (gemswallet >= 15)
        {
            gemswallet -= 15;
            balls_bought++;
            progress_buy_balls(1);
        }

        account_set_d(ACCOUNT_DATA_WALLET_GEMS, gemswallet);
        account_save();
        if (last_active && status == GAME_FALL) /* Should be respawn, or not! */
            return goto_state(&st_fail); /* Back to respawn state! */
        else if (progress_same())
            return goto_state(campaign_used() ? &st_play_ready : &st_level);
        else
            return goto_state(&st_fail); /* An error occured, because the level is not loaded! */
    }
    else
    {
        int coinwallet = account_get_d(ACCOUNT_DATA_WALLET_COINS) - 120;
        account_set_d(ACCOUNT_DATA_WALLET_COINS, coinwallet);
        account_save();

        return goto_state(&st_zen_warning);
    }
#endif
    return goto_state(&st_fail);
}

/*---------------------------------------------------------------------------*/

struct state st_fail = {
    fail_enter,
    shared_leave,
    fail_paint, // Default: shared_paint
    fail_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    fail_keybd,
    fail_buttn
};

#if NB_HAVE_PB_BOTH==1

struct state st_zen_warning = {
    zen_warning_enter,
    shared_leave,
    shared_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    zen_warning_keybd,
    zen_warning_buttn
};

#if UNUSED

struct state st_ask_more = {
    ask_more_enter,
    shared_leave,
    shared_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    ask_more_keybd,
    ask_more_buttn
};

struct state st_dedicated_buyballsqueue = {
    ask_more_dedicated_buyballsqueue_enter,
    shared_leave,
    shared_paint,
    ask_more_dedicated_buyballsqueue_timer,
    NULL, NULL, NULL, NULL, NULL, NULL
};

#endif

struct state st_ask_more_purchased = {
    ask_more_purchased_enter,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

#endif
