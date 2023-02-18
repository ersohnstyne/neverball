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

#include <stdio.h>
#include <assert.h>

#include "networking.h"

#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
#include "console_control_gui.h"
#include "campaign.h"
#include "account.h"
#include "mediation.h"
#endif

#ifdef CONFIG_INCLUDES_ACCOUNT
#include "powerup.h"
#endif
#include "currency.h"
#include "common.h"
#include "config.h"
#include "geom.h"
#include "hud.h"
#include "gui.h"
#include "vec3.h"
#include "audio.h"
#include "image.h"
#include "video.h"

#include "game_payment.h"
#include "game_client.h"
#include "game_common.h"

#include "st_conf.h"
#include "st_shop.h"
#include "st_title.h"
#include "st_shared.h"
#include "st_name.h"

/*---------------------------------------------------------------------------*/

struct state st_shop_rename;
struct state st_shop_unregistered;
struct state st_shop_iap;
struct state st_shop_buy;
struct state st_expenses_export;

/*---------------------------------------------------------------------------*/

static int productkey;

static int coinwallet;
static int gemwallet;

static int evalue;
static int fvalue;
static int svalue;
static int lvalue;

enum
{
    SHOP_CHANGE_NAME = GUI_LAST,
    SHOP_IAP,
    SHOP_BUY
};

void shop_set_product_key(int newkey);

static int purchase_product_usegems = 0;
static int inaccept_playername = 0;

static int shop_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;
    
    inaccept_playername = 0;

    char newPlayername[MAXSTR];
    SAFECPY(newPlayername, config_get_s(CONFIG_PLAYER));

    for (int i = 0; i < strlen(newPlayername); i++)
    {
        if (newPlayername[i] == '\\' || newPlayername[i] == '/' || newPlayername[i] == ':' || newPlayername[i] == '*' || newPlayername[i] == '?' || newPlayername[i] == '"' || newPlayername[i] == '<' || newPlayername[i] == '>' || newPlayername[i] == '|')
            inaccept_playername = 1;
    }

    switch (tok)
    {
    case GUI_BACK:
        return goto_state_full(&st_title, GUI_ANIMATION_N_CURVE, 0, 0);
        break;
    case SHOP_CHANGE_NAME:
        return goto_shop_rename(&st_shop, &st_shop, 0);
        break;
    case SHOP_IAP:
        if (!inaccept_playername && strlen(config_get_s(CONFIG_PLAYER)) >= 3)
        {
#if NB_STEAM_API==1 || NB_EOS_SDK==1 || ENABLE_IAP==1
            return goto_shop_iap(&st_shop, &st_shop, 0, 0, 0, 0, 1);
#else
            return goto_shop_iap(&st_shop, &st_shop, 0, 0, 0, 0, 0);
#endif
        }
        break;
    case SHOP_BUY:
        purchase_product_usegems = val == 7;
        shop_set_product_key(val);
        if (inaccept_playername)
            return goto_state(&st_shop_unregistered);
        else if (strlen(config_get_s(CONFIG_PLAYER)) < 3)
            return goto_state(&st_shop_unregistered);
        else
            return goto_state(&st_shop_buy);
        break;
    }
    return 1;
}

static int shop_gui(void)
{
#ifdef CONFIG_INCLUDES_ACCOUNT
    evalue = account_get_d(ACCOUNT_CONSUMEABLE_EARNINATOR);
    fvalue = account_get_d(ACCOUNT_CONSUMEABLE_FLOATIFIER);
    svalue = account_get_d(ACCOUNT_CONSUMEABLE_SPEEDIFIER);
    lvalue = account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES);
#endif

    int w = video.device_w;
    int h = video.device_h;

    int id, jd, kd, ld;
    if ((id = gui_vstack(0)))
    {
        if ((jd = gui_hstack(id)))
        {
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
            if (current_platform == PLATFORM_PC && !inaccept_playername && strlen(config_get_s(CONFIG_PLAYER)) >= 3)
#else
            if (!inaccept_playername && strlen(config_get_s(CONFIG_PLAYER)) >= 3)
#endif
            {
                if (server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_IAP))
                    gui_state(jd, "+", GUI_SML, SHOP_IAP, 0);
            }

            char gemsattr[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(gemsattr, dstSize, "%s: %d", GUI_DIAMOND, gemwallet);
#else
            sprintf(gemsattr, "%s: %d", GUI_DIAMOND, gemwallet);
#endif
            gui_label(jd, gemsattr, GUI_SML, gui_wht, gui_cya);

            gui_space(jd);

            char coinsattr[MAXSTR];
            char currency_name[4];

#define LANG_CURRENCY_RESET_DEFAULTS                            \
    do {                                                        \
        GetSystemDefaultLocaleName(pWLocaleName, 85);           \
        wcstombs_s(&pCharC, pChar, MAXSTR, pWLocaleName, 2);    \
        wcstombs_s(&pCharC, pCharExt, MAXSTR, pWLocaleName, 5); \
        for (size_t i = 0; i < strlen(pCharExt); i++)           \
            if (pCharExt[i] == '-')                             \
                pCharExt[i] = '_';                              \
    } while (0)

            wchar_t pWLocaleName[MAXSTR];
            size_t pCharC;
            char pCharExt[MAXSTR], pChar[MAXSTR];
            if (strlen(config_get_s(CONFIG_LANGUAGE)) < 2)
                LANG_CURRENCY_RESET_DEFAULTS;
            else
                SAFECPY(pChar, config_get_s(CONFIG_LANGUAGE));

            if (str_starts_with(pChar, "de")
                || str_starts_with(pChar, "es")
                || str_starts_with(pChar, "fr")
                || str_starts_with(pChar, "it")
                || str_starts_with(pChar, "nl"))
                SAFECPY(currency_name, CURRENCY_FINANCE_EU);
            else if (str_starts_with(pChar, "br"))
                SAFECPY(currency_name, CURRENCY_FINANCE_BR);
            else if (str_starts_with(pChar, "ch"))
                SAFECPY(currency_name, CURRENCY_FINANCE_CH);
            else if (str_starts_with(pChar, "en_GB"))
                SAFECPY(currency_name, CURRENCY_FINANCE_GB);
            else if (str_starts_with(pChar, "hu"))
                SAFECPY(currency_name, CURRENCY_FINANCE_HU);
            else if (str_starts_with(pChar, "ja"))
                SAFECPY(currency_name, CURRENCY_FINANCE_JA);
            else if (str_starts_with(pChar, "ko"))
                SAFECPY(currency_name, CURRENCY_FINANCE_KR);
            else if (str_starts_with(pChar, "id"))
                SAFECPY(currency_name, CURRENCY_FINANCE_ID);
            else if (str_starts_with(pChar, "th"))
                SAFECPY(currency_name, CURRENCY_FINANCE_TH);
            else
                SAFECPY(currency_name, CURRENCY_FINANCE_US);

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(coinsattr, dstSize, "%s: %d", currency_name, coinwallet);
#else
            sprintf(coinsattr, "%s: %d", currency_name, coinwallet);
#endif
            gui_label(jd, coinsattr, GUI_SML, gui_wht, gui_yel);

            if (!inaccept_playername && strlen(config_get_s(CONFIG_PLAYER)) >= 3 && video.aspect_ratio >= 1.0f)
            {
                gui_space(jd);
                int player_id;
                if ((player_id = gui_label(jd, "XXXXXXXXXXXX", GUI_SML, gui_wht, gui_yel)))
                {
                    gui_set_trunc(player_id, TRUNC_TAIL);

                    const char *player = config_get_s(CONFIG_PLAYER);
                    gui_set_label(player_id, player);

#if NB_STEAM_API==0 && NB_EOS_SDK==0
                    if (!online_mode) gui_set_state(player_id, SHOP_CHANGE_NAME, 0);
#endif
                }

            }

            gui_filler(jd);

#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
            if (current_platform == PLATFORM_PC)
#endif
            {
                gui_space(jd);
                gui_start(jd, _("Back"), GUI_SML, GUI_BACK, 0);
            }
        }

#if defined(ENABLE_POWERUP) && defined(CONFIG_INCLUDES_ACCOUNT)
        if ((account_get_d(ACCOUNT_SET_UNLOCKS) > 0 || server_policy_get_d(SERVER_POLICY_EDITION) > 1)
            && (server_policy_get_d(SERVER_POLICY_PLAYMODES_UNLOCKED_MODE_CAREER) || campaign_career_unlocked()
#if NB_STEAM_API==0 && NB_EOS_SDK==0
                || config_cheat()
#endif
                ) && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_CONSUMABLES))
        {
            gui_space(id);

            if ((jd = gui_vstack(id)))
            {
                char powerups[MAXSTR];

                int temp_lives = lvalue;

                /*
                 * One crown (Player): +1000 balls = 15000 gems
                 * Two crowns (Player): +1100 balls = 16500 gems
                 * Three crowns (Player): 1110 balls = 16650 gems
                 * 
                 * One crown (Game Banker): +1000000 balls = 15000000 gems
                 * Two crowns (Game Banker): +1100000 balls = 16500000 gems
                 * Three crowns (Game Banker): +1110000 balls = 16650000 gems
                 * Four crowns (Game Banker): +1111000 balls = 16665000 gems
                 * Five crowns (Game Banker): +1111100 balls = 16666500 gems
                 * Six crowns (Game Banker): +1111110 balls = 16666650 gems
                 */

                if (temp_lives >= 1110)
                    sprintf_s(powerups, dstSize, "%s (" GUI_CROWN GUI_CROWN GUI_CROWN ")", _("Balls"));
                else if (temp_lives >= 1100)
                    sprintf_s(powerups, dstSize, "%s (" GUI_CROWN GUI_CROWN "%01d)", _("Balls"), (lvalue) - 1100);
                else if (temp_lives >= 100)
                    sprintf_s(powerups, dstSize, "%s (" GUI_CROWN "%02d)", _("Balls"), (lvalue) - 1000);
                else
                    sprintf_s(powerups, dstSize, "%s (%d)", _("Balls"), lvalue);

                gui_label(jd, powerups, GUI_SML, gui_wht, gui_cya);

                if ((kd = gui_harray(jd)))
                {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                    sprintf_s(powerups, dstSize, "%s (%i)", _("Speedifier"), svalue);
                    gui_label(kd, powerups, GUI_SML, gui_wht, gui_grn);

                    sprintf_s(powerups, dstSize, "%s (%i)", _("Floatifier"), fvalue);
                    gui_label(kd, powerups, GUI_SML, gui_wht, gui_blu);

                    sprintf_s(powerups, dstSize, "%s (%i)", _("Earninator"), evalue);
                    gui_label(kd, powerups, GUI_SML, gui_wht, gui_red);
#else
                    sprintf(powerups, "%s (%i)", _("Speedifier"), svalue);
                    gui_label(kd, powerups, GUI_SML, gui_wht, gui_grn);

                    sprintf(powerups, "%s (%i)", _("Floatifier"), fvalue);
                    gui_label(kd, powerups, GUI_SML, gui_wht, gui_blu);

                    sprintf(powerups, "%s (%i)", _("Earninator"), evalue);
                    gui_label(kd, powerups, GUI_SML, gui_wht, gui_red);
#endif
                }

                gui_set_rect(jd, GUI_ALL);
            }
        }
