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

#include <ctype.h>
#include <string.h>
#include <math.h>

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

#include "gui.h"
#include "lang.h"
#include "util.h"
#include "config.h"

/*---------------------------------------------------------------------------*/

static int is_special_name(const char *n)
{
    return (strcmp(n, N_("Hard"))   == 0 ||
            strcmp(n, N_("Medium")) == 0 ||
            strcmp(n, N_("Easy"))   == 0);
}

/*---------------------------------------------------------------------------*/

static int coin_btn_id;
static int time_btn_id;
static int goal_btn_id;

static void set_score_color(int id, int hi,
                            const GLubyte *c0,
                            const GLubyte *c1)
{
    if (hi >= RANK_HARD)
    {
        if (hi < RANK_LAST)
            gui_set_color(id, c0, c0);
        else
            gui_set_color(id, c1, c1);
    }
}

/*---------------------------------------------------------------------------*/

static int score_label;

static int score_coin[4];
static int score_name[4];
static int score_time[4];

struct game_stats {
    int completed;
    int timeout;
    int fallout;
    int clear_rate;
} stats_labels;

static int score_extra_row, stats_extra_row;

/* Build a top three score list with default values. */

static void gui_scores(int id, int e)
{
    const char *s = "1234567";

    int j, jd, kd, ld;

    score_extra_row = e;
    stats_extra_row = e;

    if ((jd = gui_vstack(id)))
    {
        if ((kd = gui_vstack(jd)))
        {
            score_label = gui_label(kd, _("Unavailable"), GUI_SML, 0, 0);

            for (j = RANK_HARD; j < RANK_LAST; j++)
                if ((ld = gui_hstack(kd)))
                {
                    score_coin[j] = gui_count(ld, 1000, GUI_SML);
                    score_name[j] = gui_label(ld, s, GUI_SML, gui_yel, gui_wht);
                    score_time[j] = gui_clock(ld, 359999, GUI_SML);

                    gui_set_trunc(score_name[j], TRUNC_TAIL);
                    gui_set_fill (score_name[j]);
                }

            gui_set_rect(kd, GUI_ALL);
        }

        if (e)
        {
            gui_space(jd);

            if ((kd = gui_hstack(jd)))
            {
                j = RANK_LAST;

                score_coin[j] = gui_count(kd, 1000, GUI_SML);
                score_name[j] = gui_label(kd, s, GUI_SML, gui_yel, gui_wht);
                score_time[j] = gui_clock(kd, 359999, GUI_SML);

                gui_set_trunc(score_name[j], TRUNC_TAIL);
                gui_set_fill (score_name[j]);

                gui_set_rect(kd, GUI_ALL);
            }
        }
    }
}

static void gui_stats(int id)
{
    int at;

    if ((at = gui_vstack(id)))
    {
        gui_filler(at);
        gui_label(at, _("Stats"), GUI_SML, 0, 0);

        stats_labels.completed = gui_label(at, "XXXXXXXXXXX",
                                               GUI_SML, gui_grn, gui_wht);
        stats_labels.timeout   = gui_label(at, "XXXXXXXXXXX",
                                               GUI_SML, gui_yel, gui_wht);
        stats_labels.fallout   = gui_label(at, "XXXXXXXXXXX",
                                               GUI_SML, gui_red, gui_wht);

        gui_set_label(stats_labels.completed, " ");
        gui_set_label(stats_labels.timeout,   " ");
        gui_set_label(stats_labels.fallout,   " ");

        if (stats_extra_row)
        {
            stats_labels.clear_rate = gui_label(at, "XXXXXXXXXXX",
                                                    GUI_SML, GUI_COLOR_RED);

            gui_set_label(stats_labels.clear_rate, " ");
        }

        gui_set_rect(at, GUI_ALL);
        gui_filler(at);
    }
}

/* Set the top three score list values. */

static void gui_set_scores(const char *label, const struct score *s, int hilite)
{
    const char *name;
    int j, n = score_extra_row ? RANK_LAST : RANK_EASY;

    if (s == NULL)
    {
        gui_set_label(score_label, _("Unavailable"));

        for (j = RANK_HARD; j <= n; j++)
        {
            gui_set_count(score_coin[j], -1);
            gui_set_label(score_name[j], "");
            gui_set_clock(score_time[j], -1);
        }
    }
    else
    {
        gui_set_label(score_label, label);

        for (j = RANK_HARD; j <= n; j++)
        {
            name = s->player[j];

            if (j == hilite)
                set_score_color(score_name[j], j, gui_grn, gui_red);
            else
                gui_set_color(score_name[j], gui_yel, gui_wht);

            gui_set_count(score_coin[j], s->coins[j]);
            gui_set_label(score_name[j], is_special_name(name) ? _(name) :
                                                                   name);
            gui_set_clock(score_time[j], s->timer[j]);
        }
    }
}

/*---------------------------------------------------------------------------*/

