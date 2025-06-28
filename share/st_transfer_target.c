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

#include "st_transfer.h"

#if defined(GAME_TRANSFER_TARGET) && ENABLE_GAME_TRANSFER==1 && \
    !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
#if _WIN32
#include <ShlObj.h>
#endif

#ifndef DRIVE_REMOVEABLE
#define DRIVE_REMOVEABLE 2
#endif

#ifndef NDEBUG
#include <assert.h>
#elif defined(_MSC_VER) && defined(_AFXDLL)
#include <afx.h>
/**
 * HACK: assert() for Microsoft Windows Apps in Release builds
 * will be replaced to VERIFY() - Ersohn Styne
 */
#define assert VERIFY
#else
#define assert(_x) (_x)
#endif

#include "fs.h"
#include "audio.h"
#include "ball.h"
#include "config.h"
#include "geom.h"
#include "gui.h"
#include "transition.h"
#include "lang.h"
#include "networking.h"

#include "account.h"
#include "account_transfer.h"

#include "st_common.h"

#if _DEBUG && _MSC_VER
#ifndef _CRTDBG_MAP_ALLOC
#pragma message(__FILE__": Missing _CRT_MAP_ALLOC, recreate: _CRTDBG_MAP_ALLOC + crtdbg.h")
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

#define AUD_MENU "snd/menu.ogg"
#define AUD_BACK "snd/back.ogg"

#define TRANSFER_OFFLINE_ONLY
#define TITLE_TARGET_TRANSFER N_("Neverball Game Transfer")

#define TARGET_TRANSFER_WARNING_EXTERNAL \
    N_("Do not touch the External Drive, exit the\\" \
       "game or turn off the PC, otherwise data\\may be lost.")
#define TARGET_TRANSFER_WARNING_INTERNAL \
    N_("Do not exit the game or turn off the PC,\\" \
       "otherwise data may be lost.")
#define TARGET_TRANSFER_WARNING \
    N_("Do not exit the game or turn off the PC.")

static int transfer_process;
static int transfer_ui_transition_busy;
static int have_entered;

static int about_pageindx;
static int show_about;

#if ENABLE_DEDICATED_SERVER==1 && !defined(TRANSFER_OFFLINE_ONLY)
static int preparations_internet = 0;
#endif
static int preparations_working  = 0;
static int preparations_pageindx = 0;
static int show_preparations;

static int transfer_walletamount[2];

static int transfer_working  = 0;
static int transfer_pageindx = 0;
static int show_transfer     = 0;

struct state st_transfer;
static struct state *transfer_back;

int goto_game_transfer(struct state *back)
{
    transfer_back = back;

    return goto_state(&st_transfer);
}

/* introducory (target) */
static int transfer_introducory_gui(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        gui_label(id, _(TITLE_TARGET_TRANSFER), GUI_SML, 0, 0);

        gui_space(id);

        if ((jd = gui_vstack(id)))
        {
            gui_multi(jd, _("This apps allows you to transfer data\\"
                            "from a source game your own to\\"
                            "the modern player of this target game."),
                          GUI_SML, gui_wht, gui_wht);
            gui_multi(jd, _("Once transferred, the data cannot be moved back\\"
                            "to the source game."),
                          GUI_SML, gui_red, gui_red);
            gui_multi(jd, _("The data on the target game will be overwritten\\"
                            "with that of the source game."),
                          GUI_SML, gui_wht, gui_wht);
            gui_multi(jd, _("Please read the following explanation thoroughly\\"
                            "before starting the transfer."),
                          GUI_SML, gui_wht, gui_wht);
            gui_set_rect(jd, GUI_ALL);
        }

        gui_space(id);

        if ((jd = gui_harray(id)))
        {
            gui_start(jd, _("Next"), GUI_SML, GUI_NEXT, 0);
            gui_state(jd, _("Skip"), GUI_SML, GUI_NEXT, 1);
            gui_state(jd, _("Quit"), GUI_SML, GUI_BACK, 0);
        }
    }

    gui_layout(id, 0, 0);

    return id;
}

#pragma region about transferring (target)
/*
 * requires source java or bedrock and target bedrock + external drive + keyboard and mouse
 * nvb game network agreement
 * step 1: prepare
 * step 2: transfer from the java game
 * step 3: transfer to bedrock game
 * allow transfer
 * not allow transfer
 * other notes
 */
static int transfer_about_transferring_gui(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        gui_label(id, _("About Transferring"), GUI_SML, 0, 0);

        gui_space(id);

        if ((jd = gui_vstack(id)))
        {
            switch (about_pageindx)
            {
            case 1:
                gui_multi(jd, _("A game transfer requires\\"
                                "- One source Neverball game and one target\\"
                                "Pennyball game with highest version"),
                              GUI_SML, gui_wht, gui_wht);
                gui_multi(jd, _("- At least one keyboard and mouse"),
                              GUI_SML, gui_wht, gui_wht);
                gui_multi(jd, _("- An External Drive with at least 1 GB of free space"),
                              GUI_SML, gui_wht, gui_wht);
#if ENABLE_DEDICATED_SERVER==1 && !defined(TRANSFER_OFFLINE_ONLY)
                gui_multi(jd, _("- An Internet connection"),
                              GUI_SML, gui_wht, gui_wht);
#endif
                break;
            case 2:
#if ENABLE_DEDICATED_SERVER==1 && !defined(TRANSFER_OFFLINE_ONLY)
                gui_multi(jd, _("To perform a game transfer, you will need to\\"
                                "connect both the source game and the target game\\"
                                "to the internet."),
                              GUI_SML, gui_wht, gui_wht);
#else
                gui_multi(jd, _("To perform a game transfer, you will need to\\"
                                "connect both the source game and the target game."),
                              GUI_SML, gui_wht, gui_wht);
#endif
                gui_multi(jd, _("When performing a transfer, terms of the\\"
                                "Microsoft Services Agreement and PB+NB ToS will apply."),
                              GUI_SML, gui_wht, gui_wht);
                break;
            case 3:
                gui_multi(jd, _("There are three steps to a game transfer."),
                              GUI_SML, gui_wht, gui_wht);
                gui_label(jd, _("Step 1: Prepare the target game"),
                              GUI_SML, 0, 0);
                gui_multi(jd, _("Data used to prepare for the transfer is written\\"
                                "to an External Drive inserted into this game."),
                              GUI_SML, gui_wht, gui_wht);
                break;
            case 4:
                gui_label(jd, _("Step 2: Transfer from the source game"),
                              GUI_SML, 0, 0);
                gui_multi(jd, _("The prepared External Drive is inserted into the source\\"
                                "Neverball, and all data to be transferred is\\"
                                "transferred to the External Drive and deleted\\"
                                "progress from the source game."),
                              GUI_SML, gui_wht, gui_wht);
                break;
            case 5:
                gui_label(jd, _("Step 3: Transfer to target game"),
                              GUI_SML, 0, 0);
                gui_multi(jd, _("Take out the External Drive from the source Neverball,\\"
                                "and insert back into this target Pennyball\\"
                                "to complete the game transfer."),
                              GUI_SML, gui_wht, gui_wht);
                break;
            case 6:
                gui_label(jd, _("The following data will be transferred:"),
                              GUI_SML, gui_grn, gui_grn);
                gui_multi(jd, _("- Account (Player name and marble ball models)"),
                              GUI_SML, gui_wht, gui_wht);
                gui_multi(jd, _("- Highscores (for Campaign and Level Sets)"),
                              GUI_SML, gui_wht, gui_wht);
                break;
            case 7:
                gui_label(jd, _("The following data cannot be transferred\\"
                                "and will be remain on the source game:"),
                              GUI_SML, gui_red, gui_red);
                gui_multi(jd, _("- Data from game settings"),
                              GUI_SML, gui_wht, gui_wht);
                gui_multi(jd, _("- Legacy Neverball replays"),
                              GUI_SML, gui_wht, gui_wht);
                gui_multi(jd, _("- Games that is present on both PCs\\"
                                "(saved data will be transferred and overwritten)"),
                              GUI_SML, gui_wht, gui_wht);
                break;
            case 8:
                gui_label(jd, _("Other notes:"), GUI_SML, gui_yel, gui_yel);
                gui_multi(jd, _("Save data that may already exist on External Drive\\"
                                "cannot be transferred directly."),
                              GUI_SML, gui_wht, gui_wht);
                gui_multi(jd, _("For save data, transfer it from the External Drive\\"
                                "to the source Neverball, before performing\\"
                                "the transfer.\\"
                                "After the transfer on this game, you must do so beforehand."),
                              GUI_SML, gui_wht, gui_wht);
                break;
            case 9:
                gui_label(jd, _("Other notes:"), GUI_SML, gui_yel, gui_yel);
                gui_multi(jd, _("For level sets, either transfer it back to the\\"
                                "source Neverball, before performing the transfer,\\"
                                "or redownload it after the transfer\\"
                                "using the Neverball."),
                              GUI_SML, gui_wht, gui_wht);
                break;
            case 10:
                gui_label(jd, _("Other notes:"), GUI_SML, gui_yel, gui_yel);
                gui_multi(jd, _("Your wallet balance in the account on the\\"
                                "source game will be added to your\\"
                                "balance of wallet on this game.\\"
                                "If the total would exceed the maximum\\"
                                "balance of coins that can be stored,\\"
                                "you will not be able to proceed."),
                              GUI_SML, gui_wht, gui_wht);
                break;
            case 11:
                gui_label(jd, _("Other notes:"), GUI_SML, gui_yel, gui_yel);
                gui_multi(jd, _("- Depending on this app, some features\\"
                                "may be limited after the transfer."),
                              GUI_SML, gui_wht, gui_wht);
                gui_multi(jd, _("- Data may be lost if you turn off your PC\\"
                                "or exit the game during the transfer."),
                              GUI_SML, gui_wht, gui_wht);
                gui_multi(jd, _("- All player models in the game of the target game\\"
                                "will be replaced with the models."),
                              GUI_SML, gui_wht, gui_wht);
                break;
            }

            gui_set_rect(jd, GUI_ALL);
        }

        gui_space(id);

        if ((jd = gui_harray(id)))
        {
            gui_start(jd, _("Next"), GUI_SML, GUI_NEXT, 0);
            gui_state(jd, _("Back"), GUI_SML, GUI_PREV, 0);
        }
    }

    gui_layout(id, 0, 0);

    return id;
}
#pragma endregion