#endif

        if ((jd = gui_hstack(id)))
        {
            gui_filler(jd);

#if defined(ENABLE_POWERUP) && defined(CONFIG_INCLUDES_ACCOUNT)
            if ((account_get_d(ACCOUNT_SET_UNLOCKS) > 0 || server_policy_get_d(SERVER_POLICY_EDITION) > 1)
                && (server_policy_get_d(SERVER_POLICY_PLAYMODES_UNLOCKED_MODE_CAREER) || campaign_career_unlocked()
#if NB_STEAM_API==0 && NB_EOS_SDK==0
                || config_cheat()
#endif
                ) && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_CONSUMABLES))
            {
                /* Consumables */
                if ((kd = gui_vstack(jd)))
                {
                    if ((ld = gui_vstack(kd)))
                    {
                        gui_space(ld);
                        int nid = gui_label(ld, "XXXXXXXX", GUI_SML, gui_wht, gui_wht);
                        gui_set_trunc(nid, TRUNC_TAIL);
                        gui_set_label(nid, _("Speedifier"));
                        gui_image(ld, "gui/shop/consum_speedifier.jpg", w / 7, h / 6);
                        gui_filler(ld);
                        gui_set_state(ld, SHOP_BUY, 6);
                    }

                    if ((ld = gui_vstack(kd)))
                    {
                        gui_space(ld);
                        int nid = gui_label(ld, "XXXXXXXX", GUI_SML, gui_wht, gui_wht);
                        gui_set_trunc(nid, TRUNC_TAIL);
                        gui_set_label(nid, _("Extra Balls"));
                        gui_image(ld, "gui/shop/consum_balls.jpg", w / 7, h / 6);
                        gui_filler(ld);
                        gui_set_state(ld, SHOP_BUY, 7);
                    }

                    gui_filler(kd);
                }

                if ((kd = gui_vstack(jd)))
                {
                    if ((ld = gui_vstack(kd)))
                    {
                        gui_space(ld);
                        int nid = gui_label(ld, "XXXXXXXX", GUI_SML, gui_wht, gui_wht);
                        gui_set_trunc(nid, TRUNC_TAIL);
                        gui_set_label(nid, _("Earninator"));
                        gui_image(ld, "gui/shop/consum_earninator.jpg", w / 7, h / 6);
                        gui_filler(ld);
                        gui_set_state(ld, SHOP_BUY, 4);
                    }

                    if ((ld = gui_vstack(kd)))
                    {
                        gui_space(ld);
                        int nid = gui_label(ld, "XXXXXXXX", GUI_SML, gui_wht, gui_wht);
                        gui_set_trunc(nid, TRUNC_TAIL);
                        gui_set_label(nid, _("Floatifier"));
                        gui_image(ld, "gui/shop/consum_floatifier.jpg", w / 7, h / 6);
                        gui_filler(ld);
                        gui_set_state(ld, SHOP_BUY, 5);
                    }
                }
            }

            if (account_get_d(ACCOUNT_SET_UNLOCKS) > 0 || server_policy_get_d(SERVER_POLICY_EDITION) > 1
                && (server_policy_get_d(SERVER_POLICY_PLAYMODES_UNLOCKED_MODE_CAREER) || campaign_career_unlocked())
                && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_CONSUMABLES) && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_MANAGED))
                gui_space(jd);
#endif

#ifdef CONFIG_INCLUDES_ACCOUNT
            if (server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_MANAGED))
            {
                /* Managed products */
                if ((kd = gui_vstack(jd)))
                {
                    if ((ld = gui_vstack(kd)))
                    {
                        gui_space(ld);
                        int nid = gui_label(ld, "XXXXXXXXX", GUI_SML, gui_wht, account_get_d(ACCOUNT_PRODUCT_BONUS) ? gui_grn : gui_wht);
                        gui_set_trunc(nid, TRUNC_TAIL);
                        gui_set_label(nid, _("Bonus Pack"));
                        gui_image(ld, "gui/shop/bonus.jpg", w / 6, h / 6);
                        gui_filler(ld);
                        gui_set_state(ld, (account_get_d(ACCOUNT_PRODUCT_BONUS) ? GUI_NONE : SHOP_BUY), 2);
                    }
#ifdef LEVELGROUPS_INCLUDES_ZEN
                    if ((ld = gui_vstack(kd)))
                    {
                        gui_space(ld);
                        int nid = gui_label(ld, "XXXXXXXXX", GUI_SML, gui_wht, account_get_d(ACCOUNT_PRODUCT_MEDIATION) ? gui_grn : gui_wht);
                        gui_set_trunc(nid, TRUNC_TAIL);
                        gui_set_label(nid, _("Mediation"));
                        gui_image(ld, "gui/shop/mediation.jpg", w / 6, h / 6);
                        gui_filler(ld);
                        gui_set_state(ld, (account_get_d(ACCOUNT_PRODUCT_MEDIATION) ? GUI_NONE : SHOP_BUY), 3);
                    }
#else
                    if ((ld = gui_vstack(kd)))
                    {
                        gui_space(ld);
                        int nid = gui_label(ld, "XXXXXXXXX", GUI_SML, gui_gry, gui_gry);
                        gui_set_trunc(nid, TRUNC_TAIL);
                        gui_set_label(nid, _("Mediation"));
                        gui_image(ld, "gui/shop/mediation.jpg", w / 6, h / 6);
                        gui_filler(ld);
                        gui_set_state(ld, GUI_NONE, 3);
                    }
#endif
                }

                if ((kd = gui_vstack(jd)))
                {
                    if ((ld = gui_vstack(kd)))
                    {
                        gui_space(ld);
                        int nid = gui_label(ld, "XXXXXXXXX", GUI_SML, gui_wht, account_get_d(ACCOUNT_PRODUCT_LEVELS) ? gui_grn : gui_wht);
                        gui_set_trunc(nid, TRUNC_TAIL);
                        gui_set_label(nid, _("Extra Levels"));
                        gui_image(ld, "gui/shop/levels.jpg", w / 6, h / 6);
                        gui_filler(ld);
                        gui_set_state(ld, (account_get_d(ACCOUNT_PRODUCT_LEVELS) ? GUI_NONE : SHOP_BUY), 0);
                    }

                    if ((ld = gui_vstack(kd)))
                    {
                        gui_space(ld);
                        int nid = gui_label(ld, "XXXXXXXXX", GUI_SML, gui_wht, account_get_d(ACCOUNT_PRODUCT_BALLS) ? gui_grn : gui_wht);
                        gui_set_trunc(nid, TRUNC_TAIL);
                        gui_set_label(nid, _("Online Balls"));
                        gui_image(ld, "gui/shop/balls.jpg", w / 6, h / 6);
                        gui_filler(ld);
                        gui_set_state(ld, (account_get_d(ACCOUNT_PRODUCT_BALLS) ? GUI_NONE : SHOP_BUY), 1);
                    }
                }
            }
