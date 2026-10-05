/*
 * Copyright (C) 2026 Microsoft / Neverball authors / Ersohn Styne
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

#ifndef GAME_SHA256
#define GAME_SHA256

#define SHA256_DIGEST_SIZE 32

/**
 * Computes the SHA-256 hash of the given input message.
 *
 * @param msg        Pointer to the input message (raw bytes).
 * @param msgLen     Length of the input message in bytes.
 * @param digest     Pointer to a buffer of at least SHA256_DIGEST_SIZE bytes.
 *                   The resulting 32-byte hash will be written here.
 *
 * @return           0 on success, non-zero on failure.
 */
int SHA256(const unsigned char *msg, size_t msgLen, unsigned char *digest);

/*---------------------------------------------------------------------------*/

#if !defined(NDEBUG) && _MSC_VER
#define GAME_SHA256_CHECK_ERROR do { __debugbreak(); return 0; } while (0)
#else
#define GAME_SHA256_CHECK_ERROR do { return 0; } while (0)
#endif

struct game_sha256_digest
{
    unsigned char balls[SHA256_DIGEST_SIZE];
    unsigned char score[SHA256_DIGEST_SIZE];
    unsigned char timer[SHA256_DIGEST_SIZE];

#if NB_HAVE_PB_BOTH==1
    unsigned char flawless_runs[SHA256_DIGEST_SIZE];
#endif
};

int  game_sha256_init (void);
void game_sha256_free (void);
int  game_sha256_state(void);

int game_sha256_compare_date(void);

int game_sha256_play(void);
int game_sha256_stat(int);
int game_sha256_same(void);

int game_sha256_check(int);

/*---------------------------------------------------------------------------*/

struct game_server_sha256_digest
{
    unsigned char timer_hold[SHA256_DIGEST_SIZE];
    unsigned char time_limit[SHA256_DIGEST_SIZE];

    unsigned char status[SHA256_DIGEST_SIZE];
    unsigned char coins [SHA256_DIGEST_SIZE];
    unsigned char goal_e[SHA256_DIGEST_SIZE];
    unsigned char jump_e[SHA256_DIGEST_SIZE];
    unsigned char jump_b[SHA256_DIGEST_SIZE];

#ifdef MAPC_INCLUDES_CHKP
    unsigned char chkp_e [SHA256_DIGEST_SIZE];
    unsigned char chkp_id[SHA256_DIGEST_SIZE];
#endif
};

#ifdef MAPC_INCLUDES_CHKP
/**
 * Initialize SHA256 game server (WGCL). Similar from `game_sha256_server_init`.
 *
 * @param curr_timer_hold Timer should be hold during init
 * @param curr_time_limit The maximum time limit during init (Must be specify only for seconds, not hundredth)
 * @param curr_status The current level status during init
 * @param curr_coins The collected coins during init
 * @param curr_goal_e The enabled goal state during init
 * @param curr_jump_e The enabled jump state during init
 * @param curr_jump_b The jump-in-progress state during init
 * @param curr_chkp_e The enabled checkpoint state during init
 * @param curr_chkp_id The checkpoint index during init
 *
 * @return 1 = success; 0 = error
 */
int  game_sha256_server_init_chkp(int, int, int, int, int, int, int, int, int);
#endif

/**
 * Initialize SHA256 game server.
 * Use `game_sha256_server_init_chkp` for the better experience!
 *
 * @param curr_timer_hold Timer should be hold during init
 * @param curr_time_limit The maximum time limit during init (Must be specify only for seconds, not hundredth)
 * @param curr_status The current level status during init
 * @param curr_coins The collected coins during init
 * @param curr_goal_e The enabled goal state during init
 * @param curr_jump_e The enabled jump state during init
 * @param curr_jump_b The jump-in-progress state during init
 *
 * @return 1 = success; 0 = error
 */
int  game_sha256_server_init(int, int, int, int, int, int, int);
void game_sha256_server_free(void);
int  game_sha256_server_state(void);

#ifdef MAPC_INCLUDES_CHKP
int game_sha256_server_check_chkp(int, int, int, int, int, int, int, int, int);
int game_sha256_server_update_chkp(int, int);
#endif

int game_sha256_server_check(int, int, int, int, int, int, int);
int game_sha256_server_update(int, int, int, int, int, int, int);

/*---------------------------------------------------------------------------*/

#endif