static int score_type = GUI_SCORE_COIN;

void gui_score_board(int pd, unsigned int types, int e, int h)
{
    int id, jd, kd;

#ifndef NDEBUG
    assert((types & GUI_SCORE_COIN) ||
           (types & GUI_SCORE_TIME) ||
           (types & GUI_SCORE_GOAL) );
#endif

    /* Make sure current score type matches the spec. */

    while (!(types & score_type))
        score_type = GUI_SCORE_NEXT(score_type);

    if ((id = gui_hstack(pd)))
    {
        gui_filler(id);

        if ((jd = gui_vstack(id)))
        {
            gui_filler(jd);

            if (types & GUI_SCORE_COIN)
            {
                coin_btn_id = gui_state(jd, _("Most Coins"), GUI_SML,
                                        GUI_SCORE, GUI_SCORE_COIN);

                gui_set_hilite(coin_btn_id, score_type == GUI_SCORE_COIN);
            }
            if (types & GUI_SCORE_TIME)
            {
                time_btn_id = gui_state(jd, _("Best Time"), GUI_SML,
                                        GUI_SCORE, GUI_SCORE_TIME);

                gui_set_hilite(time_btn_id, score_type == GUI_SCORE_TIME);
            }
            if (types & GUI_SCORE_GOAL)
            {
                goal_btn_id = gui_state(jd, _("Fast Unlock"), GUI_SML,
                                        GUI_SCORE, GUI_SCORE_GOAL);

                gui_set_hilite(goal_btn_id, score_type == GUI_SCORE_GOAL);
            }

#if NB_STEAM_API==0 && NB_EOS_SDK==0
            if (h)
            {
                gui_space(jd);

                if ((kd = gui_hstack(jd)))
                {
                    gui_filler(kd);
                    gui_state(kd, _("Change Name"), GUI_SML, GUI_NAME, 0);
                    gui_filler(kd);
                }
            }
#endif

            gui_filler(jd);
        }

        gui_filler(id);
        gui_space(id);

        gui_scores(id, e);

        if ((types & GUI_SCORE_TIME) &&
            (types & GUI_SCORE_GOAL))
        {
            gui_space(id);
            gui_filler(id);
            gui_stats(id);
        }

        gui_filler(id);
    }
}

void set_score_board(const struct score *sc, int hc,
                     const struct score *st, int ht,
                     const struct score *sg, int hg)
{
    switch (score_type)
    {
        case GUI_SCORE_COIN:
            gui_set_scores(_("Most Coins"), sc, hc);
            break;

        case GUI_SCORE_TIME:
            gui_set_scores(_("Best Time"), st, ht);
            break;

        case GUI_SCORE_GOAL:
            gui_set_scores(_("Fast Unlock"), sg, hg);
            break;
    }

    set_score_color(coin_btn_id, hc, gui_grn, gui_wht);
    set_score_color(time_btn_id, ht, gui_grn, gui_wht);
    set_score_color(goal_btn_id, hg, gui_grn, gui_wht);

    gui_set_hilite(coin_btn_id, (score_type == GUI_SCORE_COIN));
    gui_set_hilite(time_btn_id, (score_type == GUI_SCORE_TIME));
    gui_set_hilite(goal_btn_id, (score_type == GUI_SCORE_GOAL));
}

void gui_score_set(int t)
{
    score_type = t;
}

int  gui_score_get(void)
{
    return score_type;
}

void gui_campaign_stats(const struct level *l)
{
    gui_levelgroup_stats(l);
}

void gui_set_stats(const struct level *l)
{
    gui_levelgroup_stats(l);
}

void gui_levelgroup_stats(const struct level *l)
{
    char buffer[4][12];

    /* Calculate the clear rate per levels. */

    const int total_attempts         = l->stats.completed + l->stats.timeout + l->stats.fallout;
    const int total_attempts_cleared = l->stats.completed;

    float clr_rate_val = 0.0f;
    if (total_attempts >= 1)
        clr_rate_val = ROUND((total_attempts_cleared / total_attempts) * 10000.0f) / 100.0f;

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    sprintf_s(buffer[0], 12, "%d",   l->stats.completed);
    sprintf_s(buffer[1], 12, "%d",   l->stats.timeout);
    sprintf_s(buffer[2], 12, "%d",   l->stats.fallout);
    sprintf_s(buffer[3], 12, "%.2f%%", CLAMP(0, clr_rate_val, 100));
#else
    sprintf(buffer[0], "%d",   l->stats.completed);
    sprintf(buffer[1], "%d",   l->stats.timeout);
    sprintf(buffer[2], "%d",   l->stats.fallout);
    sprintf(buffer[3], "%.2f%%", CLAMP(0, clr_rate_val, 100));
#endif

    gui_set_label(stats_labels.completed, buffer[0]);
    gui_set_label(stats_labels.timeout,   buffer[1]);
    gui_set_label(stats_labels.fallout,   buffer[2]);

    if (stats_extra_row)
    {
        gui_set_label(stats_labels.clear_rate, buffer[3]);

        if      (clr_rate_val < 5)
            gui_set_color(stats_labels.clear_rate, gui_red, gui_blk);
        else if (clr_rate_val < 10)
            gui_set_color(stats_labels.clear_rate, gui_red, gui_gry);
        else if (clr_rate_val < 25)
            gui_set_color(stats_labels.clear_rate, GUI_COLOR_RED);
        else if (clr_rate_val < 50)
            gui_set_color(stats_labels.clear_rate, GUI_COLOR_YEL);
        else if (clr_rate_val < 75)
            gui_set_color(stats_labels.clear_rate, GUI_COLOR_GRN);
        else if (clr_rate_val < 85)
            gui_set_color(stats_labels.clear_rate, GUI_COLOR_CYA);
        else
            gui_set_color(stats_labels.clear_rate, gui_wht, gui_cya);

        if (l->stats.completed == 0 &&
            l->stats.timeout   == 0 &&
            l->stats.fallout   == 0)
        {
            gui_set_label(stats_labels.clear_rate, "---.-- %");
            gui_set_color(stats_labels.clear_rate, gui_red, gui_blk);
        }
        else gui_set_label(stats_labels.clear_rate, buffer[3]);
    }
}

