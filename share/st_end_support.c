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

/*
 * This header and source is prewritten. DO NOT EDIT!
 * Editing files may misleadingly matches when the lifecycle
 * is running via the game.
 */

/* TODO: Uncomment, if you want to test the end support. */
//#define TEST_END_SUPPORT

#ifndef NDEBUG
#include <assert.h>
#endif
#include <time.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "st_end_support.h"

#include "audio.h"
#include "config.h"
#include "gui.h"
#include "transition.h"
#include "lang.h"
#include "common.h"

#include "st_common.h"

#define AUD_MENU "snd/menu.ogg" /* DO NOT EDIT! */
#define AUD_BACK "snd/back.ogg" /* DO NOT EDIT! */

#define END_SUPPORT_YEAR  2024 /* DO NOT EDIT! */
#define END_SUPPORT_MONTH    4 /* DO NOT EDIT! */
#define END_SUPPORT_DAY     21 /* DO NOT EDIT! */

#define END_SUPPORT_TITLE_1  N_("Simple entities support is ending May 21, 2024.") /* DO NOT EDIT! */
#define END_SUPPORT_TITLE_2  N_("After %d years, support for Neverball 1.6 is nearing the end.") /* DO NOT EDIT! */
#define END_SUPPORT_TITLE_3  N_("Your Neverball 1.6 is out of support!") /* DO NOT EDIT! */

#define END_SUPPORT_IMAGE    "gui/end_support_%d.png" /* DO NOT EDIT! */

#define END_SUPPORT_DESC_1   N_("After %d years, support for Neverball 1.6 is coming to an end.\nThe best two things you can do to prepare for the transition are,\nback up your levels and highscores, and then get ready for what's next.\nWe have tools to help you with both.") /* DO NOT EDIT! */
#define END_SUPPORT_DESC_2   N_("May 21, 2024 is the last day Jānis Rūcis will offer\nsimple entities and technical support for running Neverball 1.6.\nWe know change can be difficult, that's why we're reaching out early\nto help you back up your levels and highscores, and prepare for what's next.") /* DO NOT EDIT! */
#define END_SUPPORT_DESC_3_1 N_("As of May 21, 2024, support for Neverball 1.6\nhas come to an end. Your entities is\nmore vulnerable to legacies due to:") /* DO NOT EDIT! */
#define END_SUPPORT_DESC_3_2 N_("- No simple start position\n- No goal decals\n- No simple switch and simple platform") /* DO NOT EDIT! */
#define END_SUPPORT_DESC_3_3 N_("PennySchloss requires using\nPennyball 2.2.0 on a new campaigns\nfor the latest huge guideline features.") /* DO NOT EDIT! */

#if defined(__WII__)
/* We're using SDL 1.2 on Wii, which has SDLKey instead of SDL_Keycode. */
typedef SDLKey SDL_Keycode;
#endif

/*---------------------------------------------------------------------------*/

/* DO NOT EDIT! */
static int check_leap_year(int year)
{
    if (year % 4 == 0 && year % 100 != 0 || year % 400 == 0)   //if year is a leap year
        return 1;

    else
        return 0;
}

/* DO NOT EDIT! */
static int no_of_days_in_month(int month, int year)
{
    // jan, march, may, july, aug, oct, dec contains 31 days
    if (month == 1 || month == 3 || month == 5 || month == 7 ||
        month == 8 || month == 10 || month == 12)
        return 31;

    if (month == 2)
    {
        int n = check_leap_year(year);

        // if year is a leap year then Feb will contain 29 days, otherwise it contains 28 days
        return n == 1 ? 29 : 28;
    }

    // april, jun, sept, nov or others contains 30 days
    return 30;
}

