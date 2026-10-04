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

#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "log.h"
#include "progress.h"

#include "game_sha256.h"

#include "game_common.h"
#include "game_client.h"

/*
 * Experience the encrypted gameplay progress data values one more time!
 * Source: https://github.com/CurryB0i/SHA-256-in-C
 */

/*---------------------------------------------------------------------------*/

static void padMsg(unsigned char *paddedBinaryMsg, const unsigned char *binary, int len, int padLen) {
    memcpy(paddedBinaryMsg, binary, len * sizeof (unsigned char));
    paddedBinaryMsg[len++] = 0x80;
    while (padLen > 0) {
        paddedBinaryMsg[len++] = 0;
        padLen--;
    }
}

static void intToBin(unsigned char *paddedBinaryMsg, int offset, uint64_t len) {
    for (size_t i = 0; i < 8; i++) {
        paddedBinaryMsg[offset + i] = (len >> (56 - 8 * i)) & 0xFF;
    }
}

static int getBlocks(unsigned char **blocks, int noOfBlocks, unsigned char *binMsg, int msgLen) {
    for (int i = 0; i < noOfBlocks; i++) {
        blocks[i] = malloc(64 * sizeof (unsigned char));
        if (blocks[i] == NULL) {
            log_errorf("Memory allocation failed\n");
            free(binMsg);
            return 1;
        }

        memcpy(blocks[i], binMsg + 64 * i, 64 * sizeof (unsigned char));
    }
    return 0;
}

static uint32_t rightRotate(const uint32_t word, int offset) {
    uint32_t temp = word;
    offset = offset % 32;
    return (temp >> offset) | (temp << (32 - offset));
}

