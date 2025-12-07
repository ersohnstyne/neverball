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

#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

#include <stdio.h>

/*
 * HACK: Used with console version
 */
#include "console_control_gui.h"

#if NB_HAVE_PB_BOTH==1

#include "networking.h"

#include "campaign.h" /* New: Campaign */
#include "mediation.h"
#include "boost_rush.h"
#include "account.h"

#include "lang_switchball.h"
#endif

#ifdef MAPC_INCLUDES_CHKP
#include "checkpoints.h" /* New: Checkpoints */
#endif

#if ENABLE_MOON_TASKLOADER!=0
#define SKIP_MOON_TASKLOADER
#include "moon_taskloader.h"
#endif

#include "gui.h"
#include "transition.h"
#include "hud.h"
#include "set.h"
#include "progress.h"
#include "audio.h"
#include "config.h"
#include "demo.h"
#include "lang.h"
#include "key.h"
#include "geom.h"
#include "text.h"

#ifdef CONFIG_INCLUDES_ACCOUNT
#include "powerup.h"
#endif

#include "activity_services.h"

#include "game_draw.h"
#include "game_server.h"
#include "game_client.h"
#include "game_common.h"

#include "st_common.h"
#include "st_pause.h"
#include "st_level.h"
#include "st_play.h"
#include "st_start.h"
#include "st_set.h"
#include "st_over.h"
#include "st_done.h"
#include "st_shared.h"
#include "st_title.h"
#ifdef SWITCHBALL_HAVE_TIP_AND_TUTORIAL
#include "st_tutorial.h"
#endif
#include "st_name.h"

#if NB_HAVE_PB_BOTH!=1 && (defined(CONFIG_INCLUDES_ACCOUNT) || defined(MAPC_INCLUDES_CHKP))
#error Security compilation error: Preprocessor definitions can be used it, \
       once you have transferred or joined into the target Discord Server, \
       and verified and promoted as Developer Role. \
       This invite link can be found under https://discord.gg/qnJR263Hm2/.
#endif

/*---------------------------------------------------------------------------*/

struct state st_level_loading;
struct state st_nodemo;

struct state st_poser;
struct state st_level_signin_required;

/*---------------------------------------------------------------------------*/

static int level_check_playername(const char *regname)
{
    for (int i = 0; i < text_length(regname); i++)
    {
        if (regname[i] == '\\' ||
            regname[i] == '/'  ||
            regname[i] == ':'  ||
            regname[i] == '*'  ||
            regname[i] == '?'  ||
            regname[i] == '"'  ||
            regname[i] == '<'  ||
            regname[i] == '>'  ||
            regname[i] == '|')
            return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/

#ifdef SWITCHBALL_HAVE_TIP_AND_TUTORIAL

const char level_loading_tip[][256] = {
    TIP_1,
    TIP_2,
    TIP_3,
    TIP_4,
    TIP_5,
    TIP_6,
    TIP_7,
    TIP_8
};

const char level_loading_tip_xbox[][256] = {
    TIP_1,
    TIP_2,
    TIP_3_XBOX,
    TIP_4_XBOX,
    TIP_5,
    TIP_6_XBOX
};

const char level_loading_tip_ps4[][256] = {
    TIP_1,
    TIP_2,
    TIP_3_PS4,
    TIP_4_XBOX,
    TIP_5,
    TIP_6_PS4
};

const char level_loading_covid_highrisk[][256] = {
    N_("Stash your game transfer\nto reduce risks!"),
    N_("Stash your replays with exceeded\nlevel status to reduce risks!"),
    N_("Don't use challenge game mode\nto reduce risks!"),
    N_("Use 3G+ rule to reduce risks!"),
};

#endif

#define LEVEL_MESSAGE_CHKP_POST_RESPAWN \
    _("The checkpoint is in the\nlast position as last time.\n\n" \
      "Click to continue.")

static int level_loading_enter(struct state *st, struct state *prev, int intent)
{
    int id;

    int max_index = 7;

#if NB_HAVE_PB_BOTH==1
#ifndef __EMSCRIPTEN__
    if (current_platform != PLATFORM_PC)
        max_index = 5;
#endif
#endif

    int index_affect = rand_between(0, max_index);

#ifdef SWITCHBALL_HAVE_TIP_AND_TUTORIAL
    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("Loading..."), GUI_MED, GUI_COLOR_DEFAULT);
        gui_space(id);

        while (index_affect > max_index)
            index_affect -= max_index;

#ifndef __EMSCRIPTEN__
        if (current_platform == PLATFORM_PC)
            gui_multi(id, _(level_loading_tip[index_affect]),
                          GUI_SML, GUI_COLOR_WHT);
        else if (current_platform == PLATFORM_PS)
            gui_multi(id, _(level_loading_tip_ps4[index_affect]),
                          GUI_SML, GUI_COLOR_WHT);
        else
            gui_multi(id, _(level_loading_tip_xbox[index_affect]),
                          GUI_SML, GUI_COLOR_WHT);
#else
        gui_multi(id, _(level_loading_tip[index_affect]),
                      GUI_SML, GUI_COLOR_WHT);
#endif
        gui_layout(id, 0, 0);
    }
#else
    if ((id = gui_title_header(0, _("Loading..."), GUI_MED, GUI_COLOR_DEFAULT)))
        gui_layout(id, 0, 0);
#endif

    return transition_slide(id, 1, intent);
}

