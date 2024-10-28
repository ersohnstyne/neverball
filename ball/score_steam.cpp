/*
 * Copyright (C) 2023 Microsoft / Valve / Neverball authors
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

/*
 * HACK: Remembering the code file differences:
 * Developers  who  programming  C++  can see more bedrock declaration
 * than C.  Developers  who  programming  C  can  see  few  procedural
 * declaration than  C++.  Keep  in  mind  when making  sure that your
 * extern code must associated. The valid file types are *.c and *.cpp,
 * so it's always best when making cross C++ compiler to keep both.
 * - Ersohn Styne
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "score_online.h"
#include "set.h"
#include "common.h"
#include "log.h"

#ifdef __cplusplus
}
#endif

#if NB_STEAM_API==1 && !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)

#define STEAM_DISABLE_HIGHSCORES

#include <steam/steam_api.h>

/*---------------------------------------------------------------------------*/

class CScoreSteamLeaderboards
{
private:
    SteamLeaderboard_t curr_score;

public:
    int entry_count;
    LeaderboardEntry_t entry[10];

    bool rank_loading, rank_online;

    CScoreSteamLeaderboards();
    ~CScoreSteamLeaderboards() {};

    void FindRank(const char *pchLeaderboardName);
    bool UploadRank(int value);
    bool DownloadRank();

    void OnFindRank(LeaderboardFindResult_t *pResult,
                    bool bIOFailure);
    CCallResult<CScoreSteamLeaderboards, LeaderboardFindResult_t>
        call_find_rank;
    void OnUploadRank(LeaderboardScoreUploaded_t *pResult,
                      bool bIOFailure);
    CCallResult<CScoreSteamLeaderboards, LeaderboardScoreUploaded_t>
        call_upld_rank;
    void OnDownloadRank(LeaderboardScoresDownloaded_t *pResult,
                        bool bIOFailure);
    CCallResult<CScoreSteamLeaderboards, LeaderboardScoresDownloaded_t>
        call_dnld_rank;
};

/*---------------------------------------------------------------------------*/

CScoreSteamLeaderboards::CScoreSteamLeaderboards() :
    curr_score(0), entry_count(0), rank_loading(false), rank_online(false)
{
}

void CScoreSteamLeaderboards::FindRank(const char *pchLeaderboardName)
{
    rank_loading = true;
    curr_score   = NULL;

    SteamAPICall_t call = SteamUserStats()->FindLeaderboard(pchLeaderboardName);
    call_find_rank.Set(call, this, &CScoreSteamLeaderboards::OnFindRank);
}

bool CScoreSteamLeaderboards::UploadRank(int value)
{
    if (!curr_score) return false;

    SteamAPICall_t call = SteamUserStats()->UploadLeaderboardScore(
        curr_score,
        k_ELeaderboardUploadScoreMethodKeepBest,
        value,
        0, 0);

    call_upld_rank.Set(call, this, &CScoreSteamLeaderboards::OnUploadRank);

    return true;
}

bool CScoreSteamLeaderboards::DownloadRank()
{
    if (!curr_score) return false;

    SteamAPICall_t call = SteamUserStats()->DownloadLeaderboardEntries(
        curr_score,
        k_ELeaderboardDataRequestGlobalAroundUser,
        -4, 5);

    call_dnld_rank.Set(call, this, &CScoreSteamLeaderboards::OnDownloadRank);
}

/*---------------------------------------------------------------------------*/

void CScoreSteamLeaderboards::OnFindRank(LeaderboardFindResult_t *pResult,
                                         bool bIOFailure)
{
    rank_loading = false;
    if (!pResult->m_bLeaderboardFound || bIOFailure)
    {
        log_errorf("Failure to find Steam leaderboards!\n");
        return;
    }

    rank_online = true;
    curr_score  = pResult->m_hSteamLeaderboard;
}

void CScoreSteamLeaderboards::OnUploadRank(LeaderboardScoreUploaded_t *pResult,
                                           bool bIOFailure)
{
    rank_online = false;

    if (!pResult->m_bSuccess || bIOFailure)
        log_errorf("Failure to upload Steam leaderboards!\n");
    else rank_online = true;
}

void CScoreSteamLeaderboards::OnDownloadRank(LeaderboardScoresDownloaded_t *pResult,
                                             bool bIOFailure)
{
    rank_online = false;

    if (!bIOFailure)
    {
        entry_count = MIN(pResult->m_cEntryCount, 10);

        for (int i = 0; i < entry_count; i++)
        {
            SteamUserStats()->GetDownloadedLeaderboardEntry(
                pResult->m_hSteamLeaderboardEntries, i,
                &entry[i], 0, 0);
        }

        rank_online = true;
    }
    else
        log_errorf("Failure to download Steam leaderboards!\n");
}

/*---------------------------------------------------------------------------*/

static CScoreSteamLeaderboards g_score_online_coins;
static CScoreSteamLeaderboards g_score_online_time;

#endif

static int online_init = 0;

/*
 * TODO: Only most coins and best time in level set,
 * to upload the Steam leaderboard.
 */

#ifdef __cplusplus
extern "C"
{
#endif

void score_steam_init_hs(int hs_index, int incl_set)
{
#if NB_STEAM_API==1 && !defined(STEAM_DISABLE_HIGHSCORES)
    if (!online_init)
        online_init = 1;

    char coin_rank_name[MAXSTR], time_rank_name[MAXSTR];

    const char *curr_setname = set_name_n(hs_index);

    if (curr_setname)
    {
        if (incl_set != 0)
        {
            sprintf(coin_rank_name, "Most Coins - %s", curr_setname);
            sprintf(time_rank_name, "Best Time - %s", curr_setname;
        }
        else if (incl_set == 0)
        {
            sprintf(coin_rank_name, "Most Coins - %s", "Campaign (Hardcore)");
            sprintf(time_rank_name, "Best Time - %s", "Campaign (Hardcore)");
        }
    }

    g_score_online_coins.FindRank(coin_rank_name);
    g_score_online_time.FindRank(time_rank_name);
#endif
}

int score_steam_hs_loading(void)
{
#if NB_STEAM_API==1 && !defined(STEAM_DISABLE_HIGHSCORES)
    if (!online_init) return 0;
    return
        g_score_online_coins.rank_loading ||
        g_score_online_time.rank_loading ?
        1 : 0;
#else
    return 0;
#endif
}

int score_steam_hs_online(void)
{
#if NB_STEAM_API==1 && !defined(STEAM_DISABLE_HIGHSCORES)
    if (!online_init) return 0;
    return
        g_score_online_coins.rank_online &&
        g_score_online_time.rank_online ?
        1 : 0;
#else
    return 0;
#endif
}

void score_steam_hs_save(int coins, int time)
{
#if NB_STEAM_API==1 && !defined(STEAM_DISABLE_HIGHSCORES)
    if (!online_init) return;
    g_score_online_coins.UploadRank(coins);
    g_score_online_time.UploadRank(time);
#endif
}

#ifdef __cplusplus
}
#endif

/*---------------------------------------------------------------------------*/
