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

#include "st_transfer.h"

#if !defined(GAME_TRANSFER_TARGET) && ENABLE_GAME_TRANSFER==1 && \
    !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
#if _WIN32
#include <ShlObj.h>

#define rmdir RemoveDirectoryA
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
#include "config.h"
#include "geom.h"
#include "gui.h"
#include "transition.h"
#include "lang.h"
#include "networking.h"

#include "account_transfer.h"

#include "st_common.h"

/*---------------------------------------------------------------------------*/

/**
 * We will transfer user datas from the multiple source Neverball game to ONLY
 * Pennyball game user datas.
 *
 * FIXME: Copy this function reference into the file ball/st_conf.c below:
 *
 * void demo_transfer_request_addreplay_dispatch_event(int status_limit)
 * {
 *     Array items = demo_dir_scan();
 *     int total = array_len(items);
 *     demo_dir_load(items, 0, total - 1);
 *
 *     for (int i = 0; i < total; i++)
 *     {
 *         struct demo *demo_data = (
 *             (struct demo *) ((struct dir_item*) array_get(items, i))->data
 *         );
 *
 *          if (!demo_data)
 *              continue;
 *
 *         int limit = config_get_d(CONFIG_ACCOUNT_LOAD);
 *         int max = 0;
 *
 *         if (demo_data->status == 3)
 *             max = 3;
 *         else if (demo_data->status == 1 || demo_data->status == 0)
 *             max = 2;
 *         else if (demo_data->status == 2)
 *             max = 1;
 *
 *         if (max <= limit)
 *             transfer_addreplay(demo_data->path);
 *         else
 *             transfer_addreplay_exceeded();
 *     }
 *
 *     demo_dir_free(items);
 * }
 *
 *
 * Function "int conf_action(int tok, int val)" for ball/st_conf.c:
 *
 *     #if ENABLE_GAME_TRANSFER==1
 *     #if !defined(GAME_TRANSFER_TARGET)
 *         case CONF_SYSTEMTRANSFER_TARGET:
 *     #else
 *         case CONF_SYSTEMTRANSFER_SOURCE:
 *         transfer_add_dispatch_event(demo_transfer_request_addreplay_dispatch_event);
 *     #endif
 *         goto_state(&st_transfer);
 *         break;
 *     #endif
 */

#define AUD_MENU "snd/menu.ogg"
#define AUD_BACK "snd/back.ogg"

#define TRANSFER_OFFLINE_ONLY
#define TITLE_SOURCE_TRANSFER \
    N_("Pennyball Transfer Tool")

#define SOURCE_TRANSFER_WARNING_DELETE_DATA \
    N_("Once deleted, this data cannot be restored.\n" \
       "Do you want to continue?")

#define SOURCE_TRANSFER_WARNING_EXTERNAL \
    N_("Do not touch the External Drive, exit the\n" \
       "game or turn off the PC, otherwise data\n" \
       "may be lost.")
#define SOURCE_TRANSFER_WARNING_INTERNAL \
    N_("Do not exit the game or turn off the PC,\n" \
       "otherwise data may be lost.")

#define SOURCE_TRANSFER_WARNING_REPLAY_LIMITED_COVID \
    N_("There are exceeded limits of level status or legacy replays\n" \
       "currently stored in the Replay of this game. The Replay in\n" \
       "the target Pennyball game has an either premade filters,\n" \
       "modern replays or COVID-19 features, so your Replays will be lost,\n" \
       "if you perform this transfer now.")
#define SOURCE_TRANSFER_WARNING_REPLAY_LIMITED_USER \
    N_("There are exceeded limits of level status or legacy replays\n" \
       "currently stored in the Replay of this game. The Replay in\n" \
       "the target Pennyball game has an either premade filters or\n" \
       "modern replays, so your Replays will be lost, if you perform\n" \
       "this transfer now.")
#define SOURCE_TRANSFER_WARNING_REPLAY_BACKUP \
    N_("If you do not want to delete these Replays,\n" \
       "you should move them to the backup folder\n" \
       "before performing this transfer.")
#define SOURCE_TRANSFER_WARNING_REPLAY_CONTINUE \
    N_("Do you wish to continue with the transfer?")

#define SOURCE_TRANSFER_WARNING_REPLAY_SAMENAME_1 \
    N_("There are same replay names on both games.")
#define SOURCE_TRANSFER_WARNING_REPLAY_SAMENAME_2 \
    N_("Would you like delete the replay from the\n" \
       "target game and replace with new ones?\n" \
       "(Replays will be deleted after the transfer.)")

#define SOURCE_TRANSFER_WARNING_REPLAY_SAMENAME_CONFIRM_1 \
    N_("The saved replay names will be overwritten\n" \
       "with that of this game.\n" \
       "Do you really want to continue?")
#define SOURCE_TRANSFER_WARNING_REPLAY_SAMENAME_CONFIRM_2 \
    N_("(Replays, that you used on the target game\n" \
       "before the transfer, will no longer be\n" \
       "used after the transfer.)")

/*---------------------------------------------------------------------------*/

static int transfer_levelstatus_limit;

static int transfer_process;
static int transfer_ui_transition_busy;
static int have_entered;

#if ENABLE_DEDICATED_SERVER==1 && !defined(TRANSFER_OFFLINE_ONLY)
static int preparations_internet = 0;
#endif
static int show_preparations = 0;

enum
{
    PRETRANSFER_EXCEEDED_STATE_NONE = 0, /* no restrictions        */

    PRETRANSFER_EXCEEDED_STATE_USER,     /* From the prepared user */
    PRETRANSFER_EXCEEDED_STATE_COVID,    /* From RKI               */

    PRETRANSFER_EXCEEDED_STATE_MAX
};

static int pretransfer_replay_status_limit = 3;
static int pretransfer_replay_status_covid = 0;

static int pretransfer_exceeded_state = 0;

static int replayfilepath_unsupported_found = 0;
static int replayfilepath_exceed_found = 0;

static int transfer_walletamount[2];

enum
{
    TRANSFER_WORKING_STATE_NONE = 0,
    TRANSFER_WORKING_STATE_LOAD_EXTERNAL,
    TRANSFER_WORKING_STATE_SAVE_INTERNAL,
    TRANSFER_WORKING_STATE_LOAD_INTERNAL,
    TRANSFER_WORKING_STATE_CONNECT_INTERNET,
    TRANSFER_WORKING_STATE_PROCESS
};