/*---------------------------------------------------------------------------*/

static int show_info = 0;

#if ENABLE_MOON_TASKLOADER!=0 && !defined(SKIP_MOON_TASKLOADER)
static int level_loading_with_moon_taskloader = 1;
#endif

static int check_nodemo = 1;
static int nodemo_warnonlyonce = 1;

static int evalue;
static int fvalue;
static int svalue;

static int level_infocard_intro;

static int level_infocard_wallet_id;
static int level_infocard_title_id;
static int level_infocard_msg_id;
static int level_infocard_ctrls_id;
static int level_infocard_power_id;

enum
{
    LEVEL_START = GUI_LAST,
    LEVEL_START_POWERUP
};

static int level_action(int tok, int val)
{
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    /* HACK: Do not attempt, when the level is loading. */

    if (EM_ASM_INT({ return Neverball.wgclIsLevelLoading; })) return 1;
#endif

    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
        case LEVEL_START_POWERUP:
#if NB_HAVE_PB_BOTH==1 && \
    defined(CONFIG_INCLUDES_ACCOUNT) && defined(ENABLE_POWERUP)
            if      (val == 3)
            {
                activity_services_powerup_update(AS_POWERUP_SPEEDIFIER);
                audio_play("snd/speedifier.ogg", 1.0f);
                powerup_init_speedifier();
            }
            else if (val == 2)
            {
                activity_services_powerup_update(AS_POWERUP_FLOATIFIER);
                audio_play("snd/floatifier.ogg", 1.0f);
                powerup_init_floatifier();
            }
            else if (val == 1)
            {
                activity_services_powerup_update(AS_POWERUP_EARNINATOR);
                audio_play("snd/earninator.ogg", 1.0f);
                powerup_init_earninator();
            }

            /* Combined enum buttons: LEVEL_START */
#else
            break;
#endif

        case LEVEL_START:
            show_info = 0;

#ifdef SWITCHBALL_HAVE_TIP_AND_TUTORIAL
            if (!tutorial_check() && !hint_check())
#endif
                return goto_state(&st_play_ready);
            break;

        case GUI_BACK:
            return goto_pause(curr_state());
    }

    return 1;
}