/* DO NOT EDIT! */
static long long int difference_of_days(int day1, int month1, int year1,
                                        int day2, int month2, int year2)
{
    if (year1 == year2)
    {
        if (month1 == month2)
        {
            /*
             * day1 == day2: for same dates
             * abs(): for same year, same month but diff days
             */

            return day1 == day2 ? 0 : abs(day1 - day2);
        }
        else if (month1 < month2)
        {
            long long int result = 0;
            for (int i = month1; i < month2; i++)
                result = result + no_of_days_in_month(i, year1);

            // for same year, same day but diff month

            if      (day1 == day2)
                return result;
            else if (day1 < day2)
            {
                result = result + (day2 - day1);
                return result;
            }
            else
            {
                result = result - (day1 - day2);
                return result;
            }
        }
        else
        {
            long long int result = 0;
            for (int i = month2; i < month1; i++)
                result = result + no_of_days_in_month(i, year1);

            if      (day1 == day2)
                return result;
            else if (day2 < day1)
            {
                result = result + (day1 - day2);
                return result;
            }
            else
            {
                result = result - (day2 - day1);
                return result;
            }
        }
    }
    else if (year1 < year2)
    {
        long long int temp = 0;
        for (int i = year1; i < year2; i++)
        {
            if (check_leap_year(i))
                temp = temp + 366;
            else
                temp = temp + 365;
        }

        if (month1 == month2)
        {
            // for same month, same day but diff year

            if      (day1 == day2)
                return temp;
            else if (day1 < day2)
                return temp + (day2 - day1);
            else
                return temp - (day1 - day2);
        }
        else if (month1 < month2)
        {
            long long int result = 0;
            for (int i = month1; i < month2; i++)
                result = result + no_of_days_in_month(i, year2);

            // for same day, diff year and diff month

            if      (day1 == day2)
                return temp + result;
            else if (day1 < day2)
            {
                result = result + (day2 - day1);
                return temp + result;
            }
            else
            {
                result = result - (day1 - day2);
                return temp + result;
            }
        }
        else
        {
            long long int result = 0;
            for (int i = month2; i < month1; i++)
                result = result + no_of_days_in_month(i, year2);

            if      (day1 == day2)
                return temp - result;
            else if (day2 < day1)
            {
                result = result + (day1 - day2);
                return temp - result;
            }
            else
            {
                result = result - (day2 - day1);
                return temp - result;
            }
        }
    }
    else
    {
        long long int temp = 0;
        for (int i = year2; i < year1; i++)
        {
            if (check_leap_year(i))
                temp = temp + 366;
            else
                temp = temp + 365;
        }

        if (month1 == month2)
        {
            // for same day, same month but diff year

            if      (day1 == day2)
                return temp;
            else if (day2 < day1)
                return temp + (day1 - day2);
            else
                return temp - (day2 - day1);
        }
        else if (month2 < month1)
        {
            long long int result = 0;
            for (int i = month2; i < month1; i++)
                result = result + no_of_days_in_month(i, year1);

            if (day1 == day2)
                return temp + result;
            else if (day2 < day1)
            {
                result = result + (day1 - day2);
                return temp + result;
            }
            else
            {
                result = result - (day2 - day1);
                return temp + result;
            }
        }
        else
        {
            long long int result = 0;
            for (int i = month1; i < month2; i++)
                result = result + no_of_days_in_month(i, year1);

            // for same day, diff year and diff month

            if      (day1 == day2)
                return temp - result;
            else if (day1 < day2)
            {
                result = result + (day2 - day1);
                return temp - result;
            }
            else
            {
                result = result - (day1 - day2);
                return temp - result;
            }
        }
    }
}

/*---------------------------------------------------------------------------*/