/*---------------------------------------------------------------------------*/

static int lock = 1;
static int keyd[127];
static int keyd_en[127];
static int keyd_de[256];

/* Generic Keyboards */
void gui_keyboard(int id)
{
    int jd, kd, ld;

    lock = 1;

    if ((jd = gui_hstack(id)))
    {
        gui_filler(jd);

        if ((kd = gui_vstack(jd)))
        {
            if ((ld = gui_hstack(kd)))
            {
                gui_filler(ld);
                keyd['9'] = gui_state(ld, "9", GUI_SML, GUI_CHAR, '9');
                keyd['8'] = gui_state(ld, "8", GUI_SML, GUI_CHAR, '8');
                keyd['7'] = gui_state(ld, "7", GUI_SML, GUI_CHAR, '7');
                keyd['6'] = gui_state(ld, "6", GUI_SML, GUI_CHAR, '6');
                keyd['5'] = gui_state(ld, "5", GUI_SML, GUI_CHAR, '5');
                keyd['4'] = gui_state(ld, "4", GUI_SML, GUI_CHAR, '4');
                keyd['3'] = gui_state(ld, "3", GUI_SML, GUI_CHAR, '3');
                keyd['2'] = gui_state(ld, "2", GUI_SML, GUI_CHAR, '2');
                keyd['1'] = gui_state(ld, "1", GUI_SML, GUI_CHAR, '1');
                keyd['0'] = gui_state(ld, "0", GUI_SML, GUI_CHAR, '0');
                gui_filler(ld);
            }
            if ((ld = gui_hstack(kd)))
            {
                gui_filler(ld);
                keyd['J'] = gui_state(ld, "J", GUI_SML, GUI_CHAR, 'J');
                keyd['I'] = gui_state(ld, "I", GUI_SML, GUI_CHAR, 'I');
                keyd['H'] = gui_state(ld, "H", GUI_SML, GUI_CHAR, 'H');
                keyd['G'] = gui_state(ld, "G", GUI_SML, GUI_CHAR, 'G');
                keyd['F'] = gui_state(ld, "F", GUI_SML, GUI_CHAR, 'F');
                keyd['E'] = gui_state(ld, "E", GUI_SML, GUI_CHAR, 'E');
                keyd['D'] = gui_state(ld, "D", GUI_SML, GUI_CHAR, 'D');
                keyd['C'] = gui_state(ld, "C", GUI_SML, GUI_CHAR, 'C');
                keyd['B'] = gui_state(ld, "B", GUI_SML, GUI_CHAR, 'B');
                keyd['A'] = gui_state(ld, "A", GUI_SML, GUI_CHAR, 'A');
                gui_filler(ld);
            }
            if ((ld = gui_hstack(kd)))
            {
                gui_filler(ld);
                keyd['T'] = gui_state(ld, "T", GUI_SML, GUI_CHAR, 'T');
                keyd['S'] = gui_state(ld, "S", GUI_SML, GUI_CHAR, 'S');
                keyd['R'] = gui_state(ld, "R", GUI_SML, GUI_CHAR, 'R');
                keyd['Q'] = gui_state(ld, "Q", GUI_SML, GUI_CHAR, 'Q');
                keyd['P'] = gui_state(ld, "P", GUI_SML, GUI_CHAR, 'P');
                keyd['O'] = gui_state(ld, "O", GUI_SML, GUI_CHAR, 'O');
                keyd['N'] = gui_state(ld, "N", GUI_SML, GUI_CHAR, 'N');
                keyd['M'] = gui_state(ld, "M", GUI_SML, GUI_CHAR, 'M');
                keyd['L'] = gui_state(ld, "L", GUI_SML, GUI_CHAR, 'L');
                keyd['K'] = gui_state(ld, "K", GUI_SML, GUI_CHAR, 'K');
                gui_filler(ld);
            }
            if ((ld = gui_hstack(kd)))
            {
                gui_filler(ld);
                const int bksp_id = gui_state(ld, GUI_TRIANGLE_LEFT, GUI_SML, GUI_BS, 0);
                keyd['Z'] = gui_state(ld, "Z", GUI_SML, GUI_CHAR, 'Z');
                keyd['Y'] = gui_state(ld, "Y", GUI_SML, GUI_CHAR, 'Y');
                keyd['X'] = gui_state(ld, "X", GUI_SML, GUI_CHAR, 'X');
                keyd['W'] = gui_state(ld, "W", GUI_SML, GUI_CHAR, 'W');
                keyd['V'] = gui_state(ld, "V", GUI_SML, GUI_CHAR, 'V');
                keyd['U'] = gui_state(ld, "U", GUI_SML, GUI_CHAR, 'U');
                gui_state(ld, _("caps"), GUI_SML, GUI_CL, 0);
                gui_filler(ld);

                gui_set_font(bksp_id, "ttf/DejaVuSans-Bold.ttf");
            }
        }
        gui_filler(jd);
    }
}