static int level_gui(void)
{
#if NB_HAVE_PB_BOTH==1 && defined(ENABLE_POWERUP)
#ifdef CONFIG_INCLUDES_ACCOUNT
    evalue = account_get_d(ACCOUNT_CONSUMEABLE_EARNINATOR);
    fvalue = account_get_d(ACCOUNT_CONSUMEABLE_FLOATIFIER);
    svalue = account_get_d(ACCOUNT_CONSUMEABLE_SPEEDIFIER);
#else
    evalue = 0; fvalue = 0; svalue = 0;
#endif

#if ENABLE_RFD==1
    evalue += progress_rfd_get_powerup(0);
    fvalue += progress_rfd_get_powerup(1);
    svalue += progress_rfd_get_powerup(2);
#endif
#endif

    int id, jd, kd;

#ifdef MAPC_INCLUDES_CHKP
    const char *message = last_active ? LEVEL_MESSAGE_CHKP_POST_RESPAWN :
                                        level_msg(curr_level());
#else
    const char *message = level_msg(curr_level());
#endif

    if ((id = gui_vstack(0)))
    {
        int  set_special_lbl = 0;
        char set_special_txt[MAXSTR];

        const char *curr_setid = set_id(curr_set());
        char curr_setid_final[MAXSTR];

        if (!curr_setid)
        {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(curr_setid_final, MAXSTR,
#else
            sprintf(curr_setid_final,
#endif
                    _("none_%d"), curr_set());
        }
        else
            SAFECPY(curr_setid_final, curr_setid);

        if (str_starts_with(curr_setid_final, "SB") ||
            str_starts_with(curr_setid_final, "sb") ||
            str_starts_with(curr_setid_final, "Sb") ||
            str_starts_with(curr_setid_final, "sB"))
        {
            SAFECPY(set_special_txt, _("Pre-Classic Campaign"));
            set_special_lbl = 1;
        }

        if (str_starts_with(curr_setid_final, "anime"))
        {
            SAFECPY(set_special_txt, _("ANA-Exclusive"));
            set_special_lbl = 1;
        }

        if (set_special_lbl)
        {
            gui_label(id, set_special_txt, GUI_SML, gui_cya, gui_blu);
            gui_space(id);
        }

#ifdef CONFIG_INCLUDES_ACCOUNT
        if ((level_master(curr_level())
          || curr_mode() == MODE_CHALLENGE
          || curr_mode() == MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
          || curr_mode() == MODE_HARDCORE
#endif
            ) &&
            server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_CONSUMABLES))
        {
            char account_coinsattr[MAXSTR], account_gemsattr[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(account_gemsattr, MAXSTR, "%s %d", GUI_DIAMOND,
                      account_get_d(ACCOUNT_DATA_WALLET_GEMS));
            sprintf_s(account_coinsattr, MAXSTR, "%s %d", GUI_COIN,
                      account_get_d(ACCOUNT_DATA_WALLET_COINS));
#else
            sprintf(account_gemsattr, "%s %d", GUI_DIAMOND,
                    account_get_d(ACCOUNT_DATA_WALLET_GEMS));
            sprintf(account_coinsattr, "%s %d", GUI_COIN,
                    account_get_d(ACCOUNT_DATA_WALLET_COINS));
#endif

            if ((jd = gui_hstack(id)))
            {
                const int coin_id = gui_label(jd, account_gemsattr,
                                                  GUI_SML, gui_wht, gui_cya);
                const int gems_id = gui_label(jd, account_coinsattr,
                                                  GUI_SML, gui_wht, gui_yel);

                gui_set_font(coin_id, "ttf/DejaVuSans-Bold.ttf");
                gui_set_font(gems_id, "ttf/DejaVuSans-Bold.ttf");

                level_infocard_wallet_id = jd;

                if (level_infocard_intro)
                    gui_slide(level_infocard_wallet_id,
                              GUI_N | GUI_FLING | GUI_EASE_ELASTIC, 0.5f, 1.0f, 0.2f);
            }

            gui_space(id);
        }
#endif

        if ((jd = gui_hstack(id)))
        {
            gui_filler(jd);

            if ((kd = gui_vstack(jd)))
            {
                const char *ln = level_name(curr_level());
                int b = level_bonus(curr_level());
                int m = level_master(curr_level());

                const char *curr_setname = set_name(curr_set());
                char curr_setname_final[MAXSTR];

                if (!curr_setname)
                {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                    sprintf_s(curr_setname_final, MAXSTR,
#else
                    sprintf(curr_setname_final,
#endif
                            _("Untitled set name (%d)"), curr_set());
                }
                else
                        SAFECPY(curr_setname_final, curr_setname);

                char setattr[MAXSTR], lvlattr[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                if (m && curr_mode() == MODE_STANDALONE)
                    sprintf_s(lvlattr, MAXSTR, _("Master Level"));
                else if (m)
                    sprintf_s(lvlattr, MAXSTR, _("Master Level %s"), ln);
                else if (b && curr_mode() == MODE_STANDALONE)
                    sprintf_s(lvlattr, MAXSTR, _("Bonus Level"));
                else if (b)
                    sprintf_s(lvlattr, MAXSTR, _("Bonus Level %s"), ln);
                else if (curr_mode() == MODE_STANDALONE)
                    sprintf_s(lvlattr, MAXSTR, _("Level ---"));
                else
                    sprintf_s(lvlattr, MAXSTR, _("Level %s"), ln);

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                if (curr_mode() == MODE_CAMPAIGN)
                    sprintf_s(setattr, MAXSTR, "%s", mode_to_str(curr_mode(), 1));
                else
#endif
                if (curr_mode() == MODE_STANDALONE)
                    sprintf_s(setattr, MAXSTR, _("Standalone level"));
                else if (curr_mode() == MODE_NORMAL)
                    sprintf_s(setattr, MAXSTR, "%s", curr_setname_final);
                else if (curr_mode() != MODE_NONE)
                    sprintf_s(setattr, MAXSTR, _("%s: %s"), curr_setname_final, mode_to_str(curr_mode(), 1));
#else
                if (m && curr_mode() == MODE_STANDALONE)
                    sprintf(lvlattr, _("Master Level"));
                else if (m)
                    sprintf(lvlattr, _("Master Level %s"), ln);
                else if (b && curr_mode() == MODE_STANDALONE)
                    sprintf(lvlattr, _("Bonus Level"));
                else if (b)
                    sprintf(lvlattr, _("Bonus Level %s"), ln);
                else if (curr_mode() == MODE_STANDALONE)
                    sprintf(lvlattr, _("Level ---"));
                else
                    sprintf(lvlattr, _("Level %s"), ln);

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                if (curr_mode() == MODE_CAMPAIGN)
                    sprintf(setattr, "%s", mode_to_str(curr_mode(), 1));
                else
#endif
                if (curr_mode() == MODE_STANDALONE)
                    sprintf(setattr, _("Standalone level"));
                else if (curr_mode() == MODE_NORMAL)
                    sprintf(setattr, "%s", curr_setname_final);
                else if (curr_mode() != MODE_NONE)
                    sprintf(setattr, _("%s: %s"), curr_setname_final, mode_to_str(curr_mode(), 1));
#endif
                gui_title_header(kd, lvlattr,
                                     m || b ? GUI_MED : GUI_LRG,
                                     m ? gui_wht : (b ? gui_wht : 0),
                                     m ? gui_red : (b ? gui_grn : 0));

                const int mode_id = gui_label(kd, setattr, GUI_SML, GUI_COLOR_WHT);

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                if (curr_mode() == MODE_HARDCORE)
                    gui_set_color(mode_id, GUI_COLOR_RED);
#endif

                gui_set_rect(kd, GUI_ALL);
            }

            gui_filler(jd);

            level_infocard_title_id = jd;

            if (level_infocard_intro)
                gui_slide(level_infocard_title_id,
                          GUI_N | GUI_FLING | GUI_EASE_BACK, 0.0f, 0.5f, 0.0f);
        }

        gui_space(id);

#if NB_HAVE_PB_BOTH==1 && \
    defined(CONFIG_INCLUDES_ACCOUNT) && defined(ENABLE_POWERUP)
        if (show_info)
#endif
        {
            if (message && *message)
            {
                level_infocard_msg_id = gui_multi(id, message, GUI_SML, GUI_COLOR_WHT);
                gui_space(id);

                if (level_infocard_intro)
                    gui_slide(level_infocard_msg_id,
                              GUI_S | GUI_FLING | GUI_EASE_BACK, 0.0f, 0.5f, 0.0f);
            }
        }

        if ((jd = gui_hstack(id)))
        {
            gui_filler(jd);

            if ((kd = gui_start_button(jd, LEVEL_START)))
                gui_focus(kd);

#ifndef __EMSCRIPTEN__
            if ((current_platform != PLATFORM_PC &&
                 current_platform != PLATFORM_XBOX &&
                 current_platform != PLATFORM_PS &&
                 current_platform != PLATFORM_WII) &&
                !console_gui_shown())
#endif
            {
                gui_filler(jd);
                gui_back_button(jd);
            }

            gui_filler(jd);

            level_infocard_ctrls_id = jd;

            if (level_infocard_intro)
                gui_slide(level_infocard_ctrls_id,
                          GUI_S | GUI_FLING | GUI_EASE_ELASTIC, 0.0f, 1.0f, 0.2f);
        }

#if NB_HAVE_PB_BOTH==1 && \
    defined(CONFIG_INCLUDES_ACCOUNT) && defined(ENABLE_POWERUP)
        if (!show_info &&
            ((level_master(curr_level())
           || curr_mode() == MODE_CHALLENGE
           || curr_mode() == MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
           || curr_mode() == MODE_HARDCORE
#endif
             ) &&
             server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_CONSUMABLES)))
        {
            gui_space(id);

            if ((jd = gui_harray(id)))
            {
                int ced, cfd, csd;
                char pow1attr[MAXSTR], pow2attr[MAXSTR], pow3attr[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(pow3attr, MAXSTR,
#else
                sprintf(pow3attr,
#endif
                    _("Speedifier (%i)"), svalue);

                if ((csd = gui_varray(jd)))
                {
                    gui_label(csd, pow3attr, GUI_SML,
                                   svalue > 0 ? gui_grn : gui_gry,
                                   svalue > 0 ? gui_wht : gui_gry);
                    if (svalue > 0)
                        gui_set_state(csd, LEVEL_START_POWERUP, 3);
                    else
                        gui_set_state(csd, GUI_NONE, 0);
                }

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(pow2attr, MAXSTR,
#else
                sprintf(pow2attr,
#endif
                    _("Floatifier (%i)"), fvalue);

                if ((cfd = gui_varray(jd)))
                {
                    gui_label(cfd, pow2attr, GUI_SML,
                                   fvalue > 0 && curr_mode() != MODE_BOOST_RUSH ? gui_blu : gui_gry,
                                   fvalue > 0 && curr_mode() != MODE_BOOST_RUSH ? gui_wht : gui_gry);
                    if (fvalue > 0 && curr_mode() != MODE_BOOST_RUSH)
                        gui_set_state(cfd, LEVEL_START_POWERUP, 2);
                    else
                        gui_set_state(cfd, GUI_NONE, 0);
                }

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(pow1attr, MAXSTR,
#else
                sprintf(pow1attr,
#endif
                    _("Earninator (%i)"), evalue);

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
                        gui_set_state(ced, LEVEL_START_POWERUP, 1);
                    else
                        gui_set_state(ced, GUI_NONE, 0);
                }
            }

            level_infocard_power_id = jd;

            if (level_infocard_intro)
                gui_slide(level_infocard_power_id,
                          GUI_S | GUI_FLING | GUI_EASE_ELASTIC, 0.8f, 1.0f, 0.2f);
        }
#endif

        gui_layout(id, 0, 0);
    }

    return id;
}

static int level_enter(struct state *st, struct state *prev, int intent)
{
    activity_services_powerup_update(AS_POWERUP_NONE);
    //game_lerp_pose_point_reset();
    game_client_fly(1.0f);

    nodemo_warnonlyonce = prev != &st_level  &&
                          prev != &st_nodemo &&
                          prev != &st_pause;
    check_nodemo        = nodemo_warnonlyonce;

    level_infocard_intro = prev != &st_level;

    level_infocard_wallet_id = 0;
    level_infocard_title_id  = 0;
    level_infocard_msg_id    = 0;
    level_infocard_ctrls_id  = 0;
    level_infocard_power_id  = 0;

    if (level_infocard_intro)
        show_info = !((level_master(curr_level())
                    || curr_mode() == MODE_CHALLENGE
                    || curr_mode() == MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                    || curr_mode() == MODE_HARDCORE
#endif
                      ) &&
                     server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_CONSUMABLES));

    /* New: Checkpoints */
    game_client_sync(
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                     !campaign_hardcore_norecordings() &&
#endif
                     curr_mode() != MODE_NONE ? demo_fp : NULL);

    game_client_fly(1.0f);
    hud_update(0, 0.0f);

    if (level_infocard_intro)
        return level_gui();

    return transition_slide(level_gui(), 1, intent);
}

static int level_leave(struct state *st, struct state *next, int id, int intent)
{
    if (next == &st_null)
    {
        progress_exit();

        campaign_quit();
        set_quit();

        game_server_free(NULL);
        game_client_free(NULL);
    }

    if (next != &st_level)
    {
        if (config_get_d(CONFIG_SCREEN_ANIMATIONS))
        {
            if (st == &st_nodemo)
                return transition_slide(id, 0, intent);

            gui_slide(id, GUI_REMOVE, 0, 0.16f, 0);

            if (level_infocard_wallet_id)
                gui_slide(level_infocard_wallet_id,
                          GUI_N | GUI_FLING | GUI_BACKWARD, 0.0f, 0.16f, 0.0f);

            if (level_infocard_title_id)
                gui_slide(level_infocard_title_id,
                          GUI_N | GUI_FLING | GUI_BACKWARD, 0.0f, 0.16f, 0.0f);

            if (level_infocard_msg_id)
                gui_slide(level_infocard_msg_id,
                          GUI_S | GUI_FLING | GUI_BACKWARD, 0.0f, 0.16f, 0.0f);

            if (level_infocard_ctrls_id)
                gui_slide(level_infocard_ctrls_id,
                          GUI_S | GUI_FLING | GUI_BACKWARD, 0.0f, 0.16f, 0.0f);

            if (level_infocard_power_id)
                gui_slide(level_infocard_power_id,
                          GUI_S | GUI_FLING | GUI_BACKWARD, 0.0f, 0.16f, 0.0f);

            transition_add(id);
            return id;
        }
        else
        {
            gui_delete(id);
            return 0;
        }
    }

    return transition_slide(id, 0, intent);
}

static void level_paint(int id, float t)
{
#if ENABLE_MOON_TASKLOADER!=0 && !defined(SKIP_MOON_TASKLOADER)
    if (!level_loading_with_moon_taskloader)
#endif
        game_client_draw(0, t);

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    if (console_gui_shown())
    {
        hud_cam_paint();
        console_gui_desc_paint();
    }
#endif

    gui_paint(id);
}

static void level_timer(int id, float dt)
{
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
    /* HACK: Do not attempt, when the level is loading. */

    if (EM_ASM_INT({ return Neverball.wgclIsLevelLoading; })) return;
#endif

    /* HACK: This shouldn't have a bug. This has been fixed. */

    if ((text_length(config_get_s(CONFIG_PLAYER)) < 3 ||
         !level_check_playername(config_get_s(CONFIG_PLAYER))) &&
        !st_global_animating())
        goto_state(&st_level_signin_required);

    if (nodemo_warnonlyonce && config_get_d(CONFIG_ACCOUNT_SAVE) > 0 &&
        !st_global_animating())
    {
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        if (check_nodemo == 1 && !demo_fp &&
            !campaign_hardcore_norecordings() && curr_mode() != MODE_NONE)
#else
        if (check_nodemo == 1 && !demo_fp && curr_mode() != MODE_NONE)
#endif
            goto_state(&st_nodemo);
    }

    geom_step(dt);
    gui_timer(id, dt);
    //game_lerp_pose_point_tick(dt);

#if ENABLE_MOON_TASKLOADER!=0 && !defined(SKIP_MOON_TASKLOADER)
    if (!level_loading_with_moon_taskloader)
#endif
        game_step_fade(dt);
}

static int level_keybd(int c, int d)
{
#ifdef MAPC_INCLUDES_CHKP
    const char *message = last_active ? LEVEL_MESSAGE_CHKP_POST_RESPAWN :
                                        level_msg(curr_level());
#else
    const char *message = level_msg(curr_level());
#endif

    if (d)
    {
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        if (current_platform == PLATFORM_PC)
#endif
        {
            if (c == KEY_EXIT)
                return goto_pause(&st_level);
            if (c == KEY_POSE)
                return goto_state(&st_poser);
        }

        if (config_tst_d(CONFIG_KEY_SCORE_NEXT, c))
        {
            if ((level_master(curr_level())
              || curr_mode() == MODE_CHALLENGE
              || curr_mode() == MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
              || curr_mode() == MODE_HARDCORE
#endif
                ) &&
                server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_CONSUMABLES) &&
                (message && *message))
            {
                show_info = show_info == 0 ? 1 : 0;
                show_info ? goto_state(&st_level) : exit_state(&st_level);
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
            int active = gui_active();
#if defined(ENABLE_POWERUP) && defined(CONFIG_INCLUDES_ACCOUNT)
            if ((level_master(curr_level())
              || curr_mode() == MODE_CHALLENGE
              || curr_mode() == MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
              || curr_mode() == MODE_HARDCORE
#endif
                ) &&
                server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_CONSUMABLES))
            {
                if (curr_state() == &st_level)
                    return level_action(gui_token(active), gui_value(active));
            }
            else
#endif
                return level_action(gui_token(active), gui_value(active));
        }
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return goto_pause(curr_state());
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_X, b) && curr_state() == &st_level)
        {
            if ((level_master(curr_level())
              || curr_mode() == MODE_CHALLENGE
              || curr_mode() == MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
              || curr_mode() == MODE_HARDCORE
#endif
                ) &&
                server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_CONSUMABLES))
            {
                show_info = show_info == 0 ? 1 : 0;
                show_info ? goto_state(&st_level) : exit_state(&st_level);
            }
        }
    }
    return 1;
}