/* DO NOT EDIT! */
static int switchball_useable(void)
{
    const SDL_Keycode k_auto = config_get_d(CONFIG_KEY_CAMERA_TOGGLE);
    const SDL_Keycode k_cam1 = config_get_d(CONFIG_KEY_CAMERA_1);
    const SDL_Keycode k_cam2 = config_get_d(CONFIG_KEY_CAMERA_2);
    const SDL_Keycode k_cam3 = config_get_d(CONFIG_KEY_CAMERA_3);
    const SDL_Keycode k_caml = config_get_d(CONFIG_KEY_CAMERA_L);
    const SDL_Keycode k_camr = config_get_d(CONFIG_KEY_CAMERA_R);

    SDL_Keycode k_arrowkey[4] = {
        config_get_d(CONFIG_KEY_FORWARD),
        config_get_d(CONFIG_KEY_LEFT),
        config_get_d(CONFIG_KEY_BACKWARD),
        config_get_d(CONFIG_KEY_RIGHT)
    };

    if (k_auto == SDLK_c && k_cam1 == SDLK_3 && k_cam2 == SDLK_1 && k_cam3 == SDLK_2 &&
        k_caml == SDLK_RIGHT && k_camr == SDLK_LEFT &&
        k_arrowkey[0] == SDLK_w && k_arrowkey[1] == SDLK_a && k_arrowkey[2] == SDLK_s && k_arrowkey[3] == SDLK_d)
        return 1;
    else if (k_auto == SDLK_c && k_cam1 == SDLK_3 && k_cam2 == SDLK_1 && k_cam3 == SDLK_2 &&
             k_caml == SDLK_d && k_camr == SDLK_a &&
             k_arrowkey[0] == SDLK_UP && k_arrowkey[1] == SDLK_LEFT && k_arrowkey[2] == SDLK_DOWN && k_arrowkey[3] == SDLK_RIGHT)
        return 1;

    /*
     * If the Switchball input preset is not detected,
     * Try it with the Neverball by default.
     */

    return 0;
}

/*---------------------------------------------------------------------------*/

struct state st_end_support;

/* DO NOT EDIT! */
static struct state *st_hide;

/* DO NOT EDIT! */
int goto_end_support(struct state *scontinue)
{
    st_hide = scontinue;

    time_t local = time(0);
    struct tm timestamp;
#if _WIN32 && _MSC_VER
    gmtime_s(&timestamp, &local);
#else
    timestamp = *gmtime(&local);
#endif

    int curryear = timestamp.tm_year + 1900;
    long long int diff = difference_of_days(timestamp.tm_mday, timestamp.tm_mon, curryear,
                                            END_SUPPORT_DAY, END_SUPPORT_MONTH - 1, END_SUPPORT_YEAR);

    return goto_state(diff > 30 ? st_hide : &st_end_support);
}

/*---------------------------------------------------------------------------*/

/* DO NOT EDIT! */
enum
{
    END_SUPPORT_INVITE = GUI_LAST
};

/* DO NOT EDIT! */
static int end_support_action(int tok, int val)
{
    switch (tok)
    {
        case GUI_BACK:
            exit_state(st_hide);
            break;
        case END_SUPPORT_INVITE:
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
#if defined(__EMSCRIPTEN__)
            EM_ASM({ window.open("https://discord.gg/qnJR263Hm2/"); });
#elif _WIN32
            system("explorer https://discord.gg/qnJR263Hm2/");
#elif defined(__APPLE__)
            system("open https://discord.gg/qnJR263Hm2/");
#elif defined(__linux__)
            system("x-www-browser https://discord.gg/qnJR263Hm2/");
#endif
#endif
            break;
    }

    return 1;
}