/* Generic Keyboards */
void gui_keyboard_lock(void)
{
    lock = lock ? 0 : 1;

    gui_set_label(keyd['A'], lock ? "A" : "a");
    gui_set_label(keyd['B'], lock ? "B" : "b");
    gui_set_label(keyd['C'], lock ? "C" : "c");
    gui_set_label(keyd['D'], lock ? "D" : "d");
    gui_set_label(keyd['E'], lock ? "E" : "e");
    gui_set_label(keyd['F'], lock ? "F" : "f");
    gui_set_label(keyd['G'], lock ? "G" : "g");
    gui_set_label(keyd['H'], lock ? "H" : "h");
    gui_set_label(keyd['I'], lock ? "I" : "i");
    gui_set_label(keyd['J'], lock ? "J" : "j");
    gui_set_label(keyd['K'], lock ? "K" : "k");
    gui_set_label(keyd['L'], lock ? "L" : "l");
    gui_set_label(keyd['M'], lock ? "M" : "m");
    gui_set_label(keyd['N'], lock ? "N" : "n");
    gui_set_label(keyd['O'], lock ? "O" : "o");
    gui_set_label(keyd['P'], lock ? "P" : "p");
    gui_set_label(keyd['Q'], lock ? "Q" : "q");
    gui_set_label(keyd['R'], lock ? "R" : "r");
    gui_set_label(keyd['S'], lock ? "S" : "s");
    gui_set_label(keyd['T'], lock ? "T" : "t");
    gui_set_label(keyd['U'], lock ? "U" : "u");
    gui_set_label(keyd['V'], lock ? "V" : "v");
    gui_set_label(keyd['W'], lock ? "W" : "w");
    gui_set_label(keyd['X'], lock ? "X" : "x");
    gui_set_label(keyd['Y'], lock ? "Y" : "y");
    gui_set_label(keyd['Z'], lock ? "Z" : "z");
}