#endif

            gui_filler(jd);
        }

        gui_layout(id, 0, 0);
    }
    return id;
}

static int shop_enter(struct state *st, struct state *prev)
{
    char newPlayername[MAXSTR];
    SAFECPY(newPlayername, config_get_s(CONFIG_PLAYER));
    inaccept_playername = 0;
    for (int i = 0; i < strlen(newPlayername); i++)
    {
        if (newPlayername[i] == '\\' || newPlayername[i] == '/' || newPlayername[i] == ':' || newPlayername[i] == '*' || newPlayername[i] == '?' || newPlayername[i] == '"' || newPlayername[i] == '<' || newPlayername[i] == '>' || newPlayername[i] == '|')
            inaccept_playername = 1;
    }

#ifdef CONFIG_INCLUDES_ACCOUNT
    coinwallet = account_get_d(ACCOUNT_DATA_WALLET_COINS);
    gemwallet = account_get_d(ACCOUNT_DATA_WALLET_GEMS);
#endif

    return shop_gui();
}

static void shop_paint(int id, float t)
{
    game_client_draw(0, t);

    gui_paint(id);
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
    xbox_control_shop_gui_paint();
#endif
}

static int shop_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT)
            return shop_action(GUI_BACK, 0);
    }
    return 1;
}

static int shop_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return shop_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return shop_action(GUI_BACK, 0);
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_Y, b) && server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_IAP))
            return shop_action(SHOP_IAP, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

int shop_unlocked_gui(void)
{
    int id;

    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("Warning!"), GUI_MED, gui_red, gui_red);
        gui_space(id);
        gui_multi(id, _("The goal state is still unlocked\\during completed levels!\\\\Please lock the goal state first\\before you go to the shop."), GUI_SML, gui_wht, gui_wht);

#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
        if (current_platform == PLATFORM_PC)
#endif
        {
            gui_space(id);
            gui_state(id, _("Back"), GUI_SML, GUI_BACK, 0);
        }
    }
    gui_layout(id, 0, 0);

    return id;
}

int shop_unlocked_enter(struct state *st, struct state *prev)
{
    return shop_unlocked_gui();
}

/*---------------------------------------------------------------------------*/

enum
{
    SHOP_RENAME_YES = GUI_LAST
};

static struct state *ok_state;
static struct state *cancel_state;

static unsigned int draw_back;

int goto_shop_rename(struct state *ok, struct state *cancel, unsigned int back)
{
    ok_state = ok;
    cancel_state = cancel;

    draw_back = back;

    if (strlen(config_get_s(CONFIG_PLAYER)) < 3)
        return goto_name(ok_state, cancel_state, 0, 0, draw_back);

    return goto_state(&st_shop_rename);
}

static int shop_rename_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
    case SHOP_RENAME_YES:
        account_save();
        return goto_name(ok_state, cancel_state, 0, 0, draw_back);
        break;
    case GUI_BACK:
        return goto_state(cancel_state);
        break;
    }
    return 1;
}

static int shop_rename_gui(void)
{
    int id, jd;
    if ((id = gui_vstack(0)))
    {
        gui_title_header(id, _("Rename player?"), GUI_MED, gui_gry, gui_red);

        gui_space(id);

        gui_multi(id, _("Renaming players will log in\\to another account."), GUI_SML, gui_wht, gui_wht);

        gui_space(id);

        if ((jd = gui_harray(id)))
        {
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
            if (current_platform == PLATFORM_PC)
#endif
            {
                gui_start(jd, _("No"), GUI_SML, GUI_BACK, 0);
                gui_state(jd, _("Yes"), GUI_SML, SHOP_RENAME_YES, 0);
            }
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
            else
                gui_start(jd, _("Yes"), GUI_SML, SHOP_RENAME_YES, 0);
#endif
        }

        gui_layout(id, 0, 0);
    }
    return id;
}

static int shop_rename_enter(struct state *st, struct state *prev)
{
    if (draw_back)
    {
        game_client_free(NULL);
        back_init(config_get_d(CONFIG_ACCOUNT_MAYHEM) ? "back/gui-mayhem.png" : "back/gui.png");
    }

    return shop_rename_gui();
}

static void shop_rename_leave(struct state *st, struct state *next, int id)
{
    if (draw_back)
        back_free();

    gui_delete(id);
}

static void shop_rename_paint(int id, float t)
{
    if (draw_back)
    {
        video_push_persp((float) config_get_d(CONFIG_VIEW_FOV), 0.1f, FAR_DIST);
        {
            back_draw_easy();
        }
        video_pop_matrix();
    }
    else
        game_client_draw(0, t);

    gui_paint(id);
}

static int shop_rename_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT)
            return shop_rename_action(GUI_BACK, 0);
    }
    return 1;
}

static int shop_rename_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return shop_rename_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return shop_rename_action(GUI_BACK, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

enum
{
    SHOP_UNREGISTERED_YES = GUI_LAST
};

static int shop_unregistered_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;
    
    switch (tok)
    {
    case SHOP_UNREGISTERED_YES:
        return goto_name(&st_shop_buy, &st_shop, 0, 0, 0);
        break;
    case GUI_BACK:
        return goto_state(&st_shop);
        break;
    }
    return 1;
}

static int shop_unregistered_gui(void)
{
    int id, jd;
    if ((id = gui_vstack(0)))
    {
        int fewest = (strlen(config_get_s(CONFIG_PLAYER)) < 3 && !strlen(config_get_s(CONFIG_PLAYER)) == 0);

        const char *toptxt = inaccept_playername ? _("Invalid Player Name!") : (fewest ? _("Too few characters!") : _("Unregistered!"));
        const char *multitxt = inaccept_playername ? _("You have an invalid player name using the\\special chars! Would you like modify\\player name first before you buy?") : (fewest ? _("You didn't enough letters on your player name!\\Would you like extend player name first\\before you buy?") : _("You didn't registered your player name yet!\\Would you like register now before you buy?"));
        const char *yestxt = _("Yes");

        gui_title_header(id, toptxt, GUI_MED, gui_gry, gui_red);

        gui_space(id);		

        gui_multi(id, multitxt, GUI_SML, gui_wht, gui_wht);

        gui_space(id);

        if ((jd = gui_harray(id)))
        {
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
            if (current_platform == PLATFORM_PC)
#endif
            {
                gui_start(jd, _("No"), GUI_SML, GUI_BACK, 0);
                gui_state(jd, yestxt, GUI_SML, SHOP_UNREGISTERED_YES, 0);
            }
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
            else
                gui_start(jd, yestxt, GUI_SML, SHOP_UNREGISTERED_YES, 0);
#endif
        }

        gui_layout(id, 0, 0);
    }
    return id;
}