static int transfer_working = 0;
static int transfer_pageindx = 0;
static int show_transfer = 0;

static int show_transfer_completed = 0;

struct state st_transfer;
static struct state *transfer_back;

int goto_game_transfer(struct state *back)
{
    transfer_back = back;

    return goto_state(&st_transfer);
}

static int transfer_introducory_gui(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        gui_label(id, _(TITLE_SOURCE_TRANSFER), GUI_SML, 0, 0);

        gui_space(id);

        if ((jd = gui_vstack(id)))
        {
            switch (transfer_pageindx)
            {
            case 0:
                gui_multi(jd, _("This apps allows you to transfer data from\n"
                                "this game to a target Pennyball game."),
                              GUI_SML, gui_wht, gui_wht);
                gui_multi(jd, _("Do you have a target game and an external drive?\n"
                                "(These are required to perform a transfer.)"),
                              GUI_SML, gui_wht, gui_wht);
                gui_multi(jd, _("If you are under the age of 18, call an adult\n"
                                "and have them perform the transfer."),
                              GUI_SML, gui_blu, gui_blu);
                break;
            case 1:
                gui_multi(jd, _("Select Neverball Game Transfer\n"
                                "and make all preparations on the\n"
                                "target game necessary."),
                              GUI_SML, gui_wht, gui_wht);
                break;
            }
            gui_set_rect(jd, GUI_ALL);
        }

        gui_space(id);

        if ((jd = gui_harray(id)))
        {
            switch (transfer_pageindx)
            {
            case 0:
                gui_state(jd, _("Yes"), GUI_SML, GUI_NEXT, 0);
                gui_state(jd, _("No"), GUI_SML, GUI_BACK, 0);
                break;
            case 1:
                gui_state(jd, _("OK"), GUI_SML, GUI_BACK, 0);
                break;
            }
        }
    }

    gui_layout(id, 0, 0);

    return id;
}

#pragma region preparing for transfer
static int transfer_prepare_gui(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        gui_label(id, _(TITLE_SOURCE_TRANSFER), GUI_SML, 0, 0);

        gui_space(id);

        if ((jd = gui_vstack(id)))
        {
#if ENABLE_DEDICATED_SERVER==1 && !defined(TRANSFER_OFFLINE_ONLY)
            if (preparations_internet)
                gui_multi(jd, _("Connecting to the Internet to confirm that\n"
                                "the transfer can be performed..."),
                              GUI_SML, gui_wht, gui_wht);
            else
#endif
            {
#if defined(TRANSFER_OFFLINE_ONLY)
                gui_multi(jd, _("In order to perform a game transfer, you\n"
                                "first need to make preparations on the\n"
                                "target game."),
                              GUI_SML, gui_wht, gui_wht);
#else
                gui_multi(jd, _("In order to perform a game transfer, you\n"
                                "first need to make preparations on the\n"
                                "target game and connect this\n"
                                "game to the internet."),
                              GUI_SML, gui_wht, gui_wht);
#endif
                gui_multi(jd, _("Have you made these preparations?"),
                              GUI_SML, gui_wht, gui_wht);
            }

            gui_set_rect(jd, GUI_ALL);
        }

        gui_space(id);

        if ((jd = gui_harray(id)))
        {
            gui_state(jd, _("Yes"), GUI_SML, GUI_NEXT, 0);
            gui_state(jd, _("No"), GUI_SML, GUI_BACK, 0);
        }
    }

    gui_layout(id, 0, 0);

    return id;
}

static int transfer_replay_gui(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        gui_label(id, _(TITLE_SOURCE_TRANSFER), GUI_SML, 0, 0);

        gui_space(id);

        if ((jd = gui_vstack(id)))
        {
            if (pretransfer_replay_status_covid)
                gui_multi(jd, _(SOURCE_TRANSFER_WARNING_REPLAY_LIMITED_COVID),
                              GUI_SML, gui_wht, gui_wht);
            else
                gui_multi(jd, _(SOURCE_TRANSFER_WARNING_REPLAY_LIMITED_USER),
                              GUI_SML, gui_wht, gui_wht);
            gui_multi(jd, _(SOURCE_TRANSFER_WARNING_REPLAY_BACKUP),
                          GUI_SML, gui_wht, gui_wht);
            gui_multi(jd, _(SOURCE_TRANSFER_WARNING_REPLAY_CONTINUE),
                          GUI_SML, gui_wht, gui_wht);

            gui_set_rect(jd, GUI_ALL);
        }

        gui_space(id);

        if ((jd = gui_harray(id)))
        {
            gui_state(jd, _("Yes"), GUI_SML, GUI_NEXT, 0);
            gui_state(jd, _("No"), GUI_SML, GUI_BACK, 0);
        }
    }

    gui_layout(id, 0, 0);

    return id;
}