/* Britain Keyboards */
void gui_keyboard_en(int id)
{
    int jd, kd, ld;

    lock = 1;

    if ((jd = gui_hstack(id)))
    {
        gui_filler(jd);

        if ((kd = gui_vstack(jd)))
        {
            if ((ld = gui_hstack(kd)))
            {
                gui_filler(ld);
                const int bksp_id = gui_state(ld, GUI_TRIANGLE_LEFT, GUI_SML, GUI_BS, 0);
                keyd_en['='] = gui_state(ld, "+", GUI_SML, GUI_CHAR, '+');
                keyd_en['-'] = gui_state(ld, "_", GUI_SML, GUI_CHAR, '_');
                keyd_en['0'] = gui_state(ld, ")", GUI_SML, GUI_CHAR, ')');
                keyd_en['9'] = gui_state(ld, "(", GUI_SML, GUI_CHAR, '(');
                keyd_en['8'] = gui_state(ld, "*", GUI_SML, GUI_CHAR, '*');
                keyd_en['7'] = gui_state(ld, "&", GUI_SML, GUI_CHAR, '&');
                keyd_en['6'] = gui_state(ld, "^", GUI_SML, GUI_CHAR, '^');
                keyd_en['5'] = gui_state(ld, "%", GUI_SML, GUI_CHAR, '%');
                keyd_en['4'] = gui_state(ld, "$", GUI_SML, GUI_CHAR, '$');
                keyd_en['3'] = gui_state(ld, "#", GUI_SML, GUI_CHAR, '#');
                keyd_en['2'] = gui_state(ld, "@", GUI_SML, GUI_CHAR, '@');
                keyd_en['1'] = gui_state(ld, "!", GUI_SML, GUI_CHAR, '!');
                gui_filler(ld);

                gui_set_font(bksp_id, "ttf/DejaVuSans-Bold.ttf");
            }
            if ((ld = gui_hstack(kd)))
            {
                gui_filler(ld);
                gui_space(ld);
                keyd_en[']'] = gui_state(ld, "}", GUI_SML, GUI_CHAR, '}');
                keyd_en['['] = gui_state(ld, "{", GUI_SML, GUI_CHAR, '{');
                keyd_en['P'] = gui_state(ld, "P", GUI_SML, GUI_CHAR, 'P');
                keyd_en['O'] = gui_state(ld, "O", GUI_SML, GUI_CHAR, 'O');
                keyd_en['I'] = gui_state(ld, "I", GUI_SML, GUI_CHAR, 'I');
                keyd_en['U'] = gui_state(ld, "U", GUI_SML, GUI_CHAR, 'U');
                keyd_en['Y'] = gui_state(ld, "Y", GUI_SML, GUI_CHAR, 'Y');
                keyd_en['T'] = gui_state(ld, "T", GUI_SML, GUI_CHAR, 'T');
                keyd_en['R'] = gui_state(ld, "R", GUI_SML, GUI_CHAR, 'R');
                keyd_en['E'] = gui_state(ld, "E", GUI_SML, GUI_CHAR, 'E');
                keyd_en['W'] = gui_state(ld, "W", GUI_SML, GUI_CHAR, 'W');
                keyd_en['Q'] = gui_state(ld, "Q", GUI_SML, GUI_CHAR, 'Q');
                gui_space(ld);
                gui_space(ld);
                gui_filler(ld);
            }
            if ((ld = gui_hstack(kd)))
            {
                gui_filler(ld);
                keyd_en['\\'] = gui_state(ld, "|", GUI_SML, GUI_CHAR, '|');
                keyd_en['\''] = gui_state(ld, "\"", GUI_SML, GUI_CHAR, '\"');
                keyd_en[';'] = gui_state(ld, ":", GUI_SML, GUI_CHAR, ':');
                keyd_en['L'] = gui_state(ld, "L", GUI_SML, GUI_CHAR, 'L');
                keyd_en['K'] = gui_state(ld, "K", GUI_SML, GUI_CHAR, 'K');
                keyd_en['J'] = gui_state(ld, "J", GUI_SML, GUI_CHAR, 'J');
                keyd_en['H'] = gui_state(ld, "H", GUI_SML, GUI_CHAR, 'H');
                keyd_en['G'] = gui_state(ld, "G", GUI_SML, GUI_CHAR, 'G');
                keyd_en['F'] = gui_state(ld, "F", GUI_SML, GUI_CHAR, 'F');
                keyd_en['D'] = gui_state(ld, "D", GUI_SML, GUI_CHAR, 'D');
                keyd_en['S'] = gui_state(ld, "S", GUI_SML, GUI_CHAR, 'S');
                keyd_en['A'] = gui_state(ld, "A", GUI_SML, GUI_CHAR, 'A');
                const int caps_id = gui_state(ld, "⇩", GUI_SML, GUI_CL, 0);
                gui_filler(ld);

                gui_set_font(caps_id, "ttf/DejaVuSans-Bold.ttf");
            }
            if ((ld = gui_hstack(kd)))
            {
                gui_filler(ld);
                keyd_en['/'] = gui_state(ld, "?", GUI_SML, GUI_CHAR, '?');
                keyd_en['.'] = gui_state(ld, ">", GUI_SML, GUI_CHAR, '>');
                keyd_en[','] = gui_state(ld, "<", GUI_SML, GUI_CHAR, '<');
                keyd_en['M'] = gui_state(ld, "M", GUI_SML, GUI_CHAR, 'M');
                keyd_en['N'] = gui_state(ld, "N", GUI_SML, GUI_CHAR, 'N');
                keyd_en['B'] = gui_state(ld, "B", GUI_SML, GUI_CHAR, 'B');
                keyd_en['V'] = gui_state(ld, "V", GUI_SML, GUI_CHAR, 'V');
                keyd_en['C'] = gui_state(ld, "C", GUI_SML, GUI_CHAR, 'C');
                keyd_en['X'] = gui_state(ld, "X", GUI_SML, GUI_CHAR, 'X');
                keyd_en['Z'] = gui_state(ld, "Z", GUI_SML, GUI_CHAR, 'Z');
                keyd_en['`'] = gui_state(ld, "~", GUI_SML, GUI_CHAR, '~');
                gui_space(ld);
                gui_filler(ld);
            }
        }
    }
}