static int shop_unregistered_enter(struct state *st, struct state *prev)
{
    return shop_unregistered_gui();
}

static int shop_unregistered_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT)
            return shop_unregistered_action(GUI_BACK, 0);
    }
    return 1;
}

static int shop_unregistered_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return shop_unregistered_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return shop_unregistered_action(GUI_BACK, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

/*
 * Handful of ?
 * Pile of ?
 * Engineers Pack
 * Architects Stock
 * Thinkers Box
 * Philosoph
 */

static int purchased;
static int gem_to_coins;

static int show_gems;

static const char iaptiers[][16] = {
    "tier-1",
    "tier-2",
    "tier-3",
    "tier-4",
    "tier-5",
    "tier-6",
};

static int iapcoinfromgems[] = {
    10,
    20,
    50,
    100,
    190,
    380
};

static int iapcoinvalue[] = {
    50,
    100,
    250,
    500,
    960,
    1920,
};

/* Gems in IAP (1 gem = 0,16 €) */
static const float iapgemcost[] = {
    0.59,
    1.14,
    2.29,
    5.49,
    10.99,
    21.99
};

static int iapgemvalue[] = {
    25,
    50,
    100,
    275,
    550,
    1200,
};

enum
{
    SHOP_IAP_GET_BUY = GUI_LAST,
    SHOP_IAP_GET_SWITCH,
    SHOP_IAP_ENTERCODE,
    SHOP_IAP_EXPORT
};

static struct state *ok_state;
static struct state *cancel_state;

static int (*curr_ok_fn)(struct state *) = NULL;
static int (*curr_cancel_fn)(struct state *) = NULL;

static int curr_min;
static int multipage;
static int iappage;

int goto_shop_iap(struct state *ok, struct state *cancel,
                  int (*new_ok_fn)(struct state *), int (*new_cancel_fn)(struct state *),
                  int newmin, int display_gems, int allow_multipage)
{
    ok_state = ok;
    cancel_state = cancel;

    curr_ok_fn = new_ok_fn;
    curr_cancel_fn = new_cancel_fn;

    curr_min = newmin;

    multipage = allow_multipage;
    iappage = display_gems;

    return goto_state_full(&st_shop_iap, 0, 0, 0);
}

static void shop_convert_to_coins(int gems, int coins)
{
#if NB_HAVE_PB_BOTH==1
    int substr_gems = account_get_d(ACCOUNT_DATA_WALLET_GEMS) - gems;
    account_set_d(ACCOUNT_DATA_WALLET_GEMS, substr_gems);
    int add_coins = account_get_d(ACCOUNT_DATA_WALLET_COINS) + coins;
    account_set_d(ACCOUNT_DATA_WALLET_COINS, add_coins);

    coinwallet = account_get_d(ACCOUNT_DATA_WALLET_COINS);
    gemwallet = account_get_d(ACCOUNT_DATA_WALLET_GEMS);

    account_save();
#endif
}

static int shop_iap_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;
    
    switch (tok)
    {
    case GUI_BACK:
        if (curr_cancel_fn)
            return curr_cancel_fn(cancel_state);

        return goto_state(cancel_state);
        break;
    case SHOP_IAP_GET_BUY:
#ifdef CONFIG_INCLUDES_ACCOUNT
        if (!iappage)
        {
            if (account_get_d(ACCOUNT_DATA_WALLET_GEMS) >= iapcoinfromgems[val])
            {
                shop_convert_to_coins(iapcoinfromgems[val], iapcoinvalue[val]);
                log_printf("Succesfully converted: %d Gems to %d Coins\n", iapcoinfromgems[val], iapcoinvalue[val]);
                purchased = 1;
            }
        }
        else
        {
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && ENABLE_IAP==1
            game_payment_browse(val);
#endif
        }

        if (purchased)
        {
            audio_play("snd/buyproduct.ogg", 1.0f);

            if (curr_ok_fn)
                return curr_ok_fn(ok_state);

            return goto_state(ok_state);
        }
#endif

        break;

    case SHOP_IAP_GET_SWITCH:
        iappage = !iappage;
        goto_state_full(&st_shop_iap,
            iappage ? GUI_ANIMATION_W_CURVE : GUI_ANIMATION_E_CURVE,
            iappage ? GUI_ANIMATION_E_CURVE : GUI_ANIMATION_W_CURVE,
            0);
        break;
#if NB_STEAM_API==0 && NB_EOS_SDK==0 && ENABLE_IAP==1
    case SHOP_IAP_ENTERCODE:
        return goto_shop_activate(ok_state, cancel_state,
                                  curr_ok_fn, curr_cancel_fn);
        break;
#endif
    case SHOP_IAP_EXPORT:
        goto_state(&st_expenses_export);
        break;
    }
    return 1;
}