static int level_click(int b, int d)
{
#ifndef __EMSCRIPTEN__
    if (d && config_tst_d(CONFIG_MOUSE_CANCEL_MENU, b))
        return st_keybd(KEY_EXIT, d);
#endif

    if (gui_click(b, d))
        return level_buttn(config_get_d(CONFIG_JOYSTICK_BUTTON_A), 1);

    else if (b == SDL_BUTTON_LEFT && d == 0)
    {
#if defined(ENABLE_POWERUP) && defined(CONFIG_INCLUDES_ACCOUNT)
        if ((level_master(curr_level())
          || curr_mode() == MODE_CHALLENGE
          || curr_mode() == MODE_BOOST_RUSH
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
          || curr_mode() == MODE_HARDCORE
#endif
            ) &&
            server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_CONSUMABLES))
        {
            if (curr_state() == &st_level)
            {
                int active = gui_active();
                return level_action(gui_token(active), gui_value(active));
            }
        }
#elif SWITCHBALL_HAVE_TIP_AND_TUTORIAL
        return (!tutorial_check() && !hint_check()) ? goto_state(&st_play_ready) : 1;
#else
        return goto_state(&st_play_ready);
#endif
    }

    return 1;
}

/*---------------------------------------------------------------------------*/

static void poser_paint(int id, float t)
{
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && DEVEL_BUILD && !defined(NDEBUG)
    game_client_draw(config_cheat() ? 0 : POSE_LEVEL, t);
#else
    game_client_draw(0, t);
#endif
}