/* Britain Keyboards */
void gui_keyboard_lock_en(void)
{
    lock = lock ? 0 : 1;

    gui_set_label(keyd_en['-'], lock ? "_" : "-"); gui_set_state(keyd_en['-'], GUI_CHAR, lock ? '_' : '-');
    gui_set_label(keyd_en['='], lock ? "+" : "="); gui_set_state(keyd_en['='], GUI_CHAR, lock ? '+' : '=');
    gui_set_label(keyd_en['1'], lock ? "!" : "1");
    gui_set_label(keyd_en['2'], lock ? "@" : "2");
    gui_set_label(keyd_en['3'], lock ? "#" : "3");
    gui_set_label(keyd_en['4'], lock ? "$" : "4");
    gui_set_label(keyd_en['5'], lock ? "%" : "5");
    gui_set_label(keyd_en['6'], lock ? "^" : "6");
    gui_set_label(keyd_en['7'], lock ? "&" : "7");
    gui_set_label(keyd_en['8'], lock ? "*" : "8");
    gui_set_label(keyd_en['9'], lock ? "(" : "9");
    gui_set_label(keyd_en['0'], lock ? ")" : "0");
    gui_set_label(keyd_en['A'], lock ? "A" : "a");
    gui_set_label(keyd_en['B'], lock ? "B" : "b");
    gui_set_label(keyd_en['C'], lock ? "C" : "c");
    gui_set_label(keyd_en['D'], lock ? "D" : "d");
    gui_set_label(keyd_en['E'], lock ? "E" : "e");
    gui_set_label(keyd_en['F'], lock ? "F" : "f");
    gui_set_label(keyd_en['G'], lock ? "G" : "g");
    gui_set_label(keyd_en['H'], lock ? "H" : "h");
    gui_set_label(keyd_en['I'], lock ? "I" : "i");
    gui_set_label(keyd_en['J'], lock ? "J" : "j");
    gui_set_label(keyd_en['K'], lock ? "K" : "k");
    gui_set_label(keyd_en['L'], lock ? "L" : "l");
    gui_set_label(keyd_en['M'], lock ? "M" : "m");
    gui_set_label(keyd_en['N'], lock ? "N" : "n");
    gui_set_label(keyd_en['O'], lock ? "O" : "o");
    gui_set_label(keyd_en['P'], lock ? "P" : "p");
    gui_set_label(keyd_en['Q'], lock ? "Q" : "q");
    gui_set_label(keyd_en['R'], lock ? "R" : "r");
    gui_set_label(keyd_en['S'], lock ? "S" : "s");
    gui_set_label(keyd_en['T'], lock ? "T" : "t");
    gui_set_label(keyd_en['U'], lock ? "U" : "u");
    gui_set_label(keyd_en['V'], lock ? "V" : "v");
    gui_set_label(keyd_en['W'], lock ? "W" : "w");
    gui_set_label(keyd_en['X'], lock ? "X" : "x");
    gui_set_label(keyd_en['Y'], lock ? "Y" : "y");
    gui_set_label(keyd_en['Z'], lock ? "Z" : "z");
    gui_set_label(keyd_en['['], lock ? "{" : "[");
    gui_set_label(keyd_en[']'], lock ? "}" : "]");
    gui_set_label(keyd_en['`'], lock ? "~" : "`"); gui_set_state(keyd_en['`'], GUI_CHAR, lock ? '~' : '`');
    gui_set_label(keyd_en[';'], lock ? ":" : ";"); gui_set_state(keyd_en[';'], GUI_CHAR, lock ? ':' : ';');
    gui_set_label(keyd_en['\\'], lock ? "|" : "\\"); gui_set_state(keyd_en['\\'], GUI_CHAR, lock ? '|' : '\\');
    gui_set_label(keyd_en['\''], lock ? "\"" : "'"); gui_set_state(keyd_en['\''], GUI_CHAR, lock ? '"' : '\'');
    gui_set_label(keyd_en[','], lock ? "<" : ","); gui_set_state(keyd_en[','], GUI_CHAR, lock ? '<' : ',');
    gui_set_label(keyd_en['.'], lock ? ">" : "."); gui_set_state(keyd_en['.'], GUI_CHAR, lock ? '>' : '.');
    gui_set_label(keyd_en['/'], lock ? "?" : "/"); gui_set_state(keyd_en['/'], GUI_CHAR, lock ? '?' : '/');
}

#ifndef __EMSCRIPTEN__