static int transfer_gui(void)
{
    int id, jd;

    char wallet_infotext[MAXSTR];

    if ((id = gui_vstack(0)))
    {
        if (transfer_working == TRANSFER_WORKING_STATE_NONE)
        {
            switch (transfer_pageindx)
            {
            case 6:
                gui_label(id, _("Finished all steps for this game"),
                              GUI_SML, 0, 0);
                break;
            case 4:
                gui_label(id, _("Transferring Data from this game"),
                              GUI_SML, 0, 0);
                break;
            case 5:
                gui_label(id, _("Wallet transferred"), GUI_SML, 0, 0);
                break;
            default:
                gui_label(id, _(TITLE_SOURCE_TRANSFER), GUI_SML, 0, 0);
                break;
            }

            gui_space(id);
        }

        if ((jd = gui_vstack(id)))
        {
            switch (transfer_pageindx)
            {
            case 1:
                if (transfer_working != TRANSFER_WORKING_STATE_NONE)
                {
                    switch (transfer_working)
                    {
                    case TRANSFER_WORKING_STATE_LOAD_EXTERNAL:
                        gui_multi(jd, _("Loading data from the External Drive..."),
                                      GUI_SML, gui_wht, gui_wht);
                        gui_multi(jd, _(SOURCE_TRANSFER_WARNING_EXTERNAL),
                                      GUI_SML, gui_red, gui_red);
                        break;
                    case TRANSFER_WORKING_STATE_SAVE_INTERNAL:
                        gui_multi(jd, _("Saving game transfer data to the PC..."),
                                      GUI_SML, gui_wht, gui_wht);
                        gui_multi(jd, _(SOURCE_TRANSFER_WARNING_INTERNAL),
                                      GUI_SML, gui_red, gui_red);
                        break;
                    case TRANSFER_WORKING_STATE_LOAD_INTERNAL:
                        gui_multi(jd, _("Loading game transfer data on the PC..."),
                                      GUI_SML, gui_wht, gui_wht);
                        gui_multi(jd, _(SOURCE_TRANSFER_WARNING_INTERNAL),
                                      GUI_SML, gui_red, gui_red);
                        break;
#if ENABLE_DEDICATED_SERVER==1 && !defined(TRANSFER_OFFLINE_ONLY)
                    case TRANSFER_WORKING_STATE_CONNECT_INTERNET:
                        gui_multi(jd, _("Connecting to the internet and\n"
                                        "retrieving data required for transfer..."),
                                      GUI_SML, gui_wht, gui_wht);
                        break;
#endif
                    }
                }
                else
                {
                    gui_multi(jd, _("Insert the External Drive prepared on the target\n"
                                    "Pennyball game into this game, and then press Next."),
                                  GUI_SML, gui_wht, gui_wht);
                    gui_multi(jd, _(SOURCE_TRANSFER_WARNING_EXTERNAL),
                                  GUI_SML, gui_red, gui_red);
                }
                break;
            case 2:
                gui_multi(jd, _("Please read the following information\n"
                                "carefully before starting the transfer."),
                              GUI_SML, gui_wht, gui_wht);
                break;
            case 3:
                gui_multi(jd, _("The following set highscores is present on both PCs.\n"
                                "Performing the game transfer will replace\n"
                                "the highscore on this game."),
                              GUI_SML, gui_wht, gui_wht);
                break;
            case 4:
                if (transfer_working == TRANSFER_WORKING_STATE_PROCESS)
                {
                    gui_multi(jd, _("Moving Data to the External Drive..."),
                                  GUI_SML, gui_wht, gui_wht);
                    gui_multi(jd, _(SOURCE_TRANSFER_WARNING_EXTERNAL),
                                  GUI_SML, gui_red, gui_red);
                }
                else
                {
                    gui_multi(jd, _("The data to be transferred from this game\n"
                                    "will be moved to the External Drive and deleted\n"
                                    "from the game progress."),
                                  GUI_SML, gui_wht, gui_wht);
                    gui_multi(jd, _(SOURCE_TRANSFER_WARNING_DELETE_DATA),
                                  GUI_SML, gui_red, gui_red);
                    gui_multi(jd, _(SOURCE_TRANSFER_WARNING_EXTERNAL),
                                  GUI_SML, gui_red, gui_red);
                }
                break;
            case 5:
                if (transfer_walletamount[0] > 0 && transfer_walletamount[1])
                {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                    sprintf_s(wallet_infotext, MAXSTR,
#else
                    sprintf(wallet_infotext,
#endif
                            _("%d coins and %d gems have been transferred."),
                            transfer_walletamount[0], transfer_walletamount[1]);
                    gui_label(jd, wallet_infotext, GUI_SML, gui_wht, gui_wht);
                    gui_multi(jd, _("You can use these coins and gems in\n"
                                    "game shop on the target Pennyball game."),
                                  GUI_SML, gui_wht, gui_wht);
                }
                else if (transfer_walletamount[1] > 0)
                {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                    sprintf_s(wallet_infotext, MAXSTR,
#else
                    sprintf(wallet_infotext,
#endif
                            _("%d gems have been transferred."),
                            transfer_walletamount[1]);
                    gui_label(jd, wallet_infotext, GUI_SML, gui_wht, gui_wht);
                    gui_multi(jd, _("You can use these gems in the\n"
                                    "game shop on the target Pennyball game."),
                                  GUI_SML, gui_wht, gui_wht);
                }
                else if (transfer_walletamount[0] > 0)
                {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                    sprintf_s(wallet_infotext, MAXSTR,
#else
                    sprintf(wallet_infotext,
#endif
                            _("%d coins have been transferred."),
                            transfer_walletamount[0]);
                    gui_label(jd, wallet_infotext, GUI_SML, gui_wht, gui_wht);
                    gui_multi(jd, _("You can use these coins in the\n"
                                    "game shop on the target Pennyball game."),
                                  GUI_SML, gui_wht, gui_wht);
                }
                break;
            case 6:
                gui_multi(jd, _("Finished moving data to the external drive.\n"
                                "Please insert the external drive into the\n"
                                "target game."),
                              GUI_SML, gui_wht, gui_wht);
                gui_multi(jd, _("If you only have one keyboard and mouse,\n"
                                "first turn it off from this PC\n"
                                "before connecting to the target PC."),
                              GUI_SML, gui_wht, gui_wht);
                break;
            }

            gui_set_rect(jd, GUI_ALL);
        }

        if (!transfer_working)
        {
            gui_space(id);

            if ((jd = gui_harray(id)))
            {
                if (transfer_pageindx == 5)
                    gui_start(jd, _("Next"), GUI_SML, GUI_NEXT, 0);
                else if (transfer_pageindx == 4)
                {
                    gui_start(jd, _("Transfer"), GUI_SML, GUI_NEXT, 0);
                    gui_state(jd, _("Back"), GUI_SML, GUI_PREV, 0);
                }
                else if (transfer_pageindx < 3)
                {
                    gui_start(jd, _("Next"), GUI_SML, GUI_NEXT, 0);
                    gui_state(jd, _("Cancel"), GUI_SML, GUI_BACK, 0);
                }
                else if (transfer_pageindx != 6)
                {
                    gui_start(jd, _("Next"), GUI_SML, GUI_NEXT, 0);
                    gui_start(jd, _("Back"), GUI_SML, GUI_PREV, 0);
                }
            }
        }
    }

    gui_layout(id, 0, 0);

    return id;
}
#pragma endregion

static int transfer_completed_gui(void)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        if ((jd = gui_vstack(id)))
        {
            gui_multi(jd, _("All the steps required to transfer from\n"
                            "this game have been completed.\n"
                            "The game will now exit."),
                          GUI_SML, gui_wht, gui_wht);
            gui_multi(jd, _("Please switch to the target game."),
                          GUI_SML, gui_wht, gui_wht);

            gui_set_rect(jd, GUI_ALL);
        }

        gui_space(id);

        gui_state(id, _("Exit"), GUI_SML, GUI_NEXT, 0);
    }

    gui_layout(id, 0, 0);

    return id;
}

/*---------------------------------------------------------------------------*/

static int transfer_action(int tok, int val)
{
    audio_play(GUI_PREV == tok || GUI_BACK == tok ? AUD_BACK : AUD_MENU, 1.0f);

    if (pretransfer_exceeded_state && GUI_BACK != tok)
    {
        pretransfer_exceeded_state = 0;
        transfer_pageindx++;
        return exit_state(curr_state());
    }
    else if (show_transfer_completed && tok == GUI_NEXT) return 0; /* bye! */
    else if (!show_preparations && !show_transfer)
    {
        if (tok == GUI_NEXT)
        {
            transfer_pageindx = 1;
            show_preparations = 1;
            return goto_state(curr_state());
        }
        else
        {
            transfer_pageindx = 0;
            have_entered = 0;
            return goto_state(transfer_back);
        }
    }
    else if (show_preparations && !show_transfer)
    {
        if (tok == GUI_NEXT)
        {
            show_preparations = 0;
            show_transfer = 1;
            transfer_pageindx = 1;
            return goto_state(curr_state());
        }
        else if (tok == GUI_PREV || tok == GUI_BACK)
        {
            show_preparations = 0;
            show_transfer = 0;
            return goto_state(curr_state());
        }
    }
    else if (show_transfer)
    {
        switch (tok)
        {
        case GUI_NEXT:
            if (pretransfer_exceeded_state)
            {
                pretransfer_exceeded_state = 0;
                transfer_pageindx = 3;
                return goto_state(&st_transfer);
            }
            else if (transfer_pageindx == 1)
            {
                transfer_ui_transition_busy = 1;
                transfer_working = TRANSFER_WORKING_STATE_LOAD_EXTERNAL;
                return goto_state(&st_transfer);
            }
            else if (transfer_pageindx == 2)
            {
                pretransfer_exceeded_state = replayfilepath_exceed_found > 0 ||
                                             replayfilepath_unsupported_found > 0;
                if (pretransfer_exceeded_state)
                    return goto_state(curr_state());
                else
                {
                    transfer_pageindx = 3;
                    return goto_state(curr_state());
                }
            }
            else if (transfer_pageindx == 4)
            {
                transfer_ui_transition_busy = 1;
                transfer_working = TRANSFER_WORKING_STATE_PROCESS;
                return goto_state(&st_transfer);
            }
            else
            {
                transfer_ui_transition_busy = 1;
                transfer_pageindx++;
                return goto_state(curr_state());
            }
            break;

        case GUI_BACK:
            if (pretransfer_exceeded_state)
            {
                pretransfer_exceeded_state = 0;
                return goto_state(&st_transfer);
            }
            else if (transfer_pageindx < 4)
            {
                show_preparations = 0;
                show_transfer = 0;
                transfer_pageindx = 0;
                have_entered = 0;
                return goto_state(transfer_back);
            }
            else
            {
                transfer_ui_transition_busy = 1;
                transfer_pageindx--;
                return goto_state(&st_transfer);
            }
            break;

        case GUI_PREV:
            transfer_ui_transition_busy = 1;
            transfer_pageindx--;
            return goto_state(curr_state());
        }

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

int transfer_error_code;

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

static float transfer_alpha;

/*
 * Request adding replay transfer, when indeeded
 * @param Level status limit:
 *        1 = Only finish; 2 = Keep on board ; 3 = Always active
 */
static void (*transfer_request_addreplay_dispatch_event) (int status_limit);

void transfer_addreplay_quit();

static void transfer_timer_preprocess_source(float dt)
{
    FILE *external_file;
    __int64 lpFreeBytesAvailable, lpTotalNumberOfBytes, lpTotalNumberOfFreeBytes;
    DWORD dwSectPerClust, dwBytesPerSect, dwFreeClusters, dwTotalClusters;
    int ext_drive_connected, ext_drive_supported;

    if (!transfer_ui_transition_busy && transfer_alpha > 0.99f)
    {
        switch (transfer_pageindx)
        {
        case 1:
            switch (transfer_working)
            {
            case TRANSFER_WORKING_STATE_LOAD_EXTERNAL:
                for (int i = -1; i < 24 && current_drive_idx == -1;)
                {
                    /* Skip scanning, if we have prepared on the target game */
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
                        fopen_s(&external_file,
                                concat_string(drive_letters[i],
                                              ":/nvb_gametransfer/replayfilter.nbtransfer", 0),
                                "r+t");
#else
                        external_file = fopen(concat_string(drive_letters[i],
                                                            ":/nvb_gametransfer/replayfilter.nbtransfer",
                                                            0),
                                              "r+t");
#endif

                        if (external_file)
                        {
                            char outstr_replayfile[MAXSTR], outstr_group[MAXSTR];
                            fgets(outstr_replayfile, MAXSTR, external_file);
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                            sscanf_s(outstr_replayfile,
#else
                            sscanf(outstr_replayfile,
#endif
                                   "restricted-replays: %s\n", outstr_group);

                            pretransfer_replay_status_limit = 3;
                            pretransfer_replay_status_covid = 0;

                            if (strcmp(outstr_group, "covid") == 0 ||
                                strcmp(outstr_group, "keep") == 0)
                            {
                                if (strcmp(outstr_group, "covid") == 0)
                                    pretransfer_replay_status_covid = 1;

                                pretransfer_replay_status_limit = 2;
                            }
                            if (strcmp(outstr_group, "goal") == 0)
                                pretransfer_replay_status_limit = 1;

                            fclose(external_file);

                            current_drive_idx = i;

                            transfer_working = TRANSFER_WORKING_STATE_SAVE_INTERNAL;
                            transfer_ui_transition_busy = 1;
                            goto_state(&st_transfer);
                            return;
                        }

                        transfer_error_code = 2;
                    }
                    else if (!ext_drive_supported)
                    {
                        log_errorf("For disk %s:, only removable drive is allowed!\n",
                                   drive_letters[i]);
                        transfer_error_code = 2;
                    }
                    else
                    {
                        log_errorf("No external drive in %s:\n", drive_letters[i]);
                        transfer_error_code = 2;
                    }

                    i++;
                }

                if (current_drive_idx == -1)
                {
                    log_errorf("Failure to loading data! External Drive requires that you've prepared the game transfer, which you've connected to the computer!: %s\n",
                               strerror(transfer_error_code));

                    transfer_working = TRANSFER_WORKING_STATE_NONE;
                    transfer_ui_transition_busy = 1;
                    transfer_pageindx = 1;
                    goto_state_full_intent(&st_transfer,
                                    GUI_ANIMATION_E_CURVE,
                                    GUI_ANIMATION_W_CURVE, 0);
                }
                break;

            case TRANSFER_WORKING_STATE_SAVE_INTERNAL:
                assert(current_drive_idx > -1 && drive_letters[current_drive_idx]);
                {
                    const char *homepath = pick_home_path();
                    assert(homepath);
#if _WIN32
                    const char *output_creation_folder = concat_string(homepath,
                                                                       "\\",
                                                                       CONFIG_USER,
                                                                       "\\Replays_transfer", 0);
#else
                    const char *output_creation_folder = concat_string(homepath,
                                                                       "/",
                                                                       CONFIG_USER,
                                                                       "/Replays_transfer", 0);
#endif
                    assert(output_creation_folder);
                    dir_make(output_creation_folder);

                    int found_replayfilter_file = 0;
                    fs_file replayfilter_file;
                    while (!found_replayfilter_file)
                    {
                        replayfilter_file = fs_open_read("Replays_transfer\\"
                                                         "replayfilter.nbtransfer");
                        if (!replayfilter_file)
                        {
                            fclose(replayfilter_file);

                            dir_make("Replays_transfer");

                            found_replayfilter_file = 1;
                        }
                    }

                    transfer_working = TRANSFER_WORKING_STATE_LOAD_INTERNAL;
                    transfer_ui_transition_busy = 1;
                    goto_state(&st_transfer);
                }
                break;

            case TRANSFER_WORKING_STATE_LOAD_INTERNAL:
                replayfilepath_unsupported_found = 0;
                replayfilepath_exceed_found = 0;
                transfer_request_addreplay_dispatch_event(pretransfer_replay_status_limit);
                transfer_addreplay_quit();

                transfer_working = TRANSFER_WORKING_STATE_NONE;
                transfer_ui_transition_busy = 1;
                transfer_pageindx = 2;
                goto_state_full_intent(&st_transfer, GUI_ANIMATION_W_CURVE, GUI_ANIMATION_E_CURVE, 0);
                break;
            }
            break;
        }
    }
}

#define MAX_TRANSFER_FILES 1024

static char src_replayfilepath[MAX_TRANSFER_FILES][MAXSTR];
static int replayfilepath_count;

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

    char player   [MAXSTR];
    char ball_file[MAXSTR];
};

static struct account_transfer_value account_transfer_values_source,
                                     account_transfer_values_target;
static int account_transfer_count = 0;

static void transfer_reset_paths()
{
    for (int i = 0; i < MAX_TRANSFER_FILES; ++i)
    {
        //memset(&account_transfer_values_source, 0, sizeof (struct account_transfer_value));
        memset(account_transfer_values_source.player, 0, MAXSTR);
        memset(account_transfer_values_source.ball_file, 0, MAXSTR);
        //memset(&account_transfer_values_target, 0, sizeof (struct account_transfer_value));
        memset(account_transfer_values_target.player, 0, MAXSTR);
        memset(account_transfer_values_target.ball_file, 0, MAXSTR);
        memset(src_replayfilepath[i], 0, 256);
    }
}

static int replay_transfer_prepare_scriptfile_opened = 0;
static FILE *replay_transfer_prepare_scriptfile;
static FILE *replay_transfer_process_scriptfile;

void transfer_addreplay_quit()
{
    if (replay_transfer_prepare_scriptfile_opened)
    {
        fclose(replay_transfer_prepare_scriptfile);
        replay_transfer_prepare_scriptfile_opened = 0;
        Sleep(500);
    }

    const char *homepath = pick_home_path();

    char exec_script[MAXSTR];
    SAFECPY(exec_script, "\"");
#if _WIN32
    SAFECAT(exec_script, homepath);
    SAFECAT(exec_script, "\\");
    SAFECAT(exec_script, CONFIG_USER);
    SAFECAT(exec_script, "\\Replays_transfer\\pretransfer_script.bat");
#else
    SAFECPY(exec_script, homepath);
    SAFECAT(exec_script, "/");
    SAFECAT(exec_script, CONFIG_USER);
    SAFECAT(exec_script, "/Replays_transfer/pretransfer_script.bat");
#endif
    SAFECAT(exec_script, "\"");

    system(exec_script);
}

void transfer_addreplay(const char *path)
{
    /* When they exceeds, it will be truncated. */
    if (replayfilepath_count > MAX_TRANSFER_FILES - 1) return;

    if (!replay_transfer_prepare_scriptfile_opened)
    {
        const char *homepath = pick_home_path();
#if _WIN32
        const char *prepare_creation_script = concat_string(homepath, "\\", CONFIG_USER, "\\Replays_transfer\\pretransfer_script.bat", NULL);
        const char *process_creation_script = concat_string(homepath, "\\", CONFIG_USER, "\\Replays_transfer\\transfer_process_script.bat", NULL);
#else
        const char *prepare_creation_script = concat_string(homepath, "/", CONFIG_USER, "/Replays_transfer/pretransfer_script.bat", NULL);
        const char *process_creation_script = concat_string(homepath, "/", CONFIG_USER, "/Replays_transfer/transfer_process_script.bat", NULL);
#endif
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        fopen_s(&replay_transfer_prepare_scriptfile, prepare_creation_script, "w+t");
        fopen_s(&replay_transfer_process_scriptfile, process_creation_script, "w+t");
#else
        replay_transfer_prepare_scriptfile = fopen(prepare_creation_script, "w+t");
        replay_transfer_process_scriptfile = fopen(process_creation_script, "w+t");
#endif
        replay_transfer_prepare_scriptfile_opened = 1;
        fwrite("@echo off\n", 1, strlen("@echo off\n"), replay_transfer_prepare_scriptfile);

        char exec_process_script[MAXSTR];
        SAFECPY(exec_process_script, "move /y \"");
#if _WIN32
        SAFECAT(exec_process_script, homepath);
        SAFECAT(exec_process_script, "\\");
        SAFECAT(exec_process_script, CONFIG_USER);
        SAFECAT(exec_process_script, "\\Replays_transfer\\*.nbr\" \"");
#else
        SAFECAT(exec_process_script, homepath);
        SAFECAT(exec_process_script, "/");
        SAFECAT(exec_process_script, CONFIG_USER);
        SAFECAT(exec_process_script, "/Replays_transfer\\*.nbr\" \"");
#endif
        SAFECAT(exec_process_script, drive_letters[current_drive_idx]);
        SAFECAT(exec_process_script, ":/nvb_gametransfer/replays\"");
        fwrite(exec_process_script, 1, strlen(exec_process_script), replay_transfer_process_scriptfile);
        fclose(replay_transfer_process_scriptfile);
    }

    char outfile[MAXSTR];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    strcpy_s(outfile, MAXSTR, path);
#else
    strcpy(outfile, path);
#endif

#if _WIN32
    for (int i = 0; i < strlen(path); i++)
    {
        if (outfile[i] == '/')
            outfile[i] = '\\';
    }
    log_printf("Added replay into the transfer folder: %s\n",
               concat_string(fs_get_write_dir(), "\\", outfile, NULL));
#else
    log_printf("Added replay into the transfer folder: %s\n",
               concat_string(fs_get_write_dir(), "/", outfile, NULL));
#endif

    if (replay_transfer_prepare_scriptfile_opened)
        fwrite(concat_string("move /y \"", fs_get_write_dir(), "\\", outfile, "\" \"", fs_get_write_dir(), "\\Replays_transfer\"\n", NULL), 1, strlen(concat_string("copy /y \"", fs_get_write_dir(), "\\", outfile, "\" \"", fs_get_write_dir(), "\\Replays_transfer\"\n", NULL)), replay_transfer_prepare_scriptfile);

    SAFECPY(src_replayfilepath[replayfilepath_count], outfile);
    replayfilepath_count++;
}

void transfer_addreplay_unsupported(void)
{
    replayfilepath_unsupported_found++;
}

void transfer_addreplay_exceeded(void)
{
    replayfilepath_exceed_found++;
}

void transfer_add_dispatch_event(void (*request_addreplay_dispatch_event)(int status_limit))
{
    transfer_request_addreplay_dispatch_event = request_addreplay_dispatch_event;
}

void transfer_timer_process_source(float dt)
{
    if (transfer_ui_transition_busy || transfer_alpha <= 0.99f) return;

    switch (transfer_pageindx)
    {
    case 4:
    {
        FILE *transferfile;
        assert(current_drive_idx > -1 && drive_letters[current_drive_idx]);

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        fopen_s(&transferfile, concat_string(drive_letters[current_drive_idx],
                                             ":/nvb_gametransfer/fromsource", 0),
                               "w+t");
#else
        transferfile = fopen(concat_string(drive_letters[current_drive_idx],
                                           ":/nvb_gametransfer/fromsource", 0),
                             "w+t");
#endif

        if (!transferfile)
        {
            log_errorf("Failure to write data for External Drive (%s:)! External Drive must be connected to the computer!: %s\n",
                       drive_letters[current_drive_idx], fs_error());

            transfer_working = TRANSFER_WORKING_STATE_NONE;
            transfer_pageindx = 1;
            goto_state(&st_transfer);
            return;
        }

        /* Phase 1; Account */

        int account_transfer_was_new;

        WIN32_FIND_DATAA src_ffd;
        HANDLE hSrcFind = FindFirstFileA(concat_string(pick_home_path(),
                                                       "\\", CONFIG_USER,
                                                       "\\Accounts\\*", NULL),
                                         &src_ffd);
        if (INVALID_HANDLE_VALUE != hSrcFind)
        {
            FILE *wallet_file;
            char outwallet_result_csv[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            fopen_s(&wallet_file, concat_string(drive_letters[current_drive_idx],
                                                ":/nvb_gametransfer/account.nbwallet",
                                                NULL),
                    "w+t");
#else
            wallet_file = fopen(concat_string(drive_letters[current_drive_idx],
                                              ":/nvb_gametransfer/account.nbwallet",
                                              NULL),
                                "w+t");
#endif

            do
            {
                if (src_ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    /* Is directory, skipped */
                }
                else if (strcmp(src_ffd.cFileName, ".") != 0 ||
                         strcmp(src_ffd.cFileName, "..") != 0)
                {
                    account_transfer_init();
                    account_transfer_load(concat_string("Accounts/", src_ffd.cFileName, NULL));

                    account_transfer_values_source.wallet_coins = account_transfer_get_d(ACCOUNT_TRANSFER_DATA_WALLET_COINS);
                    account_transfer_values_source.wallet_gems = account_transfer_get_d(ACCOUNT_TRANSFER_DATA_WALLET_GEMS);
                    account_transfer_values_source.product_levels = account_transfer_get_d(ACCOUNT_TRANSFER_PRODUCT_LEVELS);
                    account_transfer_values_source.product_balls = account_transfer_get_d(ACCOUNT_TRANSFER_PRODUCT_BALLS);
                    account_transfer_values_source.product_bonus = account_transfer_get_d(ACCOUNT_TRANSFER_PRODUCT_BONUS);
                    account_transfer_values_source.product_mediation = account_transfer_get_d(ACCOUNT_TRANSFER_PRODUCT_MEDIATION);
                    account_transfer_values_source.set_unlocks = account_transfer_get_d(ACCOUNT_TRANSFER_SET_UNLOCKS);
                    account_transfer_values_source.consumeable_earninator = account_transfer_get_d(ACCOUNT_TRANSFER_CONSUMEABLE_EARNINATOR);
                    account_transfer_values_source.consumeable_floatifier = account_transfer_get_d(ACCOUNT_TRANSFER_CONSUMEABLE_FLOATIFIER);
                    account_transfer_values_source.consumeable_speedifier = account_transfer_get_d(ACCOUNT_TRANSFER_CONSUMEABLE_SPEEDIFIER);
                    account_transfer_values_source.consumeable_extralives = account_transfer_get_d(ACCOUNT_TRANSFER_CONSUMEABLE_EXTRALIVES);

                    SAFECPY(account_transfer_values_source.player, account_transfer_get_s(ACCOUNT_TRANSFER_PLAYER));
#if defined(CONFIG_INCLUDES_MULTIBALLS)
                    SAFECPY(account_transfer_values_source.ball_file, account_transfer_get_s(ACCOUNT_TRANSFER_BALL_FILE_C));
#else
                    SAFECPY(account_transfer_values_source.ball_file, account_transfer_get_s(ACCOUNT_TRANSFER_BALL_FILE));
#endif
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                    sprintf_s(outwallet_result_csv, MAXSTR,
#else
                    sprintf(outwallet_result_csv,
#endif
                        "coins:%d ;gems:%d ;p_levels:%d ;p_balls:%d ;p_bonus:%d ;p_mediation:%d ;set_unlocks:%d ;c_earninator:%d ;c_floatifier:%d ;c_speedifier:%d ;c_extralives:%d ;ballfile: %s ; player: %s ;\n",
                        account_transfer_values_source.wallet_coins,
                        account_transfer_values_source.wallet_gems,
                        account_transfer_values_source.product_levels,
                        account_transfer_values_source.product_balls,
                        account_transfer_values_source.product_bonus,
                        account_transfer_values_source.product_mediation,
                        account_transfer_values_source.set_unlocks,
                        account_transfer_values_source.consumeable_earninator,
                        account_transfer_values_source.consumeable_floatifier,
                        account_transfer_values_source.consumeable_speedifier,
                        account_transfer_values_source.consumeable_extralives,
                        account_transfer_values_source.ball_file,
                        account_transfer_values_source.player);

                    account_transfer_quit();

                    transfer_walletamount[0] += account_transfer_values_source.wallet_coins;
                    transfer_walletamount[1] += account_transfer_values_source.wallet_gems;

                    account_transfer_init();
                    account_transfer_was_new = !fs_exists(concat_string("Accounts_transfer/", src_ffd.cFileName, NULL));

                    if (account_transfer_was_new)
                        log_printf("New account (%s) was transferred to the target game\n", account_transfer_values_source.player);
                    else
                        account_transfer_load(concat_string("Accounts_transfer/", src_ffd.cFileName, NULL));

                    account_transfer_values_target.wallet_coins = account_transfer_get_d(ACCOUNT_TRANSFER_DATA_WALLET_COINS);
                    account_transfer_values_target.wallet_gems = account_transfer_get_d(ACCOUNT_TRANSFER_DATA_WALLET_GEMS);

                    account_transfer_set_d(ACCOUNT_TRANSFER_DATA_WALLET_COINS, account_transfer_values_source.wallet_coins + account_transfer_values_target.wallet_coins);
                    account_transfer_set_d(ACCOUNT_TRANSFER_DATA_WALLET_GEMS, account_transfer_values_source.wallet_gems + account_transfer_values_target.wallet_gems);

                    if (account_transfer_values_source.product_levels)
                        account_transfer_set_d(ACCOUNT_TRANSFER_PRODUCT_LEVELS, account_transfer_values_source.product_levels);

                    if (account_transfer_values_source.product_balls)
                        account_transfer_set_d(ACCOUNT_TRANSFER_PRODUCT_BALLS, account_transfer_values_source.product_balls);

                    if (account_transfer_values_source.product_bonus)
                        account_transfer_set_d(ACCOUNT_TRANSFER_PRODUCT_BONUS, account_transfer_values_source.product_bonus);

                    if (account_transfer_values_source.product_mediation)
                        account_transfer_set_d(ACCOUNT_TRANSFER_PRODUCT_MEDIATION, account_transfer_values_source.product_mediation);

                    if (account_transfer_values_source.set_unlocks)
                        account_transfer_set_d(ACCOUNT_TRANSFER_SET_UNLOCKS, account_transfer_values_source.set_unlocks);

                    account_transfer_set_d(ACCOUNT_TRANSFER_CONSUMEABLE_EARNINATOR, account_transfer_values_source.consumeable_earninator + account_transfer_values_target.consumeable_earninator);
                    account_transfer_set_d(ACCOUNT_TRANSFER_CONSUMEABLE_FLOATIFIER, account_transfer_values_source.consumeable_floatifier + account_transfer_values_target.consumeable_floatifier);
                    account_transfer_set_d(ACCOUNT_TRANSFER_CONSUMEABLE_SPEEDIFIER, account_transfer_values_source.consumeable_speedifier + account_transfer_values_target.consumeable_speedifier);
                    account_transfer_set_d(ACCOUNT_TRANSFER_CONSUMEABLE_SPEEDIFIER, account_transfer_values_source.consumeable_extralives + account_transfer_values_target.consumeable_extralives);

                    account_transfer_set_s(ACCOUNT_TRANSFER_PLAYER, account_transfer_values_source.player);
#if defined(CONFIG_INCLUDES_MULTIBALLS)
                    account_transfer_set_s(ACCOUNT_TRANSFER_BALL_FILE_C, account_transfer_values_source.ball_file);
#else
                    account_transfer_set_s(ACCOUNT_TRANSFER_BALL_FILE, account_transfer_values_source.ball_file);
#endif

                    account_transfer_save(src_ffd.cFileName);
                    account_transfer_quit();

                    if (wallet_file)
                        fwrite(outwallet_result_csv, 1, strlen(outwallet_result_csv), wallet_file);

                    remove(concat_string(pick_home_path(), "/", CONFIG_USER, "/Accounts/", src_ffd.cFileName, NULL));
                }
            }
            while (FindNextFileA(hSrcFind, &src_ffd) != 0);

            if (wallet_file)
                fclose(wallet_file);
        }

        /* The account folder is deleted */
#if _WIN32
        rmdir(concat_string(fs_get_write_dir(), "\\Accounts", NULL));
#else
        rmdir(concat_string(fs_get_write_dir(), "/Accounts", NULL));
#endif

        /* Phase 2: Replays */

        const char *homepath = pick_home_path();
        char exec_script[MAXSTR];
        SAFECPY(exec_script, "\"");
#if _WIN32
        SAFECAT(exec_script, homepath);
        SAFECAT(exec_script, "\\");
        SAFECAT(exec_script, CONFIG_USER);
        SAFECAT(exec_script, "\\Replays_transfer\\transfer_process_script.bat");
#else
        SAFECPY(exec_script, homepath);
        SAFECAT(exec_script, "/");
        SAFECAT(exec_script, CONFIG_USER);
        SAFECAT(exec_script, "/Replays_transfer/transfer_process_script.bat");
#endif
        SAFECAT(exec_script, "\"");

        system(exec_script);
        Sleep(3000);

        /* The replay folder is deleted */
#if _WIN32
        system(concat_string("del /f \"", fs_get_write_dir(),
                             "\\Replays_transfer\\*.bat\"", NULL));
        rmdir(concat_string(fs_get_write_dir(), "\\Replays", NULL));
        rmdir(concat_string(fs_get_write_dir(), "\\Replays_transfer", NULL));
#else
        system(concat_string("del /f \"", fs_get_write_dir(),
                             "/Replays_transfer/*.bat\"", NULL));
        rmdir(concat_string("\"", fs_get_write_dir(), "/Replays\"", NULL));
        rmdir(concat_string("\"", fs_get_write_dir(), "/Replays_transfer\"", NULL));
#endif

        /* Phase 3: Highscores */

        char hs_campaign_out_transfer[MAXSTR];
        char hs_levelset_out_transfer[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(hs_campaign_out_transfer, MAXSTR, "move /y \"%s\" \"%s\"",
            concat_string(fs_get_write_dir(), "\\Campaign\\*", NULL),
            concat_string(drive_letters[current_drive_idx],
                          ":/nvb_gametransfer/campaign", NULL)
        );
        sprintf_s(hs_levelset_out_transfer, MAXSTR, "move /y \"%s\" \"%s\"",
            concat_string(fs_get_write_dir(), "\\Scores\\*", NULL),
            concat_string(drive_letters[current_drive_idx],
                          ":/nvb_gametransfer/scores", NULL)
        );
#elif _WIN32 && !defined(__EMSCRIPTEN__) && _CRT_SECURE_NO_WARNINGS
        sprintf(hs_campaign_out_transfer, "move /y \"%s\" \"%s\"",
            concat_string(fs_get_write_dir(), "\\Campaign\\*", NULL),
            concat_string(drive_letters[current_drive_idx],
                          ":/nvb_gametransfer/campaign", NULL)
        );
        sprintf(hs_levelset_out_transfer, "move /y \"%s\" \"%s\"",
            concat_string(fs_get_write_dir(), "\\Scores\\*", NULL),
            concat_string(drive_letters[current_drive_idx],
                          ":/nvb_gametransfer/scores", NULL)
        );
#else
        sprintf(hs_campaign_out_transfer, "move /y \"%s\" \"%s\"",
            concat_string(fs_get_write_dir(), "/Campaign/*", NULL),
            concat_string(drive_letters[current_drive_idx],
                          ":/nvb_gametransfer/campaign", NULL)
        );
        sprintf(hs_levelset_out_transfer, "move /y \"%s\" \"%s\"",
            concat_string(fs_get_write_dir(), "/Scores/*", NULL),
            concat_string(drive_letters[current_drive_idx],
                          ":/nvb_gametransfer/scores", NULL)
        );
#endif

        system(hs_campaign_out_transfer);
        Sleep(200);
        system(hs_levelset_out_transfer);
        Sleep(500);

        /* The highscores folder is deleted */
        rmdir(concat_string(fs_get_write_dir(), "/Campaign", NULL));
        rmdir(concat_string(fs_get_write_dir(), "/Scores", NULL));

        /* Phase 4: Screenshots */

        char screenshots_out_transfer[MAXSTR];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(screenshots_out_transfer, MAXSTR, "move /y \"%s\" \"%s\"",
            concat_string(fs_get_write_dir(), "\\Screenshots\\*", NULL),
            concat_string(drive_letters[current_drive_idx],
                          ":/nvb_gametransfer/screenshots", NULL)
        );
#elif _WIN32 && !defined(__EMSCRIPTEN__) && _CRT_SECURE_NO_WARNINGS
        sprintf(screenshots_out_transfer, "move /y \"%s\" \"%s\"",
            concat_string(fs_get_write_dir(), "\\Screenshots\\*", NULL),
            concat_string(drive_letters[current_drive_idx],
                          ":/nvb_gametransfer/screenshots", NULL)
        );
#else
        sprintf(screenshots_out_transfer, "move /y \"%s\" \"%s\"",
            concat_string(fs_get_write_dir(), "/Screenshots/*", NULL),
            concat_string(drive_letters[current_drive_idx],
                          ":/nvb_gametransfer/screenshots", NULL)
        );
#endif

        system(screenshots_out_transfer);
        Sleep(500);

        /* The screenshot folder is deleted */
        rmdir(concat_string(fs_get_write_dir(), "/Screenshots", NULL));

        if (transfer_walletamount[0] || transfer_walletamount[1])
            transfer_pageindx = 5;
        else
            transfer_pageindx = 6;

        transfer_working = 0;
        goto_state(&st_transfer);
    }
    break;
    }
}

/*---------------------------------------------------------------------------*/

static int transfer_enter_source(struct state *st, struct state *prev)
{
    if (!have_entered)
    {
        transfer_reset_paths();
        audio_music_fade_to(0.5f, "bgm/systemtransfer.ogg", 1);

        current_drive_idx = -1;

        have_entered = 1;
        transfer_walletamount[0] = 0;
        transfer_walletamount[1] = 0;

#if ENABLE_DEDICATED_SERVER==1 && !defined(TRANSFER_OFFLINE_ONLY)
        preparations_internet = 0;
#endif
    }

    back_init("back/gui.png");

    if (show_transfer_completed)
        return transition_slide(transfer_completed_gui(), 1, intent);
    else if (pretransfer_exceeded_state)
        return transition_slide(transfer_replay_gui(), 1, intent);
    else if (!show_transfer && !show_preparations)
        return transition_slide(transfer_introducory_gui(), 1, intent);
    else if (!show_transfer && show_preparations)
        return transition_slide(transfer_prepare_gui(), 1, intent);
    else if (show_transfer && !show_preparations)
        return transition_slide(transfer_gui(), 1, intent);

    return 0;
}

static int transfer_leave(struct state *st, struct state *next, int id, int intent)
{
    if (transfer_process == 1)
        transfer_process = 2;

    transfer_ui_transition_busy = 0;

    return conf_common_leave(st, next, id, intent);
}

/*---------------------------------------------------------------------------*/

static void transfer_timer(int id, float dt)
{
    gui_timer(id, dt);

    if (show_transfer)
    {
        if (transfer_working)
        {
            if (transfer_pageindx == 4)
                transfer_timer_process_source(dt);
            else if (transfer_pageindx == 1)
                transfer_timer_preprocess_source(dt);
        }

        if (time_state() > 30 && transfer_pageindx == 6 &&
            !transfer_ui_transition_busy)
        {
            transfer_ui_transition_busy = 1;
            show_transfer_completed = 1;
            goto_state(&st_transfer);
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
    transfer_enter_source,
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