/* starting game transfer (target) */
static int transfer_starting_gui(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        gui_label(id, _("Starting game transfer..."), GUI_SML, 0, 0);

        gui_space(id);

        if ((jd = gui_vstack(id)))
        {
            gui_multi(jd, _("This completes the explanation."),
                          GUI_SML, gui_wht, gui_wht);
            gui_multi(jd, _("If you are sure you have read through the\\"
                            "information thoroughly, select Start."),
                          GUI_SML, gui_wht, gui_wht);
            gui_multi(jd, _("If you want to look over the details again,\\"
                            "select Back."),
                          GUI_SML, gui_wht, gui_wht);
            gui_set_rect(jd, GUI_ALL);
        }

        gui_space(id);

        if ((jd = gui_harray(id)))
        {
            gui_start(jd, _("Start"), GUI_SML, GUI_NEXT, 0);
            gui_state(jd, _("Back"), GUI_SML, GUI_PREV, 0);
        }
    }

    gui_layout(id, 0, 0);

    return id;
}

#pragma region preparing for transfer (target)
/*
 * requires source game and external drive with at least 1 GB
 * connect internet
 * insert external drive (after connect to the internet)
 * data system transfer created (after verify external drive)
 * created game transfer data (after saved game transfer data to local pc)
 * preparations for game transfer complete (after writing data to the external drive)
 */
static int transfer_preparing_gui(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
#if ENABLE_DEDICATED_SERVER==1 && !defined(TRANSFER_OFFLINE_ONLY)
        if (preparations_internet)
        {
            gui_label(id, _("Connecting to the Internet."),
                          GUI_SML, 0, 0);
            gui_space(id);
        }
        else
#endif
        {
            if (!preparations_working)
            {
                if (preparations_pageindx <= 3)
                    gui_label(id, _("Preparing for transfer..."),
                                  GUI_SML, 0, 0);
                else if (preparations_pageindx == 6)
                    gui_label(id, _("Preparation for game transfer complete"),
                                  GUI_SML, 0, 0);
                else
                    gui_label(id, _(TITLE_TARGET_TRANSFER), GUI_SML, 0, 0);

                gui_space(id);
            }
        }

        if ((jd = gui_vstack(id)))
        {
            switch (preparations_pageindx)
            {
            case 0:
#if ENABLE_DEDICATED_SERVER==1 && !defined(TRANSFER_OFFLINE_ONLY)
                if (preparations_internet)
                    gui_multi(jd, _("Connecting to the Internet to confirm that\\"
                                    "the transfer can be performed..."),
                                  GUI_SML, gui_wht, gui_wht);
                else
#endif
                    gui_multi(jd, _("Game transfer preparations is not completed.\\"
                                    "Continue the transfer preparation?"),
                                  GUI_SML, gui_wht, gui_wht);
                break;
            case 1:
                gui_multi(jd, _("Performing a game transfer requires\\"
                                "a source game and an External Drive\\"
                                "with at least 1 GB of free space."),
                              GUI_SML, gui_wht, gui_wht);
                gui_multi(jd, _("Do you have both of these ready?"),
                              GUI_SML, gui_wht, gui_wht);
                break;

            case 2:
#if ENABLE_DEDICATED_SERVER==1 && !defined(TRANSFER_OFFLINE_ONLY)
                if (preparations_internet)
                    gui_multi(jd, _("Connecting to the Internet to confirm that\\"
                                    "the transfer can be performed..."),
                                  GUI_SML, gui_wht, gui_wht);
                else
#endif
                {
                    gui_multi(jd, _("The game must be able to connect to the\\"
                                    "Internet in order to perform a game transfer."),
                                  GUI_SML, gui_wht, gui_wht);
                    gui_multi(jd, _("Connect to the Internet and confirm\\"
                                    "that the transfer can be performed?"),
                                  GUI_SML, gui_wht, gui_wht);
                }
                break;

            case 3:
                if (preparations_working)
                {
                    gui_multi(jd, _("Verify External Drive..."),
                                  GUI_SML, gui_wht, gui_wht);
                    gui_multi(jd, _(TARGET_TRANSFER_WARNING_EXTERNAL),
                                  GUI_SML, gui_red, gui_red);
                }
                else
                    gui_multi(jd, _("Insert an External Drive into this game\\"
                                    "and press Next."),
                                  GUI_SML, gui_wht, gui_wht);
                break;

            case 4:
                if (preparations_working)
                {
                    gui_multi(jd, _("Saving game transfer data to the PC..."),
                                  GUI_SML, gui_wht, gui_wht);
                    gui_multi(jd, _(TARGET_TRANSFER_WARNING_INTERNAL),
                                  GUI_SML, gui_red, gui_red);
                }
                else
                {
                    gui_multi(jd, _("Data for the game transfer will now be created."),
                                  GUI_SML, gui_wht, gui_wht);
                    gui_multi(jd, _(TARGET_TRANSFER_WARNING_EXTERNAL),
                                  GUI_SML, gui_red, gui_red);
                }
                break;

            case 5:
                if (preparations_working)
                {
                    gui_multi(jd, _("Writing data to the External Drive..."),
                                  GUI_SML, gui_wht, gui_wht);
                    gui_multi(jd, _(TARGET_TRANSFER_WARNING_EXTERNAL),
                                  GUI_SML, gui_red, gui_red);
                }
                else
                    gui_multi(jd, _("Created game transfer data."),
                                  GUI_SML, gui_wht, gui_wht);
                break;

            case 6:
                gui_multi(jd, _("Game transfer preparations on this game\\"
                                "are now complete."),
                              GUI_SML, gui_wht, gui_wht);
                gui_multi(jd, _("Next, update and launch the source game,\\"
                                "and go to settings."),
                              GUI_SML, gui_cya, gui_cya);
                gui_multi(jd, _("If you do not have enough plug sockets, turn this PC off\\"
                                "in the meantime; you can continue the transfer afterwards\\"
                                "by launching the game again."),
                              GUI_SML, gui_yel, gui_yel);
                gui_multi(jd, _("If you only have one keyboard and mouse,\\"
                                "first turn it off from this PC\\"
                                "before connecting to the source PC."),
                              GUI_SML, gui_wht, gui_wht);
                break;
            }
            gui_set_rect(jd, GUI_ALL);
        }

        if (!preparations_working)
        {
            gui_space(id);

            if ((jd = gui_harray(id)))
            {
                if (preparations_pageindx < 3)
                {
                    gui_start(jd, _("Yes"), GUI_SML, GUI_NEXT, 0);
                    gui_state(jd, _("No"), GUI_SML, GUI_PREV, 0);
                }
                else if (preparations_pageindx == 3)
                {
                    gui_start(jd, _("Next"), GUI_SML, GUI_NEXT, 0);
                    gui_state(jd, _("Quit"), GUI_SML, GUI_BACK, 0);
                }
                else if (preparations_pageindx != 6)
                    gui_start(jd, _("Next"), GUI_SML, GUI_NEXT, 0);
            }
        }
    }

    gui_layout(id, 0, 0);

    return id;
}
#pragma endregion