static void hash(unsigned char **blocks, int noOfBlocks, unsigned char *hashedMsg) {
    uint32_t H[] = {
        0x6a09e667,
        0xbb67ae85,
        0x3c6ef372,
        0xa54ff53a,
        0x510e527f,
        0x9b05688c,
        0x1f83d9ab,
        0x5be0cd19
    };

    static uint32_t K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    for (int i = 0; i < noOfBlocks; i++) {
        uint32_t W[64];

        for (size_t j = 0; j < 16; j++) {
            W[j] = (((uint32_t) blocks[i][j * 4] << 24) |
                    ((uint32_t) blocks[i][j * 4 + 1] << 16) |
                    ((uint32_t) blocks[i][j * 4 + 2] << 8) |
                    ((uint32_t) blocks[i][j * 4 + 3]));
        }

        for (size_t j = 16; j < 64; j++) {
            uint32_t s0 = rightRotate(W[j - 15], 7) ^ rightRotate(W[j - 15], 18) ^ (W[j - 15] >>  3);
            uint32_t s1 = rightRotate(W[j - 2], 17) ^ rightRotate(W[j - 2],  19) ^ (W[j - 2]  >> 10);
            W[j] = W[j - 16] + s0 + W[j - 7] + s1;
        }

        uint32_t a = H[0];
        uint32_t b = H[1];
        uint32_t c = H[2];
        uint32_t d = H[3];
        uint32_t e = H[4];
        uint32_t f = H[5];
        uint32_t g = H[6];
        uint32_t h = H[7];

        uint32_t S1, ch, temp1, S0, maj, temp2;
        for (size_t j = 0; j < 64; j++) {
            S1 = rightRotate(e, 6) ^ rightRotate(e, 11) ^ rightRotate(e, 25);
            ch = (e & f) ^ (~e & g);
            temp1 = h + S1 + ch + K[j] + W[j];
            S0 = rightRotate(a, 2) ^ rightRotate(a, 13) ^ rightRotate(a, 22);
            maj = (a & b) ^ (a & c) ^ (b & c);
            temp2 = S0 + maj;

            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        H[0] = H[0] + a;
        H[1] = H[1] + b;
        H[2] = H[2] + c;
        H[3] = H[3] + d;
        H[4] = H[4] + e;
        H[5] = H[5] + f;
        H[6] = H[6] + g;
        H[7] = H[7] + h;
    }

    for (int i = 0; i < 8; i++) {
        hashedMsg[i * 4 + 0] = (H[i] >> 24) & 0xFF;
        hashedMsg[i * 4 + 1] = (H[i] >> 16) & 0xFF;
        hashedMsg[i * 4 + 2] = (H[i] >>  8) & 0xFF;
        hashedMsg[i * 4 + 3] = (H[i] >>  0) & 0xFF;
    }
}

int SHA256(const unsigned char *msg, size_t msgLen, unsigned char *digest) {
    unsigned char *binary = malloc(msgLen * sizeof (unsigned char));
    if (binary == NULL) {
        log_errorf("Memory allocation failed\n");
        return 1;
    }
    memcpy(binary, msg, msgLen);

    int padLen = 0;
    if (msgLen % 64 < 56) {
        padLen = 56 - (msgLen % 64);
    }
    else {
        padLen = 64 + 56 - (msgLen % 64);
    }

    int paddedMsgLen = msgLen + padLen + 8;
    unsigned char *paddedBinaryMsg = malloc(paddedMsgLen * sizeof (unsigned char));
    if (paddedBinaryMsg == NULL) {
        log_errorf("Memory allocation failed\n");
        if (binary) { free(binary); binary = NULL; }
        return 1;
    }

    padMsg(paddedBinaryMsg, binary, msgLen, padLen);
    intToBin(paddedBinaryMsg, msgLen + padLen, (uint64_t) msgLen * 8);
    if (binary) { free(binary); binary = NULL; }

    int noOfBlocks = paddedMsgLen / 64;
    unsigned char **blocks = malloc(noOfBlocks * sizeof (unsigned char *));
    if (blocks == NULL) {
        log_errorf("Memory allocation failed\n");
        if (paddedBinaryMsg) { free(paddedBinaryMsg); paddedBinaryMsg = NULL; }
        return 1;
    }
    int status = getBlocks(blocks, noOfBlocks, paddedBinaryMsg, paddedMsgLen);
    if (status != 0) {
        log_errorf("Failed to create blocks.\n");
        if (paddedBinaryMsg) { free(paddedBinaryMsg); paddedBinaryMsg = NULL; }
        return 1;
    }
    if (paddedBinaryMsg) { free(paddedBinaryMsg); paddedBinaryMsg = NULL; }

    unsigned char *hashedMsg = malloc(64 * sizeof (unsigned char));
    hash(blocks, noOfBlocks, hashedMsg);
#if _WIN32 && _MSC_VER
    memcpy_s(digest, 32, hashedMsg, 32);
#else
    memcpy(digest, hashedMsg, 32);
#endif

    for (size_t i = 0; i < noOfBlocks; i++) {
        if (blocks[i]) { free(blocks[i]); blocks[i] = NULL; }
    }
    if (blocks) { free(blocks); blocks = NULL; }
    if (hashedMsg) { free(hashedMsg); hashedMsg = NULL; }
    return 0;
}

/*---------------------------------------------------------------------------*/

static int sha256_state;

static char sha256_digest_date[SHA256_DIGEST_SIZE];

static struct game_sha256_digest sha256_curr, sha256_prev;

static int game_sha256_update_digest(void)
{
    char in_raw_curr_balls[256]
       , in_raw_curr_score[256]
       , in_raw_curr_timer[256]
       ;

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(in_raw_curr_balls, sizeof (in_raw_curr_balls),
#else
    sprintf(in_raw_curr_balls,
#endif
              "CHALLENGE_BALLS:%d", curr_balls());

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(in_raw_curr_score, sizeof (in_raw_curr_score),
#else
    sprintf(in_raw_curr_score,
#endif
              "CHALLENGE_SCORE:%d", curr_score());

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(in_raw_curr_timer, sizeof (in_raw_curr_timer),
#else
    sprintf(in_raw_curr_timer,
#endif
              "CHALLENGE_TIMER:%d", curr_times());

    if (SHA256((const unsigned char *) in_raw_curr_balls, strlen(in_raw_curr_balls), sha256_curr.balls) != 0) {
        log_errorf("Hashing failed!\n");
        game_sha256_free();
        return (sha256_state = 0);
    }

    if (SHA256((const unsigned char *) in_raw_curr_score, strlen(in_raw_curr_score), sha256_curr.score) != 0) {
        log_errorf("Hashing failed!\n");
        game_sha256_free();
        return (sha256_state = 0);
    }

    if (SHA256((const unsigned char *) in_raw_curr_timer, strlen(in_raw_curr_timer), sha256_curr.timer) != 0) {
        log_errorf("Hashing failed!\n");
        game_sha256_free();
        return (sha256_state = 0);
    }

    return 1;
}

int game_sha256_init(void)
{
    game_sha256_free();

    time_t     wgcl_time_now;
    struct tm *wgcl_utc_time;
    time(&wgcl_time_now);
    wgcl_utc_time = gmtime(&wgcl_time_now);

    char in_raw_date_data[256];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(in_raw_date_data, sizeof (in_raw_date_data),
#else
    sprintf(in_raw_date_data,
#endif
              "CHALLENGE_DATE:%04d-%02d-%02d",
              wgcl_utc_time->tm_year + 1900,
              wgcl_utc_time->tm_mon  + 1,
              wgcl_utc_time->tm_mday);

    if (SHA256((const unsigned char *) in_raw_date_data, strlen(in_raw_date_data), sha256_digest_date) != 0) {
        log_errorf("Hashing failed!\n");
        return (sha256_state = 0);
    }

    if (!game_sha256_update_digest()) {
        game_sha256_free();
        return (sha256_state = 0);
    }

#if NB_HAVE_PB_BOTH==1
    char in_raw_curr_flawless_runs[256];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(in_raw_curr_flawless_runs, sizeof (in_raw_curr_flawless_runs),
#else
    sprintf(in_raw_curr_flawless_runs,
#endif
              "CHALLENGE_FLAWLESS_RUNS:%d", 1);

    if (SHA256((const unsigned char *) in_raw_curr_flawless_runs, strlen(in_raw_curr_flawless_runs), sha256_curr.flawless_runs) != 0) {
        log_errorf("Hashing failed!\n");
        game_sha256_free();
        return (sha256_state = 0);
    }
#endif

    return (sha256_state = 1);
}

void game_sha256_free(void)
{
    if (!sha256_state) return;

    memset(&sha256_curr,       0, sizeof (sha256_curr));
    memset(&sha256_prev,       0, sizeof (sha256_prev));
    memset(sha256_digest_date, 0, sizeof (sha256_digest_date));

    sha256_state = 0;
}

int game_sha256_state(void)
{
    return sha256_state;
}

int game_sha256_compare_date(void)
{
    if (!sha256_state) return 0;

    time_t     wgcl_time_now;
    struct tm *wgcl_utc_time;
    time(&wgcl_time_now);
    wgcl_utc_time = gmtime(&wgcl_time_now);

    char in_raw_date_data[256];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(in_raw_date_data, sizeof (in_raw_date_data),
#else
    sprintf(in_raw_date_data,
#endif
              "CHALLENGE_DATE:%04d-%02d-%02d",
              wgcl_utc_time->tm_year + 1900,
              wgcl_utc_time->tm_mon  + 1,
              wgcl_utc_time->tm_mday);

    char sha256_client_digest_date[SHA256_DIGEST_SIZE];

    if (SHA256((const unsigned char *) in_raw_date_data, strlen(in_raw_date_data), sha256_client_digest_date) != 0) {
        log_errorf("Hashing failed!\n");
        game_sha256_free();
        return (sha256_state = 0);
    }

    if (strcmp(sha256_client_digest_date, sha256_digest_date) != 0) {
        log_errorf("No match (Datetime Checksum)!\n");
        game_sha256_free();
        return (sha256_state = 0);
    }

    return 1;
}

int game_sha256_play(void)
{
    if (!sha256_state) return 0;

    memcpy(sha256_prev.balls, sha256_curr.balls, sizeof (sha256_curr.balls));
    memcpy(sha256_prev.score, sha256_curr.score, sizeof (sha256_curr.score));
    memcpy(sha256_prev.timer, sha256_curr.timer, sizeof (sha256_curr.timer));

#if NB_HAVE_PB_BOTH==1
    memcpy(sha256_prev.flawless_runs, sha256_curr.flawless_runs, sizeof (sha256_curr.flawless_runs));
#endif

    return 1;
}

int game_sha256_stat(int s)
{
    if (!sha256_state) return 0;

    if (!game_sha256_update_digest())
    {
        game_sha256_free();
        return (sha256_state = 0);
    }

    return 1;
}

int game_sha256_same(void)
{
    if (!sha256_state) return 0;

    if (curr_status() == GAME_GOAL)
    {
        memcpy(sha256_curr.balls, sha256_prev.balls, sizeof (sha256_prev.balls));
        memcpy(sha256_curr.score, sha256_prev.score, sizeof (sha256_prev.score));
        memcpy(sha256_curr.timer, sha256_prev.timer, sizeof (sha256_prev.timer));

#if NB_HAVE_PB_BOTH==1
        memcpy(sha256_curr.flawless_runs, sha256_prev.flawless_runs, sizeof (sha256_prev.flawless_runs));
#endif
    }
    else
    {
#if NB_HAVE_PB_BOTH==1
        char in_raw_curr_flawless_runs[256];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(in_raw_curr_flawless_runs, sizeof (in_raw_curr_flawless_runs),
#else
        sprintf(in_raw_curr_flawless_runs,
#endif
                  "CHALLENGE_FLAWLESS_RUNS:%d", 0);

        if (SHA256((const unsigned char *) in_raw_curr_flawless_runs, strlen(in_raw_curr_flawless_runs), sha256_curr.flawless_runs) != 0) {
            log_errorf("Hashing failed!\n");
            game_sha256_free();
            return (sha256_state = 0);
        }
#endif

        if (!game_sha256_update_digest())
        {
            game_sha256_free();
            return (sha256_state = 0);
        }
    }

    return 1;
}

int game_sha256_check(int curr_flawless)
{
    if (!sha256_state) return 0;

    char in_raw_curr_balls[256]
       , in_raw_curr_score[256]
       , in_raw_curr_timer[256]
#if NB_HAVE_PB_BOTH==1
       , in_raw_curr_flawless_runs[256]
#endif
       ;

    unsigned char sha256_client_digest_balls[SHA256_DIGEST_SIZE]
                , sha256_client_digest_score[SHA256_DIGEST_SIZE]
                , sha256_client_digest_timer[SHA256_DIGEST_SIZE]
#if NB_HAVE_PB_BOTH==1
                , sha256_client_digest_flawless_runs[SHA256_DIGEST_SIZE]
#endif
                ;

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(in_raw_curr_balls, sizeof (in_raw_curr_balls),
#else
    sprintf(in_raw_curr_balls,
#endif
              "CHALLENGE_BALLS:%d", curr_balls());

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(in_raw_curr_score, sizeof (in_raw_curr_score),
#else
    sprintf(in_raw_curr_score,
#endif
              "CHALLENGE_SCORE:%d", curr_score());

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(in_raw_curr_timer, sizeof (in_raw_curr_timer),
#else
    sprintf(in_raw_curr_timer,
#endif
              "CHALLENGE_TIMER:%d", curr_times());

#if NB_HAVE_PB_BOTH==1
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(in_raw_curr_flawless_runs, sizeof (in_raw_curr_flawless_runs),
#else
    sprintf(in_raw_curr_flawless_runs,
#endif
              "CHALLENGE_FLAWLESS_RUNS:%d", curr_flawless);
#endif
    
    if (SHA256((const unsigned char *) in_raw_curr_balls, strlen(in_raw_curr_balls), sha256_client_digest_balls) != 0) {
        log_errorf("Hashing failed!\n");
        game_sha256_free();
        return (sha256_state = 0);
    } else for (int i = 0; i < 32; i++) {
        if (sha256_client_digest_balls[i] != sha256_curr.balls[i]) {
            log_errorf("Compare checksum failed!: Currrent (sha256_client_digest_balls): %s; Expected (sha256_curr.balls): %s\n",
                       sha256_client_digest_balls, sha256_curr.balls);
            game_sha256_free();
            return (sha256_state = 0);
        }
    }

    if (SHA256((const unsigned char *) in_raw_curr_score, strlen(in_raw_curr_score), sha256_client_digest_score) != 0) {
        log_errorf("Hashing failed!\n");
        game_sha256_free();
        return (sha256_state = 0);
    } else for (int i = 0; i < 32; i++) {
        if (sha256_client_digest_score[i] != sha256_curr.score[i]) {
            log_errorf("Compare checksum failed!: Currrent (sha256_client_digest_score): %s; Expected (sha256_curr.score): %s\n",
                       sha256_client_digest_score, sha256_curr.score);
            game_sha256_free();
            return (sha256_state = 0);
        }
    }

    if (SHA256((const unsigned char *) in_raw_curr_timer, strlen(in_raw_curr_timer), sha256_client_digest_timer) != 0) {
        log_errorf("Hashing failed!\n");
        game_sha256_free();
        return (sha256_state = 0);
    } else for (int i = 0; i < 32; i++) {
        if (sha256_client_digest_timer[i] != sha256_curr.timer[i]) {
            log_errorf("Compare checksum failed!: Currrent (sha256_client_digest_timer): %s; Expected (sha256_curr.timer): %s\n",
                       sha256_client_digest_timer, sha256_curr.timer);
            game_sha256_free();
            return (sha256_state = 0);
        }
    }

#if NB_HAVE_PB_BOTH==1
    if (SHA256((const unsigned char *) in_raw_curr_flawless_runs, strlen(in_raw_curr_flawless_runs), sha256_client_digest_flawless_runs) != 0) {
        log_errorf("Hashing failed!\n");
        game_sha256_free();
        return (sha256_state = 0);
    } else for (int i = 0; i < 32; i++) {
        if (sha256_client_digest_flawless_runs[i] != sha256_curr.flawless_runs[i]) {
            log_errorf("Compare checksum failed!: Currrent (sha256_client_digest_flawless_runs): %s; Expected (sha256_curr.flawless_runs): %s\n",
                       sha256_client_digest_flawless_runs, sha256_curr.flawless_runs);
            game_sha256_free();
            return (sha256_state = 0);
        }
    }
#endif

    return 1;
}