static int shop_iap_gui(void)
{
    int w = video.device_w;
    int h = video.device_h;

    int multiply;

    int id, jd, kd;
    if ((id = gui_vstack(0)))
    {
        if ((jd = gui_hstack(id)))
        {
            char walletattr[MAXSTR];
            if (iappage)
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(walletattr, dstSize, _("You have %i Gems"), gemwallet);
#else
                sprintf(walletattr, _("You have %i Gems"), gemwallet);
#endif
            else
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(walletattr, dstSize, _("You have %i Coins"), coinwallet);
#else
                sprintf(walletattr, _("You have %i Coins"), coinwallet);
#endif

            gui_label(jd, walletattr, GUI_SML, gui_yel, gui_red);

#if NB_STEAM_API==1 || NB_EOS_SDK==1 || ENABLE_IAP==1
            if (multipage && video.aspect_ratio >= 1.0f)
            {
                gui_space(jd);
                if (iappage)
                    gui_state(jd, _("Switch to Coins"), GUI_SML, SHOP_IAP_GET_SWITCH, 0);
                else
                    gui_state(jd, _("Switch to Gems"), GUI_SML, SHOP_IAP_GET_SWITCH, 0);
            }
#endif

            gui_filler(jd);
            gui_space(jd);

#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
            if (current_platform == PLATFORM_PC)
#endif
                gui_start(jd, _("Back"), GUI_SML, GUI_BACK, 0);
        }

#if NB_STEAM_API==1 || NB_EOS_SDK==1 || ENABLE_IAP==1
        if (multipage && video.aspect_ratio < 1.0f)
        {
            if (iappage)
                gui_state(jd, _("Switch to Coins"), GUI_SML, SHOP_IAP_GET_SWITCH, 0);
            else
                gui_state(jd, _("Switch to Gems"), GUI_SML, SHOP_IAP_GET_SWITCH, 0);
        }
#endif

#if NB_HAVE_PB_BOTH==1
        if (curr_min > 0)
        {
            /* Since that is tested the mission tasks like "Learn to fly 3" */
            gui_space(id);

            char missionattr[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            if (iappage)
                sprintf_s(missionattr, dstSize, _("Need %i gems to complete transaction!"), (curr_min - account_get_d(ACCOUNT_DATA_WALLET_GEMS)));
            else
                sprintf_s(missionattr, dstSize, _("Need %i coins to complete transaction!"), (curr_min - account_get_d(ACCOUNT_DATA_WALLET_COINS)));
#else
            if (iappage)
                sprintf(missionattr, _("Need %i gems to complete transaction!"), (curr_min - account_get_d(ACCOUNT_DATA_WALLET_GEMS)));
            else
                sprintf(missionattr, _("Need %i coins to complete transaction!"), (curr_min - account_get_d(ACCOUNT_DATA_WALLET_COINS)));
#endif

            gui_label(id, missionattr, GUI_SML, gui_red, gui_red);
        }
#endif

        gui_space(id);

        if (video.aspect_ratio >= 1.0f)
        {
            if ((jd = gui_hstack(id)))
            {
                gui_filler(jd);
                for (multiply = 6; multiply > 0; multiply--)
                {
                    switch (iappage)
                    {
                    case 0:
#ifdef CONFIG_INCLUDES_ACCOUNT
                        if (iapcoinvalue[multiply - 1] >= (curr_min - account_get_d(ACCOUNT_DATA_WALLET_COINS)))
                            if ((kd = gui_vstack(jd)))
                            {
                                const GLubyte *sufficent_col =
                                    account_get_d(ACCOUNT_DATA_WALLET_GEMS) >= iapcoinfromgems[multiply - 1] ?
                                    gui_wht : gui_red;
                                int sufficent_action =
                                    account_get_d(ACCOUNT_DATA_WALLET_GEMS) >= iapcoinfromgems[multiply - 1] ?
                                    SHOP_IAP_GET_BUY : GUI_NONE;

                                char iapattr[MAXSTR], imgattr[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                                sprintf_s(imgattr, dstSize, "gui/shop/coins-%s.png", iaptiers[multiply - 1]);
                                sprintf_s(iapattr, dstSize, GUI_DIAMOND " %d", iapcoinfromgems[multiply - 1]);
#else
                                sprintf(imgattr, "gui/shop/coins-%s.png", iaptiers[multiply - 1]);
                                sprintf(iapattr, GUI_DIAMOND " %d", iapcoinfromgems[multiply - 1]);
#endif

                                gui_image(kd, imgattr, w / 7, h / 5);
                                gui_label(kd, iapattr, GUI_SML, sufficent_col, sufficent_col);
                                gui_filler(kd);
                                gui_set_state(kd, sufficent_action, multiply - 1);
                            }
#endif
                        break;
                    case 1:
#ifdef CONFIG_INCLUDES_ACCOUNT
                        if (iapgemvalue[multiply - 1] >= (curr_min - account_get_d(ACCOUNT_DATA_WALLET_GEMS)))
                            if ((kd = gui_vstack(jd)))
                            {
                                wchar_t pWLocaleName[MAXSTR];
                                size_t pCharC;
                                char pCharExt[MAXSTR], pChar[MAXSTR];
                                if (strlen(config_get_s(CONFIG_LANGUAGE)) < 2)
                                    LANG_CURRENCY_RESET_DEFAULTS;
                                else
                                    SAFECPY(pChar, config_get_s(CONFIG_LANGUAGE));

                                char iapattr[MAXSTR], imgattr[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                                sprintf_s(imgattr, dstSize, "gui/shop/gems-%s.png", iaptiers[multiply - 1]);
                                sprintf_s(iapattr, dstSize, "%s", currency_get_price_from_locale(pChar, iapgemcost[multiply - 1]));
#else
                                sprintf(imgattr, "gui/shop/gems-%s.png", iaptiers[multiply - 1]);
                                sprintf(iapattr, "%s", currency_get_price_from_locale(pChar, iapgemcost[multiply - 1]));
#endif

                                gui_image(kd, imgattr, w / 7, h / 5);
                                gui_label(kd, iapattr, GUI_SML, gui_wht, gui_wht);
                                gui_filler(kd);
                                gui_set_state(kd, SHOP_IAP_GET_BUY, multiply - 1);
                            }
#endif
                        break;
                    }

                }
                gui_filler(jd);
            }
        }
        else
        {
            char iapattr[MAXSTR]; int btniapmobile;
            if ((jd = gui_vstack(id)))
            {
                for (multiply = 1; multiply < 7; multiply++)
                {
                    switch (iappage)
                    {
                    case 0:
#ifdef CONFIG_INCLUDES_ACCOUNT
                        if (iapcoinvalue[multiply - 1] >= (curr_min - account_get_d(ACCOUNT_DATA_WALLET_COINS)))
                        {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                            sprintf_s(iapattr, dstSize, _("%d Coins (%s %d)"), GUI_DIAMOND, iapcoinvalue[multiply - 1], iapcoinfromgems[multiply - 1]);
#else
                            sprintf(iapattr, _("%d Coins (%s %d)"), GUI_DIAMOND, iapcoinvalue[multiply - 1], iapcoinfromgems[multiply - 1]);
#endif
                            btniapmobile = gui_label(jd, iapattr, GUI_SML, account_get_d(ACCOUNT_DATA_WALLET_GEMS) >= iapcoinfromgems[multiply - 1] ? gui_wht : gui_red, account_get_d(ACCOUNT_DATA_WALLET_GEMS) >= iapcoinfromgems[multiply - 1] ? gui_wht : gui_red);
                            gui_set_state(btniapmobile, account_get_d(ACCOUNT_DATA_WALLET_GEMS) >= iapcoinfromgems[multiply - 1] ? SHOP_IAP_GET_BUY : GUI_NONE, multiply - 1);
                        }
#endif
                        break;
                    case 1:
#ifdef CONFIG_INCLUDES_ACCOUNT
                        if (iapgemvalue[multiply - 1] >= (curr_min - account_get_d(ACCOUNT_DATA_WALLET_GEMS)))
                        {
                            wchar_t pWLocaleName[MAXSTR];
                            size_t pCharC;
                            char pCharExt[MAXSTR], pChar[MAXSTR];
                            if (strlen(config_get_s(CONFIG_LANGUAGE)) < 2)
                                LANG_CURRENCY_RESET_DEFAULTS;
                            else
                                SAFECPY(pChar, config_get_s(CONFIG_LANGUAGE));

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                            sprintf_s(iapattr, dstSize, _("%d Gems (%s)"), iapgemvalue[multiply - 1], currency_get_price_from_locale(pChar, iapgemcost[multiply - 1]));
#else
                            sprintf(iapattr, _("%d Gems (%s)"), iapgemvalue[multiply - 1], currency_get_price_from_locale(pChar, iapgemcost[multiply - 1]));
#endif
                            btniapmobile = gui_label(jd, iapattr, GUI_SML, gui_wht, gui_wht);
                            gui_set_state(btniapmobile, SHOP_IAP_GET_BUY, multiply - 1);
                        }
#endif
                        break;
                    }
                }
            }
        }

#if NB_STEAM_API==0 && NB_EOS_SDK==0 && ENABLE_IAP==1
        if (iappage)
        {
            gui_space(id);
            int enterkey_btn_id = gui_state(id, _("I have an order code!"), GUI_SML, SHOP_IAP_ENTERCODE, 0);
        }
#endif

        if (server_policy_get_d(SERVER_POLICY_EDITION) >= 10000
            && ((account_get_d(ACCOUNT_DATA_WALLET_COINS) / 5) >= 1
                || account_get_d(ACCOUNT_DATA_WALLET_GEMS) >= 1)
            && curr_min == 0)
        {
            gui_space(id);
            gui_state(id, _("Export to Expenses"), GUI_SML, SHOP_IAP_EXPORT, 0);
        }

        gui_layout(id, 0, 0);
    }
    return id;
}

static int shop_iap_enter(struct state *st, struct state *prev)
{
    purchased = 0;
#ifdef CONFIG_INCLUDES_ACCOUNT
    coinwallet = account_get_d(ACCOUNT_DATA_WALLET_COINS);
#endif
    return shop_iap_gui();
}

static void shop_iap_paint(int id, float t)
{
    game_client_draw(0, t);

    gui_paint(id);
#ifndef __EMSCRIPTEN__
    xbox_control_shop_getcoins_gui_paint();
#endif
}

static int shop_iap_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT)
            return shop_iap_action(GUI_BACK, 0);
    }
    return 1;
}