#pragma region complete transfer (target)
static int transfer_process_have_wallet  = 0;
static int transfer_process_have_account = 0;

/*
 * insert the external drive (after startup and connect to the internet and remove from external drive after preparations)
 * move data from external drive (after loaded from external drive's source game)
 * transfer of wallet complete, profile transfer complete and highscore transfer completed (after deleted game transfer data from local pc)
 */
static int transfer_gui(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
#if ENABLE_DEDICATED_SERVER==1 && !defined(TRANSFER_OFFLINE_ONLY)
        if (preparations_internet)
            gui_label(id, _("Connecting to the Internet."), GUI_SML, 0, 0);
        else
#endif
        {
            if (!transfer_working)
            {
                if (transfer_pageindx == 1)
                    gui_label(id, _("Insert the External Drive"),
                                  GUI_SML, 0, 0);
                else if (transfer_pageindx == 2)
                    gui_label(id, _("Move data from the External Drive"),
                                  GUI_SML, 0, 0);
                else if (transfer_pageindx == 4)
                    gui_label(id, _("Transfer of wallet complete"),
                                  GUI_SML, 0, 0);
                else if (transfer_pageindx == 5)
                    gui_label(id, _("Account transfer complete"),
                                  GUI_SML, 0, 0);
                else
                    gui_label(id, _(TITLE_TARGET_TRANSFER),
                                  GUI_SML, 0, 0);
            }

            gui_space(id);
        }

        char wallet_infotext[MAXSTR];

        if ((jd = gui_vstack(id)))
        {
            switch (transfer_pageindx)
            {
            case 0:
#if ENABLE_DEDICATED_SERVER==1 && !defined(TRANSFER_OFFLINE_ONLY)
                if (preparations_internet)
                    gui_multi(jd, _("Connecting to the Internet to confirm that\\"
                                    "the transfer can be performed..."),
                                  GUI_SML, gui_wht, gui_wht);
                else
#endif
                    gui_multi(jd, _("Game transfer is not completed.\\"
                                    "Continue the transfer?"),
                                  GUI_SML, gui_wht, gui_wht);
                break;
            case 1:
                if (transfer_working)
                {
                    gui_multi(jd, _("Loading data from the External Drive..."),
                                  GUI_SML, gui_wht, gui_wht);
                    gui_multi(jd, _(TARGET_TRANSFER_WARNING_EXTERNAL),
                                  GUI_SML, gui_red, gui_red);
                }
                else
                    gui_multi(jd, _("Insert the External Drive onto which you have\\"
                                    "moved the data from the source game\\"
                                    "into this game, and then press Next."),
                                  GUI_SML, gui_wht, gui_wht);
                break;

            case 2:
                if (transfer_working)
                {
                    gui_multi(jd, _("Moving data from the External Drive\\"
                                    "to this game progress..."),
                                  GUI_SML, gui_wht, gui_wht);
                    gui_multi(jd, _(TARGET_TRANSFER_WARNING_EXTERNAL),
                                  GUI_SML, gui_red, gui_red);
                }
                else
                {
                    gui_multi(jd, _("The data that you transferred to the\\"
                                    "External Drive from the source game\\"
                                    "will be saved to this game progress."),
                                  GUI_SML, gui_wht, gui_wht);
                    gui_multi(jd, _("(This data will be deleted from the\\"
                                    "External Drive.)"),
                                  GUI_SML, gui_wht, gui_wht);
                    gui_multi(jd, _(TARGET_TRANSFER_WARNING_EXTERNAL),
                                  GUI_SML, gui_red, gui_red);
                }
                break;

            case 3:
                if (transfer_working)
                {
                    gui_multi(jd, _("Deleting game transfer data from the PC..."),
                                  GUI_SML, gui_wht, gui_wht);
                    gui_multi(jd, _(TARGET_TRANSFER_WARNING),
                                  GUI_SML, gui_red, gui_red);
                }
                else
                    gui_multi(jd, _("Deleted game transfer data."),
                                  GUI_SML, gui_wht, gui_wht);
                break;

            case 4:
                if (transfer_walletamount[0] > 0 && transfer_walletamount[1] > 0)
                {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                    sprintf_s(wallet_infotext, MAXSTR,
#else
                    sprintf(wallet_infotext,
#endif
                            _("Transfer of %d coins and %d gems complete."),
                            transfer_walletamount[0], transfer_walletamount[1]);
                    gui_label(jd, wallet_infotext, GUI_SML, gui_wht, gui_wht);
                    gui_multi(jd, _("You can use these coins and gems in\\"
                                    "the game shop on this game."),
                                  GUI_SML, gui_wht, gui_wht);
                }
                else if (transfer_walletamount[1] > 0)
                {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                    sprintf_s(wallet_infotext, MAXSTR,
#else
                    sprintf(wallet_infotext,
#endif
                            _("Transfer of %d gems complete."),
                            transfer_walletamount[1]);
                    gui_label(jd, wallet_infotext, GUI_SML, gui_wht, gui_wht);
                    gui_multi(jd, _("You can use these gems in the\\"
                                    "game shop on this PC."),
                                  GUI_SML, gui_wht, gui_wht);
                }
                else if (transfer_walletamount[0] > 0)
                {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                    sprintf_s(wallet_infotext, MAXSTR,
#else
                    sprintf(wallet_infotext,
#endif
                            _("Transfer of %d coins complete."),
                            transfer_walletamount[0]);
                    gui_label(jd, wallet_infotext, GUI_SML, gui_wht, gui_wht);
                    gui_multi(jd, _("You can use these coins in the\\"
                                    "game shop on this PC."),
                                  GUI_SML, gui_wht, gui_wht);
                }
                break;

            case 5:
                gui_multi(jd, _("If you want to use the transferred account on\\"
                                "this game, choose the Ball Model within account settings."),
                              GUI_SML, gui_wht, gui_wht);
                break;
            case 6:
                gui_multi(jd, _("Game transfer complete.\\"
                                "You can now enjoy your game!"),
                              GUI_SML, gui_wht, gui_wht);
                break;
            }

            gui_set_rect(jd, GUI_ALL);
        }

        if (!preparations_working && !transfer_working)
        {
            gui_space(id);

            if ((jd = gui_harray(id)))
            {
                if (transfer_pageindx < 1)
                {
                    gui_start(jd, _("Yes"), GUI_SML, GUI_NEXT, 0);
                    gui_state(jd, _("No"), GUI_SML, GUI_PREV, 0);
                }
                else if (transfer_pageindx == 1)
                {
                    gui_start(jd, _("Next"), GUI_SML, GUI_NEXT, 0);
                    gui_state(jd, _("Quit"), GUI_SML, GUI_BACK, 0);
                }
                else if (transfer_pageindx == 2)
                    gui_start(jd, _("Transfer"), GUI_SML, GUI_NEXT, 0);
                else if (transfer_pageindx == 6)
                    gui_start(jd, _("OK"), GUI_SML, GUI_BACK, 0);
                else
                    gui_start(jd, _("Next"), GUI_SML, GUI_NEXT, 0);
            }
        }
    }

    gui_layout(id, 0, 0);

    return id;
}
#pragma endregion

/*---------------------------------------------------------------------------*/

static int transfer_action(int tok, int val)
{
    audio_play(GUI_PREV == tok || GUI_BACK == tok ? AUD_BACK : AUD_MENU, 1.0f);

    if (tok == GUI_BACK)
    {
        have_entered = 0;
        transfer_process_have_wallet = 0;
        transfer_process_have_account = 0;
        return goto_state(transfer_back);
    }

    switch (tok)
    {
    case GUI_NEXT:
        if (show_about && (about_pageindx == 12 || val))
        {
            show_about = 0;
            show_preparations = 1;
            preparations_pageindx = 1;
            goto_state(curr_state());
        }
        else
        {
            if (show_about && !show_transfer)
            {
                about_pageindx++;
#if ENABLE_DEDICATED_SERVER==0 || defined(TRANSFER_OFFLINE_ONLY)
                // Must connect to the internet first, if TRANSFER_OFFLINE_ONLY is undefined
                //if (about_pageindx == 2) about_pageindx = 3;
#endif
                goto_state(curr_state());
            }
            else if (!show_about && !show_transfer)
            {
#if ENABLE_DEDICATED_SERVER==1 && !defined(TRANSFER_OFFLINE_ONLY)
                if (preparations_pageindx == 2)
                {
                    transfer_ui_transition_busy = 1;
                    preparations_internet = 1;
                }
                else
#endif
                if (preparations_pageindx == 3 ||
                    preparations_pageindx == 4 ||
                    preparations_pageindx == 5
                    )
                {
                    transfer_ui_transition_busy = 1;
                    preparations_working = 1;
                    return goto_state(&st_transfer);
                }

                preparations_pageindx++;
#if ENABLE_DEDICATED_SERVER==0 || defined(TRANSFER_OFFLINE_ONLY)
                if (preparations_pageindx == 2) preparations_pageindx = 3;
#endif
                goto_state(curr_state());
            }
            else if (!show_about && show_transfer)
            {
#if ENABLE_DEDICATED_SERVER==1 && !defined(TRANSFER_OFFLINE_ONLY)
                if (preparations_pageindx == 2)
                {
                    transfer_ui_transition_busy = 1;
                    preparations_internet = 1;
                }
                else
#endif
                if (transfer_pageindx == 1 || transfer_pageindx == 2)
                {
                    transfer_ui_transition_busy = 1;
                    transfer_process = 1;
                    transfer_working = 1;
                    return goto_state(&st_transfer);
                }

                transfer_pageindx++;

                if (transfer_pageindx == 4)
                {
                    if (!(transfer_walletamount[0] > 0 || transfer_walletamount[1] > 0))
                        transfer_pageindx++;
                }
                goto_state(curr_state());
            }
        }
        break;
    case GUI_PREV:
        if (show_about && !show_transfer)
        {
            about_pageindx--;
#if ENABLE_DEDICATED_SERVER==0 || defined(TRANSFER_OFFLINE_ONLY)
            if (about_pageindx == 2) about_pageindx = 1;
#endif
            exit_state(curr_state());
        }
        else if (!show_about && !show_transfer)
        {
            if (preparations_pageindx == 0)
            {
                have_entered = 0;
                transfer_process_have_wallet = 0;
                transfer_process_have_account = 0;
                return exit_state(transfer_back);
            }
            preparations_pageindx--;
#if ENABLE_DEDICATED_SERVER==0 || defined(TRANSFER_OFFLINE_ONLY)
            if (preparations_pageindx == 2) preparations_pageindx = 1;
#endif
            exit_state(curr_state());
        }
        else if (!show_about && show_transfer)
        {
            if (transfer_pageindx == 0)
            {
                have_entered = 0;
                transfer_process_have_wallet = 0;
                transfer_process_have_account = 0;
                return goto_state(transfer_back);
            }

            transfer_pageindx--;
            exit_state(curr_state());
        }
        break;
    }
    return 1;
}

static const char *pick_home_path(void)
{
#ifdef _WIN32
    size_t requiredSize = MAX_PATH;
    static char path[MAX_PATH];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    char userdir_env[MAX_PATH];
    memset(userdir_env, 0, sizeof (userdir_env));
    getenv_s(&requiredSize, userdir_env, requiredSize, "NEVERBALL_USERDIR");
    if (userdir_env)
        return userdir_env;
#else
    char *userdir_env;
    if ((userdir_env = getenv("NEVERBALL_USERDIR")))
        return userdir_env;
#endif

    if (SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, path) == S_OK)
    {
        static char gamepath[MAX_PATH];

        SAFECPY(gamepath, path);
        SAFECAT(gamepath, "\\My Games");

        if (dir_exists(gamepath) || dir_make(gamepath) == 0)
            return gamepath;

        return path;
    }
    else
        return fs_base_dir();
#else
    char *userdir_env;
    if ((userdir_env = getenv("NEVERBALL_USERDIR")))
        return userdir_env;

    const char *path;

    return (path = getenv("HOME")) ? path : fs_base_dir();
#endif
}

static int transfer_error_code;

const char drive_letters[23][2] = {
    "E",
    "F",
    "H",
    "I",
    "J",
    "K",
    "L",
    "M",
    "N",
    "O",
    "P",
    "Q",
    "R",
    "S",
    "T",
    "U",
    "V",
    "W",
    "X",
    "Y",
    "Z",
    "A",
    "B"
};

static int current_drive_idx = -1;

static void transfer_timer_preparation_target(float dt)
{
    fs_file internal_file_v2;
    FILE *internal_file, *transfer_file, *replayfilter_file;

    __int64 lpFreeBytesAvailable, lpTotalNumberOfBytes, lpTotalNumberOfFreeBytes;
    int ext_drive_connected, ext_drive_supported;

    switch (preparations_pageindx)
    {
    case 3:
        current_drive_idx = -1;
        transfer_error_code = 2;

        for (int i = 0; i < 24 && current_drive_idx == -1;)
        {
            /* Skip scanning, if we have at least 1 GB External Storage */
            if (current_drive_idx != -1 || i < 0)
            {
                ++i;
                continue;
            }

#if _WIN32
            ext_drive_connected = GetDiskFreeSpaceExA(concat_string(drive_letters[i], ":", 0),
                                                      (PULARGE_INTEGER) &lpFreeBytesAvailable,
                                                      (PULARGE_INTEGER) &lpTotalNumberOfBytes,
                                                      (PULARGE_INTEGER) &lpTotalNumberOfFreeBytes);

            ext_drive_supported = GetDriveTypeA(concat_string(drive_letters[i], ":\\", 0)) == DRIVE_REMOVEABLE;
#endif
            if (ext_drive_connected == 1 &&
                ext_drive_supported &&
                lpFreeBytesAvailable >= (__int64)(1000000000))
            {
                if (dir_make(concat_string(drive_letters[i], ":/nvb_gametransfer", 0)) ||
                    GetLastError() == ERROR_ALREADY_EXISTS)
                {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                    fopen_s(&internal_file, concat_string(drive_letters[i],
                                                          ":/nvb_gametransfer/fortarget", 0),
                            "w+t");
#else
                    internal_file = fopen(concat_string(drive_letters[i],
                                                        ":/nvb_gametransfer/fortarget", 0),
                                          "w+t");
#endif
                    char deviceName[MAXSTR];
                    unsigned long deviceNameCount;
#if _WIN32
                    int searches_hostname = 1;
                    while (searches_hostname)
                    {
                        if (GetComputerNameA(deviceName, &deviceNameCount))
                        {
                            fwrite(deviceName, 1,
                                   strlen(deviceName), internal_file);
                            searches_hostname = 0;
                        }
                    }
#endif
                    fclose(internal_file);
                    transfer_error_code = 0;

                    current_drive_idx = i;

                    transfer_ui_transition_busy = 1;
                    preparations_working = 0;
                    preparations_pageindx = 4;
                    goto_state(curr_state());
                    return;
                }
            }
            else
            {
                if (ext_drive_connected)
                {
                    if (!ext_drive_supported)
                    {
                        log_errorf("For disk %s:, only removable drive is allowed!\n",
                                   drive_letters[i]);
                        transfer_error_code = 2;
                    }
                    else if (lpFreeBytesAvailable < (__int64)(1000000000))
                    {
                        log_errorf("Not enough external drive space for %s:!\n",
                                   drive_letters[i]);
                        transfer_error_code = 12;
                    }
                }
                else
                {
                    log_errorf("No external drive in %s:\n",
                               drive_letters[i]);
                    if (transfer_error_code < 2) transfer_error_code = 2;
                }
            }

            i++;
        }

        if (current_drive_idx == -1)
        {
            if (transfer_error_code == 12)
                log_errorf("Failure to verify! External Drive requires 1 GB of free space!: %s\n",
                           strerror(transfer_error_code));
            else if (transfer_error_code == 2)
                log_errorf("Failure to verify! External Drive must connect to the computer!: %s\n",
                           strerror(transfer_error_code));

            transfer_ui_transition_busy = 1;
            preparations_working = 0;
            preparations_pageindx = 3;
            exit_state(curr_state());
        }
        break;
    case 4:
        fs_remove("nvb_gametransfer");
        internal_file_v2 = fs_open_write("nvb_gametransfer");

        if (internal_file_v2)
        {
            fs_write(":/nvb_gametransfer", strlen(":/nvb_gametransfer"),
                     internal_file_v2);
            fs_close(internal_file_v2);

            transfer_ui_transition_busy = 1;
            preparations_working = 0;
            preparations_pageindx = 5;
            goto_state(curr_state());
            return;
        }

        transfer_ui_transition_busy = 1;
        preparations_working = 0;
        preparations_pageindx = 3;
        goto_state(curr_state());
        break;
    case 5:
        dir_make(concat_string(pick_home_path(),
                               "\\", CONFIG_USER,
                               "\\Accounts_transfer", NULL));

        WIN32_FIND_DATAA ffd;
        char *outUserDir = strdup(concat_string(pick_home_path(),
                                  "\\", CONFIG_USER,
                                  "\\Accounts\\*", NULL));
        HANDLE hFind = FindFirstFileA(outUserDir, &ffd);

        const char *backup_playername = config_get_s(CONFIG_PLAYER);

        if (INVALID_HANDLE_VALUE != hFind)
        {
            dir_make(concat_string(drive_letters[current_drive_idx],
                                   ":/nvb_gametransfer/campaign", 0));
            dir_make(concat_string(drive_letters[current_drive_idx],
                                   ":/nvb_gametransfer/replays", 0));
            dir_make(concat_string(drive_letters[current_drive_idx],
                                   ":/nvb_gametransfer/scores", 0));
            dir_make(concat_string(drive_letters[current_drive_idx],
                                   ":/nvb_gametransfer/screenshots", 0));
            dir_make(concat_string(drive_letters[current_drive_idx],
                                   ":/nvb_gametransfer/accounts", 0));

            do
            {
                if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    /* Is directory, skipped */
                }
                else if (strcmp(ffd.cFileName, ".") != 0 ||
                         strcmp(ffd.cFileName, "..") != 0)
                {
                    account_transfer_init();

                    account_transfer_load(concat_string("Accounts/",
                                                        ffd.cFileName, NULL));
                    account_transfer_save(ffd.cFileName);

                    char copy_cmd[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                    sprintf_s(copy_cmd, "copy /y \"%s\" \"%s\"",
                        concat_string(pick_home_path(), "\\", CONFIG_USER,
                                      "\\Accounts_transfer\\", ffd.cFileName, NULL),
                        concat_string(drive_letters[current_drive_idx],
                                      ":/nvb_gametransfer/accounts", 0)
                    );
#elif _WIN32 && !defined(__EMSCRIPTEN__) && _CRT_SECURE_NO_WARNINGS
                    sprintf(copy_cmd, "copy /y \"%s\" \"%s\"",
                        concat_string(pick_home_path(), "\\", CONFIG_USER,
                                      "\\Accounts_transfer\\", ffd.cFileName, NULL),
                        concat_string(drive_letters[current_drive_idx],
                                      ":/nvb_gametransfer/accounts", 0)
                    );
#else
                    sprintf(copy_cmd, "copy /y \"%s\" \"%s\"",
                        concat_string(pick_home_path(), "/", CONFIG_USER,
                                      "/Accounts_transfer/", ffd.cFileName, NULL),
                        concat_string(drive_letters[current_drive_idx],
                                      ":/nvb_gametransfer/accounts", 0)
                    );
#endif
                    system(copy_cmd);
                }
            } while (FindNextFileA(hFind, &ffd) != 0);

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            fopen_s(&replayfilter_file, concat_string(drive_letters[current_drive_idx],
                                                      ":/nvb_gametransfer/replayfilter.nbtransfer", 0),
                    "w+t");
#else
            replayfilter_file = fopen(concat_string(drive_letters[current_drive_idx],
                                                    ":/nvb_gametransfer/replayfilter.nbtransfer", 0),
                                      "w+t");
#endif

            if (replayfilter_file)
            {
                int replay_loadfilter_transfer = config_get_d(CONFIG_ACCOUNT_LOAD);

                switch (replay_loadfilter_transfer)
                {
                case 3:
                    fwrite("restricted-replays: covid\n", 1,
                           strlen("restricted-replays: covid\n"), replayfilter_file);
                    break;
                case 2:
                    fwrite("restricted-replays: keep\n", 1,
                           strlen("restricted-replays: keep\n"), replayfilter_file);
                    break;
                case 1:
                    fwrite("restricted-replays: goal\n", 1,
                           strlen("restricted-replays: goal\n"), replayfilter_file);
                    break;
                }

                fclose(replayfilter_file);
                transfer_error_code = 0;

                transfer_ui_transition_busy = 1;
                preparations_working = 0;
                preparations_pageindx = 6;
                goto_state(curr_state());

                return;
            }
        }

        transfer_error_code = 2;

        if (transfer_error_code == 2)
            log_errorf("Failure to write! External Drive must connect to the computer!: %s\n",
                       strerror(transfer_error_code));

        transfer_ui_transition_busy = 1;
        preparations_working = 0;
        preparations_pageindx = 3;
        exit_state(curr_state());
        break;
    }
}

static float transfer_alpha;

static void transfer_timer_preprocess_target(float dt)
{
    if (transfer_ui_transition_busy || transfer_alpha < 0.99f) return;

    FILE *external_file;
    __int64 lpFreeBytesAvailable, lpTotalNumberOfBytes, lpTotalNumberOfFreeBytes;
    int ext_drive_connected, ext_drive_supported;

    current_drive_idx = -1;

    for (int i = -1; i < 24 && current_drive_idx == -1;)
    {
        /* Skip scanning, if we have moved from the source game */
        if (current_drive_idx != -1 || i < 0)
        {
            ++i;
            continue;
        }

#if _WIN32
        ext_drive_connected = GetDiskFreeSpaceExA(concat_string(drive_letters[i], ":", 0),
                                                  (PULARGE_INTEGER) &lpFreeBytesAvailable,
                                                  (PULARGE_INTEGER) &lpTotalNumberOfBytes,
                                                  (PULARGE_INTEGER) &lpTotalNumberOfFreeBytes);

        ext_drive_supported = GetDriveTypeA(concat_string(drive_letters[i], ":\\", 0)) == DRIVE_REMOVEABLE;
#endif
        if (ext_drive_connected && ext_drive_supported)
        {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            fopen_s(&external_file, concat_string(drive_letters[i],
                                                  ":/nvb_gametransfer/fromsource", 0),
                    "r+t");
#else
            external_file = fopen(concat_string(drive_letters[i],
                                                ":/nvb_gametransfer/fromsource", 0),
                                  "r+t");
#endif

            if (external_file)
            {
                fclose(external_file);

                current_drive_idx = i;
                transfer_working = 0;
                transfer_ui_transition_busy = 1;
                transfer_pageindx = 2;
                goto_state(curr_state());
                return;
            }

            transfer_error_code = 2;
        }
        else
        {
            log_errorf("No external drive in %s:\n", drive_letters[i]);
            transfer_error_code = 2;

            transfer_working = 0;
            transfer_ui_transition_busy = 1;
            transfer_pageindx = 1;
        }

        i++;
    }

    if (current_drive_idx == -1)
    {
        log_errorf("Failure to loading data! External Drive requires that you've transferred from source game, which you've connected to the computer!: %s\n",
                   strerror(transfer_error_code));

        transfer_working = 0;
        transfer_ui_transition_busy = 0;
        transfer_pageindx = 1;
        exit_state(curr_state());
    }
}

static int ext_read_line(char **dst, FILE *fin)
{
    char buff[MAXSTR];

    char *line, * newLine;
    size_t len0, len1;

    line = NULL;

    while (fgets(buff, sizeof (buff), fin))
    {
        /* Append to data read so far. */

        if (line)
        {
            newLine = concat_string(line, buff, NULL);
            free(line);
            line = newLine;
        }
        else
        {
            line = strdup(buff);
        }

        /* Strip newline, if any. */

        len0 = strlen(line);
        strip_newline(line);
        len1 = strlen(line);

        if (len1 != len0)
        {
            /* We hit a newline, clean up and break. */
            line = (char *) realloc(line, len1 + 1);
            break;
        }
    }

    return (*dst = line) ? 1 : 0;
}

struct account_transfer_value
{
    int wallet_coins;
    int wallet_gems;
    int product_levels;
    int product_balls;
    int product_bonus;
    int product_mediation;
    int set_unlocks;
    int consumeable_earninator;
    int consumeable_floatifier;
    int consumeable_speedifier;
    int consumeable_extralives;
    char player[MAXSTR];
    char ball_file[MAXSTR];
};

static struct account_transfer_value account_transfer_values_source, account_transfer_values_target;

static void transfer_timer_process_target(float dt)
{
    char src_dirpath_account[MAXSTR];
    const char *src_dirpath_campaign;
    const char *src_dirpath_demo;
    const char *src_dirpath_hs;
    const char *src_dirpath_screenshots;

    char target_dirpath_account[MAXSTR];
    char target_dirpath_campaign[MAXSTR];
    char target_dirpath_demo[MAXSTR];
    char target_dirpath_hs[MAXSTR];
    char target_dirpath_screenshots[MAXSTR];

    assert(current_drive_idx > -1 && drive_letters[current_drive_idx]);

    if (!(current_drive_idx > -1 && drive_letters[current_drive_idx]))
    {
        transfer_ui_transition_busy = 1;
        preparations_working = 0;
        preparations_pageindx = 3;
        exit_state(&st_transfer);
    }

#if _WIN32
    src_dirpath_campaign    = concat_string(drive_letters[current_drive_idx],
                                            ":\\nvb_gametransfer\\campaign\\*.txt", 0);
    src_dirpath_demo        = concat_string(drive_letters[current_drive_idx],
                                            ":\\nvb_gametransfer/replays\\*.nbr", 0);
    src_dirpath_hs          = concat_string(drive_letters[current_drive_idx],
                                            ":\\nvb_gametransfer/scores\\*.txt", 0);
    src_dirpath_screenshots = concat_string(drive_letters[current_drive_idx],
                                            ":\\nvb_gametransfer\\screenshots\\*.png", 0);
#else
    src_dirpath_campaign    = concat_string(drive_letters[current_drive_idx],
                                            ":/nvb_gametransfer/campaign/*.txt", 0);
    src_dirpath_demo        = concat_string(drive_letters[current_drive_idx],
                                            ":/nvb_gametransfer/replays/*.nbr", 0);
    src_dirpath_hs          = concat_string(drive_letters[current_drive_idx],
                                            ":/nvb_gametransfer/scores/*.txt", 0);
    src_dirpath_screenshots = concat_string(drive_letters[current_drive_idx],
                                            ":/nvb_gametransfer/screenshots/*.png", 0);
#endif

    const char *homepath = pick_home_path();

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(target_dirpath_campaign, "%s\\%s\\Campaign", homepath, CONFIG_USER);
    sprintf_s(target_dirpath_demo, "%s\\%s\\Replays", homepath, CONFIG_USER);
    sprintf_s(target_dirpath_hs, "%s\\%s\\Scores", homepath, CONFIG_USER);
    sprintf_s(target_dirpath_screenshots, "%s\\%s\\Screenshots", homepath, CONFIG_USER);
#elif _WIN32
    sprintf(target_dirpath_campaign, "%s\\%s\\Campaign", homepath, CONFIG_USER);
    sprintf(target_dirpath_demo, "%s\\%s\\Replays", homepath, CONFIG_USER);
    sprintf(target_dirpath_hs, "%s\\%s\\Scores", homepath, CONFIG_USER);
    sprintf(target_dirpath_screenshots, "%s\\%s\\Screenshots", homepath, CONFIG_USER);
#else
    sprintf(target_dirpath_campaign, "%s/%s/Campaign", homepath, CONFIG_USER);
    sprintf(target_dirpath_demo, "%s/%s/Replays", homepath, CONFIG_USER);
    sprintf(target_dirpath_hs, "%s/%s/Scores", homepath, CONFIG_USER);
    sprintf(target_dirpath_screenshots, "%s/%s/Screenshots", homepath, CONFIG_USER);
#endif

    if (transfer_pageindx == 2 && transfer_process == 1 && !transfer_ui_transition_busy && transfer_alpha > 0.99f)
    {
        transfer_process = 2;

        FILE *csv_wallet_output;

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        fopen_s(&csv_wallet_output, concat_string(drive_letters[current_drive_idx],
                                                  ":/nvb_gametransfer/account.nbwallet", 0),
                "r+t");
#else
        csv_wallet_output = fopen(concat_string(drive_letters[current_drive_idx],
                                                ":/nvb_gametransfer/account.nbwallet", 0),
                                  "r+t");
#endif

        if (csv_wallet_output)
        {
            transfer_process_have_account = 1;

            char *ext_line;
            while (ext_read_line(&ext_line, csv_wallet_output))
            {
                account_transfer_init();
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sscanf_s(ext_line,
#else
                sscanf(ext_line,
#endif
                       "coins:%d ;gems:%d ;p_levels:%d ;p_balls:%d ;p_bonus:%d ;p_mediation:%d ;set_unlocks:%d ;c_earninator:%d ;c_floatifier:%d ;c_speedifier:%d ;c_extralives:%d ;ballfile: %s ;player: %s ;",
                       &account_transfer_values_source.wallet_coins,
                       &account_transfer_values_source.wallet_gems,
                       &account_transfer_values_source.product_levels,
                       &account_transfer_values_source.product_balls,
                       &account_transfer_values_source.product_bonus,
                       &account_transfer_values_source.product_mediation,
                       &account_transfer_values_source.set_unlocks,
                       &account_transfer_values_source.consumeable_earninator,
                       &account_transfer_values_source.consumeable_floatifier,
                       &account_transfer_values_source.consumeable_speedifier,
                       &account_transfer_values_source.consumeable_extralives,
                       account_transfer_values_source.ball_file,
                       account_transfer_values_source.player);

                account_transfer_load(concat_string("Accounts/account-", account_transfer_values_source.player, ".nbaccount", NULL));

                transfer_walletamount[0] += account_transfer_values_source.wallet_coins;
                transfer_walletamount[1] += account_transfer_values_source.wallet_gems;

                if (transfer_walletamount[0] > 0 || transfer_walletamount[1] > 0)
                    transfer_process_have_wallet = 1;

                account_transfer_set_s(ACCOUNT_TRANSFER_PLAYER, account_transfer_values_source.player);
#if defined(CONFIG_INCLUDES_MULTIBALLS)
                account_transfer_set_s(ACCOUNT_TRANSFER_BALL_FILE_C, account_transfer_values_source.ball_file);
#else
                account_transfer_set_s(ACCOUNT_TRANSFER_BALL_FILE, account_transfer_values_source.ball_file);
#endif
                account_transfer_set_d(ACCOUNT_TRANSFER_DATA_WALLET_COINS, account_transfer_values_target.wallet_coins + account_transfer_values_source.wallet_coins);
                account_transfer_set_d(ACCOUNT_TRANSFER_DATA_WALLET_GEMS, account_transfer_values_target.wallet_gems + account_transfer_values_source.wallet_gems);

                if (!account_transfer_values_target.product_levels && account_transfer_values_source.product_levels)
                    account_transfer_set_d(ACCOUNT_TRANSFER_PRODUCT_LEVELS, account_transfer_values_source.product_levels);
                if (!account_transfer_values_target.product_balls && account_transfer_values_source.product_balls)
                    account_transfer_set_d(ACCOUNT_TRANSFER_PRODUCT_BALLS, account_transfer_values_source.product_balls);
                if (!account_transfer_values_target.product_bonus && account_transfer_values_source.product_bonus)
                    account_transfer_set_d(ACCOUNT_TRANSFER_PRODUCT_BONUS, account_transfer_values_source.product_bonus);
                if (!account_transfer_values_target.product_mediation && account_transfer_values_source.product_mediation)
                    account_transfer_set_d(ACCOUNT_TRANSFER_PRODUCT_MEDIATION, account_transfer_values_source.product_mediation);
                if (!account_transfer_values_target.set_unlocks && account_transfer_values_source.set_unlocks)
                    account_transfer_set_d(ACCOUNT_TRANSFER_SET_UNLOCKS, account_transfer_values_source.set_unlocks);

                account_transfer_set_d(ACCOUNT_TRANSFER_CONSUMEABLE_EARNINATOR, account_transfer_values_target.consumeable_earninator + account_transfer_values_source.consumeable_earninator);
                account_transfer_set_d(ACCOUNT_TRANSFER_CONSUMEABLE_FLOATIFIER, account_transfer_values_target.consumeable_floatifier + account_transfer_values_source.consumeable_floatifier);
                account_transfer_set_d(ACCOUNT_TRANSFER_CONSUMEABLE_SPEEDIFIER, account_transfer_values_target.consumeable_speedifier + account_transfer_values_source.consumeable_speedifier);
                account_transfer_set_d(ACCOUNT_TRANSFER_CONSUMEABLE_EXTRALIVES, account_transfer_values_target.consumeable_extralives + account_transfer_values_source.consumeable_extralives);
#if _WIN32
                SAFECPY(src_dirpath_account, homepath);
                SAFECAT(src_dirpath_account, "\\");
                SAFECAT(src_dirpath_account, CONFIG_USER);
                SAFECAT(src_dirpath_account, "\\Accounts_transfer\\account-");
                SAFECAT(src_dirpath_account, account_transfer_values_source.player);
#if ENABLE_ACCOUNT_BINARY
                SAFECAT(src_dirpath_account, ".nbaccount");
#else
                SAFECAT(src_dirpath_account, ".txt");
#endif

                SAFECPY(target_dirpath_account, homepath);
                SAFECAT(target_dirpath_account, "\\");
                SAFECAT(target_dirpath_account, CONFIG_USER);
                SAFECAT(target_dirpath_account, "\\Accounts");
#else
                SAFECPY(src_dirpath_account, homepath);
                SAFECAT(src_dirpath_account, "/");
                SAFECAT(src_dirpath_account, CONFIG_USER);
                SAFECAT(src_dirpath_account, "/Accounts_transfer/account-");
                SAFECAT(src_dirpath_account, account_transfer_values_source.player);
#if ENABLE_ACCOUNT_BINARY
                SAFECAT(src_dirpath_account, ".nbaccount");
#else
                SAFECAT(src_dirpath_account, ".txt");
#endif

                SAFECPY(target_dirpath_account, homepath);
                SAFECAT(target_dirpath_account, "/");
                SAFECAT(target_dirpath_account, CONFIG_USER);
                SAFECAT(target_dirpath_account, "/Accounts");
#endif
#if ENABLE_ACCOUNT_BINARY
                account_transfer_save(concat_string("account-", account_transfer_values_source.player, ".nbaccount", NULL));
#else
                account_transfer_save(concat_string("account-", account_transfer_values_source.player, ".txt", NULL));
#endif
                char movecmd_account[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(movecmd_account,
#else
                sprintf(movecmd_account,
#endif
                        "move /y \"%s\" \"%s\"", src_dirpath_account, target_dirpath_account);
                system(movecmd_account);
            }

            fclose(csv_wallet_output);
            remove(concat_string(drive_letters[current_drive_idx], ":/nvb_gametransfer/account.nbwallet", 0));
        }

        system(concat_string("del /f \"", drive_letters[current_drive_idx], ":/nvb_gametransfer/accounts/*\"", 0));

        remove(concat_string(drive_letters[current_drive_idx], ":/nvb_gametransfer/fromsource", 0));

        account_init();
        account_load();
#if NB_HAVE_PB_BOTH==1 && defined(CONFIG_INCLUDES_ACCOUNT) && defined(CONFIG_INCLUDES_MULTIBALLS)
        ball_multi_free();
        ball_multi_init();
#else
        ball_free();
        ball_init();
#endif

        char movecmd_campaign[MAXSTR],
             movecmd_demo[MAXSTR],
             movecmd_hs[MAXSTR],
             movecmd_screenshots[MAXSTR];

        log_printf("Campaign path from source to target: %s => %s\n", src_dirpath_campaign, target_dirpath_campaign);
        log_printf("Replay path from source to target: %s => %s\n", src_dirpath_demo, target_dirpath_demo);
        log_printf("Highscores path from source to target: %s => %s\n", src_dirpath_hs, target_dirpath_hs);
        log_printf("Screenshots path from source to target: %s => %s\n", src_dirpath_screenshots, target_dirpath_screenshots);

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(movecmd_campaign, "move /y \"%s\" \"%s\"", src_dirpath_campaign, target_dirpath_campaign);
        sprintf_s(movecmd_demo, "move /y \"%s\" \"%s\"", src_dirpath_demo, target_dirpath_demo);
        sprintf_s(movecmd_hs, "move /y \"%s\" \"%s\"", src_dirpath_hs, target_dirpath_hs);
        sprintf_s(movecmd_screenshots, "move /y \"%s\" \"%s\"", src_dirpath_screenshots, target_dirpath_screenshots);
#else
        sprintf(movecmd_campaign, "move /y \"%s\" \"%s\"", src_dirpath_campaign, target_dirpath_campaign);
        sprintf(movecmd_demo, "move /y \"%s\" \"%s\"", src_dirpath_demo, target_dirpath_demo);
        sprintf(movecmd_hs, "move /y \"%s\" \"%s\"", src_dirpath_hs, target_dirpath_hs);
        sprintf(movecmd_screenshots, "move /y \"%s\" \"%s\"", src_dirpath_screenshots, target_dirpath_screenshots);
#endif
        system(movecmd_campaign);
        system(concat_string("del /f \"", drive_letters[current_drive_idx], ":/nvb_gametransfer/campaign/*\"", 0));
        RemoveDirectoryA(concat_string(drive_letters[current_drive_idx], ":/nvb_gametransfer/campaign", 0));
        Sleep(1000);
        system(movecmd_demo);
        system(concat_string("del /f \"", drive_letters[current_drive_idx], ":/nvb_gametransfer/replays/*\"", 0));
        RemoveDirectoryA(concat_string(drive_letters[current_drive_idx], ":/nvb_gametransfer/replays", 0));
        Sleep(1000);
        system(movecmd_hs);
        system(concat_string("del /f \"", drive_letters[current_drive_idx], ":/nvb_gametransfer/scores/*\"", 0));
        RemoveDirectoryA(concat_string(drive_letters[current_drive_idx], ":/nvb_gametransfer/scores", 0));
        Sleep(1000);
        system(movecmd_screenshots);
        system(concat_string("del /f \"", drive_letters[current_drive_idx], ":/nvb_gametransfer/screenshots/*\"", 0));
        RemoveDirectoryA(concat_string(drive_letters[current_drive_idx], ":/nvb_gametransfer/screenshots", 0));
        Sleep(1000);

        remove(concat_string(drive_letters[current_drive_idx], ":/nvb_gametransfer/replayfilter.nbtransfer", 0));

        transfer_pageindx = 3;

        goto_state(&st_transfer);
    }
}

/*---------------------------------------------------------------------------*/

static int transfer_enter_target(struct state *st, struct state *prev)
{
    if (!have_entered)
    {
        audio_music_fade_to(0.5f, "bgm/systemtransfer.ogg", 1);
        have_entered = 1;

#if ENABLE_DEDICATED_SERVER==1 && !defined(TRANSFER_OFFLINE_ONLY)
        preparations_internet = 0;
#endif

        show_about = 1;
        show_preparations = 0;
        show_transfer = 0;
        about_pageindx = 0;
        preparations_pageindx = 0;
        transfer_pageindx = 0;

        transfer_walletamount[0] = 0;
        transfer_walletamount[1] = 0;

        current_drive_idx = -1;

        fs_file internal_file_v2;
        internal_file_v2 = fs_open_read("nvb_gametransfer");

        if (internal_file_v2)
        {
            fs_close(internal_file_v2);
            show_about = 0;
            show_transfer = 1;
            about_pageindx = 1;
        }
    }

    back_init("back/gui_transfer.png");

    if (show_about && !show_transfer)
    {
        if (about_pageindx == 0)
            return transition_slide(transfer_introducory_gui(), 1, intent);
        else if (about_pageindx == 12)
            return transition_slide(transfer_starting_gui(), 1, intent);
        else
            return transition_slide(transfer_about_transferring_gui(), 1, intent);
    }
    else if (!show_about && !show_transfer)
    {
        if (!show_preparations && preparations_pageindx == 0)
            return transition_slide(transfer_starting_gui(), 1, intent);
        else
            return transition_slide(transfer_preparing_gui(), 1, intent);
    }
    else if (!show_about && show_transfer)
        return transition_slide(transfer_gui(), 1, intent);
    else assert(0 && "Unknown state!");

    return 0;
}

static int transfer_leave(struct state *st, struct state *next, int id, int intent)
{
    conf_common_leave(st, next, id);
    transfer_ui_transition_busy = 0;

    return transition_slide(id, 0, intent);
}

/*---------------------------------------------------------------------------*/

static void transfer_timer(int id, float dt)
{
    gui_timer(id, dt);

    if (preparations_pageindx == 6 && show_preparations && time_state() > 30)
    {
        show_preparations = 0;
        preparations_pageindx = 0;
        transfer_pageindx = 1;
        show_transfer = 1;
        goto_state(curr_state());
    }

    if (!transfer_ui_transition_busy && transfer_alpha > 0.99f)
    {
        if (!show_about && show_preparations &&
            !show_transfer && preparations_working)
            transfer_timer_preparation_target(dt);
        else if (!show_about && !show_preparations &&
                 show_transfer && transfer_working)
        {
            switch (transfer_pageindx)
            {
            case 1:
                transfer_timer_preprocess_target(dt);
                break;
            case 2:
                transfer_timer_process_target(dt);
                break;
            case 3:
                fs_remove("nvb_gametransfer");
                //remove(concat_string(drive_letters[current_drive_idx], ":/nvb_gametransfer/replayfilter.nbtransfer", 0));
                //remove(concat_string(drive_letters[current_drive_idx], ":/nvb_gametransfer/account_transfer.csv", 0));
                //remove(concat_string(drive_letters[current_drive_idx], ":/nvb_gametransfer/fortarget", 0));
                //remove(concat_string(drive_letters[current_drive_idx], ":/nvb_gametransfer/fromsource", 0));

                //RemoveDirectoryA(concat_string(drive_letters[current_drive_idx], ":/nvb_gametransfer/campaign", 0));
                //RemoveDirectoryA(concat_string(drive_letters[current_drive_idx], ":/nvb_gametransfer/replays", 0));
                //RemoveDirectoryA(concat_string(drive_letters[current_drive_idx], ":/nvb_gametransfer/scores", 0));
                //RemoveDirectoryA(concat_string(drive_letters[current_drive_idx], ":/nvb_gametransfer/screenshots", 0));

                transfer_process = 0;
                transfer_working = 0;
                goto_state(curr_state());
                break;
            }
        }
    }
}

static void transfer_paint(int id, float t)
{
    video_set_perspective((float) config_get_d(CONFIG_VIEW_FOV), 0.1f, FAR_DIST);
    back_draw_easy();

    gui_paint(id);
}

static int transfer_click(int c, int d)
{
    if (gui_click(c, d))
    {
        int active = gui_active();
        return transfer_action(gui_token(active), gui_value(active));
    }
    return 1;
}

static int transfer_buttn(int b, int d)
{
    if (d)
    {
        int active = gui_active();

        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return transfer_action(gui_token(active), gui_value(active));
    }
    return 1;
}

static void transfer_fade(float alpha)
{
    transfer_alpha = alpha;
}

struct state st_transfer = {
    transfer_enter_target,
    transfer_leave,
    transfer_paint,
    transfer_timer,
    common_point,
    common_stick,
    NULL,
    transfer_click,
    NULL,
    transfer_buttn,
    NULL,
    NULL,
    transfer_fade
};

#endif