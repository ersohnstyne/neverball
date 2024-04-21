/*
 * Copyright (C) 2024 Microsoft / Neverball authors
 *
 * NEVERPUTT is  free software; you can redistribute  it and/or modify
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
#include <string.h>
#include <math.h>

#include "hole.h"
#include "glext.h"
#include "image.h"
#include "game.h"
#include "geom.h"
#include "hud.h"
#include "audio.h"
#include "config.h"
#include "fs.h"

/*---------------------------------------------------------------------------*/

struct hole
{
    char   file[MAXSTR];
    char   back[MAXSTR];
    char   song[MAXSTR];
    int    par;
};

static int hole;
static int party;
static int player;
static int count;
static int done;

static int         stat_v[MAXPLY];
static float       ball_p[MAXPLY][3];
static float       ball_e[MAXPLY][3][3];
static struct hole hole_v[MAXHOL];
static int        score_v[MAXHOL][MAXPLY];

static int        stroke_type_v[MAXPLY];

struct course_score course_score_v;

/*---------------------------------------------------------------------------*/

static void hole_init_rc(const char *filename)
{
    fs_file fin;
    char buff[MAXSTR];

    hole   = 0;
    player = 0;
    count  = 0;
    done   = 0;

    /* Load the holes list. */

    if ((fin = fs_open_read(filename)))
    {
        /* Skip shot and description. */

        if (fs_gets(buff, sizeof (buff), fin) &&
            fs_gets(buff, sizeof (buff), fin))
        {
            /* Read the list. */

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            while (fs_gets(buff, sizeof (buff), fin) &&
                   sscanf(buff, "%s %s %d %s",
                          hole_v[count].file,
                          hole_v[count].back,
                         &hole_v[count].par,
                          hole_v[count].song) >= 1)
#else
            while (fs_gets(buff, sizeof (buff), fin) &&
                   sscanf_s(buff, "%s %s %d %s",
                            hole_v[count].file,
                            hole_v[count].back,
                           &hole_v[count].par,
                            hole_v[count].song) >= 1)
#endif
                count++;
        }

        fs_close(fin);
    }
}

/*---------------------------------------------------------------------------*/

int hole_load(int h, const char *filename)
{
    struct s_base base;

    if (filename != hole_v[h].file)
    {
        /* Note filename if it came from elsewhere. */

        SAFECPY(hole_v[h].file, filename);
    }

    if (sol_load_meta(&base, filename))
    {
        int i;

        for (i = 0; i < base.dc; i++)
        {
            const char *k = base.av + base.dv[i].ai;
            const char *v = base.av + base.dv[i].aj;

            if      (strcmp("grad", k) == 0)
                SAFECPY(hole_v[h].back, v);
            else if (strcmp("par", k) == 0)
                hole_v[h].par = atoi(v);
            else if (strcmp("song", k) == 0)
                SAFECPY(hole_v[h].song, v);
        }

        score_v[h][0] = hole_v[h].par;

        sol_free_base(&base);
        return 1;
    }

    log_errorf("Unable to load hole in course: %s / %s\n",
               filename, fs_error());

    return 0;
}

void hole_init(const char *filename)
{
    int i;

    memset(hole_v,  0, sizeof (struct hole) * MAXHOL);
    memset(score_v, 0, sizeof (int) * MAXPLY * MAXHOL);

    course_score_init_hs(&course_score_v);

    if (filename)
    {
        hole_init_rc(filename);

        for (i = 0; i < count; i++)
            hole_load(i, hole_v[i].file);
    }
    else
    {
        /* Standalone mode.                       */
        /* Why is this 2, you ask? Good question. */

        count = 2;
    }
}

void hole_free(void)
{
    game_free();
    back_free();

    count = 0;
}

/*---------------------------------------------------------------------------*/

char *hole_player(int p)
{
    if (p == 0)               return _("Par");

    if (p == 1 && 1 <= party) return _("P1");
    if (p == 2 && 2 <= party) return _("P2");
    if (p == 3 && 3 <= party) return _("P3");
    if (p == 4 && 4 <= party) return _("P4");

    return NULL;
}