/* DO NOT EDIT! */
static int end_support_gui(void)
{
    int id, jd;

    int w = video.device_w;
    int h = video.device_h;

    /* End support dates should be local time */
    time_t local = time(0);
    struct tm timestamp;
#if _WIN32 && _MSC_VER
    gmtime_s(&timestamp, &local);
#else
    timestamp = *gmtime(&local);
#endif

    char titleattr[MAXSTR], descattr[MAXSTR];

    if ((id = gui_vstack(0)))
    {
        int curryear = timestamp.tm_year + 1900;
        long long int diff = difference_of_days(timestamp.tm_mday, timestamp.tm_mon, curryear,
                                                END_SUPPORT_DAY, END_SUPPORT_MONTH - 1, END_SUPPORT_YEAR);

        log_printf("Days left: %d / %d %d %d\n", diff, timestamp.tm_mday, timestamp.tm_mon, curryear);
#if !defined(TEST_END_SUPPORT)
        if (END_SUPPORT_YEAR >= timestamp.tm_year)
        {
            if (diff >= 30)
                return goto_state(st_hide);
            else if (diff < 30)
            {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(titleattr, MAXSTR, END_SUPPORT_TITLE_1);
                sprintf_s(descattr, MAXSTR, END_SUPPORT_DESC_1, curryear - 2014);
#else
                sprintf(titleattr, END_SUPPORT_TITLE_1);
                sprintf(descattr, END_SUPPORT_DESC_1, curryear - 2014);
#endif
                gui_label(id, titleattr, GUI_SML, gui_red, gui_red);
                gui_space(id);
                gui_multi(id, descattr, GUI_SML, gui_wht, gui_wht);
            }
            else if (diff < 2)
            {
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(titleattr, MAXSTR, END_SUPPORT_TITLE_2, curryear - 2014);
                sprintf_s(descattr, MAXSTR, END_SUPPORT_DESC_2);
#else
                sprintf(titleattr, END_SUPPORT_TITLE_2, curryear - 2014);
                sprintf(descattr, END_SUPPORT_DESC_2);
#endif
                gui_label(id, titleattr, GUI_SML, gui_red, gui_red);
                gui_space(id);
                gui_multi(id, descattr, GUI_SML, gui_wht, gui_wht);
            }
            else if (diff < 1)
            {
                gui_label(id, END_SUPPORT_TITLE_3, GUI_SML, gui_red, gui_red);
                gui_space(id);
                if ((jd = gui_vstack(id)))
                {
                    gui_label(jd, END_SUPPORT_DESC_3_1, GUI_SML, gui_wht, gui_wht);
                    gui_label(jd, END_SUPPORT_DESC_3_2, GUI_SML, gui_red, gui_red);
                    gui_label(jd, END_SUPPORT_DESC_3_3, GUI_SML, gui_wht, gui_wht);
                    gui_set_rect(jd, GUI_ALL);
                }
            }
#else
            gui_label(id, "Test title", GUI_SML, gui_red, gui_red);
            gui_space(id);
            gui_label(id, "Test description", GUI_SML, gui_wht, gui_wht);
#endif
        }

        gui_space(id);

        if ((jd = gui_varray(id)))
        {
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
            gui_start(jd, _("Transfer now!"), GUI_SML, END_SUPPORT_INVITE, 0);
            gui_state(jd, _("Remind me later"), GUI_SML, GUI_BACK, 0);
#else
            gui_start(jd, _("Remind me later"), GUI_SML, GUI_BACK, 0);
#endif

            //gui_state(jd, _("Do not remind me again"), GUI_SML, GUI_BACK, 0);
        }
    }

    gui_layout(id, 0, 0);
    return id;
}

/* DO NOT EDIT! */
static int end_support_enter(struct state *st, struct state *prev, int intent)
{
    conf_common_init(end_support_action, 1);
    audio_music_fade_to(0.5f, "gui/bgm/inter.ogg", 1);
    return transition_slide(end_support_gui(), 1, intent);
}

/* DO NOT EDIT! */
static int end_support_leave(struct state *st, struct state *next, int id, int intent)
{
    return conf_common_leave(st, next, id, intent);
}

/*---------------------------------------------------------------------------*/

/* DO NOT EDIT! */
struct state st_end_support = {
    end_support_enter,
    end_support_leave,
    conf_common_paint,
    common_timer,
    common_point,
    common_stick,
    NULL,
    common_click,
    common_keybd,
    common_buttn
};
