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

#if _WIN32 && __MINGW32__
#include <SDL2/SDL.h>
#elif _WIN32 && _MSC_VER
#include <SDL.h>
#elif _WIN32
#error Security compilation error: No target include file in path for Windows specified!
#else
#include <SDL.h>
#endif

#include <string.h>
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

#include "common.h"
#include "text.h"

/*---------------------------------------------------------------------------*/

int text_add_char(unsigned int unicode, char *string, int maxbytes)
{
    size_t pos = strlen(string);
    int l;

    if      (unicode < 0x80)    l = 1;
    else if (unicode < 0x0800)  l = 2;
    else if (unicode < 0x10000) l = 3;
    else                        l = 4;

    if (pos + l >= maxbytes)
        return 0;

    if (unicode < 0x80)
        string[pos++] = (char) unicode;
    else if (unicode < 0x0800)
    {
        string[pos++] = (char) ((unicode >> 6)   | 0xC0);
        string[pos++] = (char) ((unicode & 0x3F) | 0x80);
    }
    else if (unicode < 0x10000)
    {
        string[pos++] = (char) ((unicode >> 12)         | 0xE0);
        string[pos++] = (char) (((unicode >> 6) & 0x3F) | 0x80);
        string[pos++] = (char) ((unicode & 0x3F)        | 0x80);
    }
    else
    {
        string[pos++] = (char) ((unicode >> 18)          | 0xF0);
        string[pos++] = (char) (((unicode >> 12) & 0x3F) | 0x80);
        string[pos++] = (char) (((unicode >> 6)  & 0x3F) | 0x80);
        string[pos++] = (char) ((unicode & 0x3F)         | 0x80);
    }

    string[pos++] = 0;

    return l;
}

int text_del_char(char *string)
{
    int pos = (int) strlen(string) - 1;

    while (pos >= 0 && ((string[pos] & 0xC0) == 0x80))
        string[pos--] = 0;

    if (pos >= 0)
    {
        string[pos] = 0;
        return 1;
    }

    return 0;
}

int text_length(const char *string)
{
    int result = 0;

    while (*string != '\0')
    {
        if ((*string++ & 0xC0) != 0x80)
            result++;
    }

    return result;
}

/*---------------------------------------------------------------------------*/

char text_input[MAXSTR];

static void (*on_text_input)(int);

#ifdef CALLBACK
#pragma message(__FILE__ "("_CRT_STRINGIZE(__LINE__)")" ": " \
                "CALLBACK: Preprocessor definitions found! Replacing to functions!")
#undef CALLBACK
#endif
#define CALLBACK(typing) do {                   \
        if (on_text_input)                      \
            on_text_input(typing);              \
    } while (0)

void text_input_start(void (*cb) (int))
{
    on_text_input = cb;
    text_input[0] = 0;
    CALLBACK(0);

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
    SDL_StartTextInput();
#endif
}

void text_input_stop(void)
{
    on_text_input = NULL;
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
    SDL_StopTextInput();
#endif
}

int text_input_str(const char *input, int typing)
{
    if (input && *input)
    {
        SAFECAT(text_input, input);
        CALLBACK(typing);
        return 1;
    }
    return 0;
}

int text_input_char(int input)
{
    if (text_add_char(input, text_input, sizeof (text_input)))
    {
        CALLBACK(0);
        return 1;
    }
    return 0;
}

int text_input_del(void)
{
    if (text_del_char(text_input))
    {
        CALLBACK(0);
        return 1;
    }
    return 0;
}

int text_input_paste(void)
{
#if _WIN32 && _MSC_VER
    HANDLE clip;

    if (OpenClipboard(0))
    {
        clip = GetClipboardData(CF_TEXT);

        SAFECPY(text_input, (char *) clip);

#ifndef NDEBUG
        assert(CloseClipboard());
#endif

        CALLBACK(0);
        return 1;
    }
#endif

    return 0;
}

/*---------------------------------------------------------------------------*/