char *hole_score(int h, int p)
{
    static char str[MAXSTR];

    if (1 <= h && h <= hole)
    {
        if (h <= hole && 0 <= p && p <= party)
        {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(str, MAXSTR,
#else
            sprintf(str,
#endif
                    "%d", score_v[h][p]);

            return str;
        }
    }
    return NULL;
}

char *hole_tot(int p)
{
    static char str[MAXSTR];

    int h, T = 0;

    if (p <= party)
    {
        for (h = 1; h <= hole && h < count; h++)
            T += score_v[h][p];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(str, MAXSTR,
#else
        sprintf(str,
#endif
                "%d", T);

        return str;
    }
    return NULL;
}

char *hole_out(int p)
{
    static char str[MAXSTR];

    int h, T = 0;

    if (p <= party)
    {
        for (h = 1; h <= hole && h <= count / 2; h++)
            T += score_v[h][p];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(str, MAXSTR,
#else
        sprintf(str,
#endif
                "%d", T);

        return str;
    }
    return NULL;
}

char *hole_in(int p)
{
    static char str[MAXSTR];

    int h, T = 0;
    int out = count / 2;

    if (hole > out && p <= party)
    {
        for (h = out + 1; h <= hole && h < count; h++)
            T += score_v[h][p];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(str, MAXSTR,
#else
        sprintf(str,
#endif
                "%d", T);

        return str;
    }
    return NULL;
}

/*---------------------------------------------------------------------------*/

void stroke_set_type(int i)
{
    stroke_type_v[player] = i;
}

/*---------------------------------------------------------------------------*/

int curr_hole(void)        { return hole;   }
int curr_party(void)       { return party;  }
int curr_player(void)      { return player; }
int curr_count(void)       { return count;  }
int curr_stroke_type(void) { return stroke_type_v[player]; }

const char *curr_scr_profile(int i)
{
    static char buf[8];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(buf, 8,
#else
    sprintf(buf,
#endif
            "%d", score_v[hole][i]);

    return buf;
}

const char *curr_scr(void)
{
    static char buf[8];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(buf, 8,
#else
    sprintf(buf,
#endif
            "%d", score_v[hole][player]);

    return buf;
}

const char *curr_par(void)
{
    static char buf[8];

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(buf, 8,
#else
    sprintf(buf,
#endif
            "%d", score_v[hole][0]);

    return buf;
}

const char *curr_stroke_type_name(void)
{
    switch (stroke_type_v[player])
    {
        case 3:  return _("Driver");
        case 2:  return _("Iron");
        case 1:  return _("Wedge");
        default: return _("Putt");
    }
}

/*---------------------------------------------------------------------------*/

int hole_goto(int h, int p)
{
    int i;

    if (h < count)
    {
        if (h >= 0) hole  = h;
        if (p >= 0) party = p;

        if (game_init(hole_v[hole].file))
        {
            back_init(hole_v[hole].back);

            player = (hole - 1) % party + 1;
            done   = 0;

            for (i = 1; i <= party; i++)
            {
                game_get_pos(ball_p[i], ball_e[i]);
                stat_v[i] = 0;
            }
            game_ball(player);
            hole_song();
            return 1;
        }
    }
    return 0;
}

int hole_next(void)
{
    if (done < party)
    {
        do
        {
            player = player % party + 1;
        }
        while (stat_v[player]);

        game_ball(player);
        game_get_pos(ball_p[player], ball_e[player]);

        return 1;
    }
    return 0;
}

int hole_move(void)
{
    if (hole + 1 < count)
    {
        hole++;

        game_free();
        back_free();

        if (hole_goto(hole, party))
            return 1;
    }

    return 0;
}

void hole_goal(void)
{
    score_v[hole][player]++;

    if (score_v[hole][player] == 1)
        audio_narrator_play(AUD_ONE);

    else if (score_v[hole][player] == score_v[hole][0] - 2)
        audio_narrator_play(AUD_EAGLE);
    else if (score_v[hole][player] == score_v[hole][0] - 1)
        audio_narrator_play(AUD_BIRDIE);
    else if (score_v[hole][player] == score_v[hole][0])
        audio_narrator_play(AUD_PAR);
    else if (score_v[hole][player] == score_v[hole][0] + 1)
        audio_narrator_play(AUD_BOGEY);
    else if (score_v[hole][player] == score_v[hole][0] + 2)
        audio_narrator_play(AUD_DOUBLE);
    else
        audio_narrator_play(AUD_SUCCESS);

    stat_v[player] = 1;
    done++;

    course_score_insert(&course_score_v, player);

    if (done == party)
        audio_music_fade_out(2.0f);
}

void hole_stop(void)
{
    /* Default stroke limit for open-world: 65536 */

    int stroke_limit = score_v[hole][0] + 4;
    score_v[hole][player]++;

    /* Cap scores at specific stroke limit or par plus 3. */

    if (score_v[hole][player] >= stroke_limit &&
        score_v[hole][player] >= score_v[hole][0] + 3)
    {
        score_v[hole][player] = (score_v[hole][0] > stroke_limit - 3) ? score_v[hole][0] + 3 : stroke_limit;
        stat_v[player] = 1;
        done++;

        course_score_insert(&course_score_v, player);
    }
}

void hole_skip(void)
{
    /* Default stroke limit for open-world: 65536 */

    int stroke_limit = score_v[hole][0] + 4;

    /* Cap scores at specific stroke limit or par plus 3. */

    int maxscore = (score_v[hole][0] > stroke_limit - 3) ? score_v[hole][0] + 3 : stroke_limit;
    
    for (int i = 1; i <= party; i++)
    {
        score_v[hole][i] = maxscore;
        stat_v[i] = 1;

        course_score_insert(&course_score_v, i);
    }

    done = party;
}

void hole_fall(int split)
{
    int stroke_limit = score_v[hole][0] + 4;
    audio_narrator_play(AUD_PENALTY);

    /* Reset to the position of the putt, and apply a one-stroke penalty. */

    game_set_pos(ball_p[player], ball_e[player]);
    score_v[hole][player] += split ? 1 : 2;

    /* Cap scores at specific stroke limit or par plus 3. */

    if (score_v[hole][player] >= stroke_limit &&
        score_v[hole][player] >= score_v[hole][0] + 3)
    {
        score_v[hole][player] = (score_v[hole][0] > stroke_limit - 3) ? score_v[hole][0] + 3 : stroke_limit;
        stat_v[player] = 1;
        done++;

        course_score_insert(&course_score_v, player);
    }
}

int  hole_retry_avail(int split)
{
    int stroke_limit = score_v[hole][0] + 4;

    return score_v[hole][player] + split ? 0 : 2 <= stroke_limit;
}

void hole_retry(int split)
{
    int stroke_limit = score_v[hole][0] + 4;
    audio_play(AUD_RETRY, 1.f);

    /* Reset to the position of the putt, and apply a one-stroke penalty. */

    game_set_pos(ball_p[player], ball_e[player]);
    score_v[hole][player] += split ? 0 : 2;

    /* Cap scores at specific stroke limit or par plus 3. */

    if (score_v[hole][player] >= stroke_limit &&
        score_v[hole][player] >= score_v[hole][0] + 3)
    {
        score_v[hole][player] = (score_v[hole][0] > stroke_limit - 3) ? score_v[hole][0] + 3 : stroke_limit;
        stat_v[player] = 1;
        done++;

        course_score_insert(&course_score_v, player);
    }
}

void hole_restart(void)
{
    memset(&score_v[hole][1], 0, sizeof(int) * party);
    hole_goto(hole, party);
}

/*---------------------------------------------------------------------------*/

void hole_song(void)
{
    audio_music_fade_to(0.5f, _(hole_v[hole].song), 1);
}

/*---------------------------------------------------------------------------*/