static int shop_iap_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return shop_iap_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return shop_iap_action(GUI_BACK, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

#define INIT_AUCTION_OVERBID \
    do { \
        if (purchase_product_usegems) \
        { \
            while (auction_value + prodcost \
                   < account_get_d(ACCOUNT_DATA_WALLET_GEMS)) \
            { \
                auction_value += prodcost; \
                piece_times++; \
            } \
        } \
        else \
        { \
            while (auction_value + prodcost \
                   < account_get_d(ACCOUNT_DATA_WALLET_COINS)) \
            { \
                auction_value += prodcost; \
                piece_times++; \
            } \
        } \
    } while (0)

int prodcost1 = 0;
int prodcost2 = 0;
int confirm_multiple_items = 0;

static int max_balls_limit = 1110;

void shop_set_product_key(int newkey) {
    productkey = newkey;
}

enum
{
    SHOP_BUY_YES = GUI_LAST,
    SHOP_BUY_FIVE,
    SHOP_BUY_WHOLE,
    SHOP_BUY_IAP
};

static int shop_buy_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

#ifdef CONFIG_INCLUDES_ACCOUNT
    gemwallet = account_get_d(ACCOUNT_DATA_WALLET_GEMS);
    coinwallet = account_get_d(ACCOUNT_DATA_WALLET_COINS);
#endif

    int prodcost = 0;

    switch (productkey)
    {
    case 0:
    case 1:
        prodcost = 250; break;
    case 2:
    case 3:
        prodcost = 120; break;
    case 4:
    case 5:
    case 6:
        prodcost = 75; break;
    case 7:
        prodcost = 15; break;
    }

    int auction_value = prodcost;
    int piece_times = 1;

    switch (tok)
    {
    case SHOP_BUY_WHOLE:
        if (confirm_multiple_items == 0)
        {
            confirm_multiple_items = 2;
            return goto_state(curr_state());
        }

        confirm_multiple_items = 0;
        audio_play("snd/buyproduct.ogg", 1.0f);

#ifdef CONFIG_INCLUDES_ACCOUNT
        if (confirm_multiple_items == 2)
            INIT_AUCTION_OVERBID;
        else
        {
            piece_times = 5;
            auction_value = prodcost * piece_times;
        }

        if (productkey == 7)
        {
            while (max_balls_limit < account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES) + piece_times)
            {
                piece_times--;
                auction_value = prodcost * piece_times;
            }
        }

        if (purchase_product_usegems)
        {
            gemwallet -= auction_value;
            assert(gemwallet >= 0);
            account_set_d(ACCOUNT_DATA_WALLET_GEMS, gemwallet);
        }
        else
        {
            coinwallet -= auction_value;
            assert(coinwallet >= 0 && coinwallet <= 1000000);
            account_set_d(ACCOUNT_DATA_WALLET_COINS, coinwallet);
        }

        switch (productkey)
        {
        case 4:
            evalue += piece_times;
            account_set_d(ACCOUNT_CONSUMEABLE_EARNINATOR, evalue);
            break;

        case 5:
            fvalue += piece_times;
            account_set_d(ACCOUNT_CONSUMEABLE_FLOATIFIER, fvalue);
            break;

        case 6:
            svalue += piece_times;
            account_set_d(ACCOUNT_CONSUMEABLE_SPEEDIFIER, svalue);
            break;

        case 7:
            lvalue += piece_times;
            account_set_d(ACCOUNT_CONSUMEABLE_EXTRALIVES, lvalue);
            break;
        }
        account_save();
#endif
        return goto_state(&st_shop);
        break;

    case SHOP_BUY_FIVE:
        if (confirm_multiple_items == 0)
        {
            confirm_multiple_items = 1;
            return goto_state(curr_state());
        }

        confirm_multiple_items = 0;
        audio_play("snd/buyproduct.ogg", 1.0f);

#ifdef CONFIG_INCLUDES_ACCOUNT
        if (purchase_product_usegems)
        {
            gemwallet -= prodcost * 5;
            assert(gemwallet >= 0);
            account_set_d(ACCOUNT_DATA_WALLET_GEMS, gemwallet);
        }
        else
        {
            coinwallet -= prodcost * 5;
            assert(coinwallet >= 0 && coinwallet <= 1000000);
            account_set_d(ACCOUNT_DATA_WALLET_COINS, coinwallet);
        }

        switch (productkey)
        {
        case 4:
            evalue += 5;
            account_set_d(ACCOUNT_CONSUMEABLE_EARNINATOR, evalue);
            break;

        case 5:
            fvalue += 5;
            account_set_d(ACCOUNT_CONSUMEABLE_FLOATIFIER, fvalue);
            break;

        case 6:
            svalue += 5;
            account_set_d(ACCOUNT_CONSUMEABLE_SPEEDIFIER, svalue);
            break;

        case 7:
            lvalue += 5;
            account_set_d(ACCOUNT_CONSUMEABLE_EXTRALIVES, lvalue);
            break;
        }
        account_save();
#endif
        return goto_state(&st_shop);
        break;

    case SHOP_BUY_YES:
        audio_play("snd/buyproduct.ogg", 1.0f);

#ifdef CONFIG_INCLUDES_ACCOUNT
        if (purchase_product_usegems)
        {
            gemwallet -= prodcost;
            assert(gemwallet >= 0);
            account_set_d(ACCOUNT_DATA_WALLET_GEMS, gemwallet);
        }
        else
        {
            coinwallet -= prodcost;
            assert(coinwallet >= 0 && coinwallet <= 1000000);
            account_set_d(ACCOUNT_DATA_WALLET_COINS, coinwallet);
        }

        switch (productkey)
        {
            case 0:
                account_set_d(ACCOUNT_PRODUCT_LEVELS, 1);
                break;

            case 1:
#ifndef __EMSCRIPTEN__
                xbox_control_gui_free();
#endif
                hud_free();
                gui_free();
                account_set_d(ACCOUNT_PRODUCT_BALLS, 1);
                gui_init();
                hud_init();
#ifndef __EMSCRIPTEN__
                xbox_control_gui_init();
#endif
                break;

            case 2:
                account_set_d(ACCOUNT_PRODUCT_BONUS, 1);
                break;

            case 3:
#ifdef LEVELGROUPS_INCLUDES_ZEN
                account_set_d(ACCOUNT_PRODUCT_MEDIATION, 1);
#else
                return 1;
#endif
                break;

            case 4:
                evalue += 1;
                account_set_d(ACCOUNT_CONSUMEABLE_EARNINATOR, evalue);
                break;

            case 5:
                fvalue += 1;
                account_set_d(ACCOUNT_CONSUMEABLE_FLOATIFIER, fvalue);
                break;

            case 6:
                svalue += 1;
                account_set_d(ACCOUNT_CONSUMEABLE_SPEEDIFIER, svalue);
                break;

            case 7:
                lvalue += 1;
                account_set_d(ACCOUNT_CONSUMEABLE_EXTRALIVES, lvalue);
                break;
        }
        account_save();
#endif
        return goto_state(&st_shop);
        break;

    case SHOP_BUY_IAP:
        return goto_shop_iap(&st_shop_buy, &st_shop, 0, 0, prodcost, purchase_product_usegems, 0);
        break;

    case GUI_BACK:
        confirm_multiple_items = 0;
        return goto_state(&st_shop);
        break;
    }
    return 1;
}

static int has_enough_coins(int wprodcost)
{
    int enough = 0;
#ifdef CONFIG_INCLUDES_ACCOUNT
    int currentwallet = account_get_d(ACCOUNT_DATA_WALLET_COINS);
    if (currentwallet >= wprodcost) { enough = 1; }
#endif

    return enough;
}

static int has_enough_gems(int wprodcost)
{
    int enough = 0;
#ifdef CONFIG_INCLUDES_ACCOUNT
    int currentwallet = account_get_d(ACCOUNT_DATA_WALLET_GEMS);
    if (currentwallet >= wprodcost) { enough = 1; }
#endif

    return enough;
}

static int has_owned(void)
{
    int owned = 0;

    switch (productkey)
    {
#ifdef CONFIG_INCLUDES_ACCOUNT
    case 0: return account_get_d(ACCOUNT_PRODUCT_LEVELS); break;
    case 1: return account_get_d(ACCOUNT_PRODUCT_BALLS); break;
    case 2: return account_get_d(ACCOUNT_PRODUCT_BONUS); break;
#ifdef LEVELGROUPS_INCLUDES_ZEN
    case 3: return account_get_d(ACCOUNT_PRODUCT_MEDIATION); break;
#endif
#endif
    }

    return 0;
}