static void poser_timer(int id, float dt)
{
    geom_step(dt);
    //game_lerp_pose_point_tick(dt);
}

static void poser_point(int id, int x, int y, int dx, int dy)
{
    //game_lerp_pose_point(dx, dy);
}

static int poser_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT || c == KEY_POSE)
            return exit_state(&st_level);
    }
    return 1;
}

static int poser_buttn(int c, int d)
{
    if (d && config_tst_d(CONFIG_JOYSTICK_BUTTON_B, c))
        return exit_state(&st_level);

    return 1;
}

/*---------------------------------------------------------------------------*/

static int nodemo_enter(struct state *st, struct state *prev, int intent)
{
    audio_play(AUD_WARNING, 1.0f);

    nodemo_warnonlyonce = 0;
    check_nodemo = 0;

    //game_lerp_pose_point_reset();
    game_client_fly(1.0f);

    int id;

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("Warning!"), GUI_MED, GUI_COLOR_RED);
        gui_space(id);
        gui_multi(id, _("A replay file could not be opened for writing.\n"
                        "This game will not be recorded.\n"),
                      GUI_SML, GUI_COLOR_WHT);

        gui_layout(id, 0, 0);
    }

    return transition_slide(id, 1, intent);
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
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        if (c == KEY_EXIT && current_platform == PLATFORM_PC)
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            return campaign_used() ? goto_state(&st_play_ready) : exit_state(&st_level);
#else
            return exit_state(&st_level);