/* German Keyboards */
void gui_keyboard_de(int id)
{
    int jd, kd, ld;

    lock = 1;

    if ((jd = gui_hstack(id)))
    {
        gui_filler(jd);

        if ((kd = gui_vstack(jd)))
        {
            if ((ld = gui_hstack(kd)))
            {
                gui_filler(ld);
                const int bksp_id = gui_state(ld, GUI_TRIANGLE_LEFT, GUI_SML, GUI_BS, 0);
                keyd_de[(unsigned char) '´'] = gui_state(ld, "`", GUI_SML, GUI_CHAR, '`');
                keyd_de[(unsigned char) 'ß'] = gui_state(ld, "?", GUI_SML, GUI_CHAR, '?');
                keyd_de['0'] = gui_state(ld, "=", GUI_SML, GUI_CHAR, '=');
                keyd_de['9'] = gui_state(ld, ")", GUI_SML, GUI_CHAR, ')');
                keyd_de['8'] = gui_state(ld, "(", GUI_SML, GUI_CHAR, '(');
                keyd_de['7'] = gui_state(ld, "/", GUI_SML, GUI_CHAR, '/');
                keyd_de['6'] = gui_state(ld, "&", GUI_SML, GUI_CHAR, '&');
                keyd_de['5'] = gui_state(ld, "%", GUI_SML, GUI_CHAR, '%');
                keyd_de['4'] = gui_state(ld, "$", GUI_SML, GUI_CHAR, '$');
                keyd_de['3'] = gui_state(ld, "§", GUI_SML, GUI_CHAR, '§');
                keyd_de['2'] = gui_state(ld, "\"", GUI_SML, GUI_CHAR, '"');
                keyd_de['1'] = gui_state(ld, "!", GUI_SML, GUI_CHAR, '!');
                keyd_de['^'] = gui_state(ld, "°", GUI_SML, GUI_CHAR, '°');
                gui_filler(ld);

                gui_set_font(bksp_id, "ttf/DejaVuSans-Bold.ttf");
            }
            if ((ld = gui_hstack(kd)))
            {
                gui_filler(ld);
                keyd_de['\''] = gui_state(ld, "'", GUI_SML, GUI_CHAR, '\'');
                keyd_de['*'] = gui_state(ld, "*", GUI_SML, GUI_CHAR, '*');
                keyd_de[(unsigned char) 'Ü'] = gui_state(ld, "Ü", GUI_SML, GUI_CHAR, 'Ü');
                keyd_de['P'] = gui_state(ld, "P", GUI_SML, GUI_CHAR, 'P');
                keyd_de['O'] = gui_state(ld, "O", GUI_SML, GUI_CHAR, 'O');
                keyd_de['I'] = gui_state(ld, "I", GUI_SML, GUI_CHAR, 'I');
                keyd_de['U'] = gui_state(ld, "U", GUI_SML, GUI_CHAR, 'U');
                keyd_de['Z'] = gui_state(ld, "Z", GUI_SML, GUI_CHAR, 'Z');
                keyd_de['T'] = gui_state(ld, "T", GUI_SML, GUI_CHAR, 'T');
                keyd_de['R'] = gui_state(ld, "R", GUI_SML, GUI_CHAR, 'R');
                keyd_de['E'] = gui_state(ld, "E", GUI_SML, GUI_CHAR, 'E');
                keyd_de['W'] = gui_state(ld, "W", GUI_SML, GUI_CHAR, 'W');
                keyd_de['Q'] = gui_state(ld, "Q", GUI_SML, GUI_CHAR, 'Q');
                gui_filler(ld);
            }
            if ((ld = gui_hstack(kd)))
            {
                gui_filler(ld);
                keyd_de[(unsigned char) 'Ä'] = gui_state(ld, "Ä", GUI_SML, GUI_CHAR, 'Ä');
                keyd_de[(unsigned char) 'Ö'] = gui_state(ld, "Ö", GUI_SML, GUI_CHAR, 'Ö');
                keyd_de['L'] = gui_state(ld, "L", GUI_SML, GUI_CHAR, 'L');
                keyd_de['K'] = gui_state(ld, "K", GUI_SML, GUI_CHAR, 'K');
                keyd_de['J'] = gui_state(ld, "J", GUI_SML, GUI_CHAR, 'J');
                keyd_de['H'] = gui_state(ld, "H", GUI_SML, GUI_CHAR, 'H');
                keyd_de['G'] = gui_state(ld, "G", GUI_SML, GUI_CHAR, 'G');
                keyd_de['F'] = gui_state(ld, "F", GUI_SML, GUI_CHAR, 'F');
                keyd_de['D'] = gui_state(ld, "D", GUI_SML, GUI_CHAR, 'D');
                keyd_de['S'] = gui_state(ld, "S", GUI_SML, GUI_CHAR, 'S');
                keyd_de['A'] = gui_state(ld, "A", GUI_SML, GUI_CHAR, 'A');
                gui_state(ld, _("caps"), GUI_SML, GUI_CL, 0);
                gui_filler(ld);
            }
            if ((ld = gui_hstack(kd)))
            {
                gui_filler(ld);
                keyd_de['_'] = gui_state(ld, "_", GUI_SML, GUI_CHAR, '_');
                keyd_de[':'] = gui_state(ld, ":", GUI_SML, GUI_CHAR, ':');
                keyd_de[';'] = gui_state(ld, ";", GUI_SML, GUI_CHAR, ';');
                keyd_de['M'] = gui_state(ld, "M", GUI_SML, GUI_CHAR, 'M');
                keyd_de['N'] = gui_state(ld, "N", GUI_SML, GUI_CHAR, 'N');
                keyd_de['B'] = gui_state(ld, "B", GUI_SML, GUI_CHAR, 'B');
                keyd_de['V'] = gui_state(ld, "V", GUI_SML, GUI_CHAR, 'V');
                keyd_de['C'] = gui_state(ld, "C", GUI_SML, GUI_CHAR, 'C');
                keyd_de['X'] = gui_state(ld, "X", GUI_SML, GUI_CHAR, 'X');
                keyd_de['Y'] = gui_state(ld, "Y", GUI_SML, GUI_CHAR, 'Y');
                gui_filler(ld);
            }
        }
    }
}