#ifdef LEVELGROUPS_INCLUDES_ZEN
#define INIT_PRODUCT_DETAILS() \
do { \
    switch (productkey) \
    { \
        case 0: SAFECPY(prodname, _("Extra Levels")); prodcost = 250; break; \
        case 1: SAFECPY(prodname, _("Online Balls")); prodcost = 250; break; \
        case 2: SAFECPY(prodname, _("Bonus Pack")); prodcost = 120; break; \
        case 3: SAFECPY(prodname, _("Mediation")); prodcost = 120; break; \
        case 4: SAFECPY(prodname, _("Earninator")); prodcost = 75; prodincomsumeable = 1; break; \
        case 5: SAFECPY(prodname, _("Floatifier")); prodcost = 75; prodincomsumeable = 1; break; \
        case 6: SAFECPY(prodname, _("Speedifier")); prodcost = 75; prodincomsumeable = 1; break; \
        case 7: SAFECPY(prodname,  _("Extra Balls")); prodcost = 15; prodincomsumeable = 1; break; \
        default: SAFECPY(prodname,  _("Product item")); prodcost = 0; break; \
    } \
} while (0)
#else
#define INIT_PRODUCT_DETAILS() \
do { \
    switch (productkey) \
    { \
        case 0: SAFECPY(prodname, _("Extra Levels")); prodcost = 250; break; \
        case 1: SAFECPY(prodname, _("Online Balls")); prodcost = 250; break; \
        case 2: SAFECPY(prodname, _("Bonus Pack")); prodcost = 120; break; \
        case 4: SAFECPY(prodname, _("Earninator")); prodcost = 75; prodincomsumeable = 1; break; \
        case 5: SAFECPY(prodname, _("Floatifier")); prodcost = 75; prodincomsumeable = 1; break; \
        case 6: SAFECPY(prodname, _("Speedifier")); prodcost = 75; prodincomsumeable = 1; break; \
        case 7: SAFECPY(prodname,  _("Extra Balls")); prodcost = 15; prodincomsumeable = 1; break; \
        default: SAFECPY(prodname,  _("Product item")); prodcost = 0; break; \
    } \
} while (0)
#endif

static int shop_buy_gui(void)
{
    int id, jd;
    if ((id = gui_vstack(0)))
    {
        char prodname[MAXSTR];
        int prodcost;
        int prodincomsumeable = 0;

        INIT_PRODUCT_DETAILS();

#ifdef CONFIG_INCLUDES_ACCOUNT
        lvalue = account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES);
#endif

        if (lvalue >= max_balls_limit && productkey == 7)
            gui_title_header(id, _("Max balls!"), GUI_MED, gui_gry, gui_red);
        else if (has_owned())
            gui_title_header(id, _("Product owned!"), GUI_MED, gui_gry, gui_red);
        else if ((!purchase_product_usegems && has_enough_coins(prodcost)) || (purchase_product_usegems && has_enough_gems(prodcost)))
            gui_title_header(id, _("Buy Products?"), GUI_MED, gui_yel, gui_red);
        else
            gui_title_header(id, _("Insufficent wallet!"), GUI_MED, gui_gry, gui_red);

        gui_space(id);

        if (lvalue >= max_balls_limit && productkey == 7)
        {
            char limitattr[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(limitattr, dstSize, _("You can't buy more than %d balls\\in a single level set for\\%s."), max_balls_limit, _("Challenge Mode"));
#else
            sprintf(limitattr, _("You can't buy more than %d balls\\in a single level set for\\%s."), max_balls_limit, _("Challenge Mode"));
#endif
            gui_multi(id, limitattr, GUI_SML, gui_wht, gui_wht);
            gui_space(id);
            gui_start(id, _("Sorry, I'm too excited!"), GUI_SML, GUI_BACK, 0);
        }
        else if (!has_owned())
        {
            char prodattr[MAXSTR];
            char unloadattr[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            if (purchase_product_usegems)
            {
                if (has_enough_gems(prodcost))
                    sprintf_s(prodattr, dstSize, _("Would you like buy this Products?\\%s costs %i gems."), prodname, prodcost);
#if NB_STEAM_API==1 || NB_EOS_SDK==1 || ENABLE_IAP==1
                else if (server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_IAP) && prodcost <= 1920)
                    sprintf_s(prodattr, dstSize, _("You need at least %i gems to buy %s,\\but you can review from IAP."), prodcost, prodname);
#endif
                else
                    sprintf_s(prodattr, dstSize, _("You need at least %i gems to buy %s."), prodcost, prodname);
            }
            else
            {
                if (has_enough_coins(prodcost))
                    sprintf_s(prodattr, dstSize, _("Would you like buy this Products?\\%s costs %i coins."), prodname, prodcost);
                else if (server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_IAP) && prodcost <= 1920)
                    sprintf_s(prodattr, dstSize, _("You need at least %i coins to buy %s,\\but you can purchase from coin shop."), prodcost, prodname);
                else
                    sprintf_s(prodattr, dstSize, _("You need at least %i coins to buy %s."), prodcost, prodname);
            }
#else
            if (purchase_product_usegems)
            {
                if (has_enough_gems(prodcost))
                    sprintf(prodattr, _("Would you like buy this Products?\\%s costs %i gems."), prodname, prodcost);
#if NB_STEAM_API==1 || NB_EOS_SDK==1 || ENABLE_IAP==1
                else if (server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_IAP) && prodcost <= 1920)
                    sprintf(prodattr, _("You need at least %i gems to buy %s,\\but you can review from IAP."), prodcost, prodname);
#endif
                else
                    sprintf(prodattr, _("You need at least %i gems to buy %s."), prodcost, prodname);
            }
            else
            {
                if (has_enough_coins(prodcost))
                    sprintf(prodattr, _("Would you like buy this Products?\\%s costs %i coins."), prodname, prodcost);
                else if (server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_IAP) && prodcost <= 1920)
                    sprintf(prodattr, _("You need at least %i coins to buy %s,\\but you can purchase from coin shop."), prodcost, prodname);
                else
                    sprintf(prodattr, _("You need at least %i coins to buy %s."), prodcost, prodname);
            }
#endif
            gui_multi(id, prodattr, GUI_SML, gui_wht, gui_wht);

            if (purchase_product_usegems)
            {
                if (has_enough_gems(prodcost * 5)
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
                    && current_platform == PLATFORM_PC
#endif
                    && prodincomsumeable)
                {
                    gui_space(id);
                    gui_state(id, _("Unload balance and buy!"), GUI_SML, SHOP_BUY_WHOLE, prodcost);
                    gui_space(id);
                    gui_state(id, _("Buy 5 products!"), GUI_SML, SHOP_BUY_FIVE, prodcost);
                }
            }
            else if (has_enough_coins(prodcost * 5)
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
                && current_platform == PLATFORM_PC
#endif
                && prodincomsumeable)
            {
                gui_space(id);
                gui_state(id, _("Unload balance and buy!"), GUI_SML, SHOP_BUY_WHOLE, prodcost);
                gui_space(id);
                gui_state(id, _("Buy 5 products!"), GUI_SML, SHOP_BUY_FIVE, prodcost);
            }

            gui_space(id);

            if ((jd = gui_harray(id)))
            {
                if (purchase_product_usegems)
                {
                    if (has_enough_gems(prodcost))
                    {
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
                        if (current_platform == PLATFORM_PC)
#endif
                        {
                            gui_start(jd, _("No"), GUI_SML, GUI_BACK, 0);
                            gui_state(jd, _("Yes"), GUI_SML, SHOP_BUY_YES, prodcost);
                        }
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
                        else
                        {
                            if (has_enough_gems(prodcost * 5) && prodincomsumeable)
                                gui_state(jd, _("Buy 5 products!"), GUI_SML, SHOP_BUY_FIVE, prodcost);

                            gui_start(jd, _("Yes"), GUI_SML, SHOP_BUY_YES, prodcost);
                        }
#endif
                    }
                    else
                    {
                        int getcoins_id;
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
                        if (current_platform == PLATFORM_PC)
#endif
                        {
                            gui_start(jd, _("Back"), GUI_SML, GUI_BACK, 0);
#if NB_STEAM_API==1 || NB_EOS_SDK==1 || ENABLE_IAP==1
                            getcoins_id = gui_state(jd, _("Get Gems!"), GUI_SML, SHOP_BUY_IAP, 0);
#endif
                        }
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
                        else
                            getcoins_id = gui_start(jd, _("Get Gems!"), GUI_SML, SHOP_BUY_IAP, 0);
#endif
                        if (!server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_IAP) && prodcost >= 1920)
                        {
                            gui_set_color(getcoins_id, gui_gry, gui_gry);
                            gui_set_state(getcoins_id, GUI_NONE, 0);
                        }
                    }
                }
                else if (has_enough_coins(prodcost))
                {
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
                    if (current_platform == PLATFORM_PC)
#endif
                    {
                        gui_start(jd, _("No"), GUI_SML, GUI_BACK, 0);
                        gui_state(jd, _("Yes"), GUI_SML, SHOP_BUY_YES, prodcost);
                    }
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
                    else
                    {
                        if (has_enough_coins(prodcost * 5) && prodincomsumeable)
                            gui_state(jd, _("Buy 5 products!"), GUI_SML, SHOP_BUY_FIVE, prodcost);

                        gui_start(jd, _("Yes"), GUI_SML, SHOP_BUY_YES, prodcost);
                    }
#endif
                }
                else
                {
                    int getcoins_id;
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
                    if (current_platform == PLATFORM_PC)
#endif
                    {
                        gui_start(jd, _("Back"), GUI_SML, GUI_BACK, 0);
                        getcoins_id = gui_state(jd, _("Get coins!"), GUI_SML, SHOP_BUY_IAP, 0);
                    }
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
                    else
                        getcoins_id = gui_start(jd, _("Get coins!"), GUI_SML, SHOP_BUY_IAP, 0);
#endif
                    if (!server_policy_get_d(SERVER_POLICY_SHOP_ENABLED_IAP) && prodcost >= 1920)
                    {
                        gui_set_color(getcoins_id, gui_gry, gui_gry);
                        gui_set_state(getcoins_id, GUI_NONE, 0);
                    }
                }
            }
        }
        else
        {
            gui_multi(id, _("You've already owned this product,\\so don't buy it again!"), GUI_SML, gui_wht, gui_wht);

#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
            if (current_platform == PLATFORM_PC)
#endif
                gui_start(id, _("Buy more!"), GUI_SML, GUI_BACK, 0);
        }

        gui_layout(id, 0, 0);
    }
    return id;
}