#endif
#else
        if (c == KEY_EXIT)
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            return campaign_used() ? goto_state(&st_play_ready) : exit_state(&st_level);
#else
            return exit_state(&st_level);
#endif
#endif
    }
    return 1;
}

static int nodemo_buttn(int b, int d)
{
    if (d && (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b) ||
              config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b)))
    {
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        return campaign_used() ? goto_state(&st_play_ready) : exit_state(&st_level);
#else
        return exit_state(&st_level);
#endif
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int level_signin_required_enter(struct state *st, struct state *prev, int intent)
{
    audio_play(AUD_WARNING, 1.0f);

    int id;

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("Sign in required!"), GUI_MED, GUI_COLOR_RED);
        gui_space(id);
        gui_multi(id, _("This account must be signed in,\n"
                        "before you play this levels!"),
                      GUI_SML, GUI_COLOR_WHT);

        gui_layout(id, 0, 0);
    }

    return id;
}

static int level_signin_required_keybd(int c, int d)
{
    if (d)
    {
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        if (c == KEY_EXIT && current_platform == PLATFORM_PC)
#else
        if (c == KEY_EXIT)
#endif
            return goto_exit();
    }
    return 1;
}

static int level_signin_required_buttn(int b, int d)
{
    if (d)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            return goto_name(demo_fp ?
                             (campaign_used() ? &st_play_ready : &st_level) :
                             &st_nodemo, &st_level_signin_required, 0, 0, 0);
#else
            return goto_name(demo_fp ? &st_level : &st_nodemo,
                             &st_level_signin_required, 0, 0, 0);
#endif
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return goto_exit();
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

int goto_play_level(void)
{
    struct state *curr_st = curr_state();
    int         (*fn_state)(struct state *);

    if (curr_st == &st_pause)
        fn_state = exit_state;
    else
        fn_state = goto_state;

#if ENABLE_MOON_TASKLOADER!=0 && !defined(SKIP_MOON_TASKLOADER)
    if (level_loading_with_moon_taskloader)
        return fn_state(&st_level_loading);
#endif

    if (config_get_d(CONFIG_ACCOUNT_SAVE) > 0 &&
        curr_mode() != MODE_NONE && !demo_fp)
        return fn_state(&st_nodemo);

    return fn_state(
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                    campaign_used() ? &st_play_ready :
#endif
                    &st_level);
}

int goto_exit(void)
{
    if (curr_mode() != MODE_NONE)
        audio_play(AUD_QUITGAME, 1.0f);

    activity_services_powerup_update(AS_POWERUP_NONE);

    struct state *curr = curr_state();

    show_info = 0;

#ifdef LEVELGROUPS_INCLUDES_ZEN
    mediation_stop();
#endif

#if NB_HAVE_PB_BOTH==1 && \
    defined(CONFIG_INCLUDES_ACCOUNT) && defined(ENABLE_POWERUP)
    powerup_stop();
#endif

#ifdef MAPC_INCLUDES_CHKP
    checkpoints_stop();
#endif

/*#ifdef CONFIG_INCLUDES_ACCOUNT
    int prev_wallet_gems = account_get_d(ACCOUNT_DATA_WALLET_GEMS);
#endif*/

    int done = progress_done();

    struct state *dst = NULL;

    video_clr_grab();

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    if (campaign_used())
    {
        if (done && !progress_dead())
        {
            /* End of the level. */

            game_fade_color(0.0f, 0.25f, 0.0f);
            game_fade(+0.333f);

            dst = &st_done;
        }
        else if (campaign_hardcore())
        {
            if (progress_dead() && !done)
            {
                game_fade_color(0.25f, 0.0f, 0.0f);
                game_fade(+0.333f);

                dst = &st_over;
            }
            else return goto_playmenu(curr_mode());
        }
        else return goto_playmenu(curr_mode());
    }
    else
#endif
    if (done && !progress_dead())
    {
        game_fade(+0.333f);
/*#ifdef CONFIG_INCLUDES_ACCOUNT
        if (prev_wallet_gems < account_get_d(ACCOUNT_DATA_WALLET_GEMS) &&
            account_get_d(ACCOUNT_DATA_WALLET_GEMS) >= 1500)
        {
            game_fade_color(0.25f, 0.25f, 0.125f);
            dst = &st_capital;
        }
        else
#endif*/
        {
            game_fade_color(0.0f, 0.25f, 0.0f);
            dst = &st_done;
        }
    }
    else if (curr_mode() == MODE_BOOST_RUSH)
    {
        boost_rush_stop();

        if (progress_dead()
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
         && !campaign_used()
#endif
            )
        {
            game_fade_color(0.25f, 0.0f, 0.0f);
            game_fade(+0.333f);
        }

        dst = curr_times() > 0 && progress_dead() ? &st_over :
                                                    &st_start;
    }
    else if (curr_mode() == MODE_CHALLENGE)
    {
        if (progress_dead()
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            && !campaign_used()
#endif
            )
        {
            game_fade_color(0.25f, 0.0f, 0.0f);
            game_fade(+0.333f);
        }

        dst = curr_times() > 0 && progress_dead() ? &st_over :
                                                    &st_start;
    }
    else
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    if (!campaign_used())
#endif
        dst = curr_times() > 0 && progress_dead() ? &st_over :
                                                    &st_start;

    progress_exit();

    if (dst)
    {
        /* Visit the auxilliary screen or exit to level selection. */

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        exit_state(dst != curr ? dst : &st_start);
#else
        exit_state(&st_start);
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

struct state st_level_loading = {
    level_loading_enter,
    shared_leave,
    level_paint,
    level_timer
};

struct state st_level = {
    level_enter,
    level_leave,
    level_paint,
    level_timer,
    shared_point, /* Can hover on: point */
    shared_stick, /* Can hover on: stick */
    shared_angle, /* Can hover on: angle */
    level_click,
    level_keybd,
    level_buttn
};

struct state st_poser = {
    NULL,
    level_leave,
    poser_paint,
    poser_timer,
    poser_point,
    NULL,
    NULL,
    NULL,
    poser_keybd,
    poser_buttn
};

struct state st_nodemo = {
    nodemo_enter,
    level_leave,
    shared_paint,
    nodemo_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click_basic,
    nodemo_keybd,
    nodemo_buttn
};

struct state st_level_signin_required = {
    level_signin_required_enter,
    level_leave,
    shared_paint,
    nodemo_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click_basic,
    level_signin_required_keybd,
    level_signin_required_buttn
};