void gui_keyboard_lock_de(void)
{
    lock = lock ? 0 : 1;

    gui_set_label(keyd_de['^'], lock ? "°" : "^");
    gui_set_label(keyd_de['1'], lock ? "!" : "1");
    gui_set_label(keyd_de['2'], lock ? "\"" : "2");
    gui_set_label(keyd_de['3'], lock ? "§" : "3");
    gui_set_label(keyd_de['4'], lock ? "$" : "4");
    gui_set_label(keyd_de['5'], lock ? "%" : "5");
    gui_set_label(keyd_de['6'], lock ? "&" : "6");
    gui_set_label(keyd_de['7'], lock ? "/" : "7");
    gui_set_label(keyd_de['8'], lock ? "(" : "8");
    gui_set_label(keyd_de['9'], lock ? ")" : "9");
    gui_set_label(keyd_de['0'], lock ? "=" : "0");
    gui_set_label(keyd_de[(unsigned char) 'ß'], lock ? "?" : "ß");
    gui_set_label(keyd_de[(unsigned char) '´'], lock ? "`" : "´");
    gui_set_label(keyd_de['*'], lock ? "*" : "+");
    gui_set_label(keyd_de['\''], lock ? "'" : "#");
    gui_set_label(keyd_de[';'], lock ? ";" : ",");
    gui_set_label(keyd_de[':'], lock ? ":" : ".");
    gui_set_label(keyd_de['_'], lock ? "_" : "-");
    gui_set_label(keyd_de['A'], lock ? "A" : "a");
    gui_set_label(keyd_de['B'], lock ? "B" : "b");
    gui_set_label(keyd_de['C'], lock ? "C" : "c");
    gui_set_label(keyd_de['D'], lock ? "D" : "d");
    gui_set_label(keyd_de['E'], lock ? "E" : "e");
    gui_set_label(keyd_de['F'], lock ? "F" : "f");
    gui_set_label(keyd_de['G'], lock ? "G" : "g");
    gui_set_label(keyd_de['H'], lock ? "H" : "h");
    gui_set_label(keyd_de['I'], lock ? "I" : "i");
    gui_set_label(keyd_de['J'], lock ? "J" : "j");
    gui_set_label(keyd_de['K'], lock ? "K" : "k");
    gui_set_label(keyd_de['L'], lock ? "L" : "l");
    gui_set_label(keyd_de['M'], lock ? "M" : "m");
    gui_set_label(keyd_de['N'], lock ? "N" : "n");
    gui_set_label(keyd_de['O'], lock ? "O" : "o");
    gui_set_label(keyd_de['P'], lock ? "P" : "p");
    gui_set_label(keyd_de['Q'], lock ? "Q" : "q");
    gui_set_label(keyd_de['R'], lock ? "R" : "r");
    gui_set_label(keyd_de['S'], lock ? "S" : "s");
    gui_set_label(keyd_de['T'], lock ? "T" : "t");
    gui_set_label(keyd_de['U'], lock ? "U" : "u");
    gui_set_label(keyd_de['V'], lock ? "V" : "v");
    gui_set_label(keyd_de['W'], lock ? "W" : "w");
    gui_set_label(keyd_de['X'], lock ? "X" : "x");
    gui_set_label(keyd_de['Y'], lock ? "Y" : "y");
    gui_set_label(keyd_de['Z'], lock ? "Z" : "z");
    gui_set_label(keyd_de[(unsigned char) 'Ä'], lock ? "Ä" : "ä");
    gui_set_label(keyd_de[(unsigned char) 'Ö'], lock ? "Ö" : "ö");
    gui_set_label(keyd_de[(unsigned char) 'Ü'], lock ? "Ü" : "ü");
}

#endif

char gui_keyboard_char(char c)
{
    return lock ? toupper(c) : tolower(c);
}

/*---------------------------------------------------------------------------*/

int gui_back_button(int pd)
{
    int id;

    if ((id = gui_hstack(pd)))
    {
        gui_filler(id);
        gui_label(id, GUI_CROSS, GUI_SML, gui_red, gui_red);
        gui_label(id, _("Back"), GUI_SML, gui_wht, gui_wht);
        gui_filler(id);

        gui_set_state(id, GUI_BACK, 0);
        gui_set_rect(id, GUI_ALL);
    }

    return id;
}

/*---------------------------------------------------------------------------*/