static int shop_buy_confirmmulti_gui(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        char prodname[MAXSTR];
        int prodcost;
        int prodincomsumeable = 0;

        INIT_PRODUCT_DETAILS();

        gui_title_header(id, _("Buy multiple Products?"), GUI_MED, gui_yel, gui_red);

        int auction_value = prodcost;
        int piece_times = 1;

#ifdef CONFIG_INCLUDES_ACCOUNT
        if (confirm_multiple_items == 2)
            INIT_AUCTION_OVERBID;
        else
        {
            piece_times = 5;
            auction_value = prodcost * piece_times;
        }

        if (productkey == 7)
        {
            while (max_balls_limit < account_get_d(ACCOUNT_CONSUMEABLE_EXTRALIVES) + piece_times)
            {
                piece_times--;
                auction_value = prodcost * piece_times;
            }
        }
#endif

        char prodattr[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(prodattr, dstSize,
#else
        sprintf(prodattr,
#endif
                _("You're trying to buy multiple Products!\\%d %s costs %d %s."),
                piece_times, prodname, auction_value, _(purchase_product_usegems ? "Gems" : "Coins"));

        gui_space(id);
        gui_multi(id, prodattr, GUI_SML, gui_wht, gui_wht);
        gui_space(id);

        if ((jd = gui_harray(id)))
        {
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
            if (current_platform == PLATFORM_PC)
#endif
            {
                gui_start(jd, _("No"), GUI_SML, GUI_BACK, 0);
                gui_state(jd, _("Yes"), GUI_SML, confirm_multiple_items == 2 ? SHOP_BUY_WHOLE : SHOP_BUY_FIVE, auction_value);
            }
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
            else
                gui_start(jd, _("Yes"), GUI_SML, confirm_multiple_items == 2 ? SHOP_BUY_WHOLE : SHOP_BUY_FIVE, auction_value);
#endif
        }
    }

    gui_layout(id, 0, 0);

    return id;
}

static int shop_buy_enter(struct state *st, struct state *prev)
{
    return confirm_multiple_items == 0 ? shop_buy_gui() : shop_buy_confirmmulti_gui();
}

static int shop_buy_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT)
            return shop_buy_action(GUI_BACK, 0);
    }
    return 1;
}

static int shop_buy_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return shop_buy_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return shop_buy_action(GUI_BACK, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

enum
{
    EXPENSES_EXPORT_START = GUI_LAST
};

static int expenses_exported = 0;
static int export_totalvalue = 0;
static int export_totalgems = 0;

static void expenses_export_start(void)
{
    export_totalvalue = 0;
    export_totalgems = 0;

    int temp_totalcoins = account_get_d(ACCOUNT_DATA_WALLET_COINS);

    while (temp_totalcoins >= 5)
    {
        temp_totalcoins -= 5;
        export_totalgems++;
    }

    account_set_d(ACCOUNT_DATA_WALLET_COINS, temp_totalcoins);

    export_totalgems += account_get_d(ACCOUNT_DATA_WALLET_GEMS);

    export_totalvalue += export_totalgems * 16;
    account_set_d(ACCOUNT_DATA_WALLET_GEMS, 0);
    account_save();

    expenses_exported = 1;
}

static int expenses_export_action(int tok, int val)
{
    GENERIC_GAMEMENU_ACTION;

    switch (tok)
    {
    case GUI_BACK:
        return goto_state(&st_shop);
        break;
    case EXPENSES_EXPORT_START:
        audio_play("snd/buyproduct.ogg", 1.0f);
        expenses_export_start();
        goto_state(curr_state());
        break;
    }

    return 1;
}

static int expenses_export_gui(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        char desc_attr[MAXSTR];

        if (expenses_exported)
        {
            gui_title_header(id, _("Exported to Expenses"), GUI_MED, 0, 0);

            int cents = export_totalvalue;
            int whole = 0;
            while (cents >= 100)
            {
                cents -= 100;
                whole++;
            }

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(desc_attr, dstSize,
#else
            sprintf(desc_attr,
#endif
                    _("We have %d,%02d € on your file.\\"
                      "%d Gems has been transferred.\\"
                      "Consider entering to your Expenses app."), whole, cents, export_totalgems);
        }
        else
        {
            gui_title_header(id, _("Export to Expenses?"), GUI_MED, 0, 0);
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(desc_attr, dstSize,
#else
            sprintf(desc_attr,
#endif
                    _("Export Gems and transfer to Expenses app?"));
        }

        gui_space(id);
        gui_multi(id, desc_attr, GUI_SML, gui_wht, gui_wht);
        gui_space(id);

        if (expenses_exported)
            gui_start(id, _("OK"), GUI_SML, GUI_BACK, 0);
        else if ((jd = gui_harray(id)))
        {
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
            if (current_platform == PLATFORM_PC)
#endif
            {
                gui_start(jd, _("No"), GUI_SML, GUI_BACK, 0);
                gui_state(jd, _("Yes"), GUI_SML, EXPENSES_EXPORT_START, 0);
            }
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
            else
                gui_start(jd, _("Yes"), GUI_SML, EXPENSES_EXPORT_START, 0);
#endif
        }

        gui_layout(id, 0, 0);
    }

    return id;
}

static int expenses_export_enter(struct state* st, struct state* prev)
{
    if (prev == &st_shop_iap)
    {
        expenses_exported = 0;
        export_totalvalue = 0;
    }

    return expenses_export_gui();
}

static int expenses_export_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT)
            return expenses_export_action(GUI_BACK, 0);
    }
    return 1;
}

static int expenses_export_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return expenses_export_action(gui_token(active), gui_value(active));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            return expenses_export_action(GUI_BACK, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

struct state st_shop = {
    shop_enter,
    shared_leave,
    shop_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    shop_keybd,
    shop_buttn
};

struct state st_shop_unlocked = {
    shop_unlocked_enter,
    shared_leave,
    shared_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    shop_keybd,
    shop_buttn
};

struct state st_shop_rename = {
    shop_rename_enter,
    shop_rename_leave,
    shop_rename_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    shop_rename_keybd,
    shop_rename_buttn
};

struct state st_shop_unregistered = {
    shop_unregistered_enter,
    shared_leave,
    shared_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    shop_unregistered_keybd,
    shop_unregistered_buttn
};

struct state st_shop_iap = {
    shop_iap_enter,
    shared_leave,
    shop_iap_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    shop_iap_keybd,
    shop_iap_buttn
};

struct state st_shop_buy = {
    shop_buy_enter,
    shared_leave,
    shared_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    shop_buy_keybd,
    shop_buy_buttn
};

struct state st_expenses_export = {
    expenses_export_enter,
    shared_leave,
    shared_paint,
    shared_timer,
    shared_point,
    shared_stick,
    shared_angle,
    shared_click,
    expenses_export_keybd,
    expenses_export_buttn
};
