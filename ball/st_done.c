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

#include <string.h>

/*
 * HACK: Used with console version
 */
#include "console_control_gui.h"

#if NB_HAVE_PB_BOTH==1
#include "networking.h"
#include "campaign.h" /* New: Campaign */
#include "account.h"
#include "account_wgcl.h"
#endif

#include "gui.h"
#include "transition.h"
#include "set.h"
#include "util.h"
#include "demo.h"
#include "progress.h"
#include "audio.h"
#include "config.h"
#include "key.h"

#include "game_common.h"
#include "game_server.h"
#include "game_client.h"

#include "st_common.h"
#include "st_done.h"
#include "st_goal.h"
#include "st_start.h"
#include "st_name.h"
#include "st_set.h"
#include "st_shared.h"

#if NB_HAVE_PB_BOTH==1
#include "st_shop.h"
#endif

/*---------------------------------------------------------------------------*/

static int resume;
static int stars_gained;

enum
{
    DONE_SHOP = GUI_LAST,
    DONE_TO_GROUP
};

static int done_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
    campaign_hardcore_quit();
#endif

    switch (tok)
    {
        case GUI_BACK:
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            if (campaign_used())
            {
                campaign_hardcore_quit();
                campaign_theme_quit();
                campaign_quit();
            }
#endif
            return goto_playmenu(curr_mode());

        case GUI_NAME:
#ifdef CONFIG_INCLUDES_ACCOUNT
            return goto_shop_rename(&st_done, &st_done, 0);
#else
            return goto_name(&st_done, &st_done, 0, 0, 0);
#endif

        case GUI_SCORE:
            gui_score_set(val);
            return goto_state(&st_done);

        case DONE_SHOP:
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            if (campaign_used())
            {
                campaign_hardcore_quit();
                campaign_theme_quit();
                campaign_quit();
            }
#endif

#ifdef CONFIG_INCLUDES_ACCOUNT
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
            if (!campaign_used())
#endif
            {
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

                if (str_starts_with(curr_setid_final, "anime"))
                    audio_music_fade_to(0.5f, "bgm/jp/title.ogg", 1);
                else
                    audio_music_fade_to(0.5f, is_boost_on() ? "bgm/boostrush.ogg" :
                                                              "bgm/inter_world.ogg", 1);
            }

            return goto_shop(
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                             campaign_used() ? &st_campaign :
#endif
                             &st_start, 0);
#endif

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
        case DONE_TO_GROUP:
            if (campaign_used())
            {
                campaign_hardcore_quit();
                campaign_theme_quit();
                campaign_quit();
            }
            return goto_playmenu(curr_mode());
#endif
    }
    return 1;
}

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
static int done_gui_campaign(void)
{
    if (!resume)
    {
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
        /* FIXME: WGCL Narrator can do it! */

        EM_ASM({
            if (Neverball.gamecore_geolocation_checkisjapan() || navigator.language.startsWith("ja") || navigator.language.startsWith("jp") || gameoptions_debug_locale_japanese)
                CoreLauncherOptions_GameOptions_PlayNarratorAudio("ja-JP/corelauncher_narrator_finished_campaign.mp3");
        });
#endif
    }

    int id, jd;
    if ((id = gui_vstack(0)))
    {
        char sdescHardcore[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(sdescHardcore, MAXSTR,
#else
        sprintf(sdescHardcore,
#endif
                _("You completed all levels\n"
                  "And you collected %d coins.\n\n"
                  "A new trophy has been awarded to\n"
                  "acknowledge your achievement!"),
                curr_score());

        const char *stitle = campaign_hardcore() ? N_("WOW") : N_("Campaign Complete");
        const char *sdesc = campaign_hardcore() ? sdescHardcore :
            N_("If you want to keep exploring\n"
               "more levels, select LEVEL SET\n"
               "from the level group.");

        gui_title_header(id, _(stitle), GUI_LRG, gui_blu, gui_grn);
        gui_space(id);
        gui_multi(id, _(sdesc), GUI_SML, GUI_COLOR_WHT);
        gui_space(id);

        if ((jd = gui_harray(id)))
        {
            gui_start(jd, _("Return to group"), GUI_SML, DONE_TO_GROUP, 0);
#ifdef CONFIG_INCLUDES_ACCOUNT
            if (server_policy_get_d(SERVER_POLICY_EDITION) > -1 && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED))
                gui_state(jd, _("Shop"), GUI_SML, DONE_SHOP, 0);
#endif
        }

        gui_layout(id, 0, 0);
    }

    return id;
}
#endif

static int done_gui_set(void)
{
    const char *s1 = _("New Set Record");
    const char *s2 = _("Set Complete");

    int id, jd, kd;

    int high = progress_set_high();

    if (high && !resume)
    {
        audio_play("snd/new_time_record.ogg", 1.0f);
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
        /* FIXME: WGCL Narrator can do it! */

        EM_ASM({
            if (Neverball.gamecore_geolocation_checkisjapan() || navigator.language.startsWith("ja") || navigator.language.startsWith("jp") || gameoptions_debug_locale_japanese)
                CoreLauncherOptions_GameOptions_PlayNarratorAudio("ja-JP/corelauncher_narrator_finished_levelset_hs.mp3");
        });
#elif NB_HAVE_PB_BOTH!=1 || !defined(__EMSCRIPTEN__)
        audio_narrator_play(AUD_SCORE);
#endif
    }
    else if (!resume)
    {
#if NB_HAVE_PB_BOTH==1 && defined(__EMSCRIPTEN__)
        /* FIXME: WGCL Narrator can do it! */

        EM_ASM({
            if (Neverball.gamecore_geolocation_checkisjapan() || navigator.language.startsWith("ja") || navigator.language.startsWith("jp") || gameoptions_debug_locale_japanese)
                CoreLauncherOptions_GameOptions_PlayNarratorAudio("ja-JP/corelauncher_narrator_finished_levelset.mp3");
        });
#endif
    }

#if NB_HAVE_PB_BOTH==1
    if (!campaign_used()
        && set_star(curr_set()) > 0 && set_star_gained(curr_set())
        && !resume)
    {
        stars_gained = 1;

        set_star_update(1);
        audio_play(AUD_STARS, 1.0f);
    }
#endif

    if ((id = gui_vstack(0)))
    {
        int gid;

        if ((jd = gui_hstack(id)))
        {
            gui_filler(jd);

#if NB_HAVE_PB_BOTH==1
            if (stars_gained)
            {
                char set_star_attr[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(set_star_attr, MAXSTR,
#else
                sprintf(set_star_attr,
#endif
                        GUI_STAR " +%d", set_star(curr_set()));
                const int sid = gui_label(jd, set_star_attr,
                                              GUI_MED, gui_wht, gui_yel);
                gui_set_font(sid, "ttf/DejaVuSans-Bold.ttf");

                if (!resume)
                    gui_pulse(sid, 1.2f);

                gui_space(jd);
            }
#endif

            if (high)
                gid = gui_title_header(jd, s1, GUI_MED, GUI_COLOR_GRN);
            else
                gid = gui_title_header(jd, s2, GUI_MED, gui_blu, gui_grn);

            if (!resume)
                gui_pulse(gid, 1.2f);

            gui_filler(jd);

            gui_set_rect(jd, GUI_ALL);
        }

        gui_space(id);

#ifdef CONFIG_INCLUDES_ACCOUNT
        if (server_policy_get_d(SERVER_POLICY_EDITION) > -1)
        {
            if ((jd = gui_hstack(id)))
            {
                int calc_new_wallet_coin_id, calc_new_wallet_gem_id;

                gui_filler(jd);

                if ((kd = gui_harray(jd)))
                {
                    calc_new_wallet_gem_id = gui_count(kd, 1000, GUI_MED);
                    gui_label(kd, _("Gems"), GUI_SML, GUI_COLOR_WHT);
                    calc_new_wallet_coin_id = gui_count(kd, ACCOUNT_WALLET_MAX_COINS,
                                                            GUI_MED);
                    gui_label(kd, _("Coins"), GUI_SML, GUI_COLOR_WHT);

                    gui_set_count(calc_new_wallet_coin_id,
                                  account_get_d(ACCOUNT_DATA_WALLET_COINS));
                    gui_set_count(calc_new_wallet_gem_id,
                                  account_get_d(ACCOUNT_DATA_WALLET_GEMS));
                }

                gui_filler(jd);

                gui_set_rect(jd, GUI_ALL);
            }

            gui_space(id);
        }
#endif

        /* View the file in st_over.c */

#if NB_HAVE_PB_BOTH==1
        gui_score_board(id, GUI_SCORE_COIN | GUI_SCORE_TIME,
                            1,
                            high && !account_wgcl_name_read_only());
#else
        gui_score_board(id, GUI_SCORE_COIN | GUI_SCORE_TIME,
                            1,
                            high);
#endif
        gui_space(id);

        if ((jd = gui_harray(id)))
        {
            gui_start(jd, _("Select Level"), GUI_SML, GUI_BACK, 0);
#ifdef CONFIG_INCLUDES_ACCOUNT
            if (server_policy_get_d(SERVER_POLICY_EDITION) > -1
             && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED))
                gui_state(jd, _("Shop"), GUI_SML, DONE_SHOP, 0);
#endif
        }

        gui_layout(id, 0, 0);
    }

    /* View the file in st_over.c */

    set_score_board(set_score(curr_set(), SCORE_COIN), progress_score_rank(),
                    set_score(curr_set(), SCORE_TIME), progress_times_rank(),
                    NULL, -1);

    return id;
}

static int done_enter(struct state *st, struct state *prev, int intent)
{
#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT)
    account_wgcl_restart_attempt();
#endif

    if (prev == &st_name)
        progress_rename(1);

    resume = prev != &st_goal;

    return transition_slide(
#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
                            campaign_used() ? done_gui_campaign() :
#endif
                            done_gui_set(), 1, intent);
}

static int done_leave(struct state *st, struct state *next, int id, int intent)
{
    if (next == &st_null)
    {
        progress_exit();

        campaign_quit();
        set_quit();

        game_server_free(NULL);
        game_client_free(NULL);

        gui_delete(id);
        return 0;
    }

    if (next == &st_done)
    {
        gui_delete(id);
        return 0;
    }

    return transition_slide(id, 0, intent);
}

static void done_timer(int id, float dt)
{
    gui_timer(id, dt);
    game_step_fade(dt);
}

static int done_keybd(int c, int d)
{
    if (d)
    {
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        if (c == KEY_EXIT && current_platform == PLATFORM_PC)
#else
        if (c == KEY_EXIT)
#endif
            return done_action(GUI_BACK, 0);

        if (config_tst_d(CONFIG_KEY_SCORE_NEXT, c))
            return done_action(GUI_SCORE, GUI_SCORE_NEXT(gui_score_get()));
    }
    return 1;
}

static int done_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return done_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return done_action(GUI_BACK, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int wealthlogo_done = 0;

static int capital_enter(struct state *st, struct state *prev, int intent)
{
    wealthlogo_done = 0;

    int id;

    if ((id = gui_vstack(0)))
    {
        int logo_id = gui_label(id, GUI_CROWN, GUI_LRG, GUI_COLOR_YEL);
        int aid     = gui_label(id, GUI_DIAMOND " 1500",
                                    GUI_SML, GUI_COLOR_WHT);
        int bid     = gui_label(id, _("Wealthiest Capital"),
                                    GUI_SML, GUI_COLOR_WHT);

        gui_pulse(logo_id, 1.2f);
        gui_pulse(aid, 1.2f);
        gui_pulse(bid, 1.2f);

        gui_set_rect(id, GUI_ALL);
        gui_layout(id, 0, 0);
    }

    return transition_slide(id, 1, intent);
}

static void capital_timer(int id, float dt)
{
    gui_timer(id, dt);

    if (time_state() > 2.0f && !wealthlogo_done)
    {
        wealthlogo_done = 1;
        goto_state(&st_done);
    }
}

static int capital_keybd(int c, int d)
{
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    if (d && c == KEY_EXIT && current_platform == PLATFORM_PC)
#else
    if (d && c == KEY_EXIT)
#endif
    {
        wealthlogo_done = 1;
        goto_state(&st_done);
    }
    return 1;
}

static int capital_buttn(int b, int d)
{
    if (d && (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b)
        || config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b)))
    {
        wealthlogo_done = 1;
        goto_state(&st_done);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

struct state st_done = {
    done_enter,
    done_leave,
    shared_paint,
    done_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    done_keybd,
    done_buttn
};

struct state st_capital = {
    capital_enter,
    shared_leave,
    shared_paint,
    capital_timer,
    NULL,
    NULL,
    NULL,
    shared_click_basic,
    capital_keybd,
    capital_buttn
};

/*---------------------------------------------------------------------------*/